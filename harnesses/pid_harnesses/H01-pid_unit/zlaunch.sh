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
MAX_TIME="35"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="12000"
PORT_BASE_SET="no"
PORT_STRIDE="20"
KEEP_WORKDIRS="no"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/pid_missions/pid_unit"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
RUN_ROOT=""
CASE_RESULT_DIR=""
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"

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
        echo "  --gui              Accepted for wrapper parity"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=rudder_starboard_pass"
        echo "  ./zlaunch.sh --case=depth_elevator_pass"
        echo "  ./zlaunch.sh --jobs=4 --port_base=22000"
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
#  Part 4: Define cleanup and patch application helpers.
#------------------------------------------------------------
remove_tree() {
    local targ="$1"
    if [ "$targ" != "" ] && [ -d "$targ" ]; then
        rm -rf "$targ"
    fi
}

clear_xfiles() {
    rm -f "$SHORE_XFILE"
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
    EXPECTED="pass"
    SHORE_PATCH=""

    if [ "$CASE_NAME" = "rudder_starboard_pass" ]; then
        true
    elif [ "$CASE_NAME" = "rudder_port_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/rudder-port-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "heading_wrap_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/heading-wrap-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "speed_factor_update_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/speed-factor-update-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "speed_pid_control_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/speed-pid-control-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "max_thrust_limit_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/max-thrust-limit-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "depth_elevator_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/depth-elevator-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "manual_override_zero_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/manual-override-zero-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "stale_nav_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/stale-nav-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "nav_yaw_used_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/nav-yaw-used-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "ignore_nav_yaw_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/ignore-nav-yaw-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "heading_debug_saturation_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/heading-debug-saturation-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "speed_zero_allstop_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/speed-zero-allstop-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "speed_negative_allstop_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/speed-negative-allstop-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "output_suffix_alt_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/output-suffix-alt-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "manual_overide_alias_zero_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/manual-overide-alias-zero-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "override_release_needs_fresh_mail_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/override-release-needs-fresh-mail-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "override_recover_fresh_mail_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/override-recover-fresh-mail-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "stale_helm_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/stale-helm-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "stale_recover_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/stale-recover-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "depth_negative_elevator_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/depth-negative-elevator-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "depth_debug_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/depth-debug-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "max_elevator_limit_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/max-elevator-limit-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "speed_debug_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/speed-debug-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "nav_heading_after_yaw_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/nav-heading-after-yaw-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "simulation_mode_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/simulation-mode-fail-shoreside.xmoos"
    elif [ "$CASE_NAME" = "thrust_cap_runtime_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/thrust-cap-runtime-fail-shoreside.xmoos"
    elif [ "$CASE_NAME" = "heading_wrap_negative_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/heading-wrap-negative-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "nav_heading_normalize_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/nav-heading-normalize-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "runtime_speed_factor_zero_pid_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/runtime-speed-factor-zero-pid-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "runtime_speed_factor_negative_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/runtime-speed-factor-negative-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "missing_desired_speed_zero_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/missing-desired-speed-zero-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "missing_nav_speed_zero_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/missing-nav-speed-zero-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "depth_missing_pitch_zero_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/depth-missing-pitch-zero-pass-shoreside.xmoos"
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

#------------------------------------------------------------
#  Part 5: Run one case in the shared stem mission directory.
#------------------------------------------------------------
run_case() {
    local case_name="$1"
    local case_idx="${RUN_CASE_IDX:-0}"
    RUN_CASE_IDX=$((case_idx + 1))
    local shore_mport
    local shore_pshare
    local case_base
    local xargs
    local line
    local actual
    local status
    local launch_rc

    get_case_config "$case_name" || return 1

    vecho "Preparing case: $case_name"

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    stop_mission_apps "$MISSION_DIR"
    apply_case_patches || return 1
    : > results.txt

    xargs="--max_time=$MAX_TIME $TIME_WARP"
    if [ "$PORT_BASE_SET" = "yes" ]; then
        case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
        shore_mport=$((case_base + 0))
        shore_pshare=$((case_base + 10))
        xargs="$xargs --shore_mport=$shore_mport --shore_pshare=$shore_pshare"
    fi
    if [ "$NOGUI" != "" ]; then
        xargs="$xargs $NOGUI"
    fi
    if [ "$JUST_MAKE" = "yes" ]; then
        xargs="$xargs --just_make"
    fi

    vecho "Running case [$case_name] with xlaunch args: $xargs"
    xlaunch.sh $xargs
    launch_rc=$?

    if [ "$JUST_MAKE" = "yes" ]; then
        cd "$HARNESS_DIR"
        return "$launch_rc"
    fi

    line=$(wait_for_result_line results.txt 24)
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="success"
    if [ "$launch_rc" != 0 ] || [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
        ALL_OK="no"
    fi

    echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  $line" >> "$RESULTS_FILE"
    cd "$HARNESS_DIR"
}

#------------------------------------------------------------
#  Part 6: Prepare and run one isolated case copy.
#------------------------------------------------------------
prepare_case_dir() {
    local case_dir="$1"
    local shore_stem="$case_dir/meta_shoreside.moos"
    local shore_xfile="$case_dir/meta_shoreside.moosx"

    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
    )

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$shore_stem" "$SHORE_PATCH" --targ="$shore_xfile"
    fi
}

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    local case_tag
    local case_dir
    local case_result_file
    local shore_mport
    local shore_pshare
    local case_base
    local line
    local actual
    local status
    local xargs
    local launch_rc

    get_case_config "$case_name" || return 1

    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    case_result_file="$CASE_RESULT_DIR/${case_tag}.txt"

    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  case_result=error  expected=$EXPECTED  actual=script_error" > "$case_result_file"
        return 1
    }

    case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    shore_mport=$((case_base + 0))
    shore_pshare=$((case_base + 10))

    (
        cd "$case_dir"
        : > results.txt
        xargs="--max_time=$MAX_TIME --shore_mport=$shore_mport --shore_pshare=$shore_pshare $TIME_WARP"
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
    if [ "$launch_rc" != 0 ] || [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
    fi

    echo "case=$case_name  case_result=$status  expected=$EXPECTED  actual=$actual  $line" > "$case_result_file"

    if [ "$status" = "success" ]; then
        return 0
    fi
    return 1
}

#------------------------------------------------------------
#  Part 7: Validate the mission path, select the case set,
#          run the matrix, and report.
#------------------------------------------------------------
if [ ! -d "$MISSION_DIR" ]; then
    echo "$ME: Mission dir not found: [$MISSION_DIR]"
    exit 1
fi

trap cleanup EXIT

ALL_CASES=(
    rudder_starboard_pass
    rudder_port_pass
    heading_wrap_pass
    speed_factor_update_pass
    speed_pid_control_pass
    max_thrust_limit_pass
    depth_elevator_pass
    manual_override_zero_pass
    stale_nav_pass
    nav_yaw_used_pass
    ignore_nav_yaw_pass
    heading_debug_saturation_pass
    speed_zero_allstop_pass
    speed_negative_allstop_pass
    output_suffix_alt_pass
    manual_overide_alias_zero_pass
    override_release_needs_fresh_mail_pass
    override_recover_fresh_mail_pass
    stale_helm_pass
    stale_recover_pass
    depth_negative_elevator_pass
    depth_debug_pass
    max_elevator_limit_pass
    speed_debug_pass
    nav_heading_after_yaw_pass
    simulation_mode_fail
    thrust_cap_runtime_fail
    heading_wrap_negative_pass
    nav_heading_normalize_pass
    runtime_speed_factor_zero_pid_pass
    runtime_speed_factor_negative_pass
    missing_desired_speed_zero_pass
    missing_nav_speed_zero_pass
    depth_missing_pitch_zero_pass
)

RUN_CASES=("${ALL_CASES[@]}")
if [ "$CASE" != "" ]; then
    RUN_CASES=("$CASE")
fi

: > "$RESULTS_FILE"

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    for case_name in "${RUN_CASES[@]}"; do
        run_case "$case_name" || {
            echo "case=$case_name  case_result=error  expected=unknown  actual=script_error" >> "$RESULTS_FILE"
            ALL_OK="no"
            if [ "$JUST_MAKE" != "yes" ]; then
                break
            fi
        }
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_pid_unit_XXXXXX")
    CASE_RESULT_DIR="$RUN_ROOT/case_results"
    mkdir -p "$CASE_RESULT_DIR"

    case_index=0
    remaining_cases="${RUN_CASES[*]}"
    while [ "$remaining_cases" != "" ]; do
        launched=0
        pids=""
        wave_cases=""
        next_remaining=""

        for one_case in $remaining_cases; do
            if [ "$launched" -lt "$JOBS" ]; then
                run_case_isolated "$case_index" "$one_case" &
                pids="$pids $!"
                wave_cases="$wave_cases $(printf "%03d_%s" "$case_index" "$one_case")"
                case_index=$((case_index + 1))
                launched=$((launched + 1))
            else
                next_remaining="$next_remaining $one_case"
            fi
        done

        for pid in $pids; do
            wait "$pid" || ALL_OK="no"
        done

        for case_tag in $wave_cases; do
            if [ -f "$CASE_RESULT_DIR/${case_tag}.txt" ]; then
                cat "$CASE_RESULT_DIR/${case_tag}.txt" >> "$RESULTS_FILE"
                echo >> "$RESULTS_FILE"
                line=`tail -n 2 "$RESULTS_FILE" | head -n 1`
                case_result=`echo "$line" | sed -n 's/.*case_result=\([^ ]*\).*/\1/p'`
                if [ "$case_result" != "success" ]; then
                    ALL_OK="no"
                fi
            else
                ALL_OK="no"
            fi
        done

        stop_mission_apps "$RUN_ROOT"
        remaining_cases="$next_remaining"
    done
fi

if [ "$ALL_OK" != "yes" ]; then
    exit 1
fi

exit 0
