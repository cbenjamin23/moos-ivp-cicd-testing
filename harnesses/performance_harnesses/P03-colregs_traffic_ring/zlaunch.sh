#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

ME=$(basename "$0")
CMD_ARGS=""
PERF_PROFILE="${PERF_PROFILE:-local}"
TIME_WARP=""
VERBOSE=""
JUST_MAKE=""
MAX_TIME="700"
ENDURANCE_MAX_TIME="1400"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="9900"
KEEP_WORKDIRS="no"
RESULTS_FILE="$PWD/results.txt"
HARNESS_DIR="$PWD"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/performance_missions/P03-colregs_traffic_ring"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
ALL_OK="yes"
RUN_ROOT=""
CASE_RESULT_DIR=""

if [ -f "$TEARDOWN_HELPER" ]; then
  . "$TEARDOWN_HELPER"
else
  echo "$ME: Missing teardown helper: $TEARDOWN_HELPER"
  exit 1
fi

for ARGI; do
  CMD_ARGS+="${ARGI} "
  if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
    echo "$ME [OPTIONS] [time_warp]"
    echo "  --profile=<name>   Set perf profile (local or ci)"
    echo "  PERF_PROFILE=ci    Run with CI timing defaults"
    exit 0
  elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
    VERBOSE="yes"
  elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
    JUST_MAKE="yes"
  elif [ "${ARGI:0:11}" = "--max_time=" ]; then
    MAX_TIME="${ARGI#--max_time=*}"
  elif [ "${ARGI:0:7}" = "--case=" ]; then
    CASE="${ARGI#--case=*}"
  elif [ "${ARGI:0:7}" = "--jobs=" ]; then
    JOBS="${ARGI#--jobs=*}"
  elif [ "${ARGI:0:12}" = "--port_base=" ]; then
    PORT_BASE="${ARGI#--port_base=*}"
  elif [ "${ARGI}" = "--keep_workdirs" ]; then
    KEEP_WORKDIRS="yes"
  elif [ "${ARGI:0:10}" = "--profile=" ]; then
    PERF_PROFILE="${ARGI#--profile=*}"
  elif [ "${ARGI}" = "--gui" ]; then
    NOGUI=""
  elif [ "${ARGI//[^0-9]/}" = "$ARGI" ] && [ "$TIME_WARP" = "" ]; then
    TIME_WARP=$ARGI
  fi
done

case "$PERF_PROFILE" in
  local)
    [ "$TIME_WARP" = "" ] && TIME_WARP="10"
    ;;
  ci)
    [ "$TIME_WARP" = "" ] && TIME_WARP="5"
    ;;
  *)
    echo "$ME: Bad value for PERF_PROFILE: [$PERF_PROFILE]"
    exit 1
    ;;
esac

cleanup() {
  local start_dir="$PWD"
  if [ -d "$MISSION_DIR" ]; then
    cd "$MISSION_DIR"
    rm -f "$SHORE_XFILE"
    ./clean.sh >/dev/null 2>&1 || true
    stop_mission_apps "$MISSION_DIR"
  fi
  cd "$start_dir"
  if [ "$RUN_ROOT" != "" ]; then
    stop_mission_apps "$RUN_ROOT"
  fi
  if [ "$KEEP_WORKDIRS" != "yes" ] && [ "$RUN_ROOT" != "" ] && [ "$ALL_OK" = "yes" ]; then
    rm -rf "$RUN_ROOT"
  fi
}

stop_mission_apps() {
  local mission_root="${1:-$MISSION_DIR}"
  harness_teardown_stop_root "$mission_root"
}

prepare_case_dir() {
  local case_dir="$1"
  cp -R "$MISSION_DIR"/. "$case_dir"/
  (
    cd "$case_dir"
    ./clean.sh >/dev/null 2>&1 || true
  )
  local shore_stem="$case_dir/meta_shoreside.moos"
  local shore_xfile="$case_dir/meta_shoreside.moosx"
  nspatch --stem="$shore_stem" "$SHORE_PATCH" --targ="$shore_xfile"
}

wait_for_result_line() {
  local results_path="$1"
  local attempts="${2:-24}"
  local line=""
  for attempt in $(seq 1 "$attempts"); do
    line=$(tail -n 1 "$results_path" 2>/dev/null)
    if echo "$line" | grep -q 'grade='; then
      echo "$line"
      return 0
    fi
    sleep 0.25
  done
  echo "$line"
  return 1
}

