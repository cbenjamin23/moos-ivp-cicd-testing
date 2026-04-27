#!/bin/bash
#------------------------------------------------------------
#   Script: launch_shoreside.sh
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT

ME=$(basename "$0")
CMD_ARGS=""
TIME_WARP=1
JUST_MAKE="no"
VERBOSE=""
AUTO_LAUNCHED="no"
LAUNCH_GUI="yes"
IP_ADDR="localhost"
MOOS_PORT="9000"
PSHARE_PORT="9200"
VNAMES="abe:ben:cal:deb:eve"
MMOD="baseline_circle_pass"
REQUIRED_NODES="5"
EVAL_TIME_SECS="300"
MISSION_TIMEOUT_SECS="420"
DEPLOY_DELAY_SECS="15"
CLOSEST_MIN="5"
CLOSEST_MAX="30"
NEAR_MISS_MAX="25"
BATCHES_MIN="5"
SEED="17"
CENTER_X="23"
CENTER_Y="-82"
BASE_RADIUS="38"
RADIUS_JITTER="0"
MIN_TARGET_SEP="14"
MIN_LEG_LEN="20"
MIN_PRED_CPA="10"
REPOST_INTERVAL="10"
TRAFFIC_BIN="${PTRAFFIC_BIN}"
DUMB_VNAME="none"

for ARGI; do
  CMD_ARGS+="${ARGI} "
  if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
    echo "$ME [OPTIONS] [time_warp]"
    exit 0
  elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
    TIME_WARP=$ARGI
  elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
    JUST_MAKE="yes"
  elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
    VERBOSE="yes"
  elif [ "${ARGI}" = "--auto" -o "${ARGI}" = "-a" ]; then
    AUTO_LAUNCHED="yes"
  elif [ "${ARGI}" = "--nogui" -o "${ARGI}" = "-n" ]; then
    LAUNCH_GUI="no"
  elif [ "${ARGI:0:5}" = "--ip=" ]; then
    IP_ADDR="${ARGI#--ip=*}"
  elif [ "${ARGI:0:7}" = "--mport" ]; then
    MOOS_PORT="${ARGI#--mport=*}"
  elif [ "${ARGI:0:9}" = "--pshare=" ]; then
    PSHARE_PORT="${ARGI#--pshare=*}"
  elif [ "${ARGI:0:9}" = "--vnames=" ]; then
    VNAMES="${ARGI#--vnames=*}"
  elif [ "${ARGI:0:7}" = "--mmod=" ]; then
    MMOD="${ARGI#--mmod=*}"
  elif [ "${ARGI:0:17}" = "--required_nodes=" ]; then
    REQUIRED_NODES="${ARGI#--required_nodes=*}"
  elif [ "${ARGI:0:12}" = "--eval_time=" ]; then
    EVAL_TIME_SECS="${ARGI#--eval_time=*}"
  elif [ "${ARGI:0:18}" = "--mission_timeout=" ]; then
    MISSION_TIMEOUT_SECS="${ARGI#--mission_timeout=*}"
  elif [ "${ARGI:0:15}" = "--deploy_delay=" ]; then
    DEPLOY_DELAY_SECS="${ARGI#--deploy_delay=*}"
  elif [ "${ARGI:0:14}" = "--closest_min=" ]; then
    CLOSEST_MIN="${ARGI#--closest_min=*}"
  elif [ "${ARGI:0:14}" = "--closest_max=" ]; then
    CLOSEST_MAX="${ARGI#--closest_max=*}"
  elif [ "${ARGI:0:17}" = "--near_miss_max=" ]; then
    NEAR_MISS_MAX="${ARGI#--near_miss_max=*}"
  elif [ "${ARGI:0:14}" = "--batches_min=" ]; then
    BATCHES_MIN="${ARGI#--batches_min=*}"
  elif [ "${ARGI:0:7}" = "--seed=" ]; then
    SEED="${ARGI#--seed=*}"
  elif [ "${ARGI:0:11}" = "--center_x=" ]; then
    CENTER_X="${ARGI#--center_x=*}"
  elif [ "${ARGI:0:11}" = "--center_y=" ]; then
    CENTER_Y="${ARGI#--center_y=*}"
  elif [ "${ARGI:0:14}" = "--base_radius=" ]; then
    BASE_RADIUS="${ARGI#--base_radius=*}"
  elif [ "${ARGI:0:16}" = "--radius_jitter=" ]; then
    RADIUS_JITTER="${ARGI#--radius_jitter=*}"
  elif [ "${ARGI:0:17}" = "--min_target_sep=" ]; then
    MIN_TARGET_SEP="${ARGI#--min_target_sep=*}"
  elif [ "${ARGI:0:14}" = "--min_leg_len=" ]; then
    MIN_LEG_LEN="${ARGI#--min_leg_len=*}"
  elif [ "${ARGI:0:15}" = "--min_pred_cpa=" ]; then
    MIN_PRED_CPA="${ARGI#--min_pred_cpa=*}"
  elif [ "${ARGI:0:18}" = "--repost_interval=" ]; then
    REPOST_INTERVAL="${ARGI#--repost_interval=*}"
  elif [ "${ARGI:0:14}" = "--traffic_bin=" ]; then
    TRAFFIC_BIN="${ARGI#--traffic_bin=*}"
  elif [ "${ARGI:0:13}" = "--dumb_vname=" ]; then
    DUMB_VNAME="${ARGI#--dumb_vname=*}"
  fi
