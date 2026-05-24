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
MAX_TIME="80"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="26000"
PORT_BASE_SET="no"
PORT_STRIDE="30"
KEEP_WORKDIRS="no"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/pid_missions/pid_motion"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
RUN_ROOT=""
CASE_ROW_DIR=""
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
VEHICLE_STEM="$MISSION_DIR/meta_vehicle.moos"
VEHICLE_BHV_STEM="$MISSION_DIR/meta_vehicle.bhv"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
VEHICLE_XFILE="$MISSION_DIR/meta_vehicle.moosx"
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
        echo "  --port_base=<n>    Base port for per-case wave blocks"
        echo "  --keep_workdirs    Keep temp mission copies in wave mode"
        echo "  --gui              Launch with pMarineViewer"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=baseline_transit_pass"
        echo "  ./zlaunch.sh --case=manual_override_fail"
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

grade_from_line() {
    echo "$1" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'
}

format_case_row() {
    local case_name="$1"
    local line="$2"
    local launch_rc="${3:-0}"
    local grade

    if [ "$launch_rc" != 0 ]; then
        echo "case=$case_name  grade=fail  reason=launch_error  launch_rc=$launch_rc"
        return
    fi

    line=$(echo "$line" | sed 's/^[[:space:]]*//')
    grade=$(grade_from_line "$line")
    if [ "$grade" = "" ]; then
        echo "case=$case_name  grade=fail  reason=missing_result"
        return
    fi

    line=$(echo "$line" | sed 's/grade=[^, ]*[[:space:]]*//')


    echo "case=$case_name  grade=$grade  $line"
}

case_row_passed() {
    local line="$1"
    local grade
    grade=$(grade_from_line "$line")
    [ "$grade" = "pass" ]
}

#------------------------------------------------------------
#  Part 4: Define cleanup and patch application helpers.
#------------------------------------------------------------
remove_tree() {
    local targ="$1"
    if [ "$targ" != "" ] && [ -d "$targ" ]; then
        rm -rf "$targ"
    fi
}

