#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
ME=`basename "$0"`
TIME_WARP=20
MAX_TIME="4000"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="4000"
KEEP_WORKDIRS=""
JUST_MAKE=""
VERBOSE=""

for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo "  --case=<name>      Forward named case to harness"
        echo "  --jobs=<n>         Forward wave size to harness"
        echo "  --port_base=<n>    Base port for per-case wave blocks"
        echo "  --max_time=<secs>  Max time passed to xlaunch"
        echo "  --keep_workdirs    Keep harness temp dirs"
        echo "  --just_make, -j    Only create targ files"
        echo "  --gui              Launch with pMarineViewer"
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 10 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI:0:7}" = "--case=" ]; then
        CASE="${ARGI#--case=*}"
    elif [ "${ARGI:0:7}" = "--jobs=" ]; then
        JOBS="${ARGI#--jobs=*}"
    elif [ "${ARGI:0:12}" = "--port_base=" ]; then
        PORT_BASE="${ARGI#--port_base=*}"
    elif [ "${ARGI:0:11}" = "--max_time=" ]; then
        MAX_TIME="${ARGI#--max_time=*}"
    elif [ "${ARGI}" = "--keep_workdirs" ]; then
        KEEP_WORKDIRS="--keep_workdirs"
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE="--just_make"
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE="--verbose"
    elif [ "${ARGI}" = "--gui" ]; then
        NOGUI=""
    elif [ "${ARGI}" = "--nogui" -o "${ARGI}" = "-ng" ]; then
        NOGUI="--nogui"
    else
        echo "$ME: Bad arg:" $ARGI "Exit Code 1."
        exit 1
    fi
done

if [ "$CASE" != "" ] || [ "$JOBS" != "1" ]; then
    HARNESS_DIR="$(cd "$(dirname "$0")/../../../harnesses/ufield_comms_harnesses/H01-ufield_comms_unit" && pwd)"
    cd "$HARNESS_DIR"
    HARGS="--max_time=$MAX_TIME --jobs=$JOBS --port_base=$PORT_BASE"
    if [ "$CASE" != "" ]; then
        HARGS="$HARGS --case=$CASE"
    fi
    if [ "$NOGUI" = "" ]; then
        HARGS="$HARGS --gui"
    fi
    ./zlaunch.sh $HARGS $KEEP_WORKDIRS $JUST_MAKE $VERBOSE $TIME_WARP
    exit $?
fi

: > results.txt
xlaunch.sh --max_time=$MAX_TIME $NOGUI $JUST_MAKE $VERBOSE --shore_mport=$PORT_BASE --veh1_mport=$((PORT_BASE+1)) --veh2_mport=$((PORT_BASE+2)) --shore_pshare=$((PORT_BASE+10)) --veh1_pshare=$((PORT_BASE+11)) --veh2_pshare=$((PORT_BASE+12)) $TIME_WARP

if [ "${JUST_MAKE}" = "" ]; then
    sleep 0.5
    repo_dir=`git -C "$PWD" rev-parse --show-toplevel 2>/dev/null`
    if [ "$repo_dir" != "" ] && [ -x "$repo_dir/scripts/harness_teardown.sh" ]; then
        "$repo_dir/scripts/harness_teardown.sh" "$PWD"
    fi
fi
