#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Mission: utermcommand_unit
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
MAX_TIME="30"
NOGUI="--nogui"
MMOD=""
MOOS_PORT="9000"
PSHARE_PORT="9200"

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" ] || [ "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo ""
        echo "Options:"
        echo "  --help, -h         Show this help message"
        echo "  --verbose, -v      Verbose, confirm launch"
        echo "  --just_make, -j    Only create targ files"
        echo "  --nogui, -ng       Headless launch"
        echo "  --gui              Launch with GUI"
        echo "  --max_time=<secs>  Max time passed to xlaunch"
        echo "  --mmod=<mod>       Mission variation/mod"
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" ] && [ "$TIME_WARP" = 10 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" ] || [ "${ARGI}" = "-v" ]; then
        VERBOSE="--verbose"
    elif [ "${ARGI}" = "--just_make" ] || [ "${ARGI}" = "-j" ]; then
        JUST_MAKE="--just_make"
    elif [ "${ARGI}" = "--nogui" ] || [ "${ARGI}" = "-ng" ]; then
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
#  Part 4: Run the default client-driven mission case.
#------------------------------------------------------------
: > results.txt
vecho "Launching with max_time=$MAX_TIME"

if [ "${JUST_MAKE}" != "" ]; then
    xlaunch.sh --max_time=$MAX_TIME $NOGUI $JUST_MAKE $VERBOSE ${MMOD:+--mmod=$MMOD} $TIME_WARP
else
    {
        echo "ServerHost = localhost"
        echo "ServerPort = $MOOS_PORT"
        echo "Community  = term"
        echo ""
        echo "ProcessConfig = uTermCommand"
        echo "{"
        echo "  AppTick   = 4"
        echo "  CommsTick = 4"
        echo "  CMD = num,TERM_NUM,42"
        echo "}"
    } > termcommand.moos

    ./launch.sh --xlaunched $NOGUI $VERBOSE --shore_mport="$MOOS_PORT" --shore_pshare="$PSHARE_PORT" ${MMOD:+--mmod=$MMOD} $TIME_WARP > launch.out 2>&1
    sleep 0.5
    (
        printf "num\n"
        sleep 0.8
        printf "q"
    ) | uTermCommand termcommand.moos --verbose=false > termcommand.out 2>&1
    uPokeDB --quiet --host=localhost --port="$MOOS_PORT" TEST_EVAL_READY=true > eval_ready.out 2>&1 || true
    uMayFinish --max_time="$MAX_TIME" targ_shoreside.moos > mayfinish.out 2>&1 || true
fi

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
