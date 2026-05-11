#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: May 2026
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
MAX_TIME="50"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="15000"
PORT_STRIDE="30"
KEEP_WORKDIRS="no"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/memoryturnlimit_behavior_missions/memoryturnlimit_behavior_motion"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
RUN_ROOT=""
CASE_RESULT_DIR=""
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
VEHICLE_MOOS_STEM="$MISSION_DIR/meta_vehicle.moos"
VEHICLE_BHV_STEM="$MISSION_DIR/meta_vehicle.bhv"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
VEHICLE_MOOS_XFILE="$MISSION_DIR/meta_vehicle.moosx"
VEHICLE_BHV_XFILE="$MISSION_DIR/meta_vehicle.bhvx"

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
        echo ""
        echo "Options:"
        echo "  --help, -h         Show this help message"
        echo "  --verbose, -v      Verbose, confirm launch"
        echo "  --just_make, -j    Only create targ files"
        echo "  --max_time=<secs>  Max time passed to xlaunch"
        echo "  --case=<name>      Run one named case"
        echo "  --jobs=<n>         Run up to n cases per wave"
        echo "  --port_base=<n>    Base port for per-case wave blocks"
        echo "  --keep_workdirs    Keep temp mission copies in wave mode"
        echo "  --gui              Launch with pMarineViewer"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=baseline_memory_pass --port_base=15000 10"
        echo "  ./zlaunch.sh --jobs=4 --port_base=15000 10"
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

clear_xfiles() {
    rm -f "$SHORE_XFILE" "$VEHICLE_MOOS_XFILE" "$VEHICLE_BHV_XFILE"
}

remove_tree() {
    local targ="$1"
    if [ "$targ" != "" ] && [ -d "$targ" ]; then
        rm -rf "$targ"
    fi
}

stop_mission_apps() {
    local mission_root="${1:-$MISSION_DIR}"
    harness_teardown_stop_root "$mission_root"
}

cleanup() {
    local start_dir="$PWD"
    if [ -d "$MISSION_DIR" ]; then
        cd "$MISSION_DIR"
        ./clean.sh >/dev/null 2>&1 || true
        stop_mission_apps "$MISSION_DIR"
    fi
    cd "$start_dir"
    if [ "$RUN_ROOT" != "" ]; then
        stop_mission_apps "$RUN_ROOT"
    fi
    if [ "$KEEP_WORKDIRS" != "yes" ] && [ "$RUN_ROOT" != "" ]; then
        remove_tree "$RUN_ROOT"
    fi
}

wait_for_result_line() {
    local file="$1"
    local tries="$2"
    local line
    local i=0
    while [ "$i" -lt "$tries" ]; do
        if [ -f "$file" ]; then
            line=`tail -n 1 "$file" 2>/dev/null`
            if echo "$line" | grep -q "grade="; then
                echo "$line"
                return 0
            fi
        fi
        sleep 0.5
        i=$((i + 1))
    done
    echo ""
    return 1
}

get_case_config() {
    CASE_NAME="$1"
    EXPECTED=""
    SHORE_PATCH=""
    VEH_MOOS_PATCH=""
    VEH_BHV_PATCH=""

    case "$CASE_NAME" in
        baseline_memory_pass)
            EXPECTED="pass"
            ;;
        tight_window_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-tight-window-pass-shoreside.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/tight-window-pass-vehicle.xbhv"
            ;;
        relaxed_window_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-relaxed-window-pass-shoreside.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/relaxed-window-pass-vehicle.xbhv"
            ;;
        short_memory_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-short-memory-pass-shoreside.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/short-memory-pass-vehicle.xbhv"
            ;;
        long_memory_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-long-memory-pass-shoreside.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/long-memory-pass-vehicle.xbhv"
            ;;
        zero_memory_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-zero-memory-pass-shoreside.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/zero-memory-pass-vehicle.xbhv"
            ;;
        very_slow_speed_floor_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-very-slow-speed-floor-pass-shoreside.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/very-slow-speed-floor-pass-vehicle.xbhv"
            ;;
        weak_priority_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-weak-priority-pass-shoreside.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/weak-priority-pass-vehicle.xbhv"
            ;;
        slow_speed_scaled_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-slow-speed-scaled-pass-shoreside.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/slow-speed-scaled-pass-vehicle.xbhv"
            ;;
        fast_speed_unscaled_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-fast-speed-unscaled-pass-shoreside.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/fast-speed-unscaled-pass-vehicle.xbhv"
            ;;
        moderate_turn_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-moderate-turn-pass-shoreside.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/moderate-turn-pass-vehicle.xbhv"
            ;;
        starboard_turn_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-starboard-turn-pass-shoreside.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/starboard-turn-pass-vehicle.xbhv"
            ;;
        max_turn_range_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-max-turn-range-pass-shoreside.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/max-turn-range-pass-vehicle.xbhv"
            ;;
        zero_turn_range_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-zero-turn-range-pass-shoreside.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/zero-turn-range-pass-vehicle.xbhv"
            ;;
        runtime_widen_range_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-runtime-widen-range-pass-shoreside.xmoos"
            VEH_MOOS_PATCH="$HARNESS_DIR/runtime-widen-range-pass-vehicle.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/runtime-widen-range-pass-vehicle.xbhv"
            ;;
        runtime_tighten_range_pass)
            EXPECTED="pass"
            SHORE_PATCH="$HARNESS_DIR/eval-runtime-tighten-range-pass-shoreside.xmoos"
            VEH_MOOS_PATCH="$HARNESS_DIR/runtime-tighten-range-pass-vehicle.xmoos"
            VEH_BHV_PATCH="$HARNESS_DIR/runtime-tighten-range-pass-vehicle.xbhv"
            ;;
        bad_memory_time_fail)
            EXPECTED="fail"
            VEH_BHV_PATCH="$HARNESS_DIR/bad-memory-time-fail-vehicle.xbhv"
            ;;
        bad_turn_range_high_fail)
            EXPECTED="fail"
            VEH_BHV_PATCH="$HARNESS_DIR/bad-turn-range-high-fail-vehicle.xbhv"
            ;;
        bad_turn_range_text_fail)
            EXPECTED="fail"
            VEH_BHV_PATCH="$HARNESS_DIR/bad-turn-range-text-fail-vehicle.xbhv"
            ;;
        missing_memory_time_fail)
            EXPECTED="fail"
            VEH_BHV_PATCH="$HARNESS_DIR/missing-memory-time-fail-vehicle.xbhv"
            ;;
        missing_turn_range_fail)
            EXPECTED="fail"
            VEH_BHV_PATCH="$HARNESS_DIR/missing-turn-range-fail-vehicle.xbhv"
            ;;
        *)
            echo "$ME: Unknown case: [$CASE_NAME]"
            return 1
            ;;
    esac
    return 0
}

