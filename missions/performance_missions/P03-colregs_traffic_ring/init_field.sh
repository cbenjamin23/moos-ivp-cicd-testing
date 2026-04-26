#!/bin/bash
#------------------------------------------------------------
#   Script: init_field.sh
#   Author: Charles Benjamin
#------------------------------------------------------------

ME=$(basename "$0")
MMOD="baseline_circle_pass"
CENTER_X="23"
CENTER_Y="-82"
RADIUS="34"

for ARGI; do
  if [ "${ARGI:0:7}" = "--mmod=" ]; then
    MMOD="${ARGI#--mmod=*}"
  elif [ "${ARGI:0:11}" = "--center_x=" ]; then
    CENTER_X="${ARGI#--center_x=*}"
  elif [ "${ARGI:0:11}" = "--center_y=" ]; then
    CENTER_Y="${ARGI#--center_y=*}"
  elif [ "${ARGI:0:9}" = "--radius=" ]; then
    RADIUS="${ARGI#--radius=*}"
  elif [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
    echo "$ME [--mmod=<mode>] [--center_x=<x>] [--center_y=<y>] [--radius=<r>]"
    exit 0
  fi
done

NAMES=(abe ben cal deb eve)
COLORS=(yellow red dodger_blue green orange)
ANGLES=(90 18 -54 -126 -198)

case "$MMOD" in
  baseline_circle_pass)
    SPEEDS=(1.75 1.75 1.75 1.75 1.75)
    ;;
  noncoop_circle_pass)
    SPEEDS=(1.50 1.50 1.50 1.50 1.50)
    ;;
  mixed_speed_circle_pass|endurance_circle_pass)
    SPEEDS=(1.55 1.67 1.79 1.91 2.03)
    ;;
  *)
    echo "$ME: Unknown mission mode [$MMOD]"
    exit 1
    ;;
esac

: > vpositions.txt
: > vspeeds.txt
: > vnames.txt
: > vcolors.txt

for ix in "${!NAMES[@]}"; do
  ang="${ANGLES[$ix]}"
  pos=$(awk -v cx="$CENTER_X" -v cy="$CENTER_Y" -v r="$RADIUS" -v a="$ang" '
    BEGIN {
      rad = a * 3.141592653589793 / 180.0;
      x = cx + r * cos(rad);
      y = cy + r * sin(rad);
      hdg = a - 90.0;
      while(hdg < 0) hdg += 360;
      while(hdg >= 360) hdg -= 360;
      printf("x=%.2f,y=%.2f,heading=%.1f", x, y, hdg);
    }')
  echo "$pos" >> vpositions.txt
  echo "${SPEEDS[$ix]}" >> vspeeds.txt
  echo "${NAMES[$ix]}" >> vnames.txt
  echo "${COLORS[$ix]}" >> vcolors.txt
done

exit 0
