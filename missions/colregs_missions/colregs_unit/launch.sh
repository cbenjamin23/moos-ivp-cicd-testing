#!/bin/bash -e
#------------------------------------------------------------
#   Script: launch.sh
#  Mission: colregs_unit
#   Author: Charles Benjamin
#   LastEd: Mar 2026
#------------------------------------------------------------
#  Part 1: Set convenience functions for producing terminal
#          debugging output, and catching SIGINT (ctrl-c).
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT

#------------------------------------------------------------
#  Part 2: Set global variable default values
#------------------------------------------------------------
ME=`basename "$0"`
CMD_ARGS=""
TIME_WARP=1
VERBOSE=""
JUST_MAKE=""
LOG_CLEAN=""
LOG_MODE="minimal"
if [ "${LOG_MODE_PREPARED:-no}" = yes ] && [ -n "${LOG_MODE_PREPARED_VALUE:-}" ]; then
    LOG_MODE="$LOG_MODE_PREPARED_VALUE"
fi
VAMT="2"
MAX_VAMT="2"
RAND_VPOS=""
MAX_SPD="2"
MMOD=""

# Launch
XLAUNCHED="no"
NOGUI=""

# Custom
MIN_UTIL_CPA="10"
MAX_UTIL_CPA="18"
SHORE_MPORT="9000"
VEH_MPORT="9001"
SHORE_PSHARE="9200"
VEH_PSHARE="9201"

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+=" ${ARGI}"
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
	echo "$ME [OPTIONS] [time_warp]                      "
	echo "                                               "
	echo "Options:                                       "
	echo "  --help, -h         Show this help message    " 
	echo "  --verbose, -v      Verbose, confirm launch   "
	echo "  --just_make, -j    Only create targ files    " 
	echo "  --log=<mode>       minimal (default) or full"
	echo "  --log_clean, -lc   Run clean.sh bef launch   " 
	echo "  --amt=N            Num vehicles to launch    "
	echo "  --rand, -r         Rand vehicle positions    "
	echo "  --max_spd=N        Max helm/sim speed        "
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
	        echo "                       crossing_port_standon_cpa_wide_pass"
	        echo "                       crossing_port_standon_cpa_edge_pass"
	        echo "                       crossing_port_standon_cpa_above_pass"
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
	        echo "                     Calibration-only probes:"
        echo "                       crossing_port_standon_turn_unsure_stern_below_pass"
        echo "                       crossing_port_standon_turn_unsure_stern_edge_pass"
        echo "                       crossing_port_standon_turn_unsure_stern_above_pass"
	        echo "                       crossing_port_standon_turn_neither_edge_pass"
	        echo "                       crossing_port_standon_turn_neither_above_pass"
        echo "                       crossing_port_standon_speed_unsure_stern_probe"
        echo "                       Note: this newer turn-driven family"
        echo "                       can hit 34, but it is not stable"
	        echo "                       enough yet for the supported gate."
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
	        echo "                     Band270 probes (calibration):"
	        echo "                       crossing_port_standon_band270_bow_probe"
	        echo "                       crossing_port_standon_band270_mid_probe"
	        echo "                       crossing_port_standon_band270_near_probe"
		echo "  --shore_mport=N    Shoreside MOOSDB port     "
	echo "  --veh_mport=N      First vehicle MOOSDB port "
	echo "  --shore_pshare=N   Shoreside pShare port     "
	echo "  --veh_pshare=N     First vehicle pShare port "
	echo "                                               "
	echo "Launch Options:                               "
	echo "  --xlaunched, -x    Launched by xlaunch       "
	echo "  --nogui, -ng       Headless launch, no gui   "
	echo "                                               "
	echo "Options (custom):                              "
	echo "  --min_util_cpa=N   min_util_cpa              " 
	echo "  --max_util_cpa=N   max_util_cpa              " 
        echo "                                               "
        echo "Manual use note:                               "
        echo "  --mmod is the human-friendly scenario switch "
        echo "  H02 harness case names are not all unique    "
        echo "  --mmod values; many reuse the same stem mode "
        echo "  Formal harness cases still add nspatch-owned "
        echo "  evaluation or behavior overlays as needed.   "
	exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then 
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
	VERBOSE=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
	JUST_MAKE=$ARGI
    elif [ "${ARGI}" = "--log_clean" -o "${ARGI}" = "-lc" ]; then
	LOG_CLEAN=$ARGI
    elif [ "${ARGI:0:6}" = "--log=" ]; then
        LOG_MODE="${ARGI#--log=*}"
    elif [ "${ARGI:0:6}" = "--amt=" ]; then
        VAMT="${ARGI#--amt=*}"
	if [ $VAMT -lt 1 -o $VAMT -gt $MAX_VAMT ]; then
	    echo "$ME: Veh amt range: [1, $MAX_VAMT]. Exit Code 2."
	    exit 2
	fi
    elif [ "${ARGI}" = "--rand" -o "${ARGI}" = "-r" ]; then
        RAND_VPOS=$ARGI
    elif [ "${ARGI:0:10}" = "--max_spd=" ]; then
        MAX_SPD="${ARGI#--max_spd=*}"
    elif [ "${ARGI:0:7}" = "--mmod=" ]; then
        MMOD=$ARGI
    elif [ "${ARGI:0:14}" = "--shore_mport=" ]; then
        SHORE_MPORT="${ARGI#--shore_mport=*}"
    elif [ "${ARGI:0:12}" = "--veh_mport=" ]; then
        VEH_MPORT="${ARGI#--veh_mport=*}"
    elif [ "${ARGI:0:15}" = "--shore_pshare=" ]; then
        SHORE_PSHARE="${ARGI#--shore_pshare=*}"
    elif [ "${ARGI:0:13}" = "--veh_pshare=" ]; then
        VEH_PSHARE="${ARGI#--veh_pshare=*}"

    elif [ "${ARGI}" = "--xlaunched" -o "${ARGI}" = "-x" ]; then
	XLAUNCHED="yes"
    elif [ "${ARGI}" = "--nogui" -o "${ARGI}" = "-ng" ]; then
	NOGUI="--nogui"

    elif [ "${ARGI:0:15}" = "--min_util_cpa=" ]; then
        MIN_UTIL_CPA="${ARGI#--min_util_cpa=*}"
    elif [ "${ARGI:0:15}" = "--max_util_cpa=" ]; then
        MAX_UTIL_CPA="${ARGI#--max_util_cpa=*}"

    else 
	echo "$ME: Bad arg:" $ARGI "Exit Code 1."
	exit 1
    fi
