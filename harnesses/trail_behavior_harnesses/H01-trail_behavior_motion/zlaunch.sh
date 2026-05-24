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
MAX_TIME="120"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="9700"
PORT_BASE_SET="no"
PORT_STRIDE="30"
KEEP_WORKDIRS="no"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/trail_behavior_missions/trail_behavior_motion"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
AGG_RESULTS_FILE=""
ALL_OK="yes"
RUN_ROOT=""
RESULT_ROW_DIR=""
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
VEHICLE_MOOS_STEM="$MISSION_DIR/meta_vehicle.moos"
VEHICLE_BHV_STEM="$MISSION_DIR/meta_vehicle.bhv"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
VEHICLE_MOOS_XFILE="$MISSION_DIR/meta_vehicle.moosx"
VEHICLE_BHV_XFILE="$MISSION_DIR/meta_vehicle.bhvx"

ALL_CASES=(
    static_trail_pass
    absolute_west_pass
    relative_port_pass
    relative_starboard_pass
    lead_right_turn_pass
    lead_port_turn_pass
    lead_turn_angle_update_pass
    short_trail_range_pass
    long_trail_range_pass
    tight_radius_pass
    wide_radius_pass
    inside_nm_radius_pass
    outside_nm_radius_pass
    pwt_outer_active_pass
    pwt_outer_inactive_pass
    mod_trail_range_plus_pass
    mod_trail_range_pct_pass
    mod_trail_range_floor_pass
    runtime_range_extend_pass
    runtime_mod_range_plus_pass
    runtime_mod_range_pct_pass
    runtime_mod_range_floor_pass
    runtime_angle_update_pass
    runtime_relevance_off_pass
    runtime_bad_update_recover_pass
    idle_post_distance_pass
    idle_no_post_distance_pass
    no_extrapolate_pass
    no_alert_request_pass
    auto_alert_request_pass
    missing_contact_warn_pass
    missing_contact_fail
    missing_contact_param_fail
    bad_trail_angle_type_fail
    bad_trail_angle_fail
    bad_trail_range_fail
    bad_mod_trail_range_pct_fail
    bad_radius_fail
    bad_nm_radius_fail
    bad_pwt_outer_dist_fail
    bad_decay_fail
    bad_time_on_leg_fail
)

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
        echo "  ./zlaunch.sh --case=static_trail_pass"
        echo "  ./zlaunch.sh --case=pwt_outer_inactive_pass"
        echo "  ./zlaunch.sh --case=lead_right_turn_pass --gui 1"
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
    local attempts="${2:-60}"
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

