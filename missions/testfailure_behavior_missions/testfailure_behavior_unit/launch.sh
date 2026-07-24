#!/bin/bash -e
#------------------------------------------------------------
#   Script: launch.sh
#  Mission: testfailure_behavior_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
#  Part 1: Set convenience functions for producing terminal
#          debugging output, and catching SIGINT (ctrl-c).
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT

#------------------------------------------------------------
#  Part 2: Set global variable default values
#------------------------------------------------------------
ME=`basename "$0"`
CMD_ARGS=""
TIME_WARP=1
VERBOSE=""
JUST_MAKE=""
LOG_CLEAN=""
LOG_MODE="minimal"
if [ "${LOG_MODE_PREPARED:-no}" = yes ] && [ -n "${LOG_MODE_PREPARED_VALUE:-}" ]; then
    LOG_MODE="$LOG_MODE_PREPARED_VALUE"
fi
MOOS_PORT="9000"
PSHARE_PORT="9200"
MMOD=""
GAP_MIN="1"
GAP_MAX="1.5"
GAP_NEAR_RATIO="0.8"
XLAUNCHED="no"
NOGUI=""
SHORE_MOOS="meta_shoreside.moos"
BEHAVIOR_BHV="meta_behavior.bhv"

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo ""
        echo "Options:"
        echo "  --help, -h           Show this help message"
        echo "  --verbose, -v        Verbose, confirm launch"
        echo "  --just_make, -j      Only create targ files"
        echo "  --log=<mode>         minimal (default) or full"
        echo "  --log_clean, -lc     Run clean.sh before launch"
        echo "  --mport=N            MOOSDB port"
        echo "  --pshare=N           pShare port"
        echo "  --shore_mport=N      MOOSDB port alias for xlaunch"
        echo "  --shore_pshare=N     pShare port alias for xlaunch"
        echo "  --gap_min=N          Minimum normalized iteration gap"
        echo "  --gap_max=N          Maximum normalized iteration gap"
        echo "  --gap_near_ratio=N   uLoadWatch near-breach ratio"
        echo "  --mmod=<mod>         Mission variation/mod"
        echo "  --xlaunched, -x      Launched by xlaunch"
        echo "  --nogui, -ng         Headless launch"
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE=$ARGI
    elif [ "${ARGI:0:6}" = "--log=" ]; then
        LOG_MODE="${ARGI#--log=*}"
    elif [ "${ARGI}" = "--log_clean" -o "${ARGI}" = "-lc" ]; then
        LOG_CLEAN=$ARGI
    elif [ "${ARGI:0:8}" = "--mport=" ]; then
        MOOS_PORT="${ARGI#--mport=*}"
    elif [ "${ARGI:0:9}" = "--pshare=" ]; then
        PSHARE_PORT="${ARGI#--pshare=*}"
    elif [ "${ARGI:0:14}" = "--shore_mport=" ]; then
        MOOS_PORT="${ARGI#--shore_mport=*}"
    elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
        PSHARE_PORT="${ARGI#--shore_pshare=*}"
    elif [ "${ARGI:0:10}" = "--gap_min=" ]; then
        GAP_MIN="${ARGI#--gap_min=*}"
    elif [ "${ARGI:0:10}" = "--gap_max=" ]; then
        GAP_MAX="${ARGI#--gap_max=*}"
    elif [ "${ARGI:0:17}" = "--gap_near_ratio=" ]; then
        GAP_NEAR_RATIO="${ARGI#--gap_near_ratio=*}"
    elif [ "${ARGI:0:12}" = "--veh_mport=" ]; then
        true
    elif [ "${ARGI:0:13}" = "--veh_pshare=" ]; then
        true
    elif [ "${ARGI:0:7}" = "--mmod=" ]; then
        MMOD="${ARGI#--mmod=*}"
    elif [ "${ARGI}" = "--xlaunched" -o "${ARGI}" = "-x" ]; then
        XLAUNCHED="yes"
    elif [ "${ARGI}" = "--nogui" -o "${ARGI}" = "-ng" ]; then
        NOGUI="--nogui"
    else
        echo "$ME: Bad arg:" $ARGI "Exit Code 1."
        exit 1
    fi
done

case "$LOG_MODE" in
    minimal|full) ;;
    *) echo "$ME: --log must be minimal or full" >&2; exit 2 ;;
esac
if [ "${LOG_MODE_PREPARED:-no}" != yes ]; then
    ./prepare_logging_mode.sh "$LOG_MODE"
fi
export LOG_MODE_PREPARED=yes
export LOG_MODE_PREPARED_VALUE="$LOG_MODE"

#------------------------------------------------------------
#  Part 4: Optionally clean previous generated/run artifacts.
#------------------------------------------------------------
if [ "$LOG_CLEAN" != "" ]; then
    ./clean.sh
fi

#------------------------------------------------------------
#  Part 5: If verbose, show vars and confirm before launching.
#------------------------------------------------------------
if [ "${VERBOSE}" != "" ]; then
    echo "============================================"
    echo "  $ME SUMMARY"
    echo "============================================"
    echo "CMD_ARGS =     [${CMD_ARGS}]"
    echo "TIME_WARP =    [${TIME_WARP}]"
    echo "JUST_MAKE =    [${JUST_MAKE}]"
    echo "MOOS_PORT =    [${MOOS_PORT}]"
    echo "PSHARE_PORT =  [${PSHARE_PORT}]"
    echo "GAP_MIN =      [${GAP_MIN}]"
    echo "GAP_MAX =      [${GAP_MAX}]"
    echo "GAP_NEAR_RATIO = [${GAP_NEAR_RATIO}]"
    echo "MMOD =         [${MMOD}]"
    echo "XLAUNCHED =    [${XLAUNCHED}]"
    echo "NOGUI =        [${NOGUI}]"
    echo -n "Hit any key to continue launch "
    read ANSWER
fi

#------------------------------------------------------------
#  Part 6: Generate target files.
#------------------------------------------------------------
NSFLAGS="--strict --force -x"
if [ "${XLAUNCHED}" != "yes" ]; then
    NSFLAGS="--interactive --force -x"
fi

if [ -f "meta_shoreside.moosx" ]; then
    SHORE_MOOS="meta_shoreside.moosx"
fi
if [ -f "meta_behavior.bhvx" ]; then
    BEHAVIOR_BHV="meta_behavior.bhvx"
fi

nsplug "$BEHAVIOR_BHV" targ_testfailure.bhv $NSFLAGS
nsplug "$SHORE_MOOS" targ_shoreside.moos $NSFLAGS \
       WARP=$TIME_WARP MOOS_PORT=$MOOS_PORT PSHARE_PORT=$PSHARE_PORT \
       MMOD=${MMOD:=baseline_armed_no_trigger_pass} \
       GAP_MIN="$GAP_MIN" GAP_MAX="$GAP_MAX" \
       GAP_NEAR_RATIO="$GAP_NEAR_RATIO"

if [ "${JUST_MAKE}" != "" ]; then
    echo "$ME: Targ files made; exiting without launch."
    exit 0
fi

#------------------------------------------------------------
#  Part 7: Launch the testfailure unit community.
#------------------------------------------------------------
vecho "Launching testfailure unit community"
pAntler targ_shoreside.moos >& /dev/null &

if [ "${XLAUNCHED}" != "yes" ]; then
    uMAC --paused targ_shoreside.moos || true
    trap "" SIGINT
    kill -- -$$
fi

exit 0
