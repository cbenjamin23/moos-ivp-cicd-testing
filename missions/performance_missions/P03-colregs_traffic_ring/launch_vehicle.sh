#!/bin/bash
#------------------------------------------------------------
#   Script: launch_vehicle.sh
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT

ME=$(basename "$0")
CMD_ARGS=""
TIME_WARP=1
VERBOSE=""
JUST_MAKE="no"
AUTO_LAUNCHED="no"
IP_ADDR="localhost"
MOOS_PORT="9001"
PSHARE_PORT="9201"
SHORE_IP="localhost"
SHORE_PSHARE="9200"
VNAME="abe"
UP_VNAME="ABE"
COLOR="yellow"
XMODE="SIM"
START_POS="x=0,y=0,heading=0"
STOCK_SPD="1.9"
MAX_SPD="2.5"
AVOID_INITIAL="true"

for ARGI; do
  CMD_ARGS+=" ${ARGI}"
  if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
    echo "$ME [OPTIONS] [time_warp]"
    echo "  --avoid=<bool>      Initial AVOID value for helm behaviors"
    exit 0
  elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
    TIME_WARP=$ARGI
  elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
    VERBOSE="yes"
  elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
    JUST_MAKE="yes"
  elif [ "${ARGI}" = "--auto" -o "${ARGI}" = "-a" ]; then
    AUTO_LAUNCHED="yes"
  elif [ "${ARGI:0:5}" = "--ip=" ]; then
    IP_ADDR="${ARGI#--ip=*}"
  elif [ "${ARGI:0:7}" = "--mport" ]; then
    MOOS_PORT="${ARGI#--mport=*}"
  elif [ "${ARGI:0:9}" = "--pshare=" ]; then
    PSHARE_PORT="${ARGI#--pshare=*}"
  elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
    SHORE_PSHARE="${ARGI#--shore_pshare=*}"
  elif [ "${ARGI:0:8}" = "--vname=" ]; then
    VNAME="${ARGI#--vname=*}"
  elif [ "${ARGI:0:11}" = "--up_vname=" ]; then
    UP_VNAME="${ARGI#--up_vname=*}"
  elif [ "${ARGI:0:8}" = "--color=" ]; then
    COLOR="${ARGI#--color=*}"
  elif [ "${ARGI}" = "--sim" -o "${ARGI}" = "-s" ]; then
    XMODE="SIM"
  elif [ "${ARGI:0:12}" = "--start_pos=" ]; then
    START_POS="${ARGI#--start_pos=*}"
  elif [ "${ARGI:0:12}" = "--stock_spd=" ]; then
    STOCK_SPD="${ARGI#--stock_spd=*}"
  elif [ "${ARGI:0:10}" = "--max_spd=" ]; then
    MAX_SPD="${ARGI#--max_spd=*}"
  elif [ "${ARGI:0:8}" = "--avoid=" ]; then
    AVOID_INITIAL="${ARGI#--avoid=*}"
  fi
done

NSFLAGS="--strict --force"
if [ "${AUTO_LAUNCHED}" = "no" ]; then
  NSFLAGS="--interactive --force"
fi

nsplug meta_vehicle.moos targ_$VNAME.moos $NSFLAGS WARP=$TIME_WARP \
  IP_ADDR=$IP_ADDR MOOS_PORT=$MOOS_PORT PSHARE_PORT=$PSHARE_PORT \
  SHORE_IP=$SHORE_IP SHORE_PSHARE=$SHORE_PSHARE VNAME=$VNAME COLOR=$COLOR \
  XMODE=$XMODE START_POS=$START_POS MAX_SPD=$MAX_SPD UP_VNAME=$UP_VNAME FSEAT_IP=localhost

nsplug meta_vehicle.bhv targ_$VNAME.bhv $NSFLAGS \
  START_POS=$START_POS VNAME=$VNAME UP_VNAME=$UP_VNAME STOCK_SPD=$STOCK_SPD \
  COLOR=$COLOR AVOID_INITIAL=$AVOID_INITIAL

if [ "${JUST_MAKE}" = "yes" ]; then
  echo "$ME: Targ files made; exiting without launch."
  exit 0
fi

echo "Launching $VNAME MOOS Community. WARP=$TIME_WARP"
pAntler targ_$VNAME.moos >& /dev/null &
echo "Done Launching $VNAME MOOS Community"

if [ "${AUTO_LAUNCHED}" = "yes" ]; then
  exit 0
fi

uMAC targ_$VNAME.moos
trap "" SIGINT
kill -- -$$
exit 0
