#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: Mar 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

ME=`basename "$0"`
CMD_ARGS=""
VERBOSE=""
JUST_MAKE=""
TIME_WARP="10"
MAX_TIME="480"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="9600"
KEEP_WORKDIRS="no"
ENDURANCE_MAX_TIME="2800"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/performance_missions/P01-obstacle_gauntlet"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
BASELINE_FILE="$HARNESS_DIR/performance_baselines.tsv"
RUN_ROOT=""
CASE_RESULT_DIR=""

for ARGI; do
    CMD_ARGS+="${ARGI} "
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo ""
        echo "Options:"
        echo "  --help, -h         Show this help message"
        echo "  --verbose, -v      Verbose, confirm launch"
        echo "  --just_make, -j    Only create targ files"
        echo "  --max_time=<secs>  Max time passed to the mission"
        echo "  --case=<name>      Run one named case"
        echo "  --jobs=<n>         Run up to n cases per wave"
        echo "  --port_base=<n>    Base shoreside MOOSDB port for wave mode"
        echo "  --keep_workdirs    Keep temp mission copies in wave mode"
        echo "  --gui              Launch with pMarineViewer"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=baseline_field_pass"
        echo "  ./zlaunch.sh --case=dense_field_pass"
        echo "  ./zlaunch.sh --case=endurance_random_pass"
        echo "  ./zlaunch.sh --jobs=2"
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 10 ]; then
        TIME_WARP=$ARGI
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
    elif [ "${ARGI}" = "--gui" ]; then
        NOGUI=""
    else
        echo "$ME: Bad arg:" $ARGI "Exit Code 1."
        exit 1
    fi
done

if ! echo "$JOBS" | grep -Eq '^[0-9]+$' || [ "$JOBS" -lt 1 ]; then
    echo "$ME: Bad value for --jobs: [$JOBS]"
    exit 1
fi

if ! echo "$PORT_BASE" | grep -Eq '^[0-9]+$'; then
    echo "$ME: Bad value for --port_base: [$PORT_BASE]"
    exit 1
fi

cleanup() {
    local start_dir="$PWD"
    if [ -d "$MISSION_DIR" ]; then
        cd "$MISSION_DIR"
        clear_xfiles
        ./clean.sh >/dev/null 2>&1 || true
        ktm >/dev/null 2>&1 || true
    fi
    cd "$start_dir"
    if [ "$KEEP_WORKDIRS" != "yes" ] && [ "$RUN_ROOT" != "" ]; then
        rm -rf "$RUN_ROOT"
    fi
}

clear_xfiles() {
    rm -f "$SHORE_XFILE"
}

get_field() {
    local line="$1"
    local key="$2"
    echo "$line" | sed -n "s/.*[[:space:]]$key=\\([^ ]*\\).*/\\1/p"
}

load_baseline() {
    local case_name="$1"
    local row

    row=$(awk -v target="$case_name" '
        $0 !~ /^#/ && NF > 0 && $1 == target {print; exit}
    ' "$BASELINE_FILE")

    if [ "$row" = "" ]; then
        echo "$ME: Missing baseline row for case [$case_name]"
        return 1
    fi

    read -r _case WALL_MIN WALL_MAX CYCLES_MIN CYCLES_MAX COLLISIONS_MAX \
        NEAR_MIN NEAR_MAX ENCOUNTERS_MIN ENCOUNTERS_MAX \
        OBAVOIDING_EXPECT TIMEOUT_EXPECT WARNING_MAX <<< "$row"
}

value_in_range() {
    local value="$1"
    local min="$2"
    local max="$3"

    awk -v value="$value" -v min="$min" -v max="$max" '
        BEGIN {
            if ((value + 0) < (min + 0) || (value + 0) > (max + 0))
                exit 1;
            exit 0;
        }'
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
        grep -Eic 'BHV_ERROR|obstacle unavoidable|Obstacle Breached|Unable to init AOF_AvoidObstacleV24' || true)

    echo "$warn_count"
}

