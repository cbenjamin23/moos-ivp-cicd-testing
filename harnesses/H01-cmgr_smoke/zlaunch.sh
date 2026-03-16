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
MAX_TIME="80"
NOGUI="--nogui"
CASE=""

HARNESS_DIR="${PWD}"
MISSION_DIR="${HARNESS_DIR%/harnesses/H01-cmgr_smoke}/missions/cmgr_smoke"
RESULTS_FILE="$HARNESS_DIR/results.txt"
TMP_DIR=""
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
        echo "  ./zlaunch.sh --case=strict_fail              "
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
make_backups() {
    TMP_DIR=`mktemp -d /tmp/cmgr_harness.XXXXXX`
    for FILE in "$SHORE_XFILE" "$VEHICLE_XFILE" "$VEHICLE_BHV_XFILE"; do
        if [ -f "$FILE" ]; then
            cp "$FILE" "$TMP_DIR/`basename $FILE`"
        fi
    done
}

clear_xfiles() {
    rm -f "$SHORE_XFILE" "$VEHICLE_XFILE" "$VEHICLE_BHV_XFILE"
}

restore_xfiles() {
    clear_xfiles
    if [ -d "$TMP_DIR" ]; then
        for FILE in meta_shoreside.moosx meta_vehicle.moosx meta_vehicle.bhvx; do
            if [ -f "$TMP_DIR/$FILE" ]; then
                cp "$TMP_DIR/$FILE" "$MISSION_DIR/$FILE"
            fi
        done
    fi
}

cleanup() {
    restore_xfiles
    if [ -d "$TMP_DIR" ]; then
        rm -rf "$TMP_DIR"
    fi
}

get_case_config() {
    CASE_NAME="$1"
    EXPECTED=""
    SHORE_PATCH=""
    VEH_PATCH=""

    if [ "$CASE_NAME" = "baseline_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/baseline-shoreside.xmoos"
    elif [ "$CASE_NAME" = "strict_fail" ]; then
        EXPECTED="fail"
        SHORE_PATCH="$HARNESS_DIR/strict-fail-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/strict-fail-vehicle.xmoos"
    elif [ "$CASE_NAME" = "strict_near_pass" ]; then
        EXPECTED="pass"
        SHORE_PATCH="$HARNESS_DIR/strict-near-pass-shoreside.xmoos"
        VEH_PATCH="$HARNESS_DIR/strict-near-pass-vehicle.xmoos"
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
    apply_case_patches || return 1
    ktm >/dev/null 2>&1 || true
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

    sleep 1
    ktm >/dev/null 2>&1 || true
    sleep 1

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

make_backups
trap cleanup EXIT

: > "$RESULTS_FILE"

CASES="baseline_pass strict_fail strict_near_pass"
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