apply_case_patches() {
    clear_xfiles

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$SHORE_STEM" "$SHORE_PATCH" --targ="$SHORE_XFILE"
    fi

    if [ "$VEH_MOOS_PATCH" != "" ]; then
        nspatch --stem="$VEHICLE_MOOS_STEM" "$VEH_MOOS_PATCH" --targ="$VEHICLE_MOOS_XFILE"
    fi

    if [ "$VEH_BHV_PATCH" != "" ]; then
        nspatch --stem="$VEHICLE_BHV_STEM" "$VEH_BHV_PATCH" --targ="$VEHICLE_BHV_XFILE"
    fi
}

run_case() {
    local case_idx="$1"
    local case_name="$2"
    local line
    local actual
    local status
    local launch_rc
    local shore_mport
    local veh_mport
    local shore_pshare
    local veh_pshare
    local case_base

    get_case_config "$case_name" || return 1

    case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    shore_mport=$((case_base + 0))
    veh_mport=$((case_base + 1))
    shore_pshare=$((case_base + 10))
    veh_pshare=$((case_base + 11))

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    stop_mission_apps "$MISSION_DIR"
    apply_case_patches || return 1
    : > results.txt

    XARGS="--max_time=$MAX_TIME --mmod=$case_name --shore_mport=$shore_mport --veh_mport=$veh_mport --shore_pshare=$shore_pshare --veh_pshare=$veh_pshare $TIME_WARP"
    if [ "$NOGUI" != "" ]; then
        XARGS="$XARGS $NOGUI"
    fi
    if [ "$JUST_MAKE" = "yes" ]; then
        XARGS="$XARGS --just_make"
    fi

    vecho "Running case [$case_name] with xlaunch args: $XARGS"
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
    line=$(wait_for_result_line results.txt 80)
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="success"
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
    fi

    if [ "$launch_rc" != 0 ]; then
        echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  launch_rc=$launch_rc  $line" >> "$RESULTS_FILE"
    else
        echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  $line" >> "$RESULTS_FILE"
    fi
    cd "$HARNESS_DIR"
}

