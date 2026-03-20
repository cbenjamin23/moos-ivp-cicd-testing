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
MISSION_DIR="$REPO_DIR/missions/cmgr_missions/cmgr_motion"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
VEHICLE_STEM="$MISSION_DIR/meta_vehicle.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
VEHICLE_XFILE="$MISSION_DIR/meta_vehicle.moosx"

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
        echo "  ./zlaunch.sh --case=baseline_crossing_pass"
        echo "  ./zlaunch.sh --case=avoid_disabled_fail"
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
    rm -f "$SHORE_XFILE" "$VEHICLE_XFILE"
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

    if [ "$CASE_NAME" = "baseline_crossing_pass" ]; then
        EXPECTED="pass"
    elif [ "$CASE_NAME" = "offset_clear_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/offset-clear-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/offset-clear-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "no_detect_clear_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/no-detect-clear-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/no-detect-clear-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "delayed_crossing_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/delayed-crossing-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/delayed-crossing-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "head_on_pass" ]; then
        EXPECTED="pass"
        VEH_PATCH="$HARNESS_DIR/head-on-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "two_contact_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/two-contact-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/two-contact-pass-vehicle.xmoos"
    elif [ "$CASE_NAME" = "tight_alert_fail" ]; then
        EXPECTED="fail"
        VEH_PATCH="$HARNESS_DIR/tight-alert-fail-vehicle.xmoos"
    elif [ "$CASE_NAME" = "avoid_disabled_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/avoid-disabled-fail-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/avoid-disabled-fail-vehicle.xmoos"
    elif [ "$CASE_NAME" = "fast_intruder_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/fast-intruder-fail-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/fast-intruder-fail-vehicle.xmoos"
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
    CASES="baseline_crossing_pass offset_clear_pass no_detect_clear_pass delayed_crossing_pass head_on_pass two_contact_pass tight_alert_fail avoid_disabled_fail fast_intruder_fail"
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
