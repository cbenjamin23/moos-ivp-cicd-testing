#!/bin/bash 
#------------------------------------------------------------
#   Script: init_field.sh
#   Author: M.Benjamin
#   LastEd: May 26 2024
#------------------------------------------------------------
#  Part 1: A convenience function for producing terminal
#          debugging/status output depending on verbosity.
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }

#------------------------------------------------------------
#  Part 2: Set global variable default values
#------------------------------------------------------------
ME=`basename "$0"`
VEHICLE_AMT="2"
VERBOSE=""
RAND_VPOS="no"
MMOD="head_on_colregs_pass"

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
	echo "$ME [OPTIONS] [time_warp]                      "
	echo "                                               "
	echo "Options:                                       "
	echo "  --amt=N            Num vehicles to launch    "
	echo "  --verbose, -v      Verbose, confirm values   "
	echo "  --rand, -r         Rand vehicle positions    "
	echo "  --mmod=<mod>       Mission variation/mod     "
        echo "                     Classification:"
        echo "                       head_on_colregs_pass"
        echo "                       head_on_centerline_pass"
        echo "                       head_on_cpa_fallback_pass"
        echo "                       head_on_port_offset_pass"
        echo "                       head_on_starboard_offset_pass"
        echo "                       crossing_starboard_giveway_pass"
        echo "                       crossing_starboard_giveway_far_pass"
        echo "                       crossing_starboard_giveway_close_pass"
        echo "                       crossing_port_standon_pass"
        echo "                       crossing_port_standon_unsure_bow_pass"
        echo "                       crossing_port_standon_stern_pass"
        echo "                       crossing_port_standon_far_pass"
        echo "                       crossing_port_standon_far_unsure_bow_pass"
        echo "                       crossing_port_standon_close_pass"
        echo "                       crossing_port_standon_close_unsure_bow_pass"
        echo "                       crossing_port_standon_unsure_pass"
        echo "                       overtaking_starboard_pass"
        echo "                       overtaking_starboard_mirror_pass"
        echo "                       overtaking_starboard_small_gap_pass"
        echo "                       overtaking_starboard_large_gap_pass"
        echo "                       overtaking_starboard_mirror_small_gap_pass"
        echo "                       overtaking_starboard_mirror_large_gap_pass"
        echo "                     Thresholds:"
        echo "                       head_on_thresh_below_pass"
        echo "                       head_on_thresh_edge_pass"
        echo "                       head_on_thresh_above_pass"
        echo "                       head_on_thresh_below_mirror_pass"
        echo "                       head_on_thresh_edge_mirror_pass"
        echo "                       head_on_thresh_above_mirror_pass"
       exit 0;
    elif [ "${ARGI:0:6}" = "--amt=" ]; then
        VEHICLE_AMT="${ARGI#--amt=*}"
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
	VERBOSE=$ARGI
    elif [ "${ARGI}" = "--rand" -o "${ARGI}" = "-r" ]; then
        RAND_VPOS="yes"
    elif [ "${ARGI:0:7}" = "--mmod=" ]; then
        MMOD="${ARGI#--mmod=*}"

    else 
	echo "$ME: Bad Arg: $ARGI. Exit Code 1."
	exit 1
    fi
done

#------------------------------------------------------------
#  Part 4: Source local coordinate grid if it exits
#------------------------------------------------------------

#------------------------------------------------------------
#  Part 5: Set starting positions, speeds, vnames, colors
#------------------------------------------------------------
if [ "$VEHICLE_AMT" != "2" ]; then
    echo "$ME: colregs_unit requires --amt=2. Exit Code 2."
    exit 2
fi

