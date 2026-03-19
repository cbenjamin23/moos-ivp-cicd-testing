#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: Mar 2026
#------------------------------------------------------------
#  Part 1: Set convenience functions for producing terminal
#          debugging output, and catching SIGINT (ctrl-c).
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT

# Catch SIGTERM so we dont come down with our own kill below
trap "echo zlaunch.sh has received sigterm" SIGTERM

#------------------------------------------------------------
#  Part 2: Set global variable default values
#------------------------------------------------------------
ME=`basename "$0"`
CMD_ARGS=""
VERBOSE=""
JUST_MAKE=""
TIME_WARP="10"
MAX_TIME="65"
NOGUI="--nogui"
CASE=""

HARNESS_DIR="${PWD}"
MISSION_DIR="${HARNESS_DIR%/harnesses/H01-cmgr_tests}/missions/cmgr_tests"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
VEHICLE_STEM="$MISSION_DIR/meta_vehicle.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
VEHICLE_XFILE="$MISSION_DIR/meta_vehicle.moosx"
VEHICLE_BHV_XFILE="$MISSION_DIR/meta_vehicle.bhvx"

#-------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#-------------------------------------------------------
for ARGI; do
    CMD_ARGS+="${ARGI} "
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]                     "
        echo "                                               "
        echo "Options:                                       "
        echo "  --help, -h         Show this help message    "
        echo "  --verbose, -v      Verbose, confirm launch   "
        echo "  --just_make, -j    Only create targ files    "
        echo "  --max_time=<secs>  Max time passed to xlaunch"
        echo "  --case=<name>      Run one named case        "
        echo "  --gui              Launch with pMarineViewer "
        echo "                                               "
        echo "Examples:                                      "
        echo "  ./zlaunch.sh                                 "
        echo "  ./zlaunch.sh --case=detect_baseline_pass     "
        echo "  ./zlaunch.sh --case=count_two_pass           "
        echo "  ./zlaunch.sh --max_time=80 10                "
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
    elif [ "${ARGI}" = "--gui" ]; then
        NOGUI=""
    else
        echo "$ME: Bad arg:" $ARGI "Exit Code 1."
        exit 1
    fi
done

#------------------------------------------------------------
#  Part 4: Set convenience functions for managing x-files
#------------------------------------------------------------
clear_xfiles() {
    rm -f "$SHORE_XFILE" "$VEHICLE_XFILE" "$VEHICLE_BHV_XFILE"
}

cleanup() {
    local start_dir="$PWD"
    if [ -d "$MISSION_DIR" ]; then
        cd "$MISSION_DIR"
        ./clean.sh >/dev/null 2>&1 || true
        ktm >/dev/null 2>&1 || true
    fi
    cd "$start_dir"
}

get_case_config() {
    CASE_NAME="$1"
    EXPECTED=""
    SHORE_PATCH=""
    VEH_PATCH=""

    if [ "$CASE_NAME" = "detect_baseline_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-baseline-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "detect_strict_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-strict-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-strict-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "detect_edge_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-edge-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-edge-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "detect_edge_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-edge-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-edge-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "detect_delayed_spoof_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-delayed-spoof-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-delayed-spoof-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "detect_short_duration_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-short-duration-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-short-duration-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "detect_early_checkpoint_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-early-checkpoint-absent-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "detect_far_spoof_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-far-spoof-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-far-spoof-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "detect_near_spoof_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-near-spoof-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-near-spoof-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "detect_cpa_only_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/detect-cpa-only-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/detect-cpa-only-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "closest_contact_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/closest-contact-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "runtime_alert_add_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/runtime-alert-add-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/runtime-alert-add-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "runtime_alert_disable_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/runtime-alert-disable-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/runtime-alert-disable-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "filter_match_type_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/filter-match-type-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/filter-match-type-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "range_report_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/range-report-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "report_request_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/report-request-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/report-request-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "count_one_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/count-one-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "list_intruder_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/list-intruder-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "range_tight_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/range-tight-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "count_two_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/count-two-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/multi-baseline-vehicle.xmoos"
    elif [ "$CASE_NAME" = "list_alpha_bravo_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/list-alpha-bravo-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/multi-baseline-vehicle.xmoos"
    elif [ "$CASE_NAME" = "closest_alpha_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/closest-alpha-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/multi-baseline-vehicle.xmoos"
    elif [ "$CASE_NAME" = "closest_bravo_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/closest-bravo-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/closest-bravo-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "bravo_far_count_one_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/bravo-far-count-one-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/bravo-far-count-one-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "late_bravo_count_two_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/late-bravo-count-two-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/late-bravo-count-two-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "stale_bravo_drop_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/stale-bravo-drop-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/stale-bravo-drop-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "reject_range_retire_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/reject-range-retire-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/reject-range-retire-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "alpha_far_bravo_only_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/alpha-far-bravo-only-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/alpha-far-bravo-only-pass-vehicle.xmoos"
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
}

run_case() {
    local case_name="$1"
    get_case_config "$case_name" || return 1

    vecho "Preparing case: $case_name"

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    ktm >/dev/null 2>&1 || true
    apply_case_patches || return 1
    : > results.txt

    XARGS="--max_time=$MAX_TIME $TIME_WARP"
    if [ "$NOGUI" != "" ]; then
        XARGS="$XARGS $NOGUI"
    fi
    if [ "$JUST_MAKE" = "yes" ]; then
        XARGS="$XARGS --just_make"
    fi

    vecho "Running case [$case_name] with xlaunch args: $XARGS"
    xlaunch.sh $XARGS

    if [ "$JUST_MAKE" = "yes" ]; then
        cd "$HARNESS_DIR"
        return 0
    fi

    line=`tail -n 1 results.txt 2>/dev/null`
    actual=`echo "$line" | sed -n 's/.*grade=\\([^ ]*\\).*/\\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="ok"
    if [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
        ALL_OK="no"
    fi

    echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  $line" >> "$RESULTS_FILE"
    cd "$HARNESS_DIR"
}

#------------------------------------------------------------
#  Part 5: Prepare and run requested case set
#------------------------------------------------------------
if [ ! -d "$MISSION_DIR" ]; then
    echo "$ME: Mission dir not found: [$MISSION_DIR]"
    exit 1
fi

trap cleanup EXIT

: > "$RESULTS_FILE"

CASES="detect_baseline_pass detect_strict_absent_pass detect_edge_pass detect_edge_absent_pass detect_delayed_spoof_pass detect_short_duration_absent_pass detect_early_checkpoint_absent_pass detect_far_spoof_absent_pass detect_near_spoof_pass detect_cpa_only_pass closest_contact_pass runtime_alert_add_pass runtime_alert_disable_absent_pass filter_match_type_absent_pass count_one_pass list_intruder_pass range_report_pass report_request_pass range_tight_pass count_two_pass list_alpha_bravo_pass closest_alpha_pass closest_bravo_pass bravo_far_count_one_pass late_bravo_count_two_pass stale_bravo_drop_pass reject_range_retire_pass alpha_far_bravo_only_pass"
if [ "$CASE" != "" ]; then
    CASES="$CASE"
fi

for case_name in $CASES; do
    run_case "$case_name" || ALL_OK="no"
done

if [ "$ALL_OK" != "yes" ]; then
    exit 1
fi

exit 0