prepare_case_dir() {
    local case_dir="$1"
    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
    )

    local shore_stem="$case_dir/meta_shoreside.moos"
    local veh_moos_stem="$case_dir/meta_vehicle.moos"
    local veh_bhv_stem="$case_dir/meta_vehicle.bhv"
    local shore_xfile="$case_dir/meta_shoreside.moosx"
    local veh_moos_xfile="$case_dir/meta_vehicle.moosx"
    local veh_bhv_xfile="$case_dir/meta_vehicle.bhvx"

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$shore_stem" "$SHORE_PATCH" --targ="$shore_xfile"
    fi

    if [ "$VEH_MOOS_PATCH" != "" ]; then
        nspatch --stem="$veh_moos_stem" "$VEH_MOOS_PATCH" --targ="$veh_moos_xfile"
    fi

    if [ "$VEH_BHV_PATCH" != "" ]; then
        nspatch --stem="$veh_bhv_stem" "$VEH_BHV_PATCH" --targ="$veh_bhv_xfile"
    fi
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
    local xargs
    local launch_rc
    local case_base

    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    case_result_file="$CASE_RESULT_DIR/${case_tag}.txt"

    get_case_config "$case_name" || {
        echo "case=$case_name  case_result=error  expected=unknown  actual=script_error" > "$case_result_file"
        return 1
    }

    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  case_result=error  expected=$EXPECTED  actual=script_error" > "$case_result_file"
        return 1
    }

    case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    shore_mport=$((case_base + 0))
    veh_mport=$((case_base + 1))
    shore_pshare=$((case_base + 10))
    veh_pshare=$((case_base + 11))

    (
        cd "$case_dir"
        : > results.txt
        xargs="--max_time=$MAX_TIME --mmod=$case_name --shore_mport=$shore_mport --veh_mport=$veh_mport --shore_pshare=$shore_pshare --veh_pshare=$veh_pshare $TIME_WARP"
        if [ "$NOGUI" != "" ]; then
            xargs="$xargs $NOGUI"
        fi
        if [ "$JUST_MAKE" = "yes" ]; then
            xargs="$xargs --just_make"
        fi
        xlaunch.sh $xargs
    )
    launch_rc=$?

    if [ "$JUST_MAKE" = "yes" ]; then
        if [ "$launch_rc" = 0 ]; then
            echo "case=$case_name  case_result=success  expected=just_make  actual=just_make" > "$case_result_file"
            return 0
        fi
        echo "case=$case_name  case_result=error  expected=just_make  actual=script_error" > "$case_result_file"
        return 1
    fi

    line=$(wait_for_result_line "$case_dir/results.txt" 80)
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
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
    fi

    if [ "$launch_rc" != 0 ]; then
        echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  launch_rc=$launch_rc  $line" > "$case_result_file"
    else
        echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  $line" > "$case_result_file"
    fi

    if [ "$status" = "success" ]; then
        return 0
    fi
    return 1
}

trap cleanup EXIT

ALL_CASES=(
    baseline_memory_pass
    tight_window_pass
    relaxed_window_pass
    short_memory_pass
    long_memory_pass
    zero_memory_pass
    weak_priority_pass
    very_slow_speed_floor_pass
    slow_speed_scaled_pass
    fast_speed_unscaled_pass
    moderate_turn_pass
    starboard_turn_pass
    max_turn_range_pass
    zero_turn_range_pass
    runtime_widen_range_pass
    runtime_tighten_range_pass
    bad_memory_time_fail
    bad_turn_range_high_fail
    bad_turn_range_text_fail
    missing_memory_time_fail
    missing_turn_range_fail
)

SOLO_CASES=(
    bad_memory_time_fail
    bad_turn_range_high_fail
    bad_turn_range_text_fail
    missing_memory_time_fail
    missing_turn_range_fail
)

is_solo_case() {
    local case_name="$1"
    local solo
    for solo in "${SOLO_CASES[@]}"; do
        if [ "$case_name" = "$solo" ]; then
            return 0
        fi
    done
    return 1
}

RUN_CASES=("${ALL_CASES[@]}")
if [ "$CASE" != "" ]; then
    RUN_CASES=("$CASE")
fi

: > "$RESULTS_FILE"

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    case_idx=0
    for ONE_CASE in "${RUN_CASES[@]}"; do
        run_case "$case_idx" "$ONE_CASE" || {
            echo "case=$ONE_CASE  case_result=error  expected=unknown  actual=script_error" >> "$RESULTS_FILE"
            ALL_OK="no"
            if [ "$JUST_MAKE" != "yes" ]; then
                break
            fi
        }
        case_idx=$((case_idx + 1))
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_memoryturnlimit_XXXXXX")
    CASE_RESULT_DIR="$RUN_ROOT/case_results"
    mkdir -p "$CASE_RESULT_DIR"

    case_idx=0
    wave_pids=""
    wave_count=0
    for ONE_CASE in "${RUN_CASES[@]}"; do
        if is_solo_case "$ONE_CASE" && [ "$wave_count" -gt 0 ]; then
            for pid in $wave_pids; do
                wait "$pid" || ALL_OK="no"
            done
            wave_pids=""
            wave_count=0
            stop_mission_apps "$RUN_ROOT"
        fi

        run_case_isolated "$case_idx" "$ONE_CASE" &
        wave_pids="$wave_pids $!"
        wave_count=$((wave_count + 1))
        case_idx=$((case_idx + 1))

        if [ "$wave_count" -ge "$JOBS" ] || is_solo_case "$ONE_CASE"; then
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
fi

if [ "$JUST_MAKE" = "yes" ]; then
    echo "$ME: Just_make complete for cases: ${RUN_CASES[*]}"
    exit 0
fi

cat "$RESULTS_FILE"

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
