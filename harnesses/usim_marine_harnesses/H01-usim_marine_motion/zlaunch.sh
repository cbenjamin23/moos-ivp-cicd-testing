#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: May 2026
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
MAX_TIME="45"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="30000"
PORT_BASE_SET="no"
PORT_STRIDE="30"
KEEP_WORKDIRS="no"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/usim_marine_missions/usim_marine_motion"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
RUN_ROOT=""
CASE_ROW_DIR=""
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
        echo "  --gui              Launch with pMarineViewer if present"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=thrust_forward_pass"
        echo "  ./zlaunch.sh --jobs=4 --port_base=30000"
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
    EXPECTED=""
    SHORE_PATCH=""

    if [ "$CASE_NAME" = "thrust_forward_pass" ]; then
        EXPECTED="pass"
    elif [ "$CASE_NAME" = "rudder_turn_starboard_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/rudder-turn-starboard-pass.xmoos"
    elif [ "$CASE_NAME" = "differential_turn_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/differential-turn-pass.xmoos"
    elif [ "$CASE_NAME" = "start_pos_seed_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/start-pos-seed-pass.xmoos"
    elif [ "$CASE_NAME" = "max_speed_clamp_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/max-speed-clamp-pass.xmoos"
    elif [ "$CASE_NAME" = "acceleration_limit_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/acceleration-limit-pass.xmoos"
    elif [ "$CASE_NAME" = "drift_x_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/drift-x-pass.xmoos"
    elif [ "$CASE_NAME" = "rotate_speed_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/rotate-speed-pass.xmoos"
    elif [ "$CASE_NAME" = "sim_pause_holds_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/sim-pause-holds-pass.xmoos"
    elif [ "$CASE_NAME" = "reset_count_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/reset-count-pass.xmoos"
    elif [ "$CASE_NAME" = "disabled_nav_seed_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/disabled-nav-seed-pass.xmoos"
    elif [ "$CASE_NAME" = "reverse_thrust_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/reverse-thrust-pass.xmoos"
    elif [ "$CASE_NAME" = "elevator_dive_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/elevator-dive-pass.xmoos"
    elif [ "$CASE_NAME" = "runtime_turn_rate_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/runtime-turn-rate-pass.xmoos"
    elif [ "$CASE_NAME" = "obstacle_hit_stop_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/obstacle-hit-stop-pass.xmoos"
    elif [ "$CASE_NAME" = "current_y_alias_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/current-y-alias-pass.xmoos"
    elif [ "$CASE_NAME" = "drift_vector_mult_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/drift-vector-mult-pass.xmoos"
    elif [ "$CASE_NAME" = "water_depth_altitude_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/water-depth-altitude-pass.xmoos"
    elif [ "$CASE_NAME" = "thrust_map_interpolation_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/thrust-map-interpolation-pass.xmoos"
    elif [ "$CASE_NAME" = "max_deceleration_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/max-deceleration-pass.xmoos"
    elif [ "$CASE_NAME" = "embedded_pid_speed_factor_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/embedded-pid-speed-factor-pass.xmoos"
    elif [ "$CASE_NAME" = "drift_vector_add_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/drift-vector-add-pass.xmoos"
    elif [ "$CASE_NAME" = "runtime_buoyancy_rate_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/runtime-buoyancy-rate-pass.xmoos"
    elif [ "$CASE_NAME" = "rudder_slew_limit_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/rudder-slew-limit-pass.xmoos"
    elif [ "$CASE_NAME" = "turn_speed_map_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/turn-speed-map-pass.xmoos"
    elif [ "$CASE_NAME" = "wind_polar_speed_cap_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/wind-polar-speed-cap-pass.xmoos"
    elif [ "$CASE_NAME" = "dual_state_reset_nav_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/dual-state-reset-nav-pass.xmoos"
    elif [ "$CASE_NAME" = "wormhole_transport_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/wormhole-transport-pass.xmoos"
    elif [ "$CASE_NAME" = "runtime_wind_conditions_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/runtime-wind-conditions-pass.xmoos"
    elif [ "$CASE_NAME" = "runtime_water_depth_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/runtime-water-depth-pass.xmoos"
    elif [ "$CASE_NAME" = "runtime_thrust_mode_reverse_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/runtime-thrust-mode-reverse-pass.xmoos"
    elif [ "$CASE_NAME" = "max_depth_rate_limit_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/max-depth-rate-limit-pass.xmoos"
    elif [ "$CASE_NAME" = "positive_buoyancy_surface_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/positive-buoyancy-surface-pass.xmoos"
    elif [ "$CASE_NAME" = "embedded_pid_depth_control_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/embedded-pid-depth-control-pass.xmoos"
    elif [ "$CASE_NAME" = "buoyancy_control_report_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/buoyancy-control-report-pass.xmoos"
    elif [ "$CASE_NAME" = "trim_control_report_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/trim-control-report-pass.xmoos"
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

emit_case_row() {
    local case_name="$1"
    local status="$2"
    local expected="$3"
    local actual="$4"
    shift 4
    local line="$1"
    shift || true
    local grade

    grade=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    line=$(echo "$line" | sed 's/grade=[^, ]*[[:space:]]*//')

    if [ "$grade" != "" ]; then
        echo "case=$case_name  grade=$grade  $line  $*"
    elif [ "$status" = "success" ]; then
        echo "case=$case_name  grade=fail  reason=missing_result  $line  $*"
    else
        echo "case=$case_name  grade=fail  reason=$status  $line  $*"
    fi
}

run_case() {
    local case_name="$1"
    local case_idx="${RUN_CASE_IDX:-0}"
    RUN_CASE_IDX=$((case_idx + 1))
    local shore_mport
    local shore_pshare
    local line
    local actual
    local status
    local xargs

    get_case_config "$case_name" || return 1

    vecho "Preparing case: $case_name"

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    stop_mission_apps "$MISSION_DIR"
    apply_case_patches || return 1
    : > results.txt

    xargs="--max_time=$MAX_TIME --mmod=$case_name $TIME_WARP"
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

    if [ "$JUST_MAKE" = "yes" ]; then
        cd "$HARNESS_DIR"
        return 0
    fi

    line=$(wait_for_result_line results.txt 24)
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="success"
    if [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
        ALL_OK="no"
    fi

    emit_case_row "$case_name" "$status" "$EXPECTED" "$actual" "$line" >> "$RESULTS_FILE"
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
    local shore_xfile="$case_dir/meta_shoreside.moosx"

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$shore_stem" "$SHORE_PATCH" --targ="$shore_xfile"
    fi
}

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    local case_tag
    local case_dir
    local case_row_file
    local shore_mport
    local shore_pshare
    local line
    local actual
    local status
    local xargs
    local launch_rc

    get_case_config "$case_name" || return 1

    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    case_row_file="$CASE_ROW_DIR/${case_tag}.txt"

    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  grade=fail  reason=script_error" > "$case_row_file"
        return 1
    }

    case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    shore_mport=$((case_base + 0))
    shore_pshare=$((case_base + 10))

    (
        cd "$case_dir"
        : > results.txt
        xargs="--max_time=$MAX_TIME --mmod=$case_name --shore_mport=$shore_mport --shore_pshare=$shore_pshare $TIME_WARP"
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
        echo "case=$case_name  grade=fail  reason=script_error" > "$case_row_file"
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

    emit_case_row "$case_name" "$status" "$EXPECTED" "$actual" "$line" > "$case_row_file"

    if [ "$status" = "success" ]; then
        return 0
    fi
    return 1
}

#------------------------------------------------------------
#  Part 7: Select the case set, run, and report.
#------------------------------------------------------------
trap cleanup EXIT

ALL_CASES=(
    thrust_forward_pass
    rudder_turn_starboard_pass
    differential_turn_pass
    start_pos_seed_pass
    max_speed_clamp_pass
    acceleration_limit_pass
    drift_x_pass
    rotate_speed_pass
    sim_pause_holds_pass
    reset_count_pass
    disabled_nav_seed_pass
    reverse_thrust_pass
    elevator_dive_pass
    runtime_turn_rate_pass
    obstacle_hit_stop_pass
    current_y_alias_pass
    drift_vector_mult_pass
    water_depth_altitude_pass
    thrust_map_interpolation_pass
    max_deceleration_pass
    embedded_pid_speed_factor_pass
    drift_vector_add_pass
    runtime_buoyancy_rate_pass
    rudder_slew_limit_pass
    turn_speed_map_pass
    wind_polar_speed_cap_pass
    dual_state_reset_nav_pass
    wormhole_transport_pass
    runtime_wind_conditions_pass
    runtime_water_depth_pass
    runtime_thrust_mode_reverse_pass
    max_depth_rate_limit_pass
    positive_buoyancy_surface_pass
    embedded_pid_depth_control_pass
    buoyancy_control_report_pass
    trim_control_report_pass
)

RUN_CASES=("${ALL_CASES[@]}")
if [ "$CASE" != "" ]; then
    RUN_CASES=("$CASE")
fi

: > "$RESULTS_FILE"

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    for ONE_CASE in "${RUN_CASES[@]}"; do
        run_case "$ONE_CASE" || {
            echo "case=$ONE_CASE  grade=fail  reason=script_error" >> "$RESULTS_FILE"
            ALL_OK="no"
            if [ "$JUST_MAKE" != "yes" ]; then
                break
            fi
        }
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_usim_marine_XXXXXX")
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
                if grep -Eq '(^|[[:space:]])grade=fail([[:space:]]|$)' "$ONE_FILE"; then
                    ALL_OK="no"
                fi
                case_tag="found"
                break
            fi
        done
        if [ "$case_tag" = "" ]; then
            echo "case=$ONE_CASE  grade=fail  reason=missing_result" >> "$RESULTS_FILE"
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
