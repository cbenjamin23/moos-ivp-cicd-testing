#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: pshare_topology
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
ME=`basename "$0"`
CMD_ARGS=""
TIME_WARP=10
VERBOSE=""
JUST_MAKE=""
MAX_TIME="65"
NOGUI="--nogui"
MMOD=""

for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo "  --just_make, -j    Only create targ files"
        echo "  --nogui, -ng       Headless launch"
        echo "  --gui              Accepted for wrapper parity"
        echo "  --max_time=<secs>  Max time passed to xlaunch"
        echo "  --mmod=<mod>       Mission variation/mod"
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 10 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE="--verbose"
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE="--just_make"
    elif [ "${ARGI}" = "--nogui" -o "${ARGI}" = "-ng" ]; then
        NOGUI="--nogui"
    elif [ "${ARGI}" = "--gui" ]; then
        NOGUI=""
    elif [ "${ARGI:0:11}" = "--max_time=" ]; then
        MAX_TIME="${ARGI#--max_time=*}"
    elif [ "${ARGI:0:7}" = "--mmod=" ]; then
        MMOD="${ARGI#--mmod=*}"
    else
        echo "$ME: Bad arg:" $ARGI "Exit Code 1."
        exit 1
    fi
done

: > results.txt
xlaunch.sh --max_time=$MAX_TIME $NOGUI $JUST_MAKE $VERBOSE ${MMOD:+--mmod=$MMOD} $TIME_WARP

if [ "${JUST_MAKE}" = "" ]; then
    repo_dir=`git -C "$PWD" rev-parse --show-toplevel 2>/dev/null`
    if [ "$repo_dir" = "" ] || [ ! -x "$repo_dir/scripts/harness_teardown.sh" ]; then
        echo "$ME: Missing scoped teardown helper"
        exit 1
    fi
    "$repo_dir/scripts/harness_teardown.sh" "$PWD"
fi

exit 0