clear_xfiles() {
    rm -f "$SHORE_XFILE" "$VEHICLE_XFILE" "$VEHICLE_BHV_XFILE"
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

get_case_config() {
    CASE_NAME="$1"
    SHORE_PATCH=""
    VEH_PATCH=""
    VEH_BHV_PATCH=""

    if [ "$CASE_NAME" = "baseline_transit_pass" ]; then
        :
    elif [ "$CASE_NAME" = "turn_recover_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/turn-recover-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/turn-recover-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "hard_turn_recover_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/hard-turn-recover-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/hard-turn-recover-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "rudder_bias_recover_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/rudder-bias-recover-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/rudder-bias-recover-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "speed_pid_transit_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/speed-pid-transit-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/speed-pid-transit-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_speed_factor_update_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/runtime-speed-factor-update-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/runtime-speed-factor-update-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "depth_step_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/depth-step-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/depth-step-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/depth-step-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "override_release_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/override-release-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "low_rudder_authority_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/low-rudder-authority-fail-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/low-rudder-authority-fail-vehicle.xmoos"
    elif [ "$CASE_NAME" = "low_thrust_authority_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/low-thrust-authority-fail-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/low-thrust-authority-fail-vehicle.xmoos"
    elif [ "$CASE_NAME" = "low_elevator_authority_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/low-elevator-authority-fail-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/low-elevator-authority-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/low-elevator-authority-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "depth_control_disabled_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/depth-control-disabled-fail-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/depth-control-disabled-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/depth-control-disabled-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "manual_override_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/manual-override-fail-shoreside.xmoos"
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

    if [ "$VEH_PATCH" != "" ]; then
        nspatch --stem="$VEHICLE_STEM" "$VEH_PATCH" --targ="$VEHICLE_XFILE"
    fi

    if [ "$VEH_BHV_PATCH" != "" ]; then
        nspatch --stem="$VEHICLE_BHV_STEM" "$VEH_BHV_PATCH" --targ="$VEHICLE_BHV_XFILE"
    fi
}

#------------------------------------------------------------
#  Part 5: Run one case in the shared stem mission directory.
#------------------------------------------------------------
run_case() {
    local case_name="$1"
    local case_idx="${RUN_CASE_IDX:-0}"
    RUN_CASE_IDX=$((case_idx + 1))
    local shore_mport
    local veh_mport
    local shore_pshare
    local veh_pshare
    local line
    local result_line
    local launch_rc
    local case_base
    get_case_config "$case_name" || return 1

    vecho "Preparing case: $case_name"

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    stop_mission_apps "$MISSION_DIR"
    apply_case_patches || {
        echo "case=$case_name  grade=fail  reason=prepare_error" >> "$RESULTS_FILE"
        cd "$HARNESS_DIR"
        return 1
    }
    : > results.txt

    XARGS="--max_time=$MAX_TIME --mmod=$case_name $TIME_WARP"
    if [ "$PORT_BASE_SET" = "yes" ]; then
        case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
        shore_mport=$((case_base + 0))
        veh_mport=$((case_base + 1))
        shore_pshare=$((case_base + 10))
        veh_pshare=$((case_base + 11))
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
            echo "case=$case_name  grade=pass  reason=just_make" >> "$RESULTS_FILE"
            return 0
        fi
        echo "case=$case_name  grade=fail  reason=launch_error  launch_rc=$launch_rc" >> "$RESULTS_FILE"
        return 1
    fi

    if [ "$launch_rc" = 0 ]; then
        line=$(wait_for_result_line results.txt 24)
    else
        line=""
    fi
    result_line=$(format_case_row "$case_name" "$line" "$launch_rc")

    echo "$result_line" >> "$RESULTS_FILE"
    if ! case_row_passed "$result_line"; then
        ALL_OK="no"
        cd "$HARNESS_DIR"
        return 1
    fi
    cd "$HARNESS_DIR"
}

#------------------------------------------------------------
#  Part 6: Prepare and run one isolated case copy.
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
    local veh_stem="$case_dir/meta_vehicle.moos"
    local veh_bhv_stem="$case_dir/meta_vehicle.bhv"
    local shore_xfile="$case_dir/meta_shoreside.moosx"
    local veh_xfile="$case_dir/meta_vehicle.moosx"
    local veh_bhv_xfile="$case_dir/meta_vehicle.bhvx"

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$shore_stem" "$SHORE_PATCH" --targ="$shore_xfile"
    fi

    if [ "$VEH_PATCH" != "" ]; then
        nspatch --stem="$veh_stem" "$VEH_PATCH" --targ="$veh_xfile"
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
    local case_row_file
    local shore_mport
    local veh_mport
    local shore_pshare
    local veh_pshare
    local line
    local result_line
    local xargs
    local launch_rc

    get_case_config "$case_name" || return 1

    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    case_row_file="$CASE_ROW_DIR/${case_tag}.txt"

    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  grade=fail  reason=prepare_error" > "$case_row_file"
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
            echo "case=$case_name  grade=pass  reason=just_make" > "$case_row_file"
            return 0
        fi
        echo "case=$case_name  grade=fail  reason=launch_error  launch_rc=$launch_rc" > "$case_row_file"
        return 1
    fi

    if [ "$launch_rc" = 0 ]; then
        line=$(wait_for_result_line "$case_dir/results.txt" 24)
    else
        line=""
    fi
    result_line=$(format_case_row "$case_name" "$line" "$launch_rc")

    echo "$result_line" > "$case_row_file"

    if case_row_passed "$result_line"; then
        return 0
    fi
    return 1
}

#------------------------------------------------------------
#  Part 7: Select the case set, run, and report.
#------------------------------------------------------------
trap cleanup EXIT

ALL_CASES=(
    baseline_transit_pass
    turn_recover_pass
    hard_turn_recover_pass
    rudder_bias_recover_pass
    speed_pid_transit_pass
    runtime_speed_factor_update_pass
    depth_step_pass
    override_release_pass
    low_rudder_authority_fail
    low_thrust_authority_fail
    low_elevator_authority_fail
    depth_control_disabled_fail
    manual_override_fail
)

RUN_CASES=("${ALL_CASES[@]}")
if [ "$CASE" != "" ]; then
    RUN_CASES=("$CASE")
fi

: > "$RESULTS_FILE"

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    for ONE_CASE in "${RUN_CASES[@]}"; do
        run_case "$ONE_CASE" || {
            ALL_OK="no"
            if [ "$JUST_MAKE" != "yes" ]; then
                break
            fi
        }
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_pid_motion_XXXXXX")
    CASE_ROW_DIR="$RUN_ROOT/case_rows"
    mkdir -p "$CASE_ROW_DIR"

    TOTAL_CASES=${#RUN_CASES[@]}
    IDX=0

    while [ "$IDX" -lt "$TOTAL_CASES" ]; do
        PIDS=""
        COUNT=0

        while [ "$COUNT" -lt "$JOBS" ] && [ "$IDX" -lt "$TOTAL_CASES" ]; do
            run_case_isolated "$IDX" "${RUN_CASES[$IDX]}" &
            PIDS="$PIDS $!"
            IDX=$((IDX+1))
            COUNT=$((COUNT+1))
        done

        for PID in $PIDS; do
            wait "$PID" || true
        done

        stop_mission_apps "$RUN_ROOT"
    done

    for ONE_CASE in "${RUN_CASES[@]}"; do
        case_tag=""
        for ONE_FILE in "$CASE_ROW_DIR"/*.txt; do
            if grep -q "case=$ONE_CASE  " "$ONE_FILE" 2>/dev/null; then
                cat "$ONE_FILE" >> "$RESULTS_FILE"
                line=`tail -n 1 "$ONE_FILE"`
                if ! case_row_passed "$line"; then
                    ALL_OK="no"
                fi
                case_tag="found"
                break
            fi
        done
        if [ "$case_tag" = "" ]; then
            echo "case=$ONE_CASE  grade=fail  reason=missing_result_file" >> "$RESULTS_FILE"
            ALL_OK="no"
        fi
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
