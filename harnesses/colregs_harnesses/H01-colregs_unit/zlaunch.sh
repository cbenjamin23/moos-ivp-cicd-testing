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
TIME_WARP="10"
VERBOSE=""
JUST_MAKE=""
MAX_TIME="80"
CASE=""
RESULTS_FILE="$PWD/results.txt"
HARNESS_DIR="$PWD"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/colregs_missions/colregs_unit"
ALL_OK="yes"

for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo ""
        echo "Options:"
        echo "  --help, -h         Show this help message"
        echo "  --verbose, -v      Verbose, confirm launch"
        echo "  --just_make, -j    Only create targ files"
        echo "  --max_time=<secs>  Max time passed to xlaunch"
        echo "  --case=<name>      Run one named case"
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
    else
        echo "$ME: Bad arg: $ARGI"
        exit 1
    fi
done

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

run_case() {
    local case_name="$1"
    local expected="pass"
    local line actual status

    cd "$MISSION_DIR"
    ktm >/dev/null 2>&1 || true
    ./clean.sh >/dev/null 2>&1 || true
    : > results.txt
    xlaunch.sh --max_time=$MAX_TIME --mmod=$case_name --nogui ${JUST_MAKE:+--just_make} ${VERBOSE:+--verbose} $TIME_WARP

    if [ "$JUST_MAKE" = "yes" ]; then
        cd "$HARNESS_DIR"
        return 0
    fi

    line=$(wait_for_result_line results.txt 32)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    if [ "$actual" = "" ]; then
        actual="missing"
    fi
    status="ok"
    if [ "$actual" != "$expected" ]; then
        status="mismatch"
        ALL_OK="no"
    fi

    echo "case=$case_name  expected=$expected  actual=$actual  status=$status  $line" >> "$RESULTS_FILE"
    ktm >/dev/null 2>&1 || true
    cd "$HARNESS_DIR"
}

CASES="head_on_colregs_pass"
if [ "$CASE" != "" ]; then
    CASES="$CASE"
fi

: > "$RESULTS_FILE"
for case_name in $CASES; do
    run_case "$case_name"
done

[ "$ALL_OK" = "yes" ]
