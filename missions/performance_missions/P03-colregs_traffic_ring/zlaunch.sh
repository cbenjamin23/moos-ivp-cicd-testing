#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

ME=$(basename "$0")
TIME_WARP=10
VERBOSE=""
JUST_MAKE=""
MAX_TIME="700"
ENDURANCE_MAX_TIME="1400"
NOGUI="--nogui"
MMOD=""

for ARGI; do
  if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
    echo "$ME [OPTIONS] [time_warp]"
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
  elif [ "${ARGI:0:7}" = "--mmod=" ]; then
    MMOD="${ARGI#--mmod=*}"
  fi
done

: > results.txt
if [ "$MMOD" = "endurance_circle_pass" ] && [ "$MAX_TIME" = "700" ]; then
  MAX_TIME="$ENDURANCE_MAX_TIME"
fi

xlaunch.sh --max_time=$MAX_TIME $NOGUI $JUST_MAKE $VERBOSE ${MMOD:+--mmod=$MMOD} 10

if [ "${JUST_MAKE}" = "" ]; then
  sleep 0.5
  ktm
  sleep 0.5
fi

exit 0
