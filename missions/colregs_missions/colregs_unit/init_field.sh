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
        echo "                       crossing_starboard_giveway_bow_pass"
        echo "                       crossing_port_standon_pass"
        echo "                       crossing_port_standon_unsure_bow_pass"
        echo "                       crossing_port_standon_stern_pass"
        echo "                       crossing_port_standon_far_pass"
        echo "                       crossing_port_standon_exec_far_pass"
        echo "                       crossing_port_standon_far_unsure_bow_pass"
        echo "                       crossing_port_standon_close_pass"
        echo "                       crossing_port_standon_close_unsure_bow_pass"
        echo "                       crossing_port_standon_unsure_pass"
        echo "                       crossing_port_standon_southwest_unsure_bow_pass"
        echo "                       crossing_port_standon_southwest_neither_pass"
        echo "                       crossing_port_standon_southwest_unsure_pass"
        echo "                       crossing_port_standon_southwest_stern_pass"
        echo "                       crossing_port_standon_southwest_outer_below_pass"
        echo "                       crossing_port_standon_southwest_outer_edge_pass"
        echo "                       crossing_port_standon_southwest_outer_above_pass"
        echo "                       overtaking_starboard_pass"
        echo "                       overtaking_starboard_range_far_pass"
        echo "                       overtaking_starboard_range_far_small_gap_pass"
        echo "                       overtaking_starboard_range_far_large_gap_pass"
        echo "                       overtaking_starboard_mirror_pass"
        echo "                       overtaking_starboard_mirror_range_far_pass"
        echo "                       overtaking_starboard_mirror_range_far_small_gap_pass"
        echo "                       overtaking_starboard_mirror_range_far_large_gap_pass"
        echo "                       overtaking_starboard_small_gap_pass"
        echo "                       overtaking_starboard_large_gap_pass"
        echo "                       overtaking_starboard_mirror_small_gap_pass"
        echo "                       overtaking_starboard_mirror_large_gap_pass"
        echo "                       overtaken_port_standon_pass"
        echo "                       overtaken_port_standon_midrange_pass"
        echo "                       overtaken_port_standon_range_far_pass"
        echo "                       overtaken_port_standon_range_edge_pass"
        echo "                       overtaken_port_standon_range_close_pass"
        echo "                       overtaken_port_standon_cpa_wide_pass"
        echo "                       overtaken_port_standon_cpa_edge_pass"
        echo "                       overtaken_port_standon_cpa_above_pass"
        echo "                       overtaken_port_standon_gate_below_pass"
        echo "                       overtaken_port_standon_gate_edge_pass"
        echo "                       overtaken_port_standon_gate_above_pass"
        echo "                       overtaken_starboard_standon_pass"
        echo "                       overtaken_starboard_standon_midrange_pass"
        echo "                       overtaken_starboard_standon_range_far_pass"
        echo "                       overtaken_starboard_standon_gate_below_pass"
        echo "                       overtaken_starboard_standon_gate_edge_pass"
        echo "                       overtaken_starboard_standon_gate_above_pass"
        echo "                     Thresholds:"
	        echo "                       head_on_thresh_below_pass"
	        echo "                       head_on_thresh_edge_pass"
	        echo "                       head_on_thresh_above_pass"
	        echo "                       head_on_thresh_below_mirror_pass"
	        echo "                       head_on_thresh_edge_mirror_pass"
	        echo "                       head_on_thresh_above_mirror_pass"
	        echo "                       overtaking_thresh_below_pass"
	        echo "                       overtaking_thresh_edge_pass"
	        echo "                       overtaking_thresh_above_pass"
	        echo "                       overtaking_thresh_below_mirror_pass"
	        echo "                       overtaking_thresh_edge_mirror_pass"
	        echo "                       overtaking_thresh_above_mirror_pass"
	        echo "                       giveway_bowdist_below_pass"
	        echo "                       giveway_bowdist_edge_pass"
	        echo "                       giveway_bowdist_above_pass"
	        echo "                       giveway_bowdist_below_mirror_pass"
	        echo "                       giveway_bowdist_edge_mirror_pass"
	        echo "                       giveway_bowdist_above_mirror_pass"
	        echo "                       giveway_turngap_below_pass"
	        echo "                       giveway_turngap_edge_pass"
	        echo "                       giveway_turngap_above_pass"
	        echo "                       giveway_turngap_below_mirror_pass"
	        echo "                       giveway_turngap_edge_mirror_pass"
	        echo "                       giveway_turngap_above_mirror_pass"
	        echo "                     Calibrated turn probes:"
        echo "                       crossing_port_standon_turn_unsure_stern_below_pass"
        echo "                       crossing_port_standon_turn_unsure_stern_edge_pass"
        echo "                       crossing_port_standon_turn_unsure_stern_above_pass"
        echo "                       crossing_port_standon_turn_neither_edge_pass"
        echo "                       crossing_port_standon_turn_neither_above_pass"
        echo "                       crossing_port_standon_speed_unsure_stern_probe"
	        echo "                       Note: the turn-unsure-stern probes"
	        echo "                       remain calibration-only, while the"
	        echo "                       turn-neither pocket is the shorter"
	        echo "                       release-controlled family used by H02."
        echo "                       The new low-speed stern probe can"
	        echo "                       also hit moving 34, but repeated"
	        echo "                       cold starts still split across"
	        echo "                       30 / 34 / 50."
	        echo "                       crossing_port_standon_unsure_stern_pass"
	        echo "                       crossing_port_standon_unsure_stern_diag_probe"
	        echo "                       Note: the original unsure-stern"
	        echo "                       horizontal mode is now treated as"
	        echo "                       a legacy bad-fit source."
	        echo "                       crossing_port_standon_band350_unsure_pass"
	        echo "                       crossing_port_standon_band350_unsure_bow_pass"
	        echo "                       crossing_port_standon_band350_unsure_stern_pass"
	        echo "                       crossing_port_standon_band350_bow_pass"
	        echo "                       crossing_port_standon_band315_unsure_pass"
	        echo "                       crossing_port_standon_band315_unsure_bow_pass"
	        echo "                       crossing_port_standon_band315_bow_pass"
	        echo "                       crossing_port_standon_band350_to315_transition_below_pass"
	        echo "                       crossing_port_standon_band350_to315_transition_edge_pass"
	        echo "                       crossing_port_standon_band350_to315_transition_above_pass"
	        echo "                     Note:"
	        echo "                       H02 harness case names such as"
	        echo "                       standon_band315_* and standon_band270_*"
	        echo "                       are harness aliases, not direct --mmod"
	        echo "                       values in this mission stem."
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
    overtaking_thresh_below_pass)
        A_POS="-90,-48,90"
        B_POS="-35,-84,90"
        A_DEST="60,-48"
        B_DEST="60,-84"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    overtaking_thresh_edge_pass)
        A_POS="-90,-47,90"
        B_POS="-35,-84,90"
        A_DEST="60,-47"
        B_DEST="60,-84"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    overtaking_thresh_above_pass)
        A_POS="-90,-46,90"
        B_POS="-35,-84,90"
        A_DEST="60,-46"
        B_DEST="60,-84"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    overtaking_thresh_below_mirror_pass)
        A_POS="-90,-122,90"
        B_POS="-35,-84,90"
        A_DEST="60,-122"
        B_DEST="60,-84"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    overtaking_thresh_edge_mirror_pass)
        A_POS="-90,-121,90"
        B_POS="-35,-84,90"
        A_DEST="60,-121"
        B_DEST="60,-84"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    overtaking_thresh_above_mirror_pass)
        A_POS="-90,-120,90"
        B_POS="-35,-84,90"
        A_DEST="60,-120"
        B_DEST="60,-84"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    giveway_bowdist_below_pass)
        A_POS="-71,-48,90"
        B_POS="10,-130,0"
        A_DEST="50,-48"
        B_DEST="10,-20"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    giveway_bowdist_edge_pass)
        A_POS="-70,-48,90"
        B_POS="10,-130,0"
        A_DEST="49,-48"
        B_DEST="10,-20"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    giveway_bowdist_above_pass)
        A_POS="-61,-48,90"
        B_POS="10,-130,0"
        A_DEST="40,-48"
        B_DEST="10,-20"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    giveway_bowdist_below_mirror_pass)
        A_POS="71,48,270"
        B_POS="-10,130,180"
        A_DEST="-50,48"
        B_DEST="-10,20"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    giveway_bowdist_edge_mirror_pass)
        A_POS="70,48,270"
        B_POS="-10,130,180"
        A_DEST="-49,48"
        B_DEST="-10,20"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    giveway_bowdist_above_mirror_pass)
        A_POS="61,48,270"
        B_POS="-10,130,180"
        A_DEST="-40,48"
        B_DEST="-10,20"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    giveway_turngap_below_pass)
        A_POS="-25.377,11.833,115"
        B_POS="41.652,-50.428,330"
        A_DEST="83.38,-38.881"
        B_DEST="-18.348,53.495"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    giveway_turngap_edge_pass)
        A_POS="-25.377,11.833,115"
        B_POS="41.529,-50.552,330"
        A_DEST="83.38,-38.881"
        B_DEST="-18.471,53.371"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    giveway_turngap_above_pass)
        A_POS="-25.377,11.833,115"
        B_POS="41.26,-50.827,330"
        A_DEST="83.38,-38.881"
        B_DEST="-18.74,53.096"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    giveway_turngap_below_mirror_pass)
        A_POS="25.377,-11.833,295"
        B_POS="-41.652,50.428,150"
        A_DEST="-83.38,38.881"
        B_DEST="18.348,-53.495"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    giveway_turngap_edge_mirror_pass)
        A_POS="25.377,-11.833,295"
        B_POS="-41.600,50.480,150"
        A_DEST="-83.38,38.881"
        B_DEST="18.400,-53.443"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    giveway_turngap_above_mirror_pass)
        A_POS="25.377,-11.833,295"
        B_POS="-41.26,50.827,150"
        A_DEST="-83.38,38.881"
        B_DEST="18.74,-53.096"
        A_SPD="1.4"
        B_SPD="1.3"
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
    crossing_starboard_giveway_bow_pass)
        A_POS="-27,-95,90"
        B_POS="10,-108,0"
        A_DEST="70,-95"
        B_DEST="10,-5"
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
    crossing_port_standon_cpa_wide_pass)
        A_POS="-30,-90,90"
        B_POS="-6,-92,244"
        A_DEST="110,-90"
        B_DEST="-144.638,-105.484"
        A_SPD="1.4"
        B_SPD="1.2"
        ;;
    crossing_port_standon_cpa_edge_pass)
        A_POS="-30,-90,90"
        B_POS="-6,-88,244"
        A_DEST="110,-90"
        B_DEST="-144.638,-105.484"
        A_SPD="1.4"
        B_SPD="1.2"
        ;;
    crossing_port_standon_cpa_above_pass)
        A_POS="-30,-90,90"
        B_POS="-6,-86,244"
        A_DEST="110,-90"
        B_DEST="-144.638,-105.484"
        A_SPD="1.4"
        B_SPD="1.2"
        ;;
    crossing_port_standon_unsure_stern_pass)
        A_POS="-35,-80,90"
        B_POS="10,-76,180"
        A_DEST="50,-80"
        B_DEST="10,-145"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_unsure_stern_diag_probe)
        A_POS="-30,-90,90"
        B_POS="10,-78,180"
        A_DEST="45,-77"
        B_DEST="10,-145"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_far_pass|crossing_port_standon_far_unsure_bow_pass)
        A_POS="-80,-80,90"
        B_POS="10,-5,180"
        A_DEST="50,-80"
        B_DEST="10,-145"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_exec_far_pass)
        A_POS="-76,-80,90"
        B_POS="10,-12,180"
        A_DEST="45,-80"
        B_DEST="10,-138"
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
    crossing_port_standon_southwest_unsure_bow_pass)
        A_POS="-30,-90,90"
        B_POS="10,-82,225"
        A_DEST="110,-90"
        B_DEST="-89,-181"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_southwest_neither_pass)
        A_POS="-30,-90,90"
        B_POS="10,-79,225"
        A_DEST="250,-90"
        B_DEST="-89,-178"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_southwest_neither_edge_pass)
        A_POS="10,-90,90"
        B_POS="10,-79,225"
        A_DEST="250,-90"
        B_DEST="-89,-178"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_southwest_neither_above_pass)
        A_POS="20,-90,90"
        B_POS="10,-79,225"
        A_DEST="250,-90"
        B_DEST="-89,-178"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_southwest_unsure_pass)
        A_POS="-30,-90,90"
        B_POS="10,-76,225"
        A_DEST="110,-90"
        B_DEST="-89,-175"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_southwest_stern_pass)
        A_POS="-30,-90,90"
        B_POS="10,-70,225"
        A_DEST="110,-90"
        B_DEST="-89,-169"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_southwest_outer_below_pass)
        A_POS="-30,-90,90"
        B_POS="10,-70.5,225"
        A_DEST="110,-90"
        B_DEST="-89,-169.5"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_southwest_outer_edge_pass)
        A_POS="-30,-90,90"
        B_POS="10,-70,225"
        A_DEST="110,-90"
        B_DEST="-89,-169"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_southwest_outer_above_pass)
        A_POS="-30,-90,90"
        B_POS="10,-69,225"
        A_DEST="110,-90"
        B_DEST="-89,-168"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_turn_unsure_stern_below_pass)
        A_POS="-30,-90,90"
        B_POS="-6,-86,244"
        A_DEST="110,-90"
        B_DEST="-144.638,-105.484"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_turn_unsure_stern_edge_pass)
        A_POS="-30,-90,90"
        B_POS="-6,-87,246"
        A_DEST="110,-90"
        B_DEST="-144.638,-105.484"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_turn_unsure_stern_above_pass)
        A_POS="-30,-90,90"
        B_POS="-6,-87.5,246"
        A_DEST="110,-90"
        B_DEST="-144.638,-105.484"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_turn_neither_edge_pass)
        A_POS="-30,-90,90"
        B_POS="-6,-92,246"
        A_DEST="250,-90"
        B_DEST="-220,-117"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_turn_neither_above_pass)
        A_POS="-30,-90,90"
        B_POS="-6,-92.5,246"
        A_DEST="250,-90"
        B_DEST="-220,-117"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_turn_neither_portside_pass)
        A_POS="10,-90,90"
        B_POS="-6,-92,246"
        A_DEST="250,-90"
        B_DEST="-220,-117"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_neither_portside_edge_pass)
        A_POS="-2,-80,90"
        B_POS="0,-100,45"
        A_DEST="250,-80"
        B_DEST="60,-40"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_neither_portside_above_pass)
        A_POS="2,-80,90"
        B_POS="0,-100,45"
        A_DEST="250,-80"
        B_DEST="60,-40"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_neither_heading135_edge_pass)
        A_POS="-1,-90,90"
        B_POS="0,-80,135"
        A_DEST="250,-90"
        B_DEST="60,-120"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_neither_heading135_above_pass)
        A_POS="0,-90,90"
        B_POS="0,-80,135"
        A_DEST="250,-90"
        B_DEST="60,-120"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_speed_unsure_stern_probe)
        A_POS="-30,-90,90"
        B_POS="-6,-86,244"
        A_DEST="110,-90"
        B_DEST="-140.819,-151.756"
        A_SPD="1.4"
        B_SPD="0.7"
        ;;
    crossing_port_standon_band350_unsure_pass)
        A_POS="-35,-80,90"
        B_POS="10,-70,180"
        A_DEST="50,-80"
        B_DEST="10,-145"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_band350_unsure_bow_pass)
        A_POS="-35,-80,90"
        B_POS="10,-68,180"
        A_DEST="50,-80"
        B_DEST="10,-145"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_band350_unsure_stern_pass)
        A_POS="-35,-80,90"
        B_POS="10,-78,180"
        A_DEST="50,-80"
        B_DEST="10,-145"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_band350_bow_pass)
        A_POS="-35,-80,90"
        B_POS="10,-64,180"
        A_DEST="50,-80"
        B_DEST="10,-145"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_band315_unsure_pass)
        A_POS="-30,-90,90"
        B_POS="0,-82,225"
        A_DEST="110,-90"
        B_DEST="-89,-181"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_band315_unsure_bow_pass)
        A_POS="-30,-90,90"
        B_POS="0,-80,225"
        A_DEST="110,-90"
        B_DEST="-89,-179"
        A_SPD="1.4"
        B_SPD="1.2"
        ;;
    crossing_port_standon_band315_bow_pass)
        A_POS="-35,-80,90"
        B_POS="0,-64,180"
        A_DEST="50,-80"
        B_DEST="0,-145"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_band350_to315_transition_below_pass)
        A_POS="-35,-80,90"
        B_POS="-14.5,-74,180"
        A_DEST="50,-80"
        B_DEST="-14.5,-145"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_band350_to315_transition_edge_pass)
        A_POS="-35,-80,90"
        B_POS="-14.5,-72.5,180"
        A_DEST="50,-80"
        B_DEST="-14.5,-145"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_band350_to315_transition_above_pass)
        A_POS="-35,-80,90"
        B_POS="-14.5,-70.5,180"
        A_DEST="50,-80"
        B_DEST="-14.5,-145"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_band270_to180_transition_below_pass)
        A_POS="-30,-90,90"
        B_POS="0,-84,225"
        A_DEST="110,-90"
        B_DEST="-89,-181"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_band270_to180_transition_edge_pass)
        A_POS="-30,-90,90"
        B_POS="0,-82,225"
        A_DEST="110,-90"
        B_DEST="-89,-181"
        A_SPD="1.4"
        B_SPD="1.3"
        ;;
    crossing_port_standon_band270_to180_transition_above_pass)
        A_POS="-30,-90,90"
        B_POS="0,-80,225"
        A_DEST="110,-90"
        B_DEST="-89,-181"
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
    overtaking_starboard_range_far_pass)
        A_POS="-105,-80,90"
        B_POS="-35,-84,90"
        A_DEST="60,-80"
        B_DEST="60,-84"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    overtaking_starboard_range_far_small_gap_pass)
        A_POS="-105,-80,90"
        B_POS="-35,-82,90"
        A_DEST="60,-80"
        B_DEST="60,-82"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    overtaking_starboard_range_far_large_gap_pass)
        A_POS="-105,-80,90"
        B_POS="-35,-96,90"
        A_DEST="60,-80"
        B_DEST="60,-96"
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
    overtaking_starboard_mirror_range_far_pass)
        A_POS="-105,-84,90"
        B_POS="-35,-80,90"
        A_DEST="60,-84"
        B_DEST="60,-80"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    overtaking_starboard_mirror_range_far_small_gap_pass)
        A_POS="-105,-81,90"
        B_POS="-35,-80,90"
        A_DEST="60,-81"
        B_DEST="60,-80"
        A_SPD="1.6"
        B_SPD="1.0"
        ;;
    overtaking_starboard_mirror_range_far_large_gap_pass)
        A_POS="-105,-88,90"
        B_POS="-35,-80,90"
        A_DEST="60,-88"
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
    overtaken_port_standon_pass)
        A_POS="-35,-80,90"
        B_POS="-70,-76,90"
        A_DEST="70,-80"
        B_DEST="70,-76"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_port_standon_midrange_pass)
        A_POS="-35,-80,90"
        B_POS="-75,-76,90"
        A_DEST="50,-80"
        B_DEST="50,-76"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_port_standon_range_far_pass)
        A_POS="-35,-80,90"
        B_POS="-105,-76,90"
        A_DEST="70,-80"
        B_DEST="70,-76"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_port_standon_range_edge_pass)
        A_POS="-35,-80,90"
        B_POS="-52,-76,90"
        A_DEST="70,-80"
        B_DEST="70,-76"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_port_standon_range_close_pass)
        A_POS="-35,-80,90"
        B_POS="-50,-76,90"
        A_DEST="70,-80"
        B_DEST="70,-76"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_port_standon_cpa_wide_pass)
        A_POS="-35,-80,90"
        B_POS="-70,-64,90"
        A_DEST="70,-80"
        B_DEST="70,-64"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_port_standon_cpa_edge_pass)
        A_POS="-35,-80,90"
        B_POS="-50,-75,90"
        A_DEST="70,-80"
        B_DEST="70,-75"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_port_standon_cpa_above_pass)
        A_POS="-35,-80,90"
        B_POS="-50,-76,90"
        A_DEST="70,-80"
        B_DEST="70,-76"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_port_standon_gate_below_pass)
        A_POS="-35,-80,90"
        B_POS="-70,-58,90"
        A_DEST="70,-80"
        B_DEST="70,-58"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_port_standon_gate_edge_pass)
        A_POS="-35,-80,90"
        B_POS="-70,-62,90"
        A_DEST="70,-80"
        B_DEST="70,-62"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_port_standon_gate_above_pass)
        A_POS="-35,-80,90"
        B_POS="-70,-66,90"
        A_DEST="70,-80"
        B_DEST="70,-66"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_starboard_standon_pass)
        A_POS="-35,-80,90"
        B_POS="-70,-84,90"
        A_DEST="70,-80"
        B_DEST="70,-84"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_starboard_standon_midrange_pass)
        A_POS="-35,-80,90"
        B_POS="-85,-85,90"
        A_DEST="50,-80"
        B_DEST="54,-85"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_starboard_standon_range_far_pass)
        A_POS="-35,-80,90"
        B_POS="-105,-84,90"
        A_DEST="70,-80"
        B_DEST="70,-84"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_starboard_standon_gate_below_pass)
        A_POS="-35,-80,90"
        B_POS="-70,-102,90"
        A_DEST="70,-80"
        B_DEST="70,-102"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_starboard_standon_gate_edge_pass)
        A_POS="-35,-80,90"
        B_POS="-70,-96.5,90"
        A_DEST="70,-80"
        B_DEST="70,-96.5"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    overtaken_starboard_standon_gate_above_pass)
        A_POS="-35,-80,90"
        B_POS="-70,-94,90"
        A_DEST="70,-80"
        B_DEST="70,-94"
        A_SPD="1.0"
        B_SPD="1.6"
        ;;
    crossing_port_standon_band270_bow_probe)
        A_POS="-70,-80,90"
        B_POS="0,120,225"
        A_DEST="50,-80"
        B_DEST="-99,21"
        A_SPD="1.4"
        B_SPD="1.2"
        ;;
    crossing_port_standon_band270_mid_probe)
        A_POS="-70,-80,90"
        B_POS="0,60,225"
        A_DEST="50,-80"
        B_DEST="-99,-39"
        A_SPD="1.4"
        B_SPD="1.2"
        ;;
    crossing_port_standon_band270_near_probe)
        A_POS="-70,-80,90"
        B_POS="0,30,225"
        A_DEST="50,-80"
        B_DEST="-99,-69"
        A_SPD="1.4"
        B_SPD="1.2"
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