get_field() {
  local line="$1"
  local key="$2"
  echo "$line" | sed -n "s/.*[[:space:]]$key=\\([^ ]*\\).*/\\1/p"
}

value_in_range() {
  local value="$1"
  local min="$2"
  local max="$3"
  awk -v value="$value" -v min="$min" -v max="$max" 'BEGIN{if ((value+0)<(min+0)||(value+0)>(max+0)) exit 1; exit 0;}'
}

scan_case_warnings() {
  local run_dir="${1:-$MISSION_DIR}"
  local latest_log_dir
  local alog_file
  local warn_count="0"

  latest_log_dir=$(find "$run_dir" -maxdepth 1 -type d -name 'LOG_*' | sort | tail -n 1)
  if [ "$latest_log_dir" = "" ]; then
    echo "missing"
    return 0
  fi
  alog_file=$(find "$latest_log_dir" -maxdepth 1 -type f -name '*.alog' | head -n 1)
  if [ "$alog_file" = "" ]; then
    echo "missing"
    return 0
  fi

  warn_count=$(aloggrep APP_LOG "$alog_file" 2>/dev/null | \
    grep -Eic 'BHV_ERROR|obstacle unavoidable|Obstacle Breached|Unable to init AOF_AvoidObstacleV24|Allstop' || true)
  echo "$warn_count"
}

evaluate_performance() {
  local line="$1"
  local run_dir="${2:-$MISSION_DIR}"
  local wall_time
  local warning_count
  local notes=""

  wall_time=$(get_field "$line" "wall_time")
  warning_count=$(scan_case_warnings "$run_dir")

  if [ "$wall_time" = "" ]; then
    PERF_STATUS="error"
    PERF_NOTES="missing_metrics"
    PERF_WARNING_COUNT="$warning_count"
    return 0
  fi

  value_in_range "$wall_time" "$WALL_MIN" "$WALL_MAX" || notes="${notes} wall_time"
  if [ "$warning_count" = "missing" ]; then
    notes="${notes} missing_logs"
  else
    value_in_range "$warning_count" "0" "$WARNING_MAX" || notes="${notes} warnings"
  fi

  PERF_WARNING_COUNT="$warning_count"
  PERF_NOTES=$(echo "$notes" | sed 's/^ *//')
  if [ "$PERF_NOTES" = "" ]; then
    PERF_STATUS="ok"
    PERF_NOTES="none"
  else
    PERF_STATUS="mismatch"
  fi
}

get_case_config() {
  local case_name="$1"
  EXPECTED="pass"
  SHORE_PATCH="$HARNESS_DIR/${case_name//_/-}-shoreside.xmoos"
  WARNING_MAX="0"

  case "$case_name" in
    baseline_circle_pass)
      if [ "$PERF_PROFILE" = "ci" ]; then
        WALL_MIN="58.0"
        WALL_MAX="70.0"
      else
        WALL_MIN="30.0"
        WALL_MAX="32.0"
      fi
      ;;
    mixed_speed_circle_pass)
      if [ "$PERF_PROFILE" = "ci" ]; then
        WALL_MIN="58.0"
        WALL_MAX="70.0"
      else
        WALL_MIN="30.0"
        WALL_MAX="32.0"
      fi
      ;;
    endurance_circle_pass)
      if [ "$PERF_PROFILE" = "ci" ]; then
        WALL_MIN="175.0"
        WALL_MAX="190.0"
      else
        WALL_MIN="89.5"
        WALL_MAX="92.5"
      fi
      ;;
    noncoop_circle_pass)
      if [ "$PERF_PROFILE" = "ci" ]; then
        WALL_MIN="58.0"
        WALL_MAX="70.0"
      else
        WALL_MIN="30.0"
        WALL_MAX="32.0"
      fi
      ;;
    *)
      echo "$ME: Unknown case [$case_name]"
      return 1
      ;;
  esac

  if [ ! -f "$SHORE_PATCH" ]; then
    echo "$ME: Missing patch [$SHORE_PATCH]"
    return 1
  fi
  return 0
}

