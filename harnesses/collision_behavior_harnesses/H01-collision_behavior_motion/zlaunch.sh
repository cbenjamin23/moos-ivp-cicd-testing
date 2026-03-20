#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: Mar 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

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
MISSION_DIR="$REPO_DIR/missions/collision_behavior_missions/collision_behavior_motion"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
VEHICLE_MOOS_STEM="$MISSION_DIR/meta_vehicle.moos"
VEHICLE_BHV_STEM="$MISSION_DIR/meta_vehicle.bhv"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
VEHICLE_MOOS_XFILE="$MISSION_DIR/meta_vehicle.moosx"
VEHICLE_BHV_XFILE="$MISSION_DIR/meta_vehicle.bhvx"

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
        echo "  --gui              Launch with pMarineViewer"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=default_resolve_pass"
        echo "  ./zlaunch.sh --case=no_alert_request_fail"
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

clear_xfiles() {
    rm -f "$SHORE_XFILE" "$VEHICLE_MOOS_XFILE" "$VEHICLE_BHV_XFILE"
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
    VEH_MOOS_PATCH=""
    VEH_BHV_PATCH=""

    if [ "$CASE_NAME" = "default_resolve_pass" ]; then
        EXPECTED="pass"
    elif [ "$CASE_NAME" = "no_alert_request_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/no-alert-request-absent-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/no-alert-request-absent-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/no-alert-request-absent-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "post_per_contact_info_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/post-per-contact-info-pass-shoreside.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/post-per-contact-info-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "behavior_filter_absent_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/behavior-filter-absent-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/behavior-filter-absent-pass-vehicle.xmoos"
        VEH_BHV_PATCH="$HARNESS_DIR/behavior-filter-absent-pass-vehicle.xbhv"
    elif [ "$CASE_NAME" = "head_on_resolve_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/head-on-resolve-pass-shoreside.xmoos"
        VEH_MOOS_PATCH="$HARNESS_DIR/head-on-resolve-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "no_alert_request_fail" ]; then
        EXPECTED="fail"
        VEH_BHV_PATCH="$HARNESS_DIR/no-alert-request-fail-vehicle.xbhv"
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

run_case() {
    local case_name="$1"
    get_case_config "$case_name" || return 1

    vecho "Preparing case: $case_name"

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    ktm >/dev/null 2>&1 || true
    apply_case_patches || return 1
    : > results.txt

    XARGS="--max_time=$MAX_TIME --mmod=$case_name $TIME_WARP"
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
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
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

trap cleanup EXIT

if [ "$CASE" != "" ]; then
    CASES="$CASE"
else
    CASES="default_resolve_pass no_alert_request_absent_pass post_per_contact_info_pass behavior_filter_absent_pass head_on_resolve_pass no_alert_request_fail"
fi

: > "$RESULTS_FILE"

for ONE_CASE in $CASES; do
    run_case "$ONE_CASE" || {
        echo "case=$ONE_CASE  expected=unknown  actual=script_error  status=error" >> "$RESULTS_FILE"
        ALL_OK="no"
        if [ "$JUST_MAKE" != "yes" ]; then
            break
        fi
    }
done

if [ "$JUST_MAKE" = "yes" ]; then
    echo "$ME: Just_make complete for cases: $CASES"
    exit 0
fi

cat "$RESULTS_FILE"

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