evaluate_performance() {
    local case_name="$1"
    local line="$2"
    local run_dir="${3:-$MISSION_DIR}"
    local cycles
    local timeout
    local obavoiding
    local wall_time
    local collisions
    local near_misses
    local encounters
    local warning_count
    local perf_notes=""

    load_baseline "$case_name" || return 1

    cycles=$(get_field "$line" "cycles")
    timeout=$(get_field "$line" "timeout")
    obavoiding=$(get_field "$line" "obavoiding")
    wall_time=$(get_field "$line" "wall_time")
    collisions=$(get_field "$line" "collisions")
    near_misses=$(get_field "$line" "near_misses")
    encounters=$(get_field "$line" "encounters")
    warning_count=$(scan_case_warnings "$run_dir")

    if [ "$cycles" = "" ] || [ "$timeout" = "" ] || [ "$obavoiding" = "" ] || \
       [ "$wall_time" = "" ] || [ "$collisions" = "" ] || [ "$near_misses" = "" ] || \
       [ "$encounters" = "" ]; then
        PERF_STATUS="error"
        PERF_NOTES="missing_metrics"
        PERF_WARNING_COUNT="$warning_count"
        return 0
    fi

    if ! value_in_range "$cycles" "$CYCLES_MIN" "$CYCLES_MAX"; then
        perf_notes="${perf_notes} cycles"
    fi
    if [ "$timeout" != "$TIMEOUT_EXPECT" ]; then
        perf_notes="${perf_notes} timeout"
    fi
    if [ "$obavoiding" != "$OBAVOIDING_EXPECT" ]; then
        perf_notes="${perf_notes} obavoiding"
    fi
    if ! value_in_range "$wall_time" "$WALL_MIN" "$WALL_MAX"; then
        perf_notes="${perf_notes} wall_time"
    fi
    if ! value_in_range "$collisions" "0" "$COLLISIONS_MAX"; then
        perf_notes="${perf_notes} collisions"
    fi
    if ! value_in_range "$near_misses" "$NEAR_MIN" "$NEAR_MAX"; then
        perf_notes="${perf_notes} near_misses"
    fi
    if ! value_in_range "$encounters" "$ENCOUNTERS_MIN" "$ENCOUNTERS_MAX"; then
        perf_notes="${perf_notes} encounters"
    fi
    if [ "$warning_count" = "missing" ]; then
        perf_notes="${perf_notes} missing_logs"
    elif ! value_in_range "$warning_count" "0" "$WARNING_MAX"; then
        perf_notes="${perf_notes} warnings"
    fi

    PERF_WARNING_COUNT="$warning_count"
    PERF_NOTES=$(echo "$perf_notes" | sed 's/^ *//')
    if [ "$PERF_NOTES" = "" ]; then
        PERF_STATUS="ok"
        PERF_NOTES="none"
    else
        PERF_STATUS="mismatch"
    fi

    return 0
}

