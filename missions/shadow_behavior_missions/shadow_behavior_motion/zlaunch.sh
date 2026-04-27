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
TIME_WARP=10
VERBOSE=""
JUST_MAKE=""
MAX_TIME="120"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="9700"
KEEP_WORKDIRS=""

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo ""
        echo "Options:"
        echo "  --help, -h         Show this help message"
        echo "  --verbose, -v      Verbose, confirm launch"
        echo "  --just_make, -j    Only create targ files"
        echo "  --max_time=<secs>  Max time passed to xlaunch"
        echo "  --case=<name>      Forward named case to harness"
        echo "  --jobs=<n>         Forward wave size to harness"
        echo "  --port_base=<n>    Base port for per-case wave blocks"
        echo "  --keep_workdirs    Keep harness temp dirs"
        echo "  --gui              Launch with pMarineViewer"
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
    elif [ "${ARGI:0:7}" = "--case=" ]; then
        CASE="${ARGI#--case=*}"
    elif [ "${ARGI:0:7}" = "--jobs=" ]; then
        JOBS="${ARGI#--jobs=*}"
    elif [ "${ARGI:0:12}" = "--port_base=" ]; then
        PORT_BASE="${ARGI#--port_base=*}"
    elif [ "${ARGI}" = "--keep_workdirs" ]; then
        KEEP_WORKDIRS="--keep_workdirs"
    else
        echo "$ME: Bad arg:" $ARGI "Exit Code 1."
        exit 1
    fi
done

#------------------------------------------------------------
#  Part 4: Forward named or batch cases to the harness
#------------------------------------------------------------
if [ "$CASE" != "" ] || [ "$JOBS" != "1" ]; then
    HARNESS_DIR="$(cd "$(dirname "$0")/../../../harnesses/shadow_behavior_harnesses/H01-shadow_behavior_motion" && pwd)"
    cd "$HARNESS_DIR"

    HARGS="--max_time=$MAX_TIME --jobs=$JOBS --port_base=$PORT_BASE"
    if [ "$CASE" != "" ]; then
        HARGS="$HARGS --case=$CASE"
    fi
    if [ "$NOGUI" = "" ]; then
        HARGS="$HARGS --gui"
    fi
    if [ "$JUST_MAKE" != "" ]; then
        HARGS="$HARGS --just_make"
    fi
    if [ "$VERBOSE" != "" ]; then
        HARGS="$HARGS --verbose"
    fi
    if [ "$KEEP_WORKDIRS" != "" ]; then
        HARGS="$HARGS $KEEP_WORKDIRS"
    fi

    ./zlaunch.sh $HARGS $TIME_WARP
    exit $?
fi

#------------------------------------------------------------
#  Part 5: Run the default stem mission through xlaunch
#------------------------------------------------------------
: > results.txt
xlaunch.sh --max_time=$MAX_TIME $NOGUI $JUST_MAKE $VERBOSE $TIME_WARP

#------------------------------------------------------------
#  Part 6: Clean up live apps after ordinary default runs
#------------------------------------------------------------
if [ "${JUST_MAKE}" = "" ]; then
    sleep 0.5
    repo_dir=`git -C "$PWD" rev-parse --show-toplevel 2>/dev/null`
    if [ "$repo_dir" = "" ] || [ ! -x "$repo_dir/scripts/harness_teardown.sh" ]; then
        echo "$ME: Missing scoped teardown helper"
        exit 1
    fi
    "$repo_dir/scripts/harness_teardown.sh" "$PWD"
    sleep 0.5
fi

exit 0