run_case_isolated() {
  local case_idx="$1"
  local case_name="$2"
  local case_tag case_dir case_result_file
  local shore_mport veh_mport shore_pshare veh_pshare
  local line actual status launch_rc
  local case_max_time="$MAX_TIME"
  local perf_status="skip" perf_notes="" perf_warning_count="na"

  get_case_config "$case_name" || return 1
  if [ "$case_name" = "endurance_circle_pass" ] && [ "$case_max_time" = "700" ]; then
    case_max_time="$ENDURANCE_MAX_TIME"
  fi

  case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
  case_dir="$RUN_ROOT/$case_tag"
  case_result_file="$CASE_RESULT_DIR/${case_tag}.txt"
  prepare_case_dir "$case_dir" || {
    echo "case=$case_name  case_result=error  expected=$EXPECTED  actual=script_error  perf_status=error  perf_notes=prepare_failed  warning_count=$perf_warning_count" > "$case_result_file"
    return 1
  }

  shore_mport=$((PORT_BASE + case_idx*20))
  veh_mport=$((shore_mport + 1))
  shore_pshare=$((PORT_BASE + 200 + case_idx*20))
  veh_pshare=$((shore_pshare + 1))

  (
    cd "$case_dir"
    : > results.txt
    XARGS="--max_time=$case_max_time --mmod=$case_name --shore_mport=$shore_mport --veh_mport=$veh_mport --shore_pshare=$shore_pshare --veh_pshare=$veh_pshare $TIME_WARP"
    if [ "$NOGUI" != "" ]; then
      XARGS="$XARGS $NOGUI"
    fi
    if [ "$JUST_MAKE" = "yes" ]; then
      XARGS="$XARGS --just_make"
    fi
    if [ "$VERBOSE" = "yes" ]; then
      XARGS="$XARGS --verbose"
    fi
    PTRAFFIC_BIN="$REPO_DIR/bin/pTrafficManager" xlaunch.sh $XARGS
  )
  launch_rc=$?

  if [ "$JUST_MAKE" = "yes" ]; then
    if [ "$launch_rc" = 0 ]; then
      echo "case=$case_name  case_result=success  expected=just_make  actual=just_make  perf_status=skip  perf_notes=none  warning_count=na" > "$case_result_file"
      return 0
    fi
    echo "case=$case_name  case_result=error  expected=just_make  actual=script_error  perf_status=error  perf_notes=launch_failed  warning_count=na" > "$case_result_file"
    return 1
  fi

  line=$(wait_for_result_line "$case_dir/results.txt" 120)
  actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
  if [ "$actual" = "" ]; then
    actual="missing"
  fi

  status="success"
  if [ "$launch_rc" != 0 ]; then
    status="error"
    actual="script_error"
  elif [ "$actual" = "missing" ]; then
    status="error"
  elif [ "$actual" != "$EXPECTED" ]; then
    status="mismatch"
  else
    evaluate_performance "$line" "$case_dir"
    perf_status="$PERF_STATUS"
    perf_notes="$PERF_NOTES"
    perf_warning_count="$PERF_WARNING_COUNT"
    if [ "$perf_status" != "ok" ]; then
      status="perf_mismatch"
    fi
  fi

  echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  perf_status=$perf_status  perf_notes=$perf_notes  warning_count=$perf_warning_count  $line" > "$case_result_file"
  [ "$status" = "success" ]
}

trap cleanup EXIT

CASES="baseline_circle_pass mixed_speed_circle_pass endurance_circle_pass noncoop_circle_pass"
if [ "$CASE" != "" ]; then
  CASES="$CASE"
fi

: > "$RESULTS_FILE"

RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_colregs_circle_XXXXXX")
CASE_RESULT_DIR="$RUN_ROOT/case_results"
mkdir -p "$CASE_RESULT_DIR"

stop_mission_apps "$MISSION_DIR"

case_idx=0
wave_pids=""
wave_count=0
for case_name in $CASES; do
  run_case_isolated "$case_idx" "$case_name" &
  wave_pids="$wave_pids $!"
  wave_count=$((wave_count + 1))
  case_idx=$((case_idx + 1))
  if [ "$wave_count" -ge "$JOBS" ]; then
    for pid in $wave_pids; do
      wait "$pid" || ALL_OK="no"
    done
    wave_pids=""
    wave_count=0
    stop_mission_apps "$RUN_ROOT"
  fi
done

if [ "$wave_count" -gt 0 ]; then
  for pid in $wave_pids; do
    wait "$pid" || ALL_OK="no"
  done
  stop_mission_apps "$RUN_ROOT"
fi

for result_file in $(find "$CASE_RESULT_DIR" -type f | sort); do
  cat "$result_file" >> "$RESULTS_FILE"
done

cat "$RESULTS_FILE"
if [ "$ALL_OK" != "yes" ] && [ "$RUN_ROOT" != "" ]; then
  echo "kept_workdirs=$RUN_ROOT"
fi
[ "$ALL_OK" = "yes" ]
