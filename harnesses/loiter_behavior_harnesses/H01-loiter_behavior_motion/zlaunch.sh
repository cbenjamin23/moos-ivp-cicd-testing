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
PORT_STRIDE="30"
KEEP_WORKDIRS="no"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/loiter_behavior_missions/loiter_behavior_motion"
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
        echo "  --port_stride=<n>  Port spacing between per-case blocks"
        echo "  --keep_workdirs    Keep temp mission copies in wave mode"
        echo "  --gui              Launch with pMarineViewer"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=radial_clockwise_pass"
        echo "  ./zlaunch.sh --case=center_assign_xy_pass"
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
    elif [ "${ARGI:0:14}" = "--port_stride=" ]; then
        PORT_STRIDE="${ARGI#--port_stride=*}"
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

if ! echo "$PORT_STRIDE" | grep -Eq '^[0-9]+$' || [ "$PORT_STRIDE" -lt 12 ]; then
    echo "$ME: Bad value for --port_stride: [$PORT_STRIDE]"
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
#  Part 5: Determine the patch files for one named case.
#------------------------------------------------------------
get_case_config() {
    CASE_NAME="$1"
    SHORE_PATCH=""
    VEH_MOOS_PATCH=""
    VEH_BHV_PATCH=""

    if [ "$CASE_NAME" = "radial_clockwise_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-late-shoreside.xmoos"
    elif [ "$CASE_NAME" = "radial_counterclockwise_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/radial-counterclockwise-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "clockwise_best_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/clockwise-best-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "polygon_box_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/polygon-box-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "triangle_polygon_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/triangle-polygon-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "start_inside_loiter_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/start-inside-loiter-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "acquire_from_far_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/acquire-from-far-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "early_acquire_mode_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/early-acquire-mode-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/acquire-from-far-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "center_activate_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/center-activate-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "capture_radius_large_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/capture-radius-large-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "slip_radius_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/slip-radius-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "speed_alt_update_pass" ]; then
        VEH_MOOS_PATCH="$HARNESS_DIR/speed-alt-update-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/speed-alt-update-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "use_alt_speed_static_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/use-alt-speed-static-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "center_assign_xy_pass" ]; then
        VEH_MOOS_PATCH="$HARNESS_DIR/center-assign-xy-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "center_assign_pair_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/center-assign-pair-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/center-assign-pair-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/center-assign-pair-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "xcenter_ycenter_update_pass" ]; then
        VEH_MOOS_PATCH="$HARNESS_DIR/xcenter-ycenter-update-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "mod_poly_rad_expand_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-late-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/mod-poly-rad-expand-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "mod_poly_rad_shrink_pass" ]; then
        VEH_MOOS_PATCH="$HARNESS_DIR/mod-poly-rad-shrink-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "slingshot_bearing_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/slingshot-bearing-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/slingshot-bearing-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "post_suffix_outputs_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/post-suffix-outputs-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/post-suffix-outputs-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/post-suffix-outputs-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "eta_output_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/eta-output-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "ipf_zaic_spd_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/ipf-zaic-spd-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "slow_speed_acquire_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/slow-speed-acquire-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/slow-speed-acquire-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "empty_polygon_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/helm-malconfig-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/empty-polygon-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_polygon_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/helm-malconfig-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-polygon-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_update_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/bad-update-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/bad-update-fail-vehicle.xmoos"
    elif [ "$CASE_NAME" = "bad_clockwise_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/helm-malconfig-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-clockwise-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_use_alt_speed_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/helm-malconfig-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-use-alt-speed-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_patience_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/helm-malconfig-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-patience-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_capture_radius_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/helm-malconfig-fail-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/helm-malconfig-fail-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-capture-radius-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "center_bad_update_recover_pass" ]; then
        VEH_MOOS_PATCH="$HARNESS_DIR/center-bad-update-recover-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "spiral_factor_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/spiral-factor-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "patience_low_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/patience-low-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "patience_high_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/patience-high-pass-vehicle.xbhv"
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
    local case_idx="${RUN_CASE_IDX:-0}"
    RUN_CASE_IDX=$((case_idx + 1))
    local line
    local result_line
    local launch_rc
    local shore_mport
    local veh_mport
    local shore_pshare
    local veh_pshare
    get_case_config "$case_name" || {
        echo "case=$case_name  grade=fail  reason=case_config_error" >> "$RESULTS_FILE"
        ALL_OK="no"
        return 1
    }

    vecho "Preparing case: $case_name"

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    stop_mission_apps "$MISSION_DIR"
    apply_case_patches || return 1
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
        ALL_OK="no"
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

trap cleanup EXIT

#------------------------------------------------------------
#  Part 9: Select the case set, run the matrix, and report.
#------------------------------------------------------------
ALL_CASES=(
    radial_clockwise_pass
    radial_counterclockwise_pass
    clockwise_best_pass
    polygon_box_pass
    triangle_polygon_pass
    start_inside_loiter_pass
    acquire_from_far_pass
    early_acquire_mode_pass
    center_activate_pass
    capture_radius_large_pass
    slip_radius_pass
    speed_alt_update_pass
    use_alt_speed_static_pass
    center_assign_xy_pass
    center_assign_pair_pass
    xcenter_ycenter_update_pass
    mod_poly_rad_expand_pass
    mod_poly_rad_shrink_pass
    slingshot_bearing_pass
    post_suffix_outputs_pass
    eta_output_pass
    ipf_zaic_spd_pass
    slow_speed_acquire_pass
    empty_polygon_fail
    bad_polygon_fail
    bad_update_fail
    bad_clockwise_fail
    bad_use_alt_speed_fail
    bad_patience_fail
    bad_capture_radius_fail
    center_bad_update_recover_pass
    spiral_factor_pass
    patience_low_pass
    patience_high_pass
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
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_loiter_XXXXXX")
    CASE_ROW_DIR="$RUN_ROOT/case_rows"
    mkdir -p "$CASE_ROW_DIR"

    case_idx=0
    wave_pids=""
    wave_count=0
    for ONE_CASE in "${RUN_CASES[@]}"; do
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

    case_idx=0
    for ONE_CASE in "${RUN_CASES[@]}"; do
        case_tag=$(printf "%03d_%s" "$case_idx" "$ONE_CASE")
        result_file="$CASE_ROW_DIR/${case_tag}.txt"
        if [ -f "$result_file" ]; then
            line=$(cat "$result_file")
            echo "$line" >> "$RESULTS_FILE"
            if ! case_row_passed "$line"; then
                ALL_OK="no"
            fi
        else
            echo "case=$ONE_CASE  grade=fail  reason=missing_result_file" >> "$RESULTS_FILE"
            ALL_OK="no"
        fi
        case_idx=$((case_idx + 1))
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
