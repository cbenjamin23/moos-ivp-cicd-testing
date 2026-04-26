#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: fixedturn_behavior_motion
#   Author: Charles Benjamin
#   LastEd: Apr 2026
#------------------------------------------------------------
ME=`basename "$0"`
TIME_WARP="10"
MAX_TIME="70"
NOGUI="--nogui"
JUST_MAKE=""
VERBOSE=""
MMOD=""

for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo ""
        echo "Options:"
        echo "  --help, -h         Show this help message"
        echo "  --verbose, -v      Verbose launch"
        echo "  --just_make, -j    Only create targ files"
        echo "  --max_time=<secs>  Max time passed to xlaunch"
        echo "  --mmod=<mod>       Mission variation/mod"
        echo "  --gui              Launch with pMarineViewer"
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 10 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE="--verbose"
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE="--just_make"
    elif [ "${ARGI:0:11}" = "--max_time=" ]; then
        MAX_TIME="${ARGI#--max_time=*}"
    elif [ "${ARGI:0:7}" = "--mmod=" ]; then
        MMOD="${ARGI#--mmod=*}"
    elif [ "${ARGI}" = "--gui" ]; then
        NOGUI=""
    else
        echo "$ME: Bad arg:" $ARGI "Exit Code 1."
        exit 1
    fi
done

xlaunch.sh --max_time=$MAX_TIME $NOGUI $JUST_MAKE $VERBOSE ${MMOD:+--mmod=$MMOD} $TIME_WARP
