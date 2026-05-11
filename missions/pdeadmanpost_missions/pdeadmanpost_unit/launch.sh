#!/bin/bash -e
#------------------------------------------------------------
#   Script: launch.sh
#  Mission: pdeadmanpost_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT

ME=`basename "$0"`
TIME_WARP=10
VERBOSE=""
JUST_MAKE=""
LOG_CLEAN=""
MOOS_PORT="9000"
PSHARE_PORT="9200"
XLAUNCHED="no"
NOGUI=""
MMOD="active_start_once_posts_pass"

for ARGI; do
    if [ "${ARGI}" = "--help" ] || [ "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" ] && [ "$TIME_WARP" = 10 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" ] || [ "${ARGI}" = "-v" ]; then
        VERBOSE=$ARGI
    elif [ "${ARGI}" = "--just_make" ] || [ "${ARGI}" = "-j" ]; then
        JUST_MAKE=$ARGI
    elif [ "${ARGI}" = "--log_clean" ] || [ "${ARGI}" = "-lc" ]; then
        LOG_CLEAN=$ARGI
    elif [ "${ARGI:0:14}" = "--shore_mport=" ]; then
        MOOS_PORT="${ARGI#--shore_mport=*}"
    elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
        PSHARE_PORT="${ARGI#--shore_pshare=*}"
    elif [ "${ARGI:0:7}" = "--mmod=" ]; then
        MMOD="${ARGI#--mmod=*}"
    elif [ "${ARGI}" = "--xlaunched" ] || [ "${ARGI}" = "-x" ]; then
        XLAUNCHED="yes"
    elif [ "${ARGI}" = "--nogui" ] || [ "${ARGI}" = "-ng" ]; then
        NOGUI="--nogui"
    else
        echo "$ME: Bad arg: $ARGI"
        exit 1
    fi
done

if [ "$LOG_CLEAN" != "" ]; then
    ./clean.sh
fi

NSFLAGS="--strict --force -x"
if [ "${XLAUNCHED}" != "yes" ]; then
    NSFLAGS="--interactive --force -x"
fi

nsplug meta_shoreside.moos targ_shoreside.moos $NSFLAGS \
       WARP=$TIME_WARP MOOS_PORT=$MOOS_PORT PSHARE_PORT=$PSHARE_PORT MMOD=$MMOD

if [ "${JUST_MAKE}" != "" ]; then
    echo "$ME: Targ files made; exiting without launch."
    exit 0
fi

vecho "Launching pDeadManPost unit community"
pAntler targ_shoreside.moos >& /dev/null &

if [ "${XLAUNCHED}" != "yes" ]; then
    uMAC --paused targ_shoreside.moos || true
    trap "" SIGINT
    kill -- -$$
fi

exit 0
