#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: Apr 2026
#------------------------------------------------------------
#  Part 1: Set convenience functions for producing terminal
#          debugging output, and catching SIGINT (ctrl-c).
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

#------------------------------------------------------------
#  Part 2: Set global variable default values
#------------------------------------------------------------
ME=`basename "$0"`
CMD_ARGS=""
VERBOSE=""
JUST_MAKE=""
TIME_WARP="10"
MAX_TIME="90"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="9700"
PORT_BASE_SET="no"
KEEP_WORKDIRS="no"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/waypoint_behavior_missions/waypoint_behavior_motion"
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

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
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
        echo "  --port_base=<n>    Base shoreside MOOSDB port for wave mode"
        echo "  --keep_workdirs    Keep temp mission copies in wave mode"
        echo "  --gui              Launch with pMarineViewer"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=single_point_arrival_pass"
        echo "  ./zlaunch.sh --case=repeat_forever_cycle_pass"
        echo "  ./zlaunch.sh --jobs=4"
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
        PORT_BASE_SET="yes"
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

wait_for_result_line() {
    local results_path="$1"
    local attempts="${2:-24}"
    local line=""
    local attempt

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

#------------------------------------------------------------
#  Part 4: Set convenience functions for managing x-files
#          and per-run cleanup.
#------------------------------------------------------------
clear_xfiles() {
    rm -f "$SHORE_XFILE" "$VEHICLE_MOOS_XFILE" "$VEHICLE_BHV_XFILE"
}

remove_tree() {
    local targ="$1"
    if [ "$targ" != "" ] && [ -d "$targ" ]; then
        rm -rf "$targ"
    fi
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

stop_mission_apps() {
    local mission_root="${1:-$MISSION_DIR}"
    harness_teardown_stop_root "$mission_root"
}

#------------------------------------------------------------
#  Part 5: Determine the expected outcome and patch files
#          for one named case.
#------------------------------------------------------------
get_case_config() {
    CASE_NAME="$1"
    EXPECTED=""
    SHORE_PATCH=""
    VEH_MOOS_PATCH=""
    VEH_BHV_PATCH=""

    if [ "$CASE_NAME" = "single_point_arrival_pass" ]; then
        EXPECTED="pass"
    elif [ "$CASE_NAME" = "endflag_pass" ]; then
        EXPECTED="pass"
    elif [ "$CASE_NAME" = "multi_point_sequence_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/multi-point-sequence-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "wptflag_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/wptflag-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/multi-point-sequence-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "repeat_once_cycle_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/cycleflag-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/repeat-once-cycle-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "repeat_forever_cycle_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/timer-cycle-no-done-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/repeat-forever-cycle-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "dynamic_points_update_pass" ]; then
        EXPECTED="pass"
        VEH_MOOS_PATCH="$HARNESS_DIR/dynamic-points-update-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/points-empty-vehicle.xbhv"
    elif [ "$CASE_NAME" = "newpt_update_pass" ]; then
        EXPECTED="pass"
        VEH_MOOS_PATCH="$HARNESS_DIR/newpt-update-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/points-empty-vehicle.xbhv"
    elif [ "$CASE_NAME" = "xpoints_update_pass" ]; then
        EXPECTED="pass"
        VEH_MOOS_PATCH="$HARNESS_DIR/xpoints-update-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/xpoints-update-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "capture_radius_large_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/capture-radius-large-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "capture_line_absolute_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/capture-line-absolute-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "slip_radius_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/slip-radius-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "order_reverse_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/order-reverse-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "currix_start_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/currix-start-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "lead_to_start_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/lead-to-start-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "lead_condition_pass" ]; then
        EXPECTED="pass"
        VEH_MOOS_PATCH="$HARNESS_DIR/lead-condition-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/lead-condition-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "end_spd_slowdown_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/end-spd-slowdown-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "speed_alt_update_pass" ]; then
        EXPECTED="pass"
        VEH_MOOS_PATCH="$HARNESS_DIR/speed-alt-update-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/speed-alt-update-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "reset_on_idle_pass" ]; then
        EXPECTED="pass"
        VEH_MOOS_PATCH="$HARNESS_DIR/reset-on-idle-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/reset-on-idle-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "empty_start_then_update_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/empty-start-then-update-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/empty-start-then-update-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/points-empty-vehicle.xbhv"
    elif [ "$CASE_NAME" = "wptflag_on_start_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/wptflag-on-start-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/wptflag-on-start-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "custom_status_vars_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/custom-status-vars-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/custom-status-vars-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/custom-status-vars-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "polygon_points_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/polygon-points-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "slow_speed_no_arrival_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/timer-no-done-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/slow-speed-no-arrival-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "no_points_timeout_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/timer-wpt-done-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/points-empty-vehicle.xbhv"
    elif [ "$CASE_NAME" = "malformed_update_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/timer-wpt-done-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/malformed-update-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/points-empty-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_xpoints_size_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/timer-wpt-done-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/bad-xpoints-size-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-xpoints-size-fail-vehicle.xbhv"
    else
        echo "$ME: Unknown case: [$CASE_NAME]"
        return 1
    fi
    return 0
}

#------------------------------------------------------------
#  Part 6: Apply case-specific nspatch overlays.
#------------------------------------------------------------
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

#------------------------------------------------------------
#  Part 7: Execute one case and append its summary line.
#------------------------------------------------------------
run_case() {
    local case_name="$1"
    local line
    local actual
    local status
    local launch_rc
    local shore_mport
    local veh_mport
    local shore_pshare
    local veh_pshare
    get_case_config "$case_name" || return 1

    vecho "Preparing case: $case_name"

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    stop_mission_apps "$MISSION_DIR"
    apply_case_patches || return 1
    : > results.txt

    XARGS="--max_time=$MAX_TIME --mmod=$case_name $TIME_WARP"
    if [ "$PORT_BASE_SET" = "yes" ]; then
        shore_mport=$PORT_BASE
        veh_mport=$((shore_mport + 1))
        shore_pshare=$((PORT_BASE + 1000))
        veh_pshare=$((shore_pshare + 1))
        XARGS="$XARGS --shore_mport=$shore_mport --veh_mport=$veh_mport --shore_pshare=$shore_pshare --veh_pshare=$veh_pshare"
    fi
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

    line=$(wait_for_result_line results.txt 24)
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

#------------------------------------------------------------
#  Part 8: Prepare and run one isolated case copy.
#------------------------------------------------------------
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

    shore_mport=$((PORT_BASE + case_idx*20))
    veh_mport=$((shore_mport + 1))
    shore_pshare=$((PORT_BASE + 1000 + case_idx*20))
    veh_pshare=$((shore_pshare + 1))

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

    line=$(wait_for_result_line "$case_dir/results.txt" 24)
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

#------------------------------------------------------------
#  Part 9: Select the case set, run the matrix, and report.
#------------------------------------------------------------
if [ "$CASE" != "" ]; then
    CASES="$CASE"
else
    CASES="single_point_arrival_pass endflag_pass multi_point_sequence_pass wptflag_pass repeat_once_cycle_pass repeat_forever_cycle_pass dynamic_points_update_pass newpt_update_pass xpoints_update_pass capture_radius_large_pass capture_line_absolute_pass slip_radius_pass order_reverse_pass currix_start_pass lead_to_start_pass lead_condition_pass end_spd_slowdown_pass speed_alt_update_pass reset_on_idle_pass empty_start_then_update_pass wptflag_on_start_pass custom_status_vars_pass polygon_points_pass slow_speed_no_arrival_pass no_points_timeout_fail malformed_update_fail bad_xpoints_size_fail"
fi

: > "$RESULTS_FILE"

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    for ONE_CASE in $CASES; do
        run_case "$ONE_CASE" || {
            echo "case=$ONE_CASE  case_result=error  expected=unknown  actual=script_error" >> "$RESULTS_FILE"
            ALL_OK="no"
            if [ "$JUST_MAKE" != "yes" ]; then
                break
            fi
        }
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_waypoint_XXXXXX")
    CASE_RESULT_DIR="$RUN_ROOT/case_results"
    mkdir -p "$CASE_RESULT_DIR"

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
    echo "$ME: Just_make complete for cases: $CASES"
    exit 0
fi

cat "$RESULTS_FILE"

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
