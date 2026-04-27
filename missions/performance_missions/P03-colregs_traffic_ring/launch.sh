#!/bin/bash -e
#------------------------------------------------------------
#   Script: launch.sh
#  Mission: P03-colregs_traffic_ring
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT

ME=$(basename "$0")
CMD_ARGS=""
TIME_WARP=1
VERBOSE=""
JUST_MAKE=""
LOG_CLEAN=""
MMOD="baseline_circle_pass"
MAX_SPD="2.5"
SHORE_MPORT="9000"
VEH_MPORT="9001"
SHORE_PSHARE="9200"
VEH_PSHARE="9201"
XLAUNCHED="no"
NOGUI=""

SEED="17"
EVAL_TIME_SECS="300"
MISSION_TIMEOUT_SECS="420"
DEPLOY_DELAY_SECS="15"
CLOSEST_MIN="5"
CLOSEST_MAX="30"
NEAR_MISS_MAX="25"
BATCHES_MIN="5"
FIELD_RADIUS="34"
BASE_RADIUS="38"
RADIUS_JITTER="0"
MIN_TARGET_SEP="14"
MIN_LEG_LEN="20"
MIN_PRED_CPA="10"
REPOST_INTERVAL="10"
DUMB_VNAME="none"

for ARGI; do
  CMD_ARGS+=" ${ARGI}"
  if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
    echo "$ME [OPTIONS] [time_warp]"
    echo "  --mmod=<mod>       baseline_circle_pass | mixed_speed_circle_pass | endurance_circle_pass | noncoop_circle_pass"
    echo "  --dumb_vname=<v>   Vehicle launched with AVOID=false"
    echo "  --shore_mport=<n>  Shoreside MOOSDB port"
    echo "  --veh_mport=<n>    Base vehicle MOOSDB port"
    echo "  --shore_pshare=<n> Shoreside pShare port"
    echo "  --veh_pshare=<n>   Base vehicle pShare port"
    echo "  --just_make, -j    Only create targ files"
    echo "  --log_clean, -lc   Run clean.sh before launch"
    echo "  --verbose, -v      Verbose"
    echo "  --nogui, -ng       Headless launch"
    echo "  --gui              Launch with pMarineViewer"
    exit 0
  elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then
    TIME_WARP=$ARGI
  elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
    VERBOSE="yes"
  elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
    JUST_MAKE="yes"
  elif [ "${ARGI}" = "--log_clean" -o "${ARGI}" = "-lc" ]; then
    LOG_CLEAN="yes"
  elif [ "${ARGI:0:7}" = "--mmod=" ]; then
    MMOD="${ARGI#--mmod=*}"
  elif [ "${ARGI:0:14}" = "--shore_mport=" ]; then
    SHORE_MPORT="${ARGI#--shore_mport=*}"
  elif [ "${ARGI:0:12}" = "--veh_mport=" ]; then
    VEH_MPORT="${ARGI#--veh_mport=*}"
  elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
    SHORE_PSHARE="${ARGI#--shore_pshare=*}"
  elif [ "${ARGI:0:13}" = "--veh_pshare=" ]; then
    VEH_PSHARE="${ARGI#--veh_pshare=*}"
  elif [ "${ARGI:0:13}" = "--dumb_vname=" ]; then
    DUMB_VNAME="${ARGI#--dumb_vname=*}"
  elif [ "${ARGI}" = "--xlaunched" -o "${ARGI}" = "-x" ]; then
    XLAUNCHED="yes"
  elif [ "${ARGI}" = "--nogui" -o "${ARGI}" = "-ng" ]; then
    NOGUI="--nogui"
  elif [ "${ARGI}" = "--gui" ]; then
    NOGUI=""
  else
    echo "$ME: Bad arg: $ARGI"
    exit 1
  fi
done

case "$MMOD" in
  baseline_circle_pass)
    SEED="17"
    EVAL_TIME_SECS="300"
    MISSION_TIMEOUT_SECS="420"
    CLOSEST_MIN="0.1"
    CLOSEST_MAX="30"
    NEAR_MISS_MAX="999"
    BATCHES_MIN="25"
    FIELD_RADIUS="36"
    BASE_RADIUS="38"
    ;;
  mixed_speed_circle_pass)
    SEED="23"
    EVAL_TIME_SECS="300"
    MISSION_TIMEOUT_SECS="420"
    CLOSEST_MIN="0.1"
    CLOSEST_MAX="30"
    NEAR_MISS_MAX="999"
    BATCHES_MIN="25"
    FIELD_RADIUS="36"
    BASE_RADIUS="39"
    ;;
  endurance_circle_pass)
    SEED="31"
    EVAL_TIME_SECS="900"
    MISSION_TIMEOUT_SECS="1080"
    CLOSEST_MIN="0.1"
    CLOSEST_MAX="30"
    NEAR_MISS_MAX="999"
    BATCHES_MIN="80"
    FIELD_RADIUS="42"
    BASE_RADIUS="45"
    MIN_TARGET_SEP="18"
    MIN_PRED_CPA="14"
    ;;
  noncoop_circle_pass)
    SEED="41"
    EVAL_TIME_SECS="300"
    MISSION_TIMEOUT_SECS="420"
    CLOSEST_MIN="0.1"
    CLOSEST_MAX="30"
    NEAR_MISS_MAX="999"
    BATCHES_MIN="25"
    FIELD_RADIUS="36"
    BASE_RADIUS="38"
    if [ "$DUMB_VNAME" = "none" ]; then
      DUMB_VNAME="eve"
    fi
    ;;
  *)
    echo "$ME: Unknown mission mode [$MMOD]"
    exit 1
    ;;