done

case "$LOG_MODE" in
    minimal|full) ;;
    *) echo "$ME: --log must be minimal or full" >&2; exit 2 ;;
esac
if [ "${LOG_MODE_PREPARED:-no}" != yes ]; then
    ./prepare_logging_mode.sh "$LOG_MODE"
fi
export LOG_MODE_PREPARED=yes
export LOG_MODE_PREPARED_VALUE="$LOG_MODE"

#------------------------------------------------------------
#  Part 4: Set starting positions, speeds, vnames, colors
#------------------------------------------------------------
INIT_VARS=" --amt=$VAMT $RAND_VPOS $VERBOSE $MMOD "
./init_field.sh $INIT_VARS

VEHPOS=(`cat vpositions.txt`)
VDESTS=(`cat vdests.txt`)
SPEEDS=(`cat vspeeds.txt`)
VNAMES=(`cat vnames.txt`)
VCOLOR=(`cat vcolors.txt`)

#------------------------------------------------------------
#  Part 5: If verbose, show vars and confirm before launching
#------------------------------------------------------------
if [ "${VERBOSE}" != "" ]; then
    echo "============================================"
    echo "  $ME SUMMARY                   (ALL)       "
    echo "============================================"
    echo "CMD_ARGS =      [${CMD_ARGS}]               "
    echo "TIME_WARP =     [${TIME_WARP}]              "
    echo "JUST_MAKE =     [${JUST_MAKE}]              "
    echo "LOG_CLEAN =     [${LOG_CLEAN}]              "
    echo "VAMT =          [${VAMT}]                   "
    echo "MAX_VAMT =      [${MAX_VAMT}]               "
    echo "RAND_VPOS =     [${RAND_VPOS}]              "
    echo "MAX_SPD =       [${MAX_SPD}]                "
    echo "MMOD =          [${MMOD}]                   "
    echo "SHORE_MPORT =   [${SHORE_MPORT}]            "
    echo "VEH_MPORT =     [${VEH_MPORT}]              "
    echo "SHORE_PSHARE =  [${SHORE_PSHARE}]           "
    echo "VEH_PSHARE =    [${VEH_PSHARE}]             "
    echo "--------------------------------(VProps)----"
    echo "VNAMES =        [${VNAMES[*]}]              "
    echo "VCOLORS =       [${VCOLOR[*]}]              "
    echo "START_POS =     [${VEHPOS[*]}]              "
    echo "--------------------------------(Monte)-----"
    echo "XLAUNCHED =     [${XLAUNCHED}]              "
    echo "NOGUI =         [${NOGUI}]                  "
    echo "--------------------------------(Custom)----"
    echo "DEST_POS =      [${VDESTS[*]}]              "
    echo "MIN_UTIL_CPA    [$MIN_UTIL_CPA]             "
    echo "MAX_UTIL_CPA    [$MAX_UTIL_CPA]             "
    echo "                                            "
    echo -n "Hit any key to continue launch           "
    read ANSWER
