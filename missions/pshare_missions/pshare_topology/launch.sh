#!/bin/bash -e
#------------------------------------------------------------
#   Script: launch.sh
#  Mission: pshare_topology
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
MMOD=""
XLAUNCHED="no"
NOGUI=""
SHORE_MPORT="11000"
ALPHA_MPORT="11001"
BRAVO_MPORT="11002"
RELAY_MPORT="11003"
SHORE_PSHARE="11010"
ALPHA_PSHARE="11011"
BRAVO_PSHARE="11012"
RELAY_PSHARE="11013"
SHORE_MOOS="meta_shoreside.moos"
ALPHA_MOOS="meta_alpha.moos"
BRAVO_MOOS="meta_bravo.moos"
RELAY_MOOS="meta_relay.moos"

for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo "  --just_make, -j       Only create targ files"
        echo "  --shore_mport=N       Shoreside MOOSDB port"
        echo "  --alpha_mport=N       Alpha peer MOOSDB port"
        echo "  --bravo_mport=N       Bravo peer MOOSDB port"
        echo "  --relay_mport=N       Relay peer MOOSDB port"
        echo "  --shore_pshare=N      Shoreside pShare UDP port"
        echo "  --alpha_pshare=N      Alpha pShare UDP port"
        echo "  --bravo_pshare=N      Bravo pShare UDP port"
        echo "  --relay_pshare=N      Relay pShare UDP port"
        echo "  --mmod=<mod>          Mission variation/mod"
        echo "  --xlaunched, -x       Launched by xlaunch"
        echo "  --nogui, -ng          Accepted for wrapper parity"
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE=$ARGI
    elif [ "${ARGI}" = "--log_clean" -o "${ARGI}" = "-lc" ]; then
        LOG_CLEAN=$ARGI
    elif [ "${ARGI:0:14}" = "--shore_mport=" ]; then
        SHORE_MPORT="${ARGI#--shore_mport=*}"
    elif [ "${ARGI:0:14}" = "--alpha_mport=" ]; then
        ALPHA_MPORT="${ARGI#--alpha_mport=*}"
    elif [ "${ARGI:0:14}" = "--bravo_mport=" ]; then
        BRAVO_MPORT="${ARGI#--bravo_mport=*}"
    elif [ "${ARGI:0:14}" = "--relay_mport=" ]; then
        RELAY_MPORT="${ARGI#--relay_mport=*}"
    elif [ "${ARGI:0:12}" = "--veh_mport=" ]; then
        ALPHA_MPORT="${ARGI#--veh_mport=*}"
    elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
        SHORE_PSHARE="${ARGI#--shore_pshare=*}"
    elif [ "${ARGI:0:15}" = "--alpha_pshare=" ]; then
        ALPHA_PSHARE="${ARGI#--alpha_pshare=*}"
    elif [ "${ARGI:0:15}" = "--bravo_pshare=" ]; then
        BRAVO_PSHARE="${ARGI#--bravo_pshare=*}"
    elif [ "${ARGI:0:15}" = "--relay_pshare=" ]; then
        RELAY_PSHARE="${ARGI#--relay_pshare=*}"
    elif [ "${ARGI:0:13}" = "--veh_pshare=" ]; then
        ALPHA_PSHARE="${ARGI#--veh_pshare=*}"
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

NSFLAGS="--strict --force -x"
if [ "${XLAUNCHED}" != "yes" ]; then
    NSFLAGS="--interactive --force -x"
fi

if [ -f "meta_shoreside.moosx" ]; then
    SHORE_MOOS="meta_shoreside.moosx"
fi
if [ -f "meta_alpha.moosx" ]; then
    ALPHA_MOOS="meta_alpha.moosx"
fi
if [ -f "meta_bravo.moosx" ]; then
    BRAVO_MOOS="meta_bravo.moosx"
fi
if [ -f "meta_relay.moosx" ]; then
    RELAY_MOOS="meta_relay.moosx"
fi

COMMON_ARGS="WARP=$TIME_WARP MMOD=${MMOD:=pshare_topology_fanin_pass} SHORE_PSHARE=$SHORE_PSHARE ALPHA_PSHARE=$ALPHA_PSHARE BRAVO_PSHARE=$BRAVO_PSHARE RELAY_PSHARE=$RELAY_PSHARE"

nsplug "$SHORE_MOOS" targ_shoreside.moos $NSFLAGS \
       $COMMON_ARGS LOG_STEM=shore MOOS_PORT=$SHORE_MPORT PSHARE_PORT=$SHORE_PSHARE

nsplug "$ALPHA_MOOS" targ_alpha.moos $NSFLAGS \
       $COMMON_ARGS LOG_STEM=alpha MOOS_PORT=$ALPHA_MPORT PSHARE_PORT=$ALPHA_PSHARE

nsplug "$BRAVO_MOOS" targ_bravo.moos $NSFLAGS \
       $COMMON_ARGS LOG_STEM=bravo MOOS_PORT=$BRAVO_MPORT PSHARE_PORT=$BRAVO_PSHARE

nsplug "$RELAY_MOOS" targ_relay.moos $NSFLAGS \
       $COMMON_ARGS LOG_STEM=relay MOOS_PORT=$RELAY_MPORT PSHARE_PORT=$RELAY_PSHARE

if [ "${JUST_MAKE}" != "" ]; then
    echo "$ME: Targ files made; exiting without launch."
    exit 0
fi

vecho "Launching pShare topology evaluator"
pAntler targ_shoreside.moos >& /dev/null &
sleep 0.5
vecho "Launching pShare topology relay"
pAntler targ_relay.moos >& /dev/null &
sleep 0.5
vecho "Launching pShare topology alpha"
pAntler targ_alpha.moos >& /dev/null &
sleep 0.5
vecho "Launching pShare topology bravo"
pAntler targ_bravo.moos >& /dev/null &

if [ "${XLAUNCHED}" != "yes" ]; then
    uMAC --paused targ_shoreside.moos || true
    trap "" SIGINT
    kill -- -$$
fi

exit 0
