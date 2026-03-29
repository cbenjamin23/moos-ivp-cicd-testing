#!/bin/bash -e
#------------------------------------------------------------
#   Script: launch.sh
#  Mission: P02-colregs_joust
#   Author: Charles Benjamin
#   LastEd: Mar 2026
#------------------------------------------------------------
#  Part 1: Set convenience functions for producing terminal
#          debugging output, and catching SIGINT (ctrl-c).
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
HALTING="no"
on_exit() {
    if [ "$HALTING" = "yes" ]; then
        exit 0
    fi
    HALTING="yes"
    trap "" SIGINT SIGTERM
    echo
    echo "$ME: Halting all apps"
    kill -- -$$
}
trap on_exit SIGINT
trap on_exit SIGTERM

#------------------------------------------------------------
#  Part 2: Set global variable default values
#------------------------------------------------------------
ME=`basename "$0"`
CMD_ARGS=""
TIME_WARP=1
VERBOSE=""
JUST_MAKE=""
LOG_CLEAN=""
VAMT="4"
MAX_VAMT="10"
RAND_VPOS=""
MAX_SPD="2"

# Launch
XLAUNCHED="no"
NOGUI=""

# Custom
REUSE=""
CIRCLE="x=23,y=-82,rad=32"
START=""
COLAVD=""
NOLEGS=""
MMOD="baseline_colregs_pass"
JOUST_FILE=""
EVAL_TIME_SECS="40"
MISSION_TIMEOUT_SECS="55"
DEPLOY_DELAY_SECS="15"
CLOSEST_MIN="1"
CLOSEST_MAX="40"
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
	echo "  --log_clean, -lc   Run clean.sh bef launch   " 
	echo "  --amt=N            Num vehicles to launch    "
	echo "  --rand, -r         Rand vehicle positions    "
	echo "  --max_spd=N        Max helm/sim speed        "
	echo "  --mmod=<mod>       Mission variation/mod:    "
	echo "                     baseline_colregs_pass     "
	echo "                     dense_colregs_pass        "
	echo "                     endurance_colregs_pass    "
	echo "  --shore_mport=N    Shoreside MOOSDB port    "
	echo "  --veh_mport=N      Base vehicle MOOSDB port "
	echo "  --shore_pshare=N   Shoreside pShare port    "
	echo "  --veh_pshare=N     Base vehicle pShare port "
	echo "                                               "
	echo "Launch Options:                               "
	echo "  --xlaunched, -x    Launched by xlaunch       "
	echo "  --nogui, -ng       Headless launch, no gui   "
	echo "  --gui              Launch with pMarineViewer "
	echo "                                               "
	echo "Options (custom):                              "
	echo "  --dock, -d         Vehicles start at dock    " 
	echo "  --nolegs           Don't recalc joust legs   " 
	echo "  --colregs, -c      Enable COLREGS colavd     " 
	echo "  --cpa, -cpa        Enable CPA avoid mode     "
	exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 1 ]; then 
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
	VERBOSE=$ARGI
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
	JUST_MAKE=$ARGI
    elif [ "${ARGI}" = "--log_clean" -o "${ARGI}" = "-lc" ]; then
	LOG_CLEAN=$ARGI
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

    elif [ "${ARGI}" = "--xlaunched" -o "${ARGI}" = "-x" ]; then
	XLAUNCHED="yes"
    elif [ "${ARGI}" = "--nogui" -o "${ARGI}" = "-ng" ]; then
	NOGUI="--nogui"
    elif [ "${ARGI}" = "--gui" ]; then
	NOGUI=""

    elif [ "${ARGI}" = "--dock" -o "${ARGI}" = "-d" ]; then
	START=$ARGI
    elif [ "${ARGI}" = "--nolegs" ]; then
	NOLEGS=$ARGI
    elif [ "${ARGI}" = "--colregs" -o "${ARGI}" = "-c" ]; then
	COLAVD=$ARGI
    elif [ "${ARGI}" = "--cpa" -o "${ARGI}" = "-cpa" ]; then
	COLAVD=$ARGI
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
    else 
	echo "$ME: Bad arg:" $ARGI "Exit Code 1."
        exit 1
    fi
done

#------------------------------------------------------------
#  Part 3A: Mission-mode specific defaults
#------------------------------------------------------------
case "$MMOD" in
    baseline_colregs_pass)
        JOUST_FILE="$PWD/jousts/baseline_2v.txt"
        VAMT="2"
        EVAL_TIME_SECS="280"
        MISSION_TIMEOUT_SECS="400"
        CLOSEST_MAX="45"
        ;;
    dense_colregs_pass)
        JOUST_FILE="$PWD/jousts/dense_3v.txt"
        VAMT="3"
        EVAL_TIME_SECS="420"
        MISSION_TIMEOUT_SECS="560"
        CLOSEST_MAX="40"
        ;;
    endurance_colregs_pass)
        JOUST_FILE="$PWD/jousts/endurance_3v.txt"
        VAMT="3"
        EVAL_TIME_SECS="1200"
        MISSION_TIMEOUT_SECS="1450"
        CLOSEST_MAX="40"
        ;;
    *)
        echo "$ME: Unknown mission mode [$MMOD]"
        exit 1
        ;;
