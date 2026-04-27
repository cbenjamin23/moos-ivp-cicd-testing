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
TIME_WARP=10
VERBOSE=""
JUST_MAKE=""
MAX_TIME="60"
NOGUI="--nogui"

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
        echo "  --nogui, -ng       Headless launch, no gui"
        echo "  --gui              Launch with pMarineViewer"
        echo "  --max_time=<secs>  Max time passed to xlaunch"
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
    else
        echo "$ME: Bad arg:" $ARGI "Exit Code 1."
        exit 1
    fi
done

#------------------------------------------------------------
#  Part 4: Prepare for headless execution
#------------------------------------------------------------
: > results.txt
vecho "Launching with max_time=$MAX_TIME"

#------------------------------------------------------------
#  Part 5: Run the mission through shared xlaunch.sh
#------------------------------------------------------------
xlaunch.sh --max_time=$MAX_TIME $NOGUI $JUST_MAKE $VERBOSE $TIME_WARP

#------------------------------------------------------------
#  Part 6: Ensure clean exit before returning to caller
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