format_result_row() {
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

result_row_passed() {
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
    if [ "$AGG_RESULTS_FILE" != "" ]; then
        rm -f "$AGG_RESULTS_FILE"
    fi
}

#------------------------------------------------------------
#  Part 5: Determine the expected outcome and patch files
#          for one named case.
#------------------------------------------------------------
get_case_config() {
    CASE_NAME="$1"
    SHORE_PATCH=""
    VEH_MOOS_PATCH=""
    VEH_BHV_PATCH=""

    if [ "$CASE_NAME" = "static_trail_pass" ]; then
        :
    elif [ "$CASE_NAME" = "absolute_west_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/absolute-west-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "relative_port_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist30-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/relative-port-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "relative_starboard_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist30-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/relative-starboard-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "lead_right_turn_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist45-turn-late-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/lead-right-turn-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "lead_port_turn_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist45-turn-late-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/lead-port-turn-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "lead_turn_angle_update_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist45-turn-late-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-angle-update-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/lead-right-turn-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "short_trail_range_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist18-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/short-trail-range-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "long_trail_range_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist40-late-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/long-trail-range-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "tight_radius_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist18-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/tight-radius-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "wide_radius_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist30-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/wide-radius-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "inside_nm_radius_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/inside-nm-radius-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/inside-nm-radius-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "outside_nm_radius_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/outside-nm-radius-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/outside-nm-radius-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "pwt_outer_active_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/pwt-outer-active-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/pwt-outer-active-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "pwt_outer_inactive_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/pwt-outer-inactive-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/pwt-outer-inactive-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "mod_trail_range_plus_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist30-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/mod-trail-range-plus-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "mod_trail_range_pct_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist18-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/mod-trail-range-pct-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "mod_trail_range_floor_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/mod-trail-range-floor-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/mod-trail-range-floor-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "runtime_range_extend_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist40-late-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-range-extend-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_mod_range_plus_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist40-late-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-mod-range-plus-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_mod_range_pct_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist18-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-mod-range-pct-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_mod_range_floor_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/runtime-mod-range-floor-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-mod-range-floor-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_angle_update_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist30-late-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-angle-update-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_relevance_off_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/runtime-relevance-off-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-relevance-off-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_bad_update_recover_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/runtime-bad-update-recover-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-bad-update-recover-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "idle_post_distance_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/idle-post-distance-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/idle-post-distance-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/idle-post-distance-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "idle_no_post_distance_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/idle-no-post-distance-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/idle-no-post-distance-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/idle-no-post-distance-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "no_extrapolate_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/dist30-late-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/no-extrapolate-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "no_alert_request_pass" ]; then
        VEH_BHV_PATCH="$HARNESS_DIR/no-alert-request-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "auto_alert_request_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/auto-alert-request-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/auto-alert-request-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/auto-alert-request-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "missing_contact_warn_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/missing-contact-warn-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/missing-contact-warn-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "missing_contact_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-bhv-error-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/missing-contact-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "missing_contact_param_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-bhv-error-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/missing-contact-param-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_trail_angle_type_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-inactive-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-trail-angle-type-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_trail_angle_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-inactive-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-trail-angle-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_trail_range_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-inactive-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-trail-range-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_mod_trail_range_pct_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-inactive-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-mod-trail-range-pct-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_radius_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-inactive-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-radius-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_nm_radius_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-inactive-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-nm-radius-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_pwt_outer_dist_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-inactive-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-pwt-outer-dist-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_decay_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-inactive-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-decay-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_time_on_leg_fail" ]; then
        SHORE_PATCH="$HARNESS_DIR/eval-quick-inactive-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-time-on-leg-fail-vehicle.xbhv"
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
    local veh1_mport
    local veh2_mport
    local shore_pshare
    local veh1_pshare
    local veh2_pshare
    local case_base
    get_case_config "$case_name" || {
        echo "case=$case_name  grade=fail  reason=case_config_error" >> "$RESULTS_FILE"
        ALL_OK="no"
        return 1
    }

    vecho "Preparing case: $case_name"

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    stop_mission_apps "$MISSION_DIR"
    apply_case_patches || {
        echo "case=$case_name  grade=fail  reason=prepare_error" >> "$RESULTS_FILE"
        ALL_OK="no"
        cd "$HARNESS_DIR"
        return 1
    }
    : > results.txt

    XARGS="--max_time=$MAX_TIME --mmod=$case_name $TIME_WARP"
    if [ "$PORT_BASE_SET" = "yes" ]; then
        case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
        shore_mport=$((case_base + 0))
        veh1_mport=$((case_base + 1))
        veh2_mport=$((case_base + 2))
        shore_pshare=$((case_base + 10))
        veh1_pshare=$((case_base + 11))
        veh2_pshare=$((case_base + 12))
        XARGS="$XARGS --shore_mport=$shore_mport --veh1_mport=$veh1_mport --veh2_mport=$veh2_mport --shore_pshare=$shore_pshare --veh1_pshare=$veh1_pshare --veh2_pshare=$veh2_pshare"
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
        line=$(wait_for_result_line results.txt 60)
    else
        line=""
    fi
    result_line=$(format_result_row "$case_name" "$line" "$launch_rc")
    echo "$result_line" >> "$RESULTS_FILE"
    if ! result_row_passed "$result_line"; then
        ALL_OK="no"
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
    local result_row_file
    local shore_mport
    local veh1_mport
    local veh2_mport
    local shore_pshare
    local veh1_pshare
    local veh2_pshare
    local case_base
    local line
    local result_line
    local xargs
    local launch_rc

    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    result_row_file="$RESULT_ROW_DIR/${case_tag}.txt"

    get_case_config "$case_name" || {
        echo "case=$case_name  grade=fail  reason=case_config_error" > "$result_row_file"
        return 1
    }

    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  grade=fail  reason=prepare_error" > "$result_row_file"
        return 1
    }

    case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    shore_mport=$((case_base + 0))
    veh1_mport=$((case_base + 1))
    veh2_mport=$((case_base + 2))
    shore_pshare=$((case_base + 10))
    veh1_pshare=$((case_base + 11))
    veh2_pshare=$((case_base + 12))

    (
        cd "$case_dir"
        : > results.txt
        xargs="--max_time=$MAX_TIME --mmod=$case_name --shore_mport=$shore_mport --veh1_mport=$veh1_mport --veh2_mport=$veh2_mport --shore_pshare=$shore_pshare --veh1_pshare=$veh1_pshare --veh2_pshare=$veh2_pshare $TIME_WARP"
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
            echo "case=$case_name  grade=pass  reason=just_make" > "$result_row_file"
            return 0
        fi
        echo "case=$case_name  grade=fail  reason=launch_error  launch_rc=$launch_rc" > "$result_row_file"
        return 1
    fi

    if [ "$launch_rc" = 0 ]; then
        line=$(wait_for_result_line "$case_dir/results.txt" 60)
    else
        line=""
    fi
    result_line=$(format_result_row "$case_name" "$line" "$launch_rc")
    echo "$result_line" > "$result_row_file"

    if result_row_passed "$result_line"; then
        return 0
    fi
    return 1
}

run_wave_cases() {
    local total="$1"
    local start=0
    local idx
    local pid
    local pids
    local case_name

    while [ "$start" -lt "$total" ]; do
        pids=""
        idx=0
        while [ "$idx" -lt "$JOBS" ] && [ $((start + idx)) -lt "$total" ]; do
            case_name="${RUN_CASES[$((start + idx))]}"
            run_case_isolated "$((start + idx))" "$case_name" &
            pid=$!
            pids="$pids $pid"
            idx=$((idx + 1))
        done

        for pid in $pids; do
            wait "$pid" || ALL_OK="no"
        done
        stop_mission_apps "$RUN_ROOT"
        start=$((start + JOBS))
    done
}

aggregate_case_rows() {
    local result_file

    AGG_RESULTS_FILE="$HARNESS_DIR/.results_trail_$$.txt"
    : > "$AGG_RESULTS_FILE"
    find "$RESULT_ROW_DIR" -type f -name '*.txt' -print | sort |
    while IFS= read -r result_file; do
        cat "$result_file" >> "$AGG_RESULTS_FILE"
    done
    mv "$AGG_RESULTS_FILE" "$RESULTS_FILE"
    AGG_RESULTS_FILE=""
}

check_result_case_set() {
    local expected_count="${#RUN_CASES[@]}"
    local actual_count
    local case_name
    local hits

    actual_count=$(grep -c '^case=' "$RESULTS_FILE" 2>/dev/null || true)
    if [ "$actual_count" != "$expected_count" ]; then
        echo "$ME: Expected $expected_count result lines, found $actual_count"
        ALL_OK="no"
    fi

    for case_name in "${RUN_CASES[@]}"; do
        hits=$(grep -c "^case=$case_name  " "$RESULTS_FILE" 2>/dev/null || true)
        if [ "$hits" != "1" ]; then
            echo "$ME: Expected one result line for [$case_name], found $hits"
            ALL_OK="no"
        fi
    done
}

#------------------------------------------------------------
#  Part 9: Select cases and run the harness.
#------------------------------------------------------------
if [ ! -d "$MISSION_DIR" ]; then
    echo "$ME: Mission dir not found: [$MISSION_DIR]"
    exit 1
fi

trap cleanup EXIT

RUN_CASES=("${ALL_CASES[@]}")
if [ "$CASE" != "" ]; then
    get_case_config "$CASE" >/dev/null || exit 1
    RUN_CASES=("$CASE")
fi

: > "$RESULTS_FILE"

if [ "$JUST_MAKE" = "yes" ]; then
    if [ "$CASE" != "" ] || [ "$JOBS" -le 1 ]; then
        for case_name in "${RUN_CASES[@]}"; do
            run_case "$case_name" || ALL_OK="no"
        done
    else
        RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_trail_XXXXXX")
        RESULT_ROW_DIR="$RUN_ROOT/case_rows"
        mkdir -p "$RESULT_ROW_DIR"
        run_wave_cases "${#RUN_CASES[@]}"
        aggregate_case_rows
        check_result_case_set
    fi
    if [ "$ALL_OK" = "yes" ]; then
        echo "$ME: just_make validation passed"
        exit 0
    fi
    echo "$ME: just_make validation failed"
    exit 1
fi

if [ "$CASE" != "" ] || [ "$JOBS" -le 1 ]; then
    for case_name in "${RUN_CASES[@]}"; do
        run_case "$case_name"
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_trail_XXXXXX")
    RESULT_ROW_DIR="$RUN_ROOT/case_rows"
    mkdir -p "$RESULT_ROW_DIR"
    run_wave_cases "${#RUN_CASES[@]}"
    aggregate_case_rows
fi

check_result_case_set

echo ""
echo "$ME: Results written to $RESULTS_FILE"
cat "$RESULTS_FILE"

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