get_case_config() {
    CASE_NAME="$1"
    EXPECTED=""
    SHORE_PATCH=""

    if [ "$CASE_NAME" = "baseline_field_pass" ]; then
        EXPECTED="pass"
    elif [ "$CASE_NAME" = "dense_field_pass" ]; then
        EXPECTED="pass"
    elif [ "$CASE_NAME" = "endurance_random_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/endurance-random-pass-shoreside.xmoos"
    else
        echo "$ME: Unknown case: [$CASE_NAME]"
        return 1
    fi
    return 0
}

apply_case_patches() {
    clear_xfiles

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$SHORE_STEM" "$SHORE_PATCH" --targ="$SHORE_XFILE"
    fi
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

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$shore_stem" "$SHORE_PATCH" --targ="$shore_xfile"
    fi
}

run_case() {
    local case_name="$1"
    local line
    local actual
    local status
    local launch_rc
    local case_max_time="$MAX_TIME"
    local perf_status="skip"
    local perf_notes=""
    local perf_warning_count="na"

    get_case_config "$case_name" || return 1

    if [ "$case_name" = "endurance_random_pass" ] && [ "$case_max_time" = "480" ]; then
        case_max_time="$ENDURANCE_MAX_TIME"
    fi

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    ktm >/dev/null 2>&1 || true
    apply_case_patches || return 1
    : > results.txt

    XARGS="--max_time=$case_max_time --mmod=$case_name $TIME_WARP"
    if [ "$NOGUI" != "" ]; then
        XARGS="$XARGS $NOGUI"
    fi
    if [ "$JUST_MAKE" = "yes" ]; then
        XARGS="$XARGS --just_make"
    fi

    vecho "Running case [$case_name] with mission args: $XARGS"
    xlaunch.sh $XARGS
    launch_rc=$?

    if [ "$JUST_MAKE" = "yes" ]; then
        cd "$HARNESS_DIR"
        if [ "$launch_rc" = 0 ]; then
            return 0
        fi
        return 1
    fi

    sleep 1

    line=`tail -n 1 results.txt 2>/dev/null`
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="ok"
    if [ "$launch_rc" != 0 ]; then
        status="error"
        actual="script_error"
        ALL_OK="no"
    elif [ "$actual" = "missing" ]; then
        status="error"
        ALL_OK="no"
    elif [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
        ALL_OK="no"
    else
        evaluate_performance "$case_name" "$line"
        perf_status="$PERF_STATUS"
        perf_notes="$PERF_NOTES"
        perf_warning_count="$PERF_WARNING_COUNT"
        if [ "$perf_status" != "ok" ]; then
            status="perf_mismatch"
            ALL_OK="no"
        fi
    fi

    if [ "$launch_rc" != 0 ]; then
        echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  perf_status=$perf_status  perf_notes=$perf_notes  warning_count=$perf_warning_count  launch_rc=$launch_rc  $line" >> "$RESULTS_FILE"
    else
        echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  perf_status=$perf_status  perf_notes=$perf_notes  warning_count=$perf_warning_count  $line" >> "$RESULTS_FILE"
    fi

    cd "$HARNESS_DIR"
}

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    local case_tag
    local case_dir
    local case_result_file
    local shore_mport
    local veh_mport
    local shore_pshare
    local veh_pshare
    local line
    local actual
    local status
    local launch_rc
    local case_max_time="$MAX_TIME"
    local perf_status="skip"
    local perf_notes=""
    local perf_warning_count="na"

    get_case_config "$case_name" || return 1

    if [ "$case_name" = "endurance_random_pass" ] && [ "$case_max_time" = "480" ]; then
        case_max_time="$ENDURANCE_MAX_TIME"
    fi

    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    case_result_file="$CASE_RESULT_DIR/${case_tag}.txt"

    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  expected=$EXPECTED  actual=script_error  status=error  perf_status=error  perf_notes=prepare_failed  warning_count=$perf_warning_count" > "$case_result_file"
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
        xlaunch.sh $XARGS
    )
    launch_rc=$?

    if [ "$JUST_MAKE" = "yes" ]; then
        if [ "$launch_rc" = 0 ]; then
            echo "case=$case_name  expected=just_make  actual=just_make  status=ok  perf_status=skip  perf_notes=none  warning_count=na" > "$case_result_file"
            return 0
        fi
        echo "case=$case_name  expected=just_make  actual=script_error  status=error  perf_status=error  perf_notes=launch_failed  warning_count=na" > "$case_result_file"
        return 1
    fi

    line=`tail -n 1 "$case_dir/results.txt" 2>/dev/null`
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="ok"
    if [ "$launch_rc" != 0 ]; then
        status="error"
        actual="script_error"
    elif [ "$actual" = "missing" ]; then
        status="error"
    elif [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
    else
        evaluate_performance "$case_name" "$line" "$case_dir"
        perf_status="$PERF_STATUS"
        perf_notes="$PERF_NOTES"
        perf_warning_count="$PERF_WARNING_COUNT"
        if [ "$perf_status" != "ok" ]; then
            status="perf_mismatch"
        fi
    fi

    if [ "$launch_rc" != 0 ]; then
        echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  perf_status=$perf_status  perf_notes=$perf_notes  warning_count=$perf_warning_count  launch_rc=$launch_rc  $line" > "$case_result_file"
    else
        echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  perf_status=$perf_status  perf_notes=$perf_notes  warning_count=$perf_warning_count  $line" > "$case_result_file"
    fi

    if [ "$status" = "ok" ]; then
        return 0
    fi
    return 1
}

trap cleanup EXIT

if [ "$CASE" != "" ]; then
    CASES="$CASE"
else
    CASES="baseline_field_pass dense_field_pass endurance_random_pass"
fi

: > "$RESULTS_FILE"

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    for ONE_CASE in $CASES; do
        run_case "$ONE_CASE" || {
            echo "case=$ONE_CASE  expected=unknown  actual=script_error  status=error  perf_status=error  perf_notes=run_failed  warning_count=na" >> "$RESULTS_FILE"
            ALL_OK="no"
            if [ "$JUST_MAKE" != "yes" ]; then
                break
            fi
        }
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_obstacle_gauntlet_XXXXXX")
    CASE_RESULT_DIR="$RUN_ROOT/case_results"
    mkdir -p "$CASE_RESULT_DIR"

    ktm >/dev/null 2>&1 || true

    case_idx=0
    wave_pids=""
    wave_count=0
    for ONE_CASE in $CASES; do
        run_case_isolated "$case_idx" "$ONE_CASE" &
        wave_pids="$wave_pids $!"
        wave_count=$((wave_count + 1))
        case_idx=$((case_idx + 1))

        if [ "$wave_count" -ge "$JOBS" ]; then
            for pid in $wave_pids; do
                wait "$pid" || ALL_OK="no"
            done
            wave_pids=""
            wave_count=0
            ktm >/dev/null 2>&1 || true
        fi
    done

    if [ "$wave_count" -gt 0 ]; then
        for pid in $wave_pids; do
            wait "$pid" || ALL_OK="no"
        done
        ktm >/dev/null 2>&1 || true
    fi

    for result_file in $(find "$CASE_RESULT_DIR" -type f | sort); do
        cat "$result_file" >> "$RESULTS_FILE"
    done
fi

if [ "$JUST_MAKE" = "yes" ]; then
    echo "$ME: just_make complete for cases: $CASES"
    exit 0
fi

cat "$RESULTS_FILE"

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi

exit 1
