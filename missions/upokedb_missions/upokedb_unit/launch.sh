#!/bin/bash -e
#------------------------------------------------------------
#   Script: launch.sh
#  Mission: upokedb_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT

ME=`basename "$0"`
CMD_ARGS=""
TIME_WARP=1
VERBOSE=""
JUST_MAKE=""
LOG_CLEAN=""
MOOS_PORT="15000"
PSHARE_PORT="15010"
MMOD=""
XLAUNCHED="no"
NOGUI=""
SHORE_MOOS="meta_shoreside.moos"

for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo ""
        echo "Options:"
        echo "  --help, -h           Show this help message"
        echo "  --verbose, -v        Verbose, confirm launch"
        echo "  --just_make, -j      Only create targ files"
        echo "  --log_clean, -lc     Run clean.sh before launch"
        echo "  --mport=N            MOOSDB port"
        echo "  --pshare=N           pShare port"
        echo "  --shore_mport=N      MOOSDB port alias for xlaunch"
        echo "  --shore_pshare=N     pShare port alias for xlaunch"
        echo "  --mmod=<mod>         Mission variation/mod"
        echo "  --xlaunched, -x      Launched by a harness"
        echo "  --nogui, -ng         Headless launch"
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE=$ARGI
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

if [ "$LOG_CLEAN" != "" ]; then
    ./clean.sh
fi

if [ "${VERBOSE}" != "" ]; then
    echo "============================================"
    echo "  $ME SUMMARY"
    echo "============================================"
    echo "CMD_ARGS =     [${CMD_ARGS}]"
    echo "TIME_WARP =    [${TIME_WARP}]"
    echo "JUST_MAKE =    [${JUST_MAKE}]"
    echo "MOOS_PORT =    [${MOOS_PORT}]"
    echo "PSHARE_PORT =  [${PSHARE_PORT}]"
    echo "MMOD =         [${MMOD}]"
    echo "XLAUNCHED =    [${XLAUNCHED}]"
    echo "NOGUI =        [${NOGUI}]"
    echo -n "Hit any key to continue launch "
    read ANSWER
fi

NSFLAGS="--strict --force -x"
if [ "${XLAUNCHED}" != "yes" ]; then
    NSFLAGS="--interactive --force -x"
fi

nsplug "$SHORE_MOOS" targ_shoreside.moos $NSFLAGS \
       WARP=$TIME_WARP MOOS_PORT=$MOOS_PORT PSHARE_PORT=$PSHARE_PORT \
       MMOD=${MMOD:=numeric_direct_pass}

if [ "${JUST_MAKE}" != "" ]; then
    echo "$ME: Targ files made; exiting without launch."
    exit 0
fi

vecho "Launching uPokeDB test community"
pAntler targ_shoreside.moos >& /dev/null &

if [ "${XLAUNCHED}" != "yes" ]; then
    uMAC --paused targ_shoreside.moos || true
    trap "" SIGINT
    kill -- -$$
fi

exit 0

