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
MISSION_DIR="$REPO_DIR/missions/convoy_behavior_missions/convoy_behavior_motion"
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
        echo "  --port_base=<n>    Base port for per-case wave blocks"
        echo "  --keep_workdirs    Keep temp mission copies in wave mode"
        echo "  --gui              Launch with pMarineViewer"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=static_convoy_pass"
        echo "  ./zlaunch.sh --case=fine_mark_spacing_pass"
        echo "  ./zlaunch.sh --case=cruise_speed_cap_warn_pass"
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
    LAUNCH_ARGS=""

    if [ "$CASE_NAME" = "static_convoy_pass" ]; then
        EXPECTED="pass"
    elif [ "$CASE_NAME" = "fine_mark_spacing_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-high-queue-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/fine-mark-spacing-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "coarse_mark_spacing_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-low-queue-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/coarse-mark-spacing-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "short_mark_queue_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-short-queue-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/short-mark-queue-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "long_mark_queue_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-long-queue-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/long-mark-queue-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "tight_radius_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/tight-radius-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "wide_radius_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/wide-radius-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "cruise_speed_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-cruise-speed-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/cruise-speed-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "cruise_speed_cap_warn_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-warning-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/cruise-speed-cap-warn-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "safety_range_autoadjust_warn_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-warning-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/safety-range-autoadjust-warn-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "safety_off_bad_ranges_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/safety-off-bad-ranges-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "tailgating_speed_slow_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-tailgating-speed-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/tailgating-speed-slow-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "lagging_speed_fast_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-fast-follower-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/lagging-speed-fast-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "estop_speed_zero_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-estop-speed-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/estop-speed-zero-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "range_aliases_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/range-aliases-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "nm_radius_zero_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/nm-radius-zero-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "view_point_post_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/view-point-post-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "angled_entry_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-geometry-pass-shoreside.xmoos"
        LAUNCH_ARGS="--vpos1=x=-85,y=-125,heading=40"
    elif [ "$CASE_NAME" = "cross_track_entry_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-geometry-pass-shoreside.xmoos"
        LAUNCH_ARGS="--vpos1=x=-35,y=-145,heading=0"
    elif [ "$CASE_NAME" = "opposite_heading_recover_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-geometry-pass-shoreside.xmoos"
        LAUNCH_ARGS="--vpos1=x=-55,y=-80,heading=270"
    elif [ "$CASE_NAME" = "close_offset_tailgate_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-geometry-pass-shoreside.xmoos"
        LAUNCH_ARGS="--vpos1=x=3,y=-60,heading=180"
    elif [ "$CASE_NAME" = "lead_right_turn_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-lead-right-turn-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/lead-right-turn-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "lead_s_turn_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-lead-s-turn-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/lead-s-turn-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "short_queue_turn_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-short-queue-turn-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/short-queue-turn-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "slow_follower_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-mxrng100-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/slow-follower-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "fast_follower_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-fast-follower-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/fast-follower-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "runtime_inter_mark_coarse_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-low-queue-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-inter-mark-coarse-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_max_mark_short_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-short-queue-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-max-mark-short-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_cruise_speed_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-cruise-speed-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-cruise-speed-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_cruise_cap_warn_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-warning-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-cruise-cap-warn-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_estop_speed_zero_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-estop-speed-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-estop-speed-zero-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_bad_update_recover_pass" ]; then
        EXPECTED="pass"
        VEH_MOOS_PATCH="$HARNESS_DIR/runtime-bad-update-recover-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "no_extrapolate_pass" ]; then
        EXPECTED="pass"
        VEH_BHV_PATCH="$HARNESS_DIR/no-extrapolate-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "missing_contact_warn_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/eval-missing-contact-warn-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/missing-contact-warn-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "missing_contact_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/missing-contact-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "missing_contact_param_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/missing-contact-param-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_inter_mark_range_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-inter-mark-range-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_max_mark_range_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-max-mark-range-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_radius_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-radius-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_nm_radius_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-nm-radius-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_spd_slower_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-spd-slower-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_spd_faster_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-spd-faster-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_spd_max_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-spd-max-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_rng_estop_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-rng-estop-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_rng_tgating_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-rng-tgating-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_rng_lagging_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-rng-lagging-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_rng_safety_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-rng-safety-fail-vehicle.xbhv"
    elif [ "$CASE_NAME" = "bad_cruise_speed_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/eval-quick-fail-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/bad-cruise-speed-fail-vehicle.xbhv"
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
    local actual
    local status
    local launch_rc
    local shore_mport
    local veh1_mport
    local veh2_mport
    local shore_pshare
    local veh1_pshare
    local veh2_pshare
    get_case_config "$case_name" || return 1

    vecho "Preparing case: $case_name"

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    stop_mission_apps "$MISSION_DIR"
    apply_case_patches || return 1
    : > results.txt

    XARGS="--max_time=$MAX_TIME --mmod=$case_name $LAUNCH_ARGS $TIME_WARP"
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
    local veh1_mport
    local veh2_mport
    local shore_pshare
    local veh1_pshare
    local veh2_pshare
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
        xargs="--max_time=$MAX_TIME --mmod=$case_name $LAUNCH_ARGS --shore_mport=$shore_mport --veh1_mport=$veh1_mport --veh2_mport=$veh2_mport --shore_pshare=$shore_pshare --veh1_pshare=$veh1_pshare --veh2_pshare=$veh2_pshare $TIME_WARP"
        if [ "$NOGUI" != "" ]; then
            xargs="$xargs $NOGUI"
        fi
        if [ "$JUST_MAKE" = "yes" ]; then
            xargs="$xargs --just_make"
        fi
        xlaunch.sh $xargs
    )
    launch_rc=$?

    sleep 1

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
ALL_CASES=(
    static_convoy_pass
    fine_mark_spacing_pass
    coarse_mark_spacing_pass
    short_mark_queue_pass
    long_mark_queue_pass
    tight_radius_pass
    wide_radius_pass
    cruise_speed_pass
    cruise_speed_cap_warn_pass
    safety_range_autoadjust_warn_pass
    safety_off_bad_ranges_pass
    tailgating_speed_slow_pass
    lagging_speed_fast_pass
    estop_speed_zero_pass
    range_aliases_pass
    nm_radius_zero_pass
    view_point_post_pass
    angled_entry_pass
    cross_track_entry_pass
    opposite_heading_recover_pass
    close_offset_tailgate_pass
    lead_right_turn_pass
    lead_s_turn_pass
    short_queue_turn_pass
    slow_follower_pass
    fast_follower_pass
    runtime_inter_mark_coarse_pass
    runtime_max_mark_short_pass
    runtime_cruise_speed_pass
    runtime_cruise_cap_warn_pass
    runtime_estop_speed_zero_pass
    runtime_bad_update_recover_pass
    no_extrapolate_pass
    missing_contact_warn_pass
    missing_contact_fail
    missing_contact_param_fail
    bad_inter_mark_range_fail
    bad_max_mark_range_fail
    bad_radius_fail
    bad_nm_radius_fail
    bad_spd_slower_fail
    bad_spd_faster_fail
    bad_spd_max_fail
    bad_rng_estop_fail
    bad_rng_tgating_fail
    bad_rng_lagging_fail
    bad_rng_safety_fail
    bad_cruise_speed_fail
)

RUN_CASES=("${ALL_CASES[@]}")
if [ "$CASE" != "" ]; then
    RUN_CASES=("$CASE")
fi

: > "$RESULTS_FILE"

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    for ONE_CASE in "${RUN_CASES[@]}"; do
        run_case "$ONE_CASE" || {
            echo "case=$ONE_CASE  case_result=error  expected=unknown  actual=script_error" >> "$RESULTS_FILE"
            ALL_OK="no"
            if [ "$JUST_MAKE" != "yes" ]; then
                break
            fi
        }
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_convoy_XXXXXX")
    CASE_RESULT_DIR="$RUN_ROOT/case_results"
    mkdir -p "$CASE_RESULT_DIR"

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