case "$MMOD" in
    head_on_colregs_pass|head_on_centerline_pass)
        A_POS="0,-40,180"
        B_POS="0,-130,0"
        A_DEST="0,-130"
        B_DEST="0,-40"
        A_SPD="1.4"
        B_SPD="1.4"
        ;;
    head_on_port_offset_pass)
        A_POS="-6,-40,180"
        B_POS="6,-130,0"
        A_DEST="6,-130"
        B_DEST="-6,-40"
        A_SPD="1.4"
        B_SPD="1.4"
        ;;
    head_on_starboard_offset_pass)
        A_POS="6,-40,180"
        B_POS="-6,-130,0"
        A_DEST="-6,-130"
        B_DEST="6,-40"
        A_SPD="1.4"
        B_SPD="1.4"
        ;;
    head_on_cpa_fallback_pass)
        A_POS="0,-70,180"
        B_POS="0,-120,28"
        A_DEST="0,-130"
        B_DEST="44,-40"
        A_SPD="1.4"
        B_SPD="1.4"
        ;;
    head_on_thresh_below_pass)
        A_POS="0,-70,180"
        B_POS="0,-120,8"
        A_DEST="0,-130"
        B_DEST="13,-40"
        A_SPD="1.4"
        B_SPD="1.4"
        ;;
    head_on_thresh_edge_pass)
        A_POS="0,-70,180"
        B_POS="0,-120,12"
        A_DEST="0,-130"
        B_DEST="19,-40"
        A_SPD="1.4"
        B_SPD="1.4"
        ;;
    head_on_thresh_above_pass)
        A_POS="0,-70,180"
        B_POS="0,-120,16"
        A_DEST="0,-130"
        B_DEST="26,-40"
        A_SPD="1.4"
        B_SPD="1.4"
        ;;
    head_on_thresh_below_mirror_pass)
        A_POS="0,-70,180"
        B_POS="0,-120,352"
        A_DEST="0,-130"
        B_DEST="-13,-40"
        A_SPD="1.4"
        B_SPD="1.4"
        ;;
    head_on_thresh_edge_mirror_pass)
        A_POS="0,-70,180"
        B_POS="0,-120,348"
        A_DEST="0,-130"
        B_DEST="-19,-40"
        A_SPD="1.4"
        B_SPD="1.4"
        ;;
    head_on_thresh_above_mirror_pass)
        A_POS="0,-70,180"
        B_POS="0,-120,344"
        A_DEST="0,-130"
        B_DEST="-26,-40"
        A_SPD="1.4"
        B_SPD="1.4"
        ;;
    crossing_starboard_giveway_pass)
        A_POS="-70,-80,90"
        B_POS="10,-130,0"
        A_DEST="50,-80"
        B_DEST="10,-20"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_starboard_giveway_far_pass)
        A_POS="-80,-80,90"
        B_POS="10,-150,0"
        A_DEST="50,-80"
        B_DEST="10,-10"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_starboard_giveway_close_pass)
        A_POS="-60,-80,90"
        B_POS="10,-115,0"
        A_DEST="45,-80"
        B_DEST="10,-15"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_pass|crossing_port_standon_unsure_bow_pass)
        A_POS="-70,-80,90"
        B_POS="10,-20,180"
        A_DEST="50,-80"
        B_DEST="10,-130"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_stern_pass)
        A_POS="-70,-80,90"
        B_POS="10,15,180"
        A_DEST="50,-80"
        B_DEST="10,-150"
        A_SPD="1.4"
        B_SPD="1.2"
        ;;
    crossing_port_standon_far_pass|crossing_port_standon_far_unsure_bow_pass)
        A_POS="-80,-80,90"
        B_POS="10,-5,180"
        A_DEST="50,-80"
        B_DEST="10,-145"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_close_pass|crossing_port_standon_close_unsure_bow_pass)
        A_POS="-60,-80,90"
        B_POS="10,-30,180"
        A_DEST="45,-80"
        B_DEST="10,-125"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_unsure_pass)
        A_POS="-55,-72,90"
        B_POS="10,-18,180"
        A_DEST="55,-72"
        B_DEST="10,-130"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    overtaking_starboard_pass)
        A_POS="-90,-80,90"
        B_POS="-35,-84,90"
        A_DEST="60,-80"
        B_DEST="60,-84"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    overtaking_starboard_mirror_pass)
        A_POS="-90,-84,90"
        B_POS="-35,-80,90"
        A_DEST="60,-84"
        B_DEST="60,-80"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    overtaking_starboard_small_gap_pass)
        A_POS="-90,-80,90"
        B_POS="-35,-82,90"
        A_DEST="60,-80"
        B_DEST="60,-82"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    overtaking_starboard_large_gap_pass)
        A_POS="-90,-80,90"
        B_POS="-35,-88,90"
        A_DEST="60,-80"
        B_DEST="60,-88"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    overtaking_starboard_mirror_small_gap_pass)
        A_POS="-90,-82,90"
        B_POS="-35,-80,90"
        A_DEST="60,-82"
        B_DEST="60,-80"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    overtaking_starboard_mirror_large_gap_pass)
        A_POS="-90,-88,90"
        B_POS="-35,-80,90"
        A_DEST="60,-88"
        B_DEST="60,-80"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    *)
        echo "$ME: Unknown mmod: $MMOD. Exit Code 3."
        exit 3
        ;;
esac

echo "$A_POS" > vpositions.txt
echo "$B_POS" >> vpositions.txt

echo "$A_DEST" > vdests.txt
echo "$B_DEST" >> vdests.txt

echo "$A_SPD" > vspeeds.txt
echo "$B_SPD" >> vspeeds.txt

echo "abe" > vnames.txt
echo "ben" >> vnames.txt

echo "yellow" > vcolors.txt
echo "red" >> vcolors.txt


#------------------------------------------------------------
#  Part 6: Set other aspects of the field, e.g., obstacles
#------------------------------------------------------------

#------------------------------------------------------------
#  Part 7: If verbose, show file contents
#------------------------------------------------------------
if [ "${VERBOSE}" != "" ]; then
    echo "--------------------------------------"
    echo "VEHICLE_AMT = $VEHICLE_AMT "
    echo "RAND_VPOS   = $RAND_VPOS   "
    echo "MMOD        = $MMOD        "
    echo "--------------------------------------(pos/spd)"
    echo "vpositions.txt:"; cat  vpositions.txt
    echo "vdests.txt:"; cat vdests.txt
    echo "vspeeds.txt:"; cat vspeeds.txt
    echo "--------------------------------------(custom)"
    echo -n "Hit any key to continue"
    read ANSWER
fi

exit 0