esac

if [ "$LOG_CLEAN" = "yes" -a -f "clean.sh" ]; then
  ./clean.sh >/dev/null 2>&1 || true
fi

./init_field.sh --mmod="$MMOD" --radius="$FIELD_RADIUS"

VEHPOS=($(cat vpositions.txt))
SPEEDS=($(cat vspeeds.txt))
VNAMES=($(cat vnames.txt))
VCOLOR=($(cat vcolors.txt))
VAMT="${#VNAMES[@]}"

if [ "$VERBOSE" = "yes" ]; then
  echo "MMOD=$MMOD VAMT=$VAMT SEED=$SEED EVAL=$EVAL_TIME_SECS"
  echo "VNAMES=${VNAMES[*]}"
  echo "VEHPOS=${VEHPOS[*]}"
  echo "SPEEDS=${SPEEDS[*]}"
  read -r -p "Hit any key to continue launch "
fi

VARGS=" --sim --auto --max_spd=$MAX_SPD $TIME_WARP "
if [ "$JUST_MAKE" = "yes" ]; then
  VARGS="$VARGS --just_make"
fi
if [ "$VERBOSE" = "yes" ]; then
  VARGS="$VARGS --verbose"
fi
VARGS="$VARGS --shore_pshare=$SHORE_PSHARE "

for IX in $(seq 1 "$VAMT"); do
  idx=$((IX - 1))
  veh_mport_ix=$((VEH_MPORT + idx))
  veh_pshare_ix=$((VEH_PSHARE + idx))
  up_vname=$(echo "${VNAMES[$idx]}" | tr '[:lower:]' '[:upper:]')
  IVARGS="$VARGS --mport=$veh_mport_ix --pshare=$veh_pshare_ix"
  IVARGS="$IVARGS --start_pos=${VEHPOS[$idx]} --stock_spd=${SPEEDS[$idx]}"
  IVARGS="$IVARGS --vname=${VNAMES[$idx]} --up_vname=$up_vname --color=${VCOLOR[$idx]}"
  if [ "$DUMB_VNAME" != "none" ] && [ "${VNAMES[$idx]}" = "$DUMB_VNAME" ]; then
    IVARGS="$IVARGS --avoid=false"
  fi
  vecho "Launching vehicle: $IVARGS"
  ./launch_vehicle.sh $IVARGS
  sleep 0.4
done

SARGS=" --auto --mport=$SHORE_MPORT --pshare=$SHORE_PSHARE $TIME_WARP"
if [ "$JUST_MAKE" = "yes" ]; then
  SARGS="$SARGS --just_make"
fi
if [ "$VERBOSE" = "yes" ]; then
  SARGS="$SARGS --verbose"
fi
SARGS="$SARGS $NOGUI --vnames=$(IFS=:; echo "${VNAMES[*]}") --mmod=$MMOD"
SARGS="$SARGS --required_nodes=$VAMT --eval_time=$EVAL_TIME_SECS"
SARGS="$SARGS --mission_timeout=$MISSION_TIMEOUT_SECS --deploy_delay=$DEPLOY_DELAY_SECS"
SARGS="$SARGS --closest_min=$CLOSEST_MIN --closest_max=$CLOSEST_MAX"
SARGS="$SARGS --near_miss_max=$NEAR_MISS_MAX --batches_min=$BATCHES_MIN"
SARGS="$SARGS --seed=$SEED"
SARGS="$SARGS --center_x=23 --center_y=-82 --base_radius=$BASE_RADIUS"
SARGS="$SARGS --radius_jitter=$RADIUS_JITTER"
SARGS="$SARGS --min_target_sep=$MIN_TARGET_SEP --min_leg_len=$MIN_LEG_LEN"
SARGS="$SARGS --min_pred_cpa=$MIN_PRED_CPA --repost_interval=$REPOST_INTERVAL"
SARGS="$SARGS --dumb_vname=$DUMB_VNAME"
vecho "Launching shoreside: $SARGS"
./launch_shoreside.sh $SARGS

if [ "$JUST_MAKE" = "yes" ]; then
  echo "$ME: Targ files made; exiting without launch."
  exit 0
fi

if [ "$XLAUNCHED" != "yes" ]; then
  uMAC --paused targ_shoreside.moos
  trap "" SIGINT
  echo; echo "$ME: Halting all apps"
  kill -- -$$
fi

exit 0
