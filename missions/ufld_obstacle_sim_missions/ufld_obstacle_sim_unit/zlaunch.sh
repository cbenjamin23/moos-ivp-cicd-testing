#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: ufld_obstacle_sim_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
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
MAX_TIME="20"
NOGUI="--nogui"
MMOD=""

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

#------------------------------------------------------------
#  Part 4: Run the mission through shared xlaunch.sh.
#------------------------------------------------------------
: > results.txt
xlaunch.sh --max_time=$MAX_TIME $NOGUI $JUST_MAKE $VERBOSE ${MMOD:+--mmod=$MMOD} $TIME_WARP

#------------------------------------------------------------
#  Part 5: Apply scoped cleanup after non-generation runs.
#------------------------------------------------------------
if [ "${JUST_MAKE}" = "" ]; then
    repo_dir=`git -C "$PWD" rev-parse --show-toplevel 2>/dev/null`
    if [ "$repo_dir" = "" ] || [ ! -x "$repo_dir/scripts/harness_teardown.sh" ]; then
        echo "$ME: Missing scoped teardown helper"
        exit 1
    fi
    "$repo_dir/scripts/harness_teardown.sh" "$PWD"
fi

exit 0