done

NSFLAGS="--strict --force"
if [ "${AUTO_LAUNCHED}" = "no" ]; then
  NSFLAGS="--interactive --force"
fi

nsplug meta_shoreside.moos targ_shoreside.moos $NSFLAGS WARP=$TIME_WARP \
  IP_ADDR=$IP_ADDR MOOS_PORT=$MOOS_PORT PSHARE_PORT=$PSHARE_PORT LAUNCH_GUI=$LAUNCH_GUI \
  VNAMES=$VNAMES MMOD=$MMOD REQUIRED_NODES=$REQUIRED_NODES \
  EVAL_TIME_SECS=$EVAL_TIME_SECS MISSION_TIMEOUT_SECS=$MISSION_TIMEOUT_SECS \
  DEPLOY_DELAY_SECS=$DEPLOY_DELAY_SECS CLOSEST_MIN=$CLOSEST_MIN CLOSEST_MAX=$CLOSEST_MAX \
  NEAR_MISS_MAX=$NEAR_MISS_MAX BATCHES_MIN=$BATCHES_MIN \
  SEED=$SEED CENTER_X=$CENTER_X CENTER_Y=$CENTER_Y \
  BASE_RADIUS=$BASE_RADIUS RADIUS_JITTER=$RADIUS_JITTER \
  MIN_TARGET_SEP=$MIN_TARGET_SEP MIN_LEG_LEN=$MIN_LEG_LEN MIN_PRED_CPA=$MIN_PRED_CPA \
  REPOST_INTERVAL=$REPOST_INTERVAL DUMB_VNAME=$DUMB_VNAME

if [ "${JUST_MAKE}" = "yes" ]; then
  echo "$ME: Targ files made; exiting without launch."
  exit 0
fi

if [ "$TRAFFIC_BIN" = "" ]; then
  if [ -x "../../../bin/pTrafficManager" ]; then
    TRAFFIC_BIN="../../../bin/pTrafficManager"
  else
    TRAFFIC_BIN="$(command -v pTrafficManager 2>/dev/null)"
  fi
fi

if [ "$TRAFFIC_BIN" = "" ] || [ ! -x "$TRAFFIC_BIN" ]; then
  echo "$ME: Unable to locate pTrafficManager binary"
  exit 1
fi

echo "Launching Shoreside MOOS Community. WARP=$TIME_WARP"
pAntler targ_shoreside.moos >& /dev/null &
echo "Done Launching Shoreside Community"

if [ "${JUST_MAKE}" != "yes" ]; then
  "$TRAFFIC_BIN" targ_shoreside.moos --alias=pTrafficManager >& /dev/null &
fi

if [ "${AUTO_LAUNCHED}" = "yes" ]; then
  exit 0
fi

uMAC targ_shoreside.moos
trap "" SIGINT
kill -- -$$