esac

#------------------------------------------------------------
#  Part 4: Set starting positions, speeds, vnames, colors
#------------------------------------------------------------
INIT_VARS=" --amt=$VAMT $RAND_VPOS $VERBOSE "
INIT_VARS+=" --circle=$CIRCLE $START " #custom
INIT_VARS+=" --joust_file=$JOUST_FILE "
./init_field.sh $INIT_VARS $NOLEGS


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
    echo "--------------------------------(VProps)----"
    echo "VNAMES =        [${VNAMES[*]}]              "
    echo "VCOLORS =       [${VCOLOR[*]}]              "
    echo "START_POS =     [${VEHPOS[*]}]              "
    echo "--------------------------------(Monte)-----"
    echo "XLAUNCHED =     [${XLAUNCHED}]              "
    echo "NOGUI =         [${NOGUI}]                  "
    echo "--------------------------------(Custom)----"
    echo "CIRCLE =        [${CIRCLE}]                 "
    echo "START =         [${START}]                  "
    echo "COLAVD =        [${COLAVD}]                 "
    echo "SHORE_MPORT =   [${SHORE_MPORT}]            "
    echo "VEH_MPORT =     [${VEH_MPORT}]              "
    echo "SHORE_PSHARE =  [${SHORE_PSHARE}]           "
    echo "VEH_PSHARE =    [${VEH_PSHARE}]             "
    echo "                                            "
    echo -n "Hit any key to continue launch           "
    read ANSWER
fi

#------------------------------------------------------------
#  Part 6: Launch the Vehicles
#------------------------------------------------------------
VARGS=" --sim --auto --max_spd=$MAX_SPD "
VARGS+=" $TIME_WARP $JUST_MAKE $VERBOSE "
VARGS+=" $COLAVD "
VARGS+=" --shore_pshare=$SHORE_PSHARE "
VNAMES=""
for IX in `seq 1 $VAMT`;
do
    sleep 0.2
    VEH_MPORT_IX=$((VEH_MPORT + IX - 1))
    VEH_PSHARE_IX=$((VEH_PSHARE + IX - 1))
    IVARGS="$VARGS --mport=$VEH_MPORT_IX --pshare=$VEH_PSHARE_IX "
    
    # PX1,PY1 is first vertex of legrun and option for startpos
    PX1=`pluck joust.txt $IX --fld=px1`
    PY1=`pluck joust.txt $IX --fld=py1`
    PX2=`pluck joust.txt $IX --fld=px2`
    PY2=`pluck joust.txt $IX --fld=py2`
    HDG=`pluck joust.txt $IX --fld=hdg`
    SPD=`pluck joust.txt $IX --fld=spd`
    VNAME=`pluck joust.txt $IX --fld=vname`
    COLOR=`pluck joust.txt $IX --fld=vcolor`
    P1POS="x=${PX1},y=${PY1},heading=${HDG}"
    VX1POS="x=${PX1},y=${PY1}"
    VX2POS="x=${PX2},y=${PY2}"
    
    STAPOS=$P1POS
    if [ "${START}" != "" ]; then
	DX1=`pluck vpositions.txt $IX --fld=x`
	DY1=`pluck vpositions.txt $IX --fld=y`
	HDG=`pluck vpositions.txt $IX --fld=heading`	
	STAPOS="x=${DX1},y=${DY1},heading=${HDG}"
    fi
    
    IVARGS+=" --stock_spd=$SPD --color=$COLOR  " 
    IVARGS+=" --start_pos=${STAPOS} "
    IVARGS+=" --stock_spd=${SPD} "
    IVARGS+=" --vx1=${VX1POS} "
    IVARGS+=" --vx2=${VX2POS} "
    IVARGS+=" --vname=${VNAME} "
    IVARGS+=" --color=${COLOR} "

    if [ "${VNAMES}" != "" ]; then
	VNAMES+=":"
    fi
    VNAMES+=$VNAME
    
    vecho "Launching vehicle: $IVARGS"

    CMD="./launch_vehicle.sh $IVARGS"    
    eval $CMD
    sleep 0.5
done

#------------------------------------------------------------
#  Part 7: Launch the Shoreside mission file
#------------------------------------------------------------
SARGS=" --auto --mport=$SHORE_MPORT --pshare=$SHORE_PSHARE $NOGUI "
SARGS+=" $TIME_WARP $JUST_MAKE $VERBOSE --vnames=$VNAMES --mmod=$MMOD"
SARGS+=" --required_nodes=$VAMT"
SARGS+=" --eval_time=$EVAL_TIME_SECS"
SARGS+=" --mission_timeout=$MISSION_TIMEOUT_SECS"
SARGS+=" --deploy_delay=$DEPLOY_DELAY_SECS"
SARGS+=" --closest_min=$CLOSEST_MIN"
SARGS+=" --closest_max=$CLOSEST_MAX"
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
