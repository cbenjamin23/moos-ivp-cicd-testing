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

#------------------------------------------------------------
#  Part 1: Set global variable default values.
#------------------------------------------------------------
ME=`basename "$0"`
CMD_ARGS=""
VERBOSE=""
JUST_MAKE=""
TIME_WARP="10"
MAX_TIME="200"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="15400"
PORT_STRIDE="30"
KEEP_WORKDIRS="no"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/legrun_behavior_missions/legrun_behavior_motion"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
RUN_ROOT=""
CASE_ROW_DIR=""
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
#  Part 2: Check for and handle command-line arguments.
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
        echo "  ./zlaunch.sh --case=baseline_cycle_pass"
        echo "  ./zlaunch.sh --jobs=4 --port_base=15400"
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

#------------------------------------------------------------
#  Part 3: Utility functions.
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
#  Part 4: Case mapping.
#------------------------------------------------------------
get_case_config() {
    CASE_NAME="$1"
    SHORE_PATCH=""
    VEH_MOOS_PATCH=""
    VEH_BHV_PATCH=""

    if [ "$CASE_NAME" = "baseline_cycle_pass" ]; then
        :
    elif [ "$CASE_NAME" = "finite_complete_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/finite-complete-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/finite-complete-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "colon_leg_format_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/colon-leg-format-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "vertex_points_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/vertex-points-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "init_close_turn_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/init-close-turn-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "init_far_turn_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/init-far-turn-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/init-far-turn-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "turn_rad_alias_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/turn-rad-alias-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "turn_bias_ext_gap_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/turn-bias-ext-gap-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "individual_turn_params_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/individual-turn-params-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "turn_rad_min_clip_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/turn-rad-min-clip-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "capture_radius_alias_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/capture-radius-alias-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "capture_line_absolute_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/capture-line-absolute-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "no_capture_line_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/no-capture-line-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "leg_speed_schedule_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/leg-speed-schedule-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "leg_speed_onturn_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/leg-speed-onturn-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "leg_speed_count_repeat_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/leg-speed-count-repeat-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "runtime_length_angle_update_pass" ]; then
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-length-angle-update-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_turn_radius_update_pass" ]; then
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-turn-radius-update-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/runtime-compact-leg-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "runtime_speed_replace_reset_pass" ]; then
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-speed-replace-reset-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/runtime-compact-leg-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "runtime_shift_point_pass" ]; then
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-shift-point-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_turn_bias_gap_update_pass" ]; then
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-turn-bias-gap-update-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "flag_aliases_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/flag-aliases-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/flag-aliases-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/flag-aliases-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "mid_pct_late_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/mid-pct-late-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "patience_clip_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/patience-clip-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_leg_config_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-malconfig-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-leg-config-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_speed_schedule_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-malconfig-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-speed-schedule-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_init_mode_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-malconfig-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-init-mode-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_mid_pct_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-malconfig-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-mid-pct-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_turn_radius_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-malconfig-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-turn-radius-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_turn_ext_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-malconfig-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-turn-ext-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_turn_dir_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-malconfig-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-turn-dir-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_speed_high_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-malconfig-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-speed-high-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_speed_count_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-malconfig-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-speed-count-fail-vehicle.xbhv"
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

    if [ "$VEH_MOOS_PATCH" != "" ]; then
        nspatch --stem="$VEHICLE_MOOS_STEM" "$VEH_MOOS_PATCH" --targ="$VEHICLE_MOOS_XFILE"
    fi

    if [ "$VEH_BHV_PATCH" != "" ]; then
        nspatch --stem="$VEHICLE_BHV_STEM" "$VEH_BHV_PATCH" --targ="$VEHICLE_BHV_XFILE"
    fi
}

