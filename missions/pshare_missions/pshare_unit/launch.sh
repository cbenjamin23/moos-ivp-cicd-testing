#!/bin/bash -e
#------------------------------------------------------------
#   Script: launch.sh
#  Mission: pshare_unit
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
PEER_MPORT="11001"
SHORE_PSHARE="11010"
PEER_PSHARE="11011"
SHORE_MOOS="meta_shoreside.moos"
PEER_MOOS="meta_peer.moos"

for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo "  --just_make, -j       Only create targ files"
        echo "  --shore_mport=N       Shoreside MOOSDB port"
        echo "  --peer_mport=N        Peer MOOSDB port"
        echo "  --veh_mport=N         Peer MOOSDB alias for xlaunch"
        echo "  --shore_pshare=N      Shoreside pShare UDP port"
        echo "  --peer_pshare=N       Peer pShare UDP port"
        echo "  --veh_pshare=N        Peer pShare alias for xlaunch"
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
    elif [ "${ARGI:0:13}" = "--peer_mport=" ]; then
        PEER_MPORT="${ARGI#--peer_mport=*}"
    elif [ "${ARGI:0:12}" = "--veh_mport=" ]; then
        PEER_MPORT="${ARGI#--veh_mport=*}"
    elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
        SHORE_PSHARE="${ARGI#--shore_pshare=*}"
    elif [ "${ARGI:0:14}" = "--peer_pshare=" ]; then
        PEER_PSHARE="${ARGI#--peer_pshare=*}"
    elif [ "${ARGI:0:13}" = "--veh_pshare=" ]; then
        PEER_PSHARE="${ARGI#--veh_pshare=*}"
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
if [ -f "meta_peer.moosx" ]; then
    PEER_MOOS="meta_peer.moosx"
fi

nsplug "$SHORE_MOOS" targ_shoreside.moos $NSFLAGS \
       WARP=$TIME_WARP MOOS_PORT=$SHORE_MPORT PSHARE_PORT=$SHORE_PSHARE \
       PEER_PSHARE=$PEER_PSHARE MMOD=${MMOD:=pshare_direct_route_pass}

nsplug "$PEER_MOOS" targ_peer.moos $NSFLAGS \
       WARP=$TIME_WARP MOOS_PORT=$PEER_MPORT PSHARE_PORT=$PEER_PSHARE \
       SHORE_PSHARE=$SHORE_PSHARE MMOD=${MMOD:=pshare_direct_route_pass}

if [ "${JUST_MAKE}" != "" ]; then
    echo "$ME: Targ files made; exiting without launch."
    exit 0
fi

vecho "Launching pShare receiver community"
pAntler targ_shoreside.moos >& /dev/null &
sleep 0.5
vecho "Launching pShare sender community"
pAntler targ_peer.moos >& /dev/null &

if [ "${XLAUNCHED}" != "yes" ]; then
    uMAC --paused targ_shoreside.moos || true
    trap "" SIGINT
    kill -- -$$
fi

exit 0