fi

#------------------------------------------------------------
#  Part 6: Launch the Vehicles
#------------------------------------------------------------
VARGS=" --sim --auto --max_spd=$MAX_SPD $MMOD "
VARGS+=" $TIME_WARP $JUST_MAKE $VERBOSE "
VARGS+=" --min_util_cpa=$MIN_UTIL_CPA "
VARGS+=" --max_util_cpa=$MAX_UTIL_CPA "
for IX in `seq 1 $VAMT`;
do
    IXX=$(($IX - 1))
    V_MPORT=$((VEH_MPORT + IXX))
    V_PSHARE=$((VEH_PSHARE + IXX))
    IVARGS="$VARGS --mport=$V_MPORT --pshare=$V_PSHARE "
    IVARGS+=" --start_pos=${VEHPOS[$IXX]} "
    IVARGS+=" --stock_spd=${SPEEDS[$IXX]} "
    IVARGS+=" --vname=${VNAMES[$IXX]} "
    IVARGS+=" --vdest=${VDESTS[$IXX]} "
    IVARGS+=" --color=${VCOLOR[$IXX]} "
    IVARGS+=" --shore_pshare=$SHORE_PSHARE "
    vecho "Launching vehicle: $IVARGS"

    CMD="./launch_vehicle.sh $IVARGS"    
    eval $CMD
    sleep 0.5
done

#------------------------------------------------------------
#  Part 7: Launch the Shoreside mission file
#------------------------------------------------------------
SARGS=" --auto --mport=$SHORE_MPORT --pshare=$SHORE_PSHARE $NOGUI --vnames=abe:ben "
SARGS+=" $TIME_WARP $JUST_MAKE $VERBOSE "
SARGS+=" $MMOD "
SARGS+=" --min_util_cpa=$MIN_UTIL_CPA "
SARGS+=" --max_util_cpa=$MAX_UTIL_CPA "
vecho "Launching shoreside: $SARGS"
./launch_shoreside.sh $SARGS 

if [ "${JUST_MAKE}" != "" ]; then
    echo "$ME: Targ files made; exiting without launch."
    exit 0
fi

#------------------------------------------------------------
#  Part 8: Unless auto-launched, launch uMAC until mission quit
#------------------------------------------------------------
if [ "${XLAUNCHED}" != "yes" ]; then
    uMAC --paused targ_shoreside.moos
    on_exit
fi

exit 0