#------------------------------------------------------------
#  Part 5: Case runners.
#------------------------------------------------------------
run_case() {
    local case_idx="$1"
    local case_name="$2"
    local line
    local result_line
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
    apply_case_patches || {
        echo "case=$case_name  grade=fail  reason=prepare_error" >> "$RESULTS_FILE"
        cd "$HARNESS_DIR"
        return 1
    }
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
            echo "case=$case_name  grade=pass  reason=just_make" >> "$RESULTS_FILE"
            return 0
        fi
        echo "case=$case_name  grade=fail  reason=launch_error  launch_rc=$launch_rc" >> "$RESULTS_FILE"
        return 1
    fi

    if [ "$launch_rc" = 0 ]; then
        sleep 1
        line=$(wait_for_result_line results.txt 60)
    else
        line=""
    fi
    result_line=$(format_case_row "$case_name" "$line" "$launch_rc")
    echo "$result_line" >> "$RESULTS_FILE"
    if ! case_row_passed "$result_line"; then
        ALL_OK="no"
    fi

    stop_mission_apps "$MISSION_DIR"
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
    local case_row_file
    local shore_mport
    local veh_mport
    local shore_pshare
    local veh_pshare
    local line
    local result_line
    local xargs
    local launch_rc

    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    case_row_file="$CASE_ROW_DIR/${case_tag}.txt"

    get_case_config "$case_name" || {
        echo "case=$case_name  grade=fail  reason=case_config_error" > "$case_row_file"
        return 1
    }

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
        line=$(wait_for_result_line "$case_dir/results.txt" 60)
    else
        line=""
    fi
    result_line=$(format_case_row "$case_name" "$line" "$launch_rc")
    echo "$result_line" > "$case_row_file"

    stop_mission_apps "$case_dir"

    if case_row_passed "$result_line"; then
        return 0
    fi
    return 1
}

trap cleanup EXIT

#------------------------------------------------------------
#  Part 6: Select the cases, run, and report.
#------------------------------------------------------------
ALL_CASES=(
    baseline_cycle_pass
    finite_complete_pass
    colon_leg_format_pass
    vertex_points_pass
    init_close_turn_pass
    init_far_turn_pass
    turn_rad_alias_pass
    turn_bias_ext_gap_pass
    individual_turn_params_pass
    turn_rad_min_clip_pass
    capture_radius_alias_pass
    capture_line_absolute_pass
    no_capture_line_pass
    leg_speed_schedule_pass
    leg_speed_onturn_pass
    leg_speed_count_repeat_pass
    runtime_length_angle_update_pass
    runtime_turn_radius_update_pass
    runtime_speed_replace_reset_pass
    runtime_shift_point_pass
    runtime_turn_bias_gap_update_pass
    flag_aliases_pass
    mid_pct_late_pass
    patience_clip_pass
    bad_leg_config_fail
    bad_speed_schedule_fail
    bad_init_mode_fail
    bad_mid_pct_fail
    bad_turn_radius_fail
    bad_turn_ext_fail
    bad_turn_dir_fail
    bad_speed_high_fail
    bad_speed_count_fail
)

RUN_CASES=("${ALL_CASES[@]}")
if [ "$CASE" != "" ]; then
    RUN_CASES=("$CASE")
fi

: > "$RESULTS_FILE"

if [ "$JOBS" = "1" ]; then
    idx=0
    for case_name in "${RUN_CASES[@]}"; do
        run_case "$idx" "$case_name"
        idx=$((idx + 1))
    done
else
    RUN_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/legrun-harness.XXXXXX")"
    CASE_ROW_DIR="$RUN_ROOT/case_rows"
    mkdir -p "$CASE_ROW_DIR"

    total=${#RUN_CASES[@]}
    idx=0
    while [ "$idx" -lt "$total" ]; do
        running=0
        pids=()
        while [ "$running" -lt "$JOBS" ] && [ "$idx" -lt "$total" ]; do
            run_case_isolated "$idx" "${RUN_CASES[$idx]}" &
            pids+=($!)
            running=$((running + 1))
            idx=$((idx + 1))
        done

        for pid in "${pids[@]}"; do
            if ! wait "$pid"; then
                ALL_OK="no"
            fi
        done
    done

    for result in "$CASE_ROW_DIR"/*.txt; do
        [ -f "$result" ] || continue
        cat "$result" >> "$RESULTS_FILE"
    done
fi

clear_xfiles

echo "----------------------------------------"
cat "$RESULTS_FILE"
echo "----------------------------------------"

if grep -vq "grade=pass" "$RESULTS_FILE"; then
    ALL_OK="no"
fi

if [ "$ALL_OK" = "yes" ]; then
    echo "$ME: all cases matched expectations"
    exit 0
fi

echo "$ME: one or more cases failed"
exit 1
