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
trap "echo zlaunch.sh has received sigterm" SIGTERM

#------------------------------------------------------------
#  Part 2: Set global variable default values
#------------------------------------------------------------
ME=`basename "$0"`
CMD_ARGS=""
VERBOSE=""
JUST_MAKE=""
TIME_WARP="10"
MAX_TIME="70"
NOGUI="--nogui"
CASE=""

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/obmgr_missions/obmgr_unit"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
VEHICLE_STEM="$MISSION_DIR/meta_vehicle.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
VEHICLE_XFILE="$MISSION_DIR/meta_vehicle.moosx"
VEHICLE_BHV_XFILE="$MISSION_DIR/meta_vehicle.bhvx"

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+="${ARGI} "
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]                            "
        echo "                                                      "
        echo "Options:                                              "
        echo "  --help, -h         Show this help message           "
        echo "  --verbose, -v      Verbose, confirm launch          "
        echo "  --just_make, -j    Only create targ files           "
        echo "  --max_time=<secs>  Max time passed to xlaunch       "
        echo "  --case=<name>      Run one named case               "
        echo "  --gui              Launch with pMarineViewer        "
        echo "                                                      "
        echo "Examples:                                             "
        echo "  ./zlaunch.sh                                        "
        echo "  ./zlaunch.sh --case=given_baseline_pass             "
        echo "  ./zlaunch.sh --case=points_cluster_dist_pass        "
        echo "  ./zlaunch.sh --max_time=90 10                       "
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
#          and per-run cleanup.
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

#------------------------------------------------------------
#  Part 5: Determine the expected outcome and patch files
#          for one named case.
#------------------------------------------------------------
get_case_config() {
    CASE_NAME="$1"
    EXPECTED=""
    SHORE_PATCH=""
    VEH_PATCH=""
    REQUIRED_TOKEN=""

    if [ "$CASE_NAME" = "given_baseline_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/given-baseline-pass-shoreside.xmoos"
    elif [ "$CASE_NAME" = "no_alert_request_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/no-alert-request-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/no-alert-request-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "given_far_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/given-far-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/given-far-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "given_general_alert_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/given-general-alert-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/given-general-alert-pass-vehicle.xmoos"
        REQUIRED_TOKEN="gen_alert=name=gen_ob_far#poly=pts={35,-62:39,-62:39,-58:35,-58},label=ob_far#id=ob_far"
    elif [ "$CASE_NAME" = "general_alert_default_name_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/general-alert-default-name-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/general-alert-default-name-pass-vehicle.xmoos"
        REQUIRED_TOKEN="gen_alert=name=gen_alert_ob_far#poly=pts={35,-62:39,-62:39,-58:35,-58},label=ob_far#id=ob_far"
    elif [ "$CASE_NAME" = "given_duration_resolve_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/given-duration-resolve-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/given-duration-resolve-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "given_max_duration_reject_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/given-max-duration-reject-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/given-max-duration-reject-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "given_max_duration_missing_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/given-max-duration-missing-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/given-max-duration-missing-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "post_dist_always_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/post-dist-always-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/post-dist-always-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "post_dist_false_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/post-dist-false-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/post-dist-false-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "points_cluster_dist_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/points-cluster-dist-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/points-cluster-dist-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "custom_point_var_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/custom-point-var-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/custom-point-var-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "max_pts_per_cluster_trim_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/max-pts-per-cluster-trim-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/max-pts-per-cluster-trim-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "points_ignore_range_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/points-ignore-range-absent-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/points-ignore-range-absent-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "points_age_resolve_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/points-age-resolve-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/points-age-resolve-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "lasso_cluster_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/lasso-cluster-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/lasso-cluster-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "placeholder_hull_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/placeholder-hull-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/placeholder-hull-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "disable_obstacle_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/disable-obstacle-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/disable-obstacle-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "enable_obstacle_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/enable-obstacle-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/enable-obstacle-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "expunge_obstacle_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/expunge-obstacle-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/expunge-obstacle-pass-vehicle.xmoos"
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

    if [ "$VEH_PATCH" != "" ]; then
        nspatch --stem="$VEHICLE_STEM" "$VEH_PATCH" --targ="$VEHICLE_XFILE"
    fi
}

#------------------------------------------------------------
#  Part 7: Execute one case and append its summary line.
#------------------------------------------------------------
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

    if [ "$status" = "ok" -a "$REQUIRED_TOKEN" != "" ]; then
        echo "$line" | grep -F "$REQUIRED_TOKEN" >/dev/null 2>&1
        if [ $? != 0 ]; then
            status="mismatch"
            ALL_OK="no"
        fi
    fi

    echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  $line" >> "$RESULTS_FILE"
    cd "$HARNESS_DIR"
}

#------------------------------------------------------------
#  Part 8: Validate the mission path, select the case set,
#          run the matrix, and report.
#------------------------------------------------------------
if [ ! -d "$MISSION_DIR" ]; then
    echo "$ME: Mission dir not found: [$MISSION_DIR]"
    exit 1
fi

trap cleanup EXIT

: > "$RESULTS_FILE"

CASES="given_baseline_pass no_alert_request_absent_pass given_far_absent_pass given_general_alert_pass general_alert_default_name_pass given_duration_resolve_pass given_max_duration_reject_absent_pass given_max_duration_missing_absent_pass post_dist_always_pass post_dist_false_absent_pass points_cluster_dist_pass custom_point_var_pass max_pts_per_cluster_trim_pass points_ignore_range_absent_pass points_age_resolve_pass lasso_cluster_pass placeholder_hull_pass disable_obstacle_pass enable_obstacle_pass expunge_obstacle_pass"
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
