#!/bin/bash
#------------------------------------------------------------
#   Script: ufield_app_common.sh
#  Purpose: Shared runner for app-level uField utility harnesses
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }

ME=`basename "$0"`
HARNESS_DIR="$(cd "$(dirname "$0")" && pwd)"
HARNESS_GROUP_DIR="$(cd "$HARNESS_DIR/.." && pwd)"
REPO_DIR="$(git -C "$HARNESS_DIR" rev-parse --show-toplevel)"
MISSION_DIR="$REPO_DIR/missions/ufield_app_missions/ufield_app_unit"
RESULTS_FILE="$HARNESS_DIR/results.txt"
TIME_WARP=10
MAX_TIME=80
JOBS=1
PORT_BASE=${PORT_BASE:-4300}
PORT_STRIDE=10
CASE=""
JUST_MAKE="no"
KEEP_WORKDIRS="no"
VERBOSE=""
CASE_ROW_DIR=""
RUN_ROOT=""
RUN_ROOT_PREFIX=${RUN_ROOT_PREFIX:-ufield_app}
ALL_OK="yes"

TEARDOWN_HELPER="$REPO_DIR/scripts/moos_scoped_teardown.sh"
if [ ! -f "$TEARDOWN_HELPER" ]; then
    echo "$ME: Missing scoped teardown helper: $TEARDOWN_HELPER"
    exit 1
fi
# shellcheck source=/dev/null
source "$TEARDOWN_HELPER"

cleanup() {
    local status=$?
    local teardown_ok="yes"
    trap - EXIT
    if [ "$RUN_ROOT" != "" ]; then
        if ! moos_scoped_teardown_stop_root "$RUN_ROOT"; then
            echo "$ME: Scoped teardown failed; preserving $RUN_ROOT" >&2
            teardown_ok="no"
            if [ "$status" -eq 0 ]; then
                status=1
            fi
        fi
    fi
    if [ "$RUN_ROOT" != "" ] && [ "$KEEP_WORKDIRS" != "yes" ] && [ "$teardown_ok" = "yes" ]; then
        rm -rf "$RUN_ROOT"
    fi
    exit "$status"
}
trap cleanup EXIT

usage() {
    echo "$ME [OPTIONS] [time_warp]"
    echo ""
    echo "Options:"
    echo "  --help, -h           Show this help message"
    echo "  --case=<name>        Run one case"
    echo "  --jobs=N             Parallel jobs for grouped runs"
    echo "  --port_base=N        Base MOOSDB port; default $PORT_BASE"
    echo "  --max_time=N         Max time for uMayFinish"
    echo "  --just_make, -j      Generate target files only"
    echo "  --keep_workdirs      Keep isolated temp mission copies"
    echo "  --verbose, -v        Verbose output"
}

for ARGI; do
    if [ "$ARGI" = "--help" ] || [ "$ARGI" = "-h" ]; then
        usage
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" ] && [ "$TIME_WARP" = 10 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI:0:7}" = "--case=" ]; then
        CASE="${ARGI#--case=*}"
    elif [ "${ARGI:0:7}" = "--jobs=" ]; then
        JOBS="${ARGI#--jobs=*}"
    elif [ "${ARGI:0:12}" = "--port_base=" ]; then
        PORT_BASE="${ARGI#--port_base=*}"
    elif [ "${ARGI:0:11}" = "--max_time=" ]; then
        MAX_TIME="${ARGI#--max_time=*}"
    elif [ "$ARGI" = "--just_make" ] || [ "$ARGI" = "-j" ]; then
        JUST_MAKE="yes"
    elif [ "$ARGI" = "--keep_workdirs" ]; then
        KEEP_WORKDIRS="yes"
    elif [ "$ARGI" = "--verbose" ] || [ "$ARGI" = "-v" ]; then
        VERBOSE="yes"
    else
        echo "$ME: Bad arg: $ARGI"
        exit 1
    fi
done

node_report_alpha_0() {
    echo 'NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=100,X=0,Y=0,LAT=43.825300,LON=-70.330400,SPD=0,HDG=0'
}

node_report_alpha_5() {
    echo 'NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=101,X=3,Y=4,LAT=43.825336,LON=-70.330363,SPD=5,HDG=53'
}

node_report_alpha_10() {
    echo 'NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=102,X=6,Y=8,LAT=43.825372,LON=-70.330326,SPD=5,HDG=53'
}

node_report_alpha_15() {
    echo 'NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=103,X=9,Y=12,LAT=43.825408,LON=-70.330289,SPD=5,HDG=53'
}

node_report_alpha_20() {
    echo 'NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=104,X=12,Y=16,LAT=43.825444,LON=-70.330252,SPD=5,HDG=53'
}

node_report_bravo_50() {
    echo 'NAME=bravo,TYPE=KAYAK,GROUP=blue,UTC_TIME=100,X=50,Y=0,LAT=43.825300,LON=-70.329778,SPD=0,HDG=180'
}

node_report_bravo_behind() {
    echo 'NAME=bravo,TYPE=KAYAK,GROUP=blue,UTC_TIME=100,X=-50,Y=0,LAT=43.825300,LON=-70.331022,SPD=0,HDG=180'
}

node_report_ship_50() {
    echo 'NAME=bravo,TYPE=SHIP,GROUP=blue,UTC_TIME=100,X=50,Y=0,LAT=43.825300,LON=-70.329778,SPD=0,HDG=180'
}

node_report_charlie_80() {
    echo 'NAME=charlie,TYPE=KAYAK,GROUP=green,UTC_TIME=100,X=80,Y=0,LAT=43.825300,LON=-70.329405,SPD=0,HDG=180'
}

node_report_xy() {
    local name="$1"
    local group="$2"
    local x="$3"
    local y="$4"
    local spd="${5:-0}"
    local hdg="${6:-0}"
    echo "NAME=$name,TYPE=KAYAK,GROUP=$group,UTC_TIME=100,X=$x,Y=$y,LAT=43.825300,LON=-70.330400,SPD=$spd,HDG=$hdg"
}

obstacle_square() {
    local label="${1:-obs}"
    echo "pts={-5,-5:5,-5:5,5:-5,5},label=$label"
}

pathcheck_events_base() {
    cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_alpha_5)", time=1.5
  event = var=NODE_REPORT, val="$(node_report_alpha_10)", time=2.5
EOF
}

message_event() {
    local msg="$1"
    local time="${2:-1.0}"
    echo "  event = var=NODE_MESSAGE, val=\"$msg\", time=$time"
}

crs_events_near() {
    cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_bravo_50)", time=0.7
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=1.5
EOF
}

set_defaults() {
    APP_NAME=""
    APP_RUN=""
    APP_CONFIG=""
    TIMER_EVENTS=""
    REPORT_COLUMNS=""
    CASE_MAX_TIME="$MAX_TIME"
    READY_TIME="6.0"
}

get_pathcheck_case_config() {
    set_defaults
    APP_NAME="uFldPathCheck"
    APP_RUN="uFldPathCheck"
    APP_CONFIG="ProcessConfig = uFldPathCheck
{
  AppTick   = 4
  CommsTick = 4

  history_length = 10
}"
    TIMER_EVENTS="$(pathcheck_events_base)"
    REPORT_COLUMNS='  report_column = odom=$[UPC_ODOMETRY_REPORT]
  report_column = speed=$[UPC_SPEED_REPORT]'

    case "$CASE_NAME" in
        odometry_baseline_pass)
            ;;
        two_reports_speed_only_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_alpha_5)", time=1.5
EOF
)"
            ;;
        trip_reset_pass)
            TIMER_EVENTS="$(cat <<EOF
$(pathcheck_events_base)
  event = var=UPC_TRIP_RESET, val="alpha", time=3.0
  event = var=NODE_REPORT, val="$(node_report_alpha_15)", time=4.0
EOF
)"
            READY_TIME="7.0"
            ;;
        node_report_local_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT_LOCAL, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT_LOCAL, val="$(node_report_alpha_5)", time=1.5
  event = var=NODE_REPORT_LOCAL, val="$(node_report_alpha_10)", time=2.5
EOF
)"
            ;;
        mixed_report_sources_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT_LOCAL, val="$(node_report_alpha_5)", time=1.5
  event = var=NODE_REPORT, val="$(node_report_alpha_10)", time=2.5
EOF
)"
            ;;
        multi_node_independent_pass)
            TIMER_EVENTS="$(cat <<EOF
$(pathcheck_events_base)
  event = var=NODE_REPORT, val="NAME=bravo,TYPE=KAYAK,GROUP=blue,UTC_TIME=100,X=100,Y=0,LAT=43.825300,LON=-70.329156,SPD=0,HDG=0", time=0.7
  event = var=NODE_REPORT, val="NAME=bravo,TYPE=KAYAK,GROUP=blue,UTC_TIME=101,X=100,Y=12,LAT=43.825408,LON=-70.329156,SPD=12,HDG=0", time=1.7
  event = var=NODE_REPORT, val="NAME=bravo,TYPE=KAYAK,GROUP=blue,UTC_TIME=102,X=100,Y=24,LAT=43.825516,LON=-70.329156,SPD=12,HDG=0", time=2.7
EOF
)"
            ;;
        variable_speed_average_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=100,X=0,Y=0,LAT=43.825300,LON=-70.330400,SPD=0,HDG=0", time=0.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=101,X=3,Y=4,LAT=43.825336,LON=-70.330363,SPD=5,HDG=53", time=1.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=102,X=9,Y=12,LAT=43.825408,LON=-70.330289,SPD=10,HDG=53", time=2.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=103,X=9,Y=22,LAT=43.825498,LON=-70.330289,SPD=10,HDG=0", time=3.5
EOF
)"
            READY_TIME="7.0"
            ;;
        history_length_three_average_pass)
            APP_CONFIG="ProcessConfig = uFldPathCheck
{
  AppTick   = 4
  CommsTick = 4

  history_length = 3
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=100,X=0,Y=0,LAT=43.825300,LON=-70.330400,SPD=0,HDG=0", time=0.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=101,X=5,Y=0,LAT=43.825300,LON=-70.330338,SPD=5,HDG=90", time=1.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=102,X=15,Y=0,LAT=43.825300,LON=-70.330214,SPD=10,HDG=90", time=2.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=103,X=30,Y=0,LAT=43.825300,LON=-70.330027,SPD=15,HDG=90", time=3.5
EOF
)"
            READY_TIME="7.0"
            ;;
        invalid_history_length_default_pass)
            APP_CONFIG="ProcessConfig = uFldPathCheck
{
  AppTick   = 4
  CommsTick = 4

  history_length = 1
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=100,X=0,Y=0,LAT=43.825300,LON=-70.330400,SPD=0,HDG=0", time=0.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=101,X=5,Y=0,LAT=43.825300,LON=-70.330338,SPD=5,HDG=90", time=1.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=102,X=15,Y=0,LAT=43.825300,LON=-70.330214,SPD=10,HDG=90", time=2.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=103,X=30,Y=0,LAT=43.825300,LON=-70.330027,SPD=15,HDG=90", time=3.5
EOF
)"
            READY_TIME="7.0"
            ;;
        reverse_leg_accumulates_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=101,X=5,Y=0,LAT=43.825300,LON=-70.330338,SPD=5,HDG=90", time=1.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=102,X=0,Y=0,LAT=43.825300,LON=-70.330400,SPD=5,HDG=270", time=2.5
EOF
)"
            ;;
        history_length_two_speed_pass)
            APP_CONFIG="ProcessConfig = uFldPathCheck
{
  AppTick   = 4
  CommsTick = 4

  history_length = 2
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=100,X=0,Y=0,LAT=43.825300,LON=-70.330400,SPD=0,HDG=0", time=0.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=101,X=3,Y=4,LAT=43.825336,LON=-70.330363,SPD=5,HDG=53", time=1.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=102,X=3,Y=14,LAT=43.825426,LON=-70.330363,SPD=10,HDG=0", time=2.5
EOF
)"
            READY_TIME="7.0"
            ;;
        invalid_report_rejected_pass)
            TIMER_EVENTS='  event = var=NODE_REPORT, val="NAME=alpha,X=bad,Y=0", time=1.0'
            ;;
        stationary_zero_distance_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=101,X=0,Y=0,LAT=43.825300,LON=-70.330400,SPD=0,HDG=0", time=1.5
  event = var=NODE_REPORT, val="NAME=alpha,TYPE=KAYAK,GROUP=red,UTC_TIME=102,X=0,Y=0,LAT=43.825300,LON=-70.330400,SPD=0,HDG=0", time=2.5
EOF
)"
            ;;
        diagonal_chain_distance_pass)
            TIMER_EVENTS="$(cat <<EOF
$(pathcheck_events_base)
  event = var=NODE_REPORT, val="$(node_report_alpha_15)", time=3.5
EOF
)"
            READY_TIME="7.0"
            ;;
        trip_reset_unknown_ignored_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=UPC_TRIP_RESET, val="ghost", time=0.5
$(pathcheck_events_base)
EOF
)"
            ;;
        trip_reset_twice_pass)
            TIMER_EVENTS="$(cat <<EOF
$(pathcheck_events_base)
  event = var=UPC_TRIP_RESET, val="alpha", time=3.0
  event = var=NODE_REPORT, val="$(node_report_alpha_15)", time=4.0
  event = var=UPC_TRIP_RESET, val="alpha", time=5.0
  event = var=NODE_REPORT, val="$(node_report_alpha_20)", time=6.0
EOF
)"
            READY_TIME="8.0"
            ;;
        bad_then_good_recovery_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="NAME=alpha,X=bad,Y=0", time=0.5
$(pathcheck_events_base)
EOF
)"
            ;;
        *)
            echo "$ME: Unknown pathcheck case: [$CASE_NAME]"
            return 1
            ;;
    esac
}

get_message_case_config() {
    set_defaults
    APP_NAME="uFldMessageHandler"
    APP_RUN="uFldMessageHandler"
    APP_CONFIG="ProcessConfig = uFldMessageHandler
{
  AppTick   = 4
  CommsTick = 4

  strict_addressing = false
}"
    REPORT_COLUMNS='  report_column = handled=$[HANDLED_RESULT]
  report_column = numeric=$[HANDLED_NUMERIC]
  report_column = summary=$[UMH_SUMMARY_MSGS]
  report_column = good_flag=$[UMH_GOOD]
  report_column = bad_flag=$[UMH_BAD]'

    case "$CASE_NAME" in
        dest_all_string_pass)
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=all,var_name=HANDLED_RESULT,string_val=all_ok')"
            ;;
        dest_all_uppercase_pass)
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=ALL,var_name=HANDLED_RESULT,string_val=all_upper_ok')"
            ;;
        dest_specific_self_pass)
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=shoreside,var_name=HANDLED_RESULT,string_val=self_ok')"
            ;;
        dest_mismatch_reject_pass)
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=bravo,var_name=HANDLED_RESULT,string_val=wrong_node')"
            ;;
        strict_all_reject_pass)
            APP_CONFIG="ProcessConfig = uFldMessageHandler
{
  AppTick   = 4
  CommsTick = 4

  strict_addressing = true
}"
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=all,var_name=HANDLED_RESULT,string_val=strict_reject')"
            ;;
        strict_group_reject_pass)
            APP_CONFIG="ProcessConfig = uFldMessageHandler
{
  AppTick   = 4
  CommsTick = 4

  strict_addressing = true
}"
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_group=red,var_name=HANDLED_RESULT,string_val=strict_group_reject')"
            ;;
        strict_specific_accept_pass)
            APP_CONFIG="ProcessConfig = uFldMessageHandler
{
  AppTick   = 4
  CommsTick = 4

  strict_addressing = true
}"
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=shoreside,var_name=HANDLED_RESULT,string_val=strict_accept')"
            ;;
        numeric_payload_pass)
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=shoreside,var_name=HANDLED_NUMERIC,double_val=42.5')"
            ;;
        double_zero_payload_pass)
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=shoreside,var_name=HANDLED_NUMERIC,double_val=0')"
            ;;
        mixed_payload_invalid_pass)
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=shoreside,var_name=HANDLED_RESULT,string_val=string_wins,double_val=99')"
            ;;
        quoted_string_unquoted_pass)
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=shoreside,var_name=HANDLED_RESULT,string_val=\"quoted ok\"')"
            ;;
        src_app_forward_pass)
            TIMER_EVENTS="$(message_event 'src_node=alpha,src_app=pHelmIvP,dest_node=shoreside,var_name=HANDLED_RESULT,string_val=src_app_ok')"
            ;;
        invalid_payload_reject_pass)
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=shoreside,string_val=missing_var')"
            ;;
        missing_destination_invalid_pass)
            TIMER_EVENTS="$(message_event 'src_node=alpha,var_name=HANDLED_RESULT,string_val=missing_dest')"
            ;;
        invalid_no_bad_flag_pass)
            APP_CONFIG="ProcessConfig = uFldMessageHandler
{
  AppTick   = 4
  CommsTick = 4

  bad_msg_flag = UMH_BAD=bad_\$[BAD_CTR]_of_\$[CTR]
}"
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=shoreside,string_val=missing_var')"
            ;;
        msg_flag_macro_pass)
            APP_CONFIG="ProcessConfig = uFldMessageHandler
{
  AppTick   = 4
  CommsTick = 4

  msg_flag = UMH_GOOD=good_\$[GOOD_CTR]_of_\$[CTR]
}"
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=shoreside,var_name=HANDLED_RESULT,string_val=flagged')"
            ;;
        msg_flag_numeric_pass)
            APP_CONFIG="ProcessConfig = uFldMessageHandler
{
  AppTick   = 4
  CommsTick = 4

  msg_flag = UMH_GOOD_NUM=7
}"
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=shoreside,var_name=HANDLED_RESULT,string_val=flagged_num')"
            ;;
        bad_msg_flag_macro_pass)
            APP_CONFIG="ProcessConfig = uFldMessageHandler
{
  AppTick   = 4
  CommsTick = 4

  bad_msg_flag = UMH_BAD=bad_\$[BAD_CTR]_of_\$[CTR]
}"
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=bravo,var_name=HANDLED_RESULT,string_val=bad_flagged')"
            ;;
        bad_msg_flag_numeric_pass)
            APP_CONFIG="ProcessConfig = uFldMessageHandler
{
  AppTick   = 4
  CommsTick = 4

  bad_msg_flag = UMH_BAD_NUM=11
}"
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=bravo,var_name=HANDLED_RESULT,string_val=bad_flagged_num')"
            ;;
        multiple_bad_summary_pass)
            APP_CONFIG="ProcessConfig = uFldMessageHandler
{
  AppTick   = 4
  CommsTick = 4

  bad_msg_flag = UMH_BAD=bad_\$[BAD_CTR]_of_\$[CTR]
}"
            TIMER_EVENTS="$(cat <<EOF
$(message_event 'src_node=alpha,dest_node=bravo,var_name=HANDLED_RESULT,string_val=bad_one' 1.0)
$(message_event 'src_node=charlie,dest_node=delta,var_name=HANDLED_RESULT,string_val=bad_two' 1.5)
EOF
)"
            ;;
        aux_info_node_app_forward_pass)
            APP_CONFIG="ProcessConfig = uFldMessageHandler
{
  AppTick   = 4
  CommsTick = 4

  aux_info = node+app
}"
            TIMER_EVENTS="$(message_event 'src_node=alpha,src_app=pHelmIvP,dest_node=shoreside,var_name=HANDLED_RESULT,string_val=aux_ok')"
            ;;
        dest_group_accept_pass)
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_group=red,var_name=HANDLED_RESULT,string_val=group_ok')"
            ;;
        dest_node_overrides_group_reject_pass)
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=bravo,dest_group=red,var_name=HANDLED_RESULT,string_val=node_wins')"
            ;;
        dest_all_mixedcase_reject_pass)
            TIMER_EVENTS="$(message_event 'src_node=alpha,dest_node=All,var_name=HANDLED_RESULT,string_val=mixed_all')"
            ;;
        multiple_good_summary_pass)
            TIMER_EVENTS="$(cat <<EOF
$(message_event 'src_node=alpha,dest_node=shoreside,var_name=HANDLED_RESULT,string_val=first_ok' 1.0)
$(message_event 'src_node=bravo,dest_node=shoreside,var_name=HANDLED_RESULT,string_val=second_ok' 1.5)
EOF
)"
            ;;
        mixed_valid_rejected_summary_pass)
            TIMER_EVENTS="$(cat <<EOF
$(message_event 'src_node=alpha,dest_node=shoreside,var_name=HANDLED_RESULT,string_val=first_ok' 1.0)
$(message_event 'src_node=bravo,dest_node=charlie,var_name=HANDLED_REJECT,string_val=nope' 1.5)
EOF
)"
            ;;
        *)
            echo "$ME: Unknown message-handler case: [$CASE_NAME]"
            return 1
            ;;
    esac
}

get_crs_case_config() {
    set_defaults
    APP_NAME="uFldContactRangeSensor"
    APP_RUN="uFldContactRangeSensor"
    APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = short
}"
    TIMER_EVENTS="$(crs_events_near)"
    REPORT_COLUMNS='  report_column = short=$[CRS_RANGE_REPORT]
  report_column = long=$[CRS_RANGE_REPORT_ALPHA]
  report_column = gt=$[CRS_RANGE_REPORT_GT]
  report_column = pulse=$[VIEW_RANGE_PULSE]'

    case "$CASE_NAME" in
        baseline_short_report_pass)
            ;;
        report_vars_short_no_long_pass)
            ;;
        report_vars_long_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = long
}"
            ;;
        report_vars_both_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = both
}"
            ;;
        ground_truth_uniform_zero_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = both
  rn_algorithm = uniform,pct=0
  ground_truth = true
}"
            ;;
        ground_truth_gaussian_unsupported_no_gt_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = both
  rn_algorithm = gaussian
  rn_gaussian_sigma = 0
  ground_truth = true
}"
            ;;
        ground_truth_long_only_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = long
  rn_algorithm = uniform,pct=0
  ground_truth = true
}"
            ;;
        ground_truth_no_algorithm_absent_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = both
  ground_truth = true
}"
            ;;
        allow_echo_type_block_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = short
  allow_echo_types = kayak
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_ship_50)", time=0.7
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=1.5
EOF
)"
            ;;
        allow_echo_type_accept_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = short
  allow_echo_types = kayak
}"
            ;;
        allow_echo_type_multi_accept_ship_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = short
  allow_echo_types = kayak,ship
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_ship_50)", time=0.7
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=1.5
EOF
)"
            ;;
        sensor_arc_forward_accept_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  sensor_arc = 45:135
}"
            ;;
        sensor_arc_wrap_accept_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  sensor_arc = 315:135
}"
            ;;
        sensor_arc_multi_segment_aft_accept_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  sensor_arc = 45:135,225:315
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_bravo_behind)", time=0.7
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=1.5
EOF
)"
            ;;
        sensor_arc_aft_block_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  sensor_arc = 45:135
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_bravo_behind)", time=0.7
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=1.5
EOF
)"
            ;;
        sensor_arc_full_accept_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  sensor_arc = 360
}"
            ;;
        ping_wait_blocks_second_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=30
  report_vars = short
}"
            TIMER_EVENTS="$(cat <<EOF
$(crs_events_near)
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=2.5
EOF
)"
            ;;
        named_ping_wait_blocks_second_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  ping_wait = alpha=30
  report_vars = short
}"
            TIMER_EVENTS="$(cat <<EOF
$(crs_events_near)
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=2.5
EOF
)"
            ;;
        ping_wait_unlimited_allows_second_pass)
            TIMER_EVENTS="$(cat <<EOF
$(crs_events_near)
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=2.5
EOF
)"
            ;;
        named_ping_wait_unlimited_allows_second_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=30
  ping_wait = alpha=unlimited
  report_vars = short
}"
            TIMER_EVENTS="$(cat <<EOF
$(crs_events_near)
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=2.5
EOF
)"
            ;;
        ping_wait_nolimit_alias_allows_second_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=30
  ping_wait = alpha=nolimit
  report_vars = short
}"
            TIMER_EVENTS="$(cat <<EOF
$(crs_events_near)
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=2.5
EOF
)"
            ;;
        display_pulses_false_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  display_pulses = false
}"
            ;;
        unknown_request_no_report_pass)
            TIMER_EVENTS='  event = var=CRS_RANGE_REQUEST, val="name=ghost", time=1.0'
            ;;
        missing_name_request_no_report_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_bravo_50)", time=0.7
  event = var=CRS_RANGE_REQUEST, val="vname=alpha", time=1.5
EOF
)"
            ;;
        ground_truth_false_no_gt_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = both
  rn_algorithm = uniform,pct=0
  ground_truth = false
}"
            ;;
        crs_node_report_local_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT_LOCAL, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT_LOCAL, val="$(node_report_bravo_50)", time=0.7
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=1.5
EOF
)"
            ;;
        unlimited_far_range_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  push_dist = default=unlimited
  pull_dist = default=10
  ping_wait = default=0
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="NAME=bravo,TYPE=KAYAK,GROUP=blue,UTC_TIME=100,X=500,Y=0,LAT=43.825300,LON=-70.324178,SPD=0,HDG=180", time=0.7
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=1.5
EOF
)"
            ;;
        push_distance_alias_far_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  push_distance = default=unlimited
  pull_distance = default=10
  ping_wait = default=0
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="NAME=bravo,TYPE=KAYAK,GROUP=blue,UTC_TIME=100,X=500,Y=0,LAT=43.825300,LON=-70.324178,SPD=0,HDG=180", time=0.7
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=1.5
EOF
)"
            ;;
        named_pull_unlimited_far_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  push_dist = alpha=10
  pull_dist = bravo=unlimited
  ping_wait = default=0
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="NAME=bravo,TYPE=KAYAK,GROUP=blue,UTC_TIME=100,X=500,Y=0,LAT=43.825300,LON=-70.324178,SPD=0,HDG=180", time=0.7
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=1.5
EOF
)"
            ;;
        pull_distance_alias_far_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  push_distance = alpha=10
  pull_distance = bravo=unlimited
  ping_wait = default=0
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="NAME=bravo,TYPE=KAYAK,GROUP=blue,UTC_TIME=100,X=500,Y=0,LAT=43.825300,LON=-70.324178,SPD=0,HDG=180", time=0.7
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=1.5
EOF
)"
            ;;
        default_short_blocks_far_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  push_dist = default=10
  pull_dist = default=10
  ping_wait = default=0
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="NAME=bravo,TYPE=KAYAK,GROUP=blue,UTC_TIME=100,X=500,Y=0,LAT=43.825300,LON=-70.324178,SPD=0,HDG=180", time=0.7
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=1.5
EOF
)"
            ;;
        short_total_range_far_blocks_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  push_dist = default=25
  pull_dist = default=25
  ping_wait = default=0
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="NAME=bravo,TYPE=KAYAK,GROUP=blue,UTC_TIME=100,X=5000,Y=0,LAT=43.825300,LON=-70.268222,SPD=0,HDG=180", time=0.7
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=1.5
EOF
)"
            ;;
        named_push_short_blocks_far_pass)
            APP_CONFIG="ProcessConfig = uFldContactRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  push_dist = alpha=10
  pull_dist = bravo=10
  ping_wait = default=0
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="NAME=bravo,TYPE=KAYAK,GROUP=blue,UTC_TIME=100,X=500,Y=0,LAT=43.825300,LON=-70.324178,SPD=0,HDG=180", time=0.7
  event = var=CRS_RANGE_REQUEST, val="name=alpha", time=1.5
EOF
)"
            ;;
        reciprocal_request_report_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_alpha_0)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_bravo_50)", time=0.7
  event = var=CRS_RANGE_REQUEST, val="name=bravo", time=1.5
EOF
)"
            ;;
        *)
            echo "$ME: Unknown contact-range-sensor case: [$CASE_NAME]"
            return 1
            ;;
    esac
}

get_brs_case_config() {
    set_defaults
    APP_NAME="uFldBeaconRangeSensor"
    APP_RUN="uFldBeaconRangeSensor"
    APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = short
  beacon = x=30,y=40,label=beacon,push_dist=50,pull_dist=50
}"
    TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0)", time=0.5
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=1.2
EOF
)"
    REPORT_COLUMNS='  report_column = brs_short=$[BRS_RANGE_REPORT]
  report_column = brs_long=$[BRS_RANGE_REPORT_ALPHA]
  report_column = brs_gt=$[BRS_RANGE_REPORT_GT]
  report_column = brs_pulse=$[VIEW_RANGE_PULSE]
  report_column = brs_marker=$[VIEW_MARKER]'

    case "$CASE_NAME" in
        request_short_report_pass)
            ;;
        request_long_report_pass)
            APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = long
  beacon = x=30,y=40,label=beacon,push_dist=50,pull_dist=50
}"
            ;;
        request_both_report_pass)
            APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = both
  beacon = x=30,y=40,label=beacon,push_dist=50,pull_dist=50
}"
            ;;
        brs_ground_truth_uniform_zero_pass)
            APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = both
  rn_algorithm = uniform,pct=0
  ground_truth = true
  beacon = x=30,y=40,label=beacon,push_dist=50,pull_dist=50
}"
            ;;
        brs_ground_truth_false_no_gt_pass)
            APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  report_vars = both
  rn_algorithm = uniform,pct=0
  ground_truth = false
  beacon = x=30,y=40,label=beacon,push_dist=50,pull_dist=50
}"
            ;;
        far_request_blocked_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 500 0)", time=0.5
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=1.2
EOF
)"
            ;;
        node_push_unlimited_far_pass)
            APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  node_push_dist = default=unlimited
  node_pull_dist = default=unlimited
  ping_wait = default=0
  report_vars = short
  beacon = x=300,y=400,label=farbeacon,push_dist=10,pull_dist=10
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0)", time=0.5
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=1.2
EOF
)"
            ;;
        brs_ping_wait_blocks_second_pass)
            APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=30
  report_vars = short
  beacon = x=30,y=40,label=beacon,push_dist=50,pull_dist=50
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0)", time=0.5
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=1.2
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=2.0
EOF
)"
            ;;
        brs_named_ping_wait_blocks_only_alpha_pass)
            APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=0
  ping_wait = alpha=30
  report_vars = short
  beacon = x=30,y=40,label=beacon,push_dist=50,pull_dist=50
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 0 0)", time=0.6
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=1.2
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=2.0
  event = var=BRS_RANGE_REQUEST, val="name=bravo", time=2.2
EOF
)"
            ;;
        ping_payment_request_blocks_retry_pass)
            APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=30
  ping_payments = upon_request
  report_vars = short
  beacon = x=30,y=40,label=beacon,push_dist=50,pull_dist=50
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 500 0)", time=0.5
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=1.0
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0)", time=1.5
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=2.0
EOF
)"
            ;;
        ping_payment_response_allows_retry_pass)
            APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  ping_wait = default=30
  ping_payments = upon_response
  report_vars = short
  beacon = x=30,y=40,label=beacon,push_dist=50,pull_dist=50
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 500 0)", time=0.5
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=1.0
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0)", time=1.5
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=2.0
EOF
)"
            ;;
        ping_payment_accept_blocks_retry_pass)
            APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  node_pull_dist = default=10
  ping_wait = default=30
  ping_payments = upon_accept
  report_vars = short
  beacon = x=30,y=40,label=beacon,push_dist=10,pull_dist=100
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0)", time=0.5
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=1.0
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 25 40)", time=1.5
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=2.0
EOF
)"
            ;;
        brs_node_report_local_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT_LOCAL, val="$(node_report_xy alpha red 0 0)", time=0.5
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=1.2
EOF
)"
            ;;
        beacon_style_defaults_pass)
            APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  default_beacon_color = green
  default_beacon_shape = square
  default_beacon_width = 7
  ping_wait = default=0
  report_vars = short
  beacon = x=30,y=40,label=beacon,push_dist=50,pull_dist=50
}"
            TIMER_EVENTS='  event = var=TEST_VISUAL_ONLY, val=true, time=0.6'
            ;;
        beacon_reply_push_blocks_pass)
            APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  node_pull_dist = default=10
  ping_wait = default=0
  report_vars = short
  beacon = x=30,y=40,label=beacon,push_dist=10,pull_dist=100
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0)", time=0.5
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=1.2
EOF
)"
            ;;
        request_path_pull_blocks_pass)
            APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  node_push_dist = default=10
  ping_wait = default=0
  report_vars = short
  beacon = x=30,y=40,label=beacon,push_dist=100,pull_dist=10
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0)", time=0.5
  event = var=BRS_RANGE_REQUEST, val="name=alpha", time=1.2
EOF
)"
            ;;
        unknown_request_no_report_pass)
            TIMER_EVENTS='  event = var=BRS_RANGE_REQUEST, val="name=ghost", time=1.2'
            ;;
        missing_name_request_no_report_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0)", time=0.5
  event = var=BRS_RANGE_REQUEST, val="vehicle=alpha", time=1.2
EOF
)"
            ;;
        beacon_visual_marker_pass)
            TIMER_EVENTS='  event = var=TEST_VISUAL_ONLY, val=true, time=0.6'
            ;;
        unsolicited_frequency_pass)
            APP_CONFIG="ProcessConfig = uFldBeaconRangeSensor
{
  AppTick   = 4
  CommsTick = 4

  report_vars = short
  beacon = x=30,y=40,label=beacon,freq=0.6,push_dist=50,pull_dist=50
}"
            TIMER_EVENTS="  event = var=NODE_REPORT, val=\"$(node_report_xy alpha red 0 0)\", time=0.5"
            READY_TIME="3.0"
            ;;
        *)
            echo "$ME: Unknown beacon-range-sensor case: [$CASE_NAME]"
            return 1
            ;;
    esac
}

ucd_sequence() {
    local min_x="$1"
    cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0 0 90)", time=0.4
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 10 0 0 270)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 30 0 0 270)", time=0.9
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue "$min_x" 0 0 270)", time=1.3
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 10 0 0 270)", time=1.8
EOF
}

insert_app_config_line() {
    local line="$1"
    APP_CONFIG="$(printf "%s\n" "$APP_CONFIG" | awk -v line="$line" '/^}$/ { print line } { print }')"
}

get_ucd_case_config() {
    set_defaults
    APP_NAME="uFldCollisionDetect"
    APP_RUN="uFldCollisionDetect"
    APP_CONFIG="ProcessConfig = uFldCollisionDetect
{
  AppTick   = 10
  CommsTick = 10

  collision_range = 3
  near_miss_range = 6
  encounter_range = 20
  post_closest_range = true
  post_closest_range_ever = true
  report_all_encounters = false
}"
    TIMER_EVENTS="$(ucd_sequence 2)"
    READY_TIME="4.0"
    REPORT_COLUMNS='  report_column = ucd_report=$[UCD_REPORT]
  report_column = encounter_total=$[ENCOUNTER_TOTAL]
  report_column = near_total=$[NEAR_MISS_TOTAL]
  report_column = collision_total=$[COLLISION_TOTAL]
  report_column = closest=$[UCD_CLOSEST_RANGE]
  report_column = closest_ever=$[UCD_CLOSEST_RANGE_EVER]
  report_column = params=$[COLLISION_DETECT_PARAMS]'

    case "$CASE_NAME" in
        collision_event_pass)
            ;;
        near_miss_event_pass)
            APP_CONFIG="$(echo "$APP_CONFIG" | sed 's/collision_range = 3/collision_range = 1/')"
            TIMER_EVENTS="$(ucd_sequence 3)"
            ;;
        clear_encounter_report_pass)
            APP_CONFIG="$(echo "$APP_CONFIG" | sed 's/report_all_encounters = false/report_all_encounters = true/')"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0 0 90)", time=0.4
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 30 0 0 270)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 10 0 0 270)", time=1.0
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 30 0 0 270)", time=1.5
EOF
)"
            ;;
        closest_range_posts_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0 0 90)", time=0.4
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 8 0 0 270)", time=0.5
EOF
)"
            READY_TIME="2.5"
            ;;
        reset_closest_range_ever_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0 0 90)", time=0.4
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 8 0 0 270)", time=0.5
  event = var=UCD_RESET, val=true, time=1.4
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 12 0 0 270)", time=1.8
EOF
)"
            READY_TIME="3.5"
            ;;
        pulse_render_false_pass)
            APP_CONFIG="$(echo "$APP_CONFIG" | sed 's/report_all_encounters = false/report_all_encounters = true/')"
            insert_app_config_line '  pulse_render = false'
            ;;
        ignore_group_blocks_pass)
            insert_app_config_line '  ignore_group = red'
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0 0 90)", time=0.4
  event = var=NODE_REPORT, val="$(node_report_xy bravo red 10 0 0 270)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_xy bravo red 30 0 0 270)", time=0.9
  event = var=NODE_REPORT, val="$(node_report_xy bravo red 2 0 0 270)", time=1.3
  event = var=NODE_REPORT, val="$(node_report_xy bravo red 10 0 0 270)", time=1.8
EOF
)"
            ;;
        reject_group_blocks_pass)
            insert_app_config_line '  reject_group = blue'
            ;;
        condition_blocks_pass)
            insert_app_config_line '  condition = TEST_GATE = true'
            ;;
        condition_allows_pass)
            insert_app_config_line '  condition = TEST_GATE = true'
            TIMER_EVENTS="$(cat <<EOF
  event = var=TEST_GATE, val=true, time=0.3
$(ucd_sequence 2)
EOF
)"
            ;;
        collision_flag_macro_pass)
            insert_app_config_line '  collision_flag = UCD_HIT=hit_$V2_$CPA'
            ;;
        near_miss_flag_macro_pass)
            APP_CONFIG="$(echo "$APP_CONFIG" | sed 's/collision_range = 3/collision_range = 1/')"
            insert_app_config_line '  near_miss_flag = UCD_NEAR=near_$V2_$CPA'
            TIMER_EVENTS="$(ucd_sequence 3)"
            ;;
        encounter_flag_density_pass)
            APP_CONFIG="$(echo "$APP_CONFIG" | sed 's/report_all_encounters = false/report_all_encounters = true/')"
            insert_app_config_line '  encounter_flag = UCD_DENSITY=$[V1@40]'
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0 0 90)", time=0.4
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 30 0 0 270)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 10 0 0 270)", time=1.0
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 30 0 0 270)", time=1.5
EOF
)"
            ;;
        outside_encounter_range_blocks_pass)
            APP_CONFIG="ProcessConfig = uFldCollisionDetect
{
  AppTick   = 10
  CommsTick = 10

  collision_range = 1
  near_miss_range = 1
  encounter_range = 1
  post_closest_range = true
  post_closest_range_ever = true
  report_all_encounters = true
}"
            TIMER_EVENTS="$(ucd_sequence 3)"
            ;;
        closest_posts_disabled_absent_pass)
            APP_CONFIG="ProcessConfig = uFldCollisionDetect
{
  AppTick   = 10
  CommsTick = 10

  collision_range = 3
  near_miss_range = 6
  encounter_range = 20
  post_closest_range = false
  post_closest_range_ever = false
  report_all_encounters = true
}"
            TIMER_EVENTS="$(ucd_sequence 2)"
            ;;
        clear_encounter_default_suppressed_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0 0 90)", time=0.4
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 30 0 0 270)", time=0.5
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 10 0 0 270)", time=1.0
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 30 0 0 270)", time=1.5
EOF
)"
            ;;
        collision_flag_numeric_cpa_pass)
            insert_app_config_line '  collision_flag = UCD_COLLISION_CPA=$CPA'
            ;;
        encounter_rings_true_posts_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0 0 90)", time=0.4
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 10 0 0 270)", time=0.5
EOF
)"
            READY_TIME="2.5"
            ;;
        encounter_rings_false_absent_pass)
            insert_app_config_line '  encounter_rings = false'
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0 0 90)", time=0.4
  event = var=NODE_REPORT, val="$(node_report_xy bravo blue 10 0 0 270)", time=0.5
EOF
)"
            READY_TIME="2.5"
            ;;
        range_normalization_params_pass)
            APP_CONFIG="ProcessConfig = uFldCollisionDetect
{
  AppTick   = 10
  CommsTick = 10

  collision_range = 6
  near_miss_range = 3
  encounter_range = 4
  post_closest_range = true
  post_closest_range_ever = true
  report_all_encounters = false
}"
            TIMER_EVENTS='  event = var=TEST_VISUAL_ONLY, val=true, time=0.6'
            READY_TIME="2.0"
            ;;
        *)
            echo "$ME: Unknown collision-detect case: [$CASE_NAME]"
            return 1
            ;;
    esac
}

cod_sequence() {
    local near_x="$1"
    cat <<EOF
  event = var=KNOWN_OBSTACLE, val="$(obstacle_square obs)", time=0.4
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 25 0)", time=0.7
  event = var=NODE_REPORT, val="$(node_report_xy alpha red "$near_x" 0)", time=1.2
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 25 0)", time=1.8
EOF
}

get_cod_case_config() {
    set_defaults
    APP_NAME="uFldCollObDetect"
    APP_RUN="uFldCollObDetect"
    APP_CONFIG="ProcessConfig = uFldCollObDetect
{
  AppTick   = 10
  CommsTick = 10

  collision_dist = 1
  near_miss_dist = 5
  encounter_dist = 15
  encounter_flag = COD_ENCOUNTER=\$VNAME:\$ID:\$DIST:\$ENC_CNT
  near_miss_flag = COD_NEAR=\$VNAME:\$ID:\$DIST:\$MISS_CNT
  collision_flag = COD_COLLISION=\$VNAME:\$ID:\$DIST:\$COLL_CNT
}"
    TIMER_EVENTS="$(cod_sequence 5.5)"
    READY_TIME="4.0"
    REPORT_COLUMNS='  report_column = cod_encounter=$[COD_ENCOUNTER]
  report_column = cod_near=$[COD_NEAR]
  report_column = cod_collision=$[COD_COLLISION]
  report_column = cod_min=$[OB_GLOBAL_MIN]'

    case "$CASE_NAME" in
        collision_flag_pass)
            ;;
        near_miss_flag_pass)
            TIMER_EVENTS="$(cod_sequence 8)"
            ;;
        encounter_only_flag_pass)
            TIMER_EVENTS="$(cod_sequence 14)"
            ;;
        global_min_distance_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=KNOWN_OBSTACLE, val="$(obstacle_square obs)", time=0.4
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 12 0)", time=0.8
EOF
)"
            READY_TIME="2.5"
            ;;
        invalid_obstacle_absent_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=KNOWN_OBSTACLE, val="pts={0,0:10,0:5,5:10,10:0,10},label=bad", time=0.4
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0)", time=0.8
EOF
)"
            READY_TIME="2.5"
            ;;
        obstacle_clear_blocks_later_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=KNOWN_OBSTACLE, val="$(obstacle_square obs)", time=0.4
  event = var=KNOWN_OBSTACLE_CLEAR, val="all", time=5.0
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 0 0)", time=5.6
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 25 0)", time=6.2
EOF
)"
            READY_TIME="8.0"
            ;;
        upper_macro_flag_pass)
            APP_CONFIG="ProcessConfig = uFldCollObDetect
{
  AppTick   = 10
  CommsTick = 10

  collision_dist = 1
  near_miss_dist = 5
  encounter_dist = 15
  collision_flag = COD_COLLISION_\$UP_VNAME=\$ID:\$COLL_CNT
}"
            ;;
        fresh_clear_keeps_obstacle_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=KNOWN_OBSTACLE, val="$(obstacle_square obs)", time=0.4
  event = var=KNOWN_OBSTACLE_CLEAR, val="all", time=2.0
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 25 0)", time=2.5
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 5.5 0)", time=3.0
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 25 0)", time=3.6
EOF
)"
            READY_TIME="5.5"
            ;;
        encounter_double_flag_pass)
            APP_CONFIG="ProcessConfig = uFldCollObDetect
{
  AppTick   = 10
  CommsTick = 10

  collision_dist = 1
  near_miss_dist = 5
  encounter_dist = 15
  encounter_flag = COD_ENCOUNTER_DIST=\$DIST
}"
            TIMER_EVENTS="$(cod_sequence 14)"
            ;;
        near_miss_upper_macro_flag_pass)
            APP_CONFIG="ProcessConfig = uFldCollObDetect
{
  AppTick   = 10
  CommsTick = 10

  collision_dist = 1
  near_miss_dist = 5
  encounter_dist = 15
  near_miss_flag = COD_NEAR_\$UP_VNAME=\$ID:\$MISS_CNT
}"
            TIMER_EVENTS="$(cod_sequence 8)"
            ;;
        collision_boundary_counts_near_pass)
            TIMER_EVENTS="$(cod_sequence 6)"
            ;;
        near_boundary_counts_encounter_pass)
            TIMER_EVENTS="$(cod_sequence 10)"
            ;;
        named_clear_only_removes_target_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=KNOWN_OBSTACLE, val="$(obstacle_square obs1)", time=0.4
  event = var=KNOWN_OBSTACLE, val="pts={35,-5:45,-5:45,5:35,5},label=obs2", time=0.5
  event = var=KNOWN_OBSTACLE_CLEAR, val="obs1", time=5.2
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 65 0)", time=5.7
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 45.5 0)", time=6.2
  event = var=NODE_REPORT, val="$(node_report_xy alpha red 65 0)", time=6.8
EOF
)"
            READY_TIME="8.5"
            ;;
        range_normalization_collision_pass)
            APP_CONFIG="ProcessConfig = uFldCollObDetect
{
  AppTick   = 10
  CommsTick = 10

  collision_dist = 6
  near_miss_dist = 3
  encounter_dist = 4
  encounter_flag = COD_ENCOUNTER=\$VNAME:\$ID:\$DIST:\$ENC_CNT
  near_miss_flag = COD_NEAR=\$VNAME:\$ID:\$DIST:\$MISS_CNT
  collision_flag = COD_COLLISION=\$VNAME:\$ID:\$DIST:\$COLL_CNT
}"
            TIMER_EVENTS="$(cod_sequence 10.5)"
            ;;
        *)
            echo "$ME: Unknown coll-ob-detect case: [$CASE_NAME]"
            return 1
            ;;
    esac
}

get_scope_case_config() {
    set_defaults
    APP_NAME="uFldScope"
    APP_RUN="uFldScope"
    APP_CONFIG="ProcessConfig = uFldScope
{
  AppTick   = 4
  CommsTick = 4

  scope = var=NODE_REPORT,key=NAME,fld=MODE
  scope = var=UPC_SPEED_REPORT,key=vname,fld=avg_spd,alias=speed
  scope = var=UPC_ODOMETRY_REPORT,key=vname,fld=trip_dist
}"
    TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="NAME=alpha,MODE=survey,X=0,Y=0,SPD=0,HDG=0", time=0.5
  event = var=UPC_SPEED_REPORT, val="vname=alpha,avg_spd=2.5", time=0.7
  event = var=UPC_ODOMETRY_REPORT, val="vname=alpha,trip_dist=12,total_dist=40", time=0.9
  event = var=APPCAST_REQ, val="node=shoreside,app=uFldScope,duration=4,key=harness,thresh=any", time=2.2
EOF
)"
    READY_TIME="4.0"
    REPORT_COLUMNS='  report_column = appcast=$[APPCAST]'

    case "$CASE_NAME" in
        appcast_table_pass)
            ;;
        alias_headers_pass)
            APP_CONFIG="ProcessConfig = uFldScope
{
  AppTick   = 4
  CommsTick = 4

  scope = var=NODE_REPORT,key=NAME,fld=MODE,alias=nav_mode
  scope = var=UPC_SPEED_REPORT,key=vname,fld=avg_spd,alias=speed
  scope = var=UPC_ODOMETRY_REPORT,key=vname,fld=trip_dist,alias=trip
}"
            ;;
        update_replaces_same_key_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="NAME=alpha,MODE=survey,X=0,Y=0,SPD=0,HDG=0", time=0.5
  event = var=NODE_REPORT, val="NAME=alpha,MODE=return,X=0,Y=0,SPD=0,HDG=0", time=0.9
  event = var=APPCAST_REQ, val="node=shoreside,app=uFldScope,duration=4,key=harness,thresh=any", time=2.2
EOF
)"
            ;;
        multi_vehicle_rows_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="NAME=alpha,MODE=survey,X=0,Y=0,SPD=0,HDG=0", time=0.5
  event = var=NODE_REPORT, val="NAME=bravo,MODE=station,X=0,Y=0,SPD=0,HDG=0", time=0.7
  event = var=APPCAST_REQ, val="node=shoreside,app=uFldScope,duration=4,key=harness,thresh=any", time=2.2
EOF
)"
            ;;
        invalid_scope_ignored_pass)
            APP_CONFIG="ProcessConfig = uFldScope
{
  AppTick   = 4
  CommsTick = 4

  scope = var=NODE_REPORT,key=NAME
  scope = var=NODE_REPORT,key=NAME,fld=MODE
}"
            ;;
        missing_field_blank_cell_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="NAME=alpha,X=0,Y=0,SPD=0,HDG=0", time=0.5
  event = var=APPCAST_REQ, val="node=shoreside,app=uFldScope,duration=4,key=harness,thresh=any", time=2.2
EOF
)"
            ;;
        missing_key_blank_row_pass)
            APP_CONFIG="ProcessConfig = uFldScope
{
  AppTick   = 4
  CommsTick = 4

  scope = var=NODE_REPORT,key=NAME,fld=MODE
}"
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="MODE=survey,X=0,Y=0,SPD=0,HDG=0", time=0.5
  event = var=APPCAST_REQ, val="node=shoreside,app=uFldScope,duration=4,key=harness,thresh=any", time=2.2
EOF
)"
            ;;
        later_missing_field_replaces_value_pass)
            TIMER_EVENTS="$(cat <<EOF
  event = var=NODE_REPORT, val="NAME=alpha,MODE=survey,X=0,Y=0,SPD=0,HDG=0", time=0.5
  event = var=NODE_REPORT, val="NAME=alpha,X=0,Y=0,SPD=0,HDG=0", time=0.9
  event = var=APPCAST_REQ, val="node=shoreside,app=uFldScope,duration=4,key=harness,thresh=any", time=2.2
EOF
)"
            ;;
        *)
            echo "$ME: Unknown scope case: [$CASE_NAME]"
            return 1
            ;;
    esac
}

get_case_config() {
    CASE_NAME="$1"
    case "$HARNESS_KIND" in
        pathcheck) get_pathcheck_case_config ;;
        message_handler) get_message_case_config ;;
        contact_range_sensor) get_crs_case_config ;;
        beacon_range_sensor) get_brs_case_config ;;
        collision_detect) get_ucd_case_config ;;
        collob_detect) get_cod_case_config ;;
        scope) get_scope_case_config ;;
        *)
            echo "$ME: Unknown HARNESS_KIND: [$HARNESS_KIND]"
            return 1
            ;;
    esac
}

write_case_meta() {
    local case_dir="$1"
    cat > "$case_dir/meta_shoreside.moosx" <<EOF
//-------------------------------------------------
// FILE: meta_shoreside.moosx
// NAME: Charles Benjamin
//-------------------------------------------------

ServerHost = localhost
ServerPort = \$(MOOS_PORT)
Community  = shoreside

#include plugs.moos <origin_warp>

//----------------------------------------------------
ProcessConfig = ANTLER
{
  MSBetweenLaunches = 80

  Run = MOOSDB       @ NewConsole = false
  Run = pRealm       @ NewConsole = false
  Run = pLogger      @ NewConsole = false
  Run = $APP_RUN     @ NewConsole = false
  Run = uTimerScript @ NewConsole = false
  Run = pMissionEval @ NewConsole = false
}

#include plugs.moos <pLogger>

$APP_CONFIG

//----------------------------------------------------
// uTimerScript Config Block

ProcessConfig = uTimerScript
{
  AppTick   = 4
  CommsTick = 4

$TIMER_EVENTS
  event = var=TEST_DRIVER_DONE, val=true, time=$READY_TIME
  event = var=TEST_EVAL_READY, val=true, time=$READY_TIME
}

//----------------------------------------------------
// pMissionEval Config Block

ProcessConfig = pMissionEval
{
  AppTick   = 4
  CommsTick = 4

  mailflag = @UPC_ODOMETRY_REPORT#ODOM_SEEN=true
  mailflag = @UPC_SPEED_REPORT#SPEED_SEEN=true
  mailflag = @UMH_SUMMARY_MSGS#UMH_SUMMARY_SEEN=true
  mailflag = @HANDLED_RESULT#UMH_RESULT_SEEN=true
  mailflag = @HANDLED_NUMERIC#UMH_NUMERIC_SEEN=true
  mailflag = @CRS_RANGE_REPORT#CRS_SHORT_SEEN=true
  mailflag = @CRS_RANGE_REPORT_ALPHA#CRS_LONG_SEEN=true
  mailflag = @CRS_RANGE_REPORT_GT#CRS_GT_SEEN=true
  mailflag = @VIEW_RANGE_PULSE#CRS_PULSE_SEEN=true
  mailflag = @BRS_RANGE_REPORT#BRS_SHORT_SEEN=true
  mailflag = @BRS_RANGE_REPORT_ALPHA#BRS_LONG_SEEN=true
  mailflag = @BRS_RANGE_REPORT_GT#BRS_GT_SEEN=true
  mailflag = @UCD_REPORT#UCD_REPORT_SEEN=true
  mailflag = @COLLISION_TOTAL#UCD_COLLISION_SEEN=true
  mailflag = @NEAR_MISS_TOTAL#UCD_NEAR_SEEN=true
  mailflag = @ENCOUNTER_TOTAL#UCD_ENCOUNTER_SEEN=true
  mailflag = @COD_ENCOUNTER#COD_ENCOUNTER_SEEN=true
  mailflag = @COD_NEAR#COD_NEAR_SEEN=true
  mailflag = @COD_COLLISION#COD_COLLISION_SEEN=true
  mailflag = @APPCAST#APPCAST_SEEN=true

  lead_condition = TEST_EVAL_READY = true
  pass_condition = TEST_DRIVER_DONE = true

  result_flag  = MISSION_EVALUATED = true
  mission_form = ufield_app_unit
  mission_mod  = \$(MMOD:=$CASE_NAME)

  prereport_column = form=\$[MISSION_FORM]
  prereport_column = mmod=\$[MMOD]
  report_column    = grade=\$[GRADE]
  report_column    = done=\$[TEST_DRIVER_DONE]
  report_column    = odom_seen=\$[ODOM_SEEN]
  report_column    = speed_seen=\$[SPEED_SEEN]
  report_column    = umh_summary_seen=\$[UMH_SUMMARY_SEEN]
  report_column    = umh_result_seen=\$[UMH_RESULT_SEEN]
  report_column    = umh_numeric_seen=\$[UMH_NUMERIC_SEEN]
  report_column    = crs_short_seen=\$[CRS_SHORT_SEEN]
  report_column    = crs_long_seen=\$[CRS_LONG_SEEN]
  report_column    = crs_gt_seen=\$[CRS_GT_SEEN]
  report_column    = crs_pulse_seen=\$[CRS_PULSE_SEEN]
  report_column    = brs_short_seen=\$[BRS_SHORT_SEEN]
  report_column    = brs_long_seen=\$[BRS_LONG_SEEN]
  report_column    = brs_gt_seen=\$[BRS_GT_SEEN]
  report_column    = ucd_report_seen=\$[UCD_REPORT_SEEN]
  report_column    = cod_encounter_seen=\$[COD_ENCOUNTER_SEEN]
  report_column    = appcast_seen=\$[APPCAST_SEEN]
$REPORT_COLUMNS
  report_file      = results.txt
}
EOF
}

prepare_case_dir() {
    local case_dir="$1"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
    )
    write_case_meta "$case_dir"
}

find_alog() {
    local case_dir="$1"
    find "$case_dir" -path '*XLOG_SHORESIDE*/*.alog' -print | sort | tail -n 1
}

alog_var_lines() {
    local case_dir="$1"
    local var="$2"
    local alog
    alog=$(find_alog "$case_dir")
    [ "$alog" != "" ] || return 1
    if [ "$var" = "APPCAST" ]; then
        rg "^[[:space:]]*[0-9].*[[:space:]]${var}[[:space:]]" "$alog"
        return $?
    fi
    aloggrep "$alog" "$var" 2>/dev/null | rg "^[[:space:]]*[0-9].*[[:space:]]${var}[[:space:]]"
}

alog_var_count() {
    local case_dir="$1"
    local var="$2"
    alog_var_lines "$case_dir" "$var" 2>/dev/null | wc -l | tr -d ' '
}

alog_var_matches() {
    local case_dir="$1"
    local var="$2"
    local pattern="$3"
    alog_var_lines "$case_dir" "$var" 2>/dev/null | rg -q "$pattern"
}

alog_var_not_matches() {
    local case_dir="$1"
    local var="$2"
    local pattern="$3"
    ! alog_var_matches "$case_dir" "$var" "$pattern"
}

alog_scope_appcast_matches() {
    local case_dir="$1"
    local pattern="$2"
    alog_var_lines "$case_dir" "APPCAST" 2>/dev/null | rg 'proc=uFldScope' | rg -q "$pattern"
}

alog_var_absent() {
    local case_dir="$1"
    local var="$2"
    [ "$(alog_var_count "$case_dir" "$var")" = "0" ]
}

latest_result_line() {
    local case_dir="$1"
    if [ -s "$case_dir/results.txt" ]; then
        tail -n 1 "$case_dir/results.txt"
    fi
}

format_case_row() {
    local case_name="$1"
    local status="$2"
    local rc="$3"
    local line="$4"
    local grade

    grade=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    line=$(echo "$line" | sed 's/grade=[^, ]*[[:space:]]*//')

    if [ "$status" = "success" ]; then
        if [ "$grade" = "" ]; then
            echo "case=$case_name  grade=fail  reason=missing_result  mayfinish_rc=$rc  $line"
            return
        fi
        echo "case=$case_name  grade=$grade  $line  mayfinish_rc=$rc"
    else
        echo "case=$case_name  grade=fail  reason=$status  mayfinish_rc=$rc  $line"
    fi
}

case_evidence_ok() {
    local case_dir="$1"
    local line="$2"
    case "$CASE_NAME" in
        odometry_baseline_pass|node_report_local_pass|mixed_report_sources_pass|trip_reset_unknown_ignored_pass|bad_then_good_recovery_pass)
            alog_var_matches "$case_dir" "UPC_ODOMETRY_REPORT" 'vname=alpha.*total_dist=5\.0.*trip_dist=5\.0' && \
            alog_var_matches "$case_dir" "UPC_SPEED_REPORT" 'vname=alpha.*avg_spd=5\.00'
            return $?
            ;;
        two_reports_speed_only_pass)
            alog_var_absent "$case_dir" "UPC_ODOMETRY_REPORT" && \
            alog_var_matches "$case_dir" "UPC_SPEED_REPORT" 'vname=alpha.*avg_spd=5\.00'
            return $?
            ;;
        trip_reset_pass)
            alog_var_matches "$case_dir" "UPC_ODOMETRY_REPORT" 'vname=alpha.*total_dist=10\.0.*trip_dist=5\.0'
            return $?
            ;;
        trip_reset_twice_pass)
            alog_var_matches "$case_dir" "UPC_ODOMETRY_REPORT" 'vname=alpha.*total_dist=15\.0.*trip_dist=5\.0'
            return $?
            ;;
        multi_node_independent_pass)
            alog_var_matches "$case_dir" "UPC_ODOMETRY_REPORT" 'vname=alpha.*total_dist=5\.0.*trip_dist=5\.0' && \
            alog_var_matches "$case_dir" "UPC_ODOMETRY_REPORT" 'vname=bravo.*total_dist=12\.0.*trip_dist=12\.0'
            return $?
            ;;
        variable_speed_average_pass)
            alog_var_matches "$case_dir" "UPC_ODOMETRY_REPORT" 'vname=alpha.*total_dist=20\.0.*trip_dist=20\.0' && \
            alog_var_matches "$case_dir" "UPC_SPEED_REPORT" 'vname=alpha.*avg_spd=8\.33'
            return $?
            ;;
        history_length_three_average_pass)
            alog_var_matches "$case_dir" "UPC_ODOMETRY_REPORT" 'vname=alpha.*total_dist=25\.0.*trip_dist=25\.0' && \
            alog_var_matches "$case_dir" "UPC_SPEED_REPORT" 'vname=alpha.*avg_spd=12\.50'
            return $?
            ;;
        invalid_history_length_default_pass)
            alog_var_matches "$case_dir" "UPC_ODOMETRY_REPORT" 'vname=alpha.*total_dist=25\.0.*trip_dist=25\.0' && \
            alog_var_matches "$case_dir" "UPC_SPEED_REPORT" 'vname=alpha.*avg_spd=10\.00'
            return $?
            ;;
        reverse_leg_accumulates_pass)
            alog_var_matches "$case_dir" "UPC_ODOMETRY_REPORT" 'vname=alpha.*total_dist=5\.0.*trip_dist=5\.0' && \
            alog_var_matches "$case_dir" "UPC_SPEED_REPORT" 'vname=alpha.*avg_spd=5\.00'
            return $?
            ;;
        history_length_two_speed_pass)
            alog_var_matches "$case_dir" "UPC_ODOMETRY_REPORT" 'vname=alpha.*total_dist=10\.0.*trip_dist=10\.0' && \
            alog_var_matches "$case_dir" "UPC_SPEED_REPORT" 'vname=alpha.*avg_spd=10\.00'
            return $?
            ;;
        invalid_report_rejected_pass)
            alog_var_absent "$case_dir" "UPC_ODOMETRY_REPORT" && \
            alog_var_absent "$case_dir" "UPC_SPEED_REPORT"
            return $?
            ;;
        stationary_zero_distance_pass)
            alog_var_matches "$case_dir" "UPC_ODOMETRY_REPORT" 'vname=alpha.*total_dist=0\.0.*trip_dist=0\.0' && \
            alog_var_matches "$case_dir" "UPC_SPEED_REPORT" 'vname=alpha.*avg_spd=0\.00'
            return $?
            ;;
        diagonal_chain_distance_pass)
            alog_var_matches "$case_dir" "UPC_ODOMETRY_REPORT" 'vname=alpha.*total_dist=10\.0.*trip_dist=10\.0'
            return $?
            ;;
        dest_all_string_pass)
            alog_var_matches "$case_dir" "HANDLED_RESULT" 'all_ok' && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=1,valid=1,rejected=0'
            return $?
            ;;
        dest_all_uppercase_pass)
            alog_var_matches "$case_dir" "HANDLED_RESULT" 'all_upper_ok' && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=1,valid=1,rejected=0'
            return $?
            ;;
        dest_specific_self_pass)
            alog_var_matches "$case_dir" "HANDLED_RESULT" 'self_ok'
            return $?
            ;;
        dest_mismatch_reject_pass|strict_all_reject_pass|strict_group_reject_pass)
            alog_var_absent "$case_dir" "HANDLED_RESULT" && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=1,valid=1,rejected=1'
            return $?
            ;;
        strict_specific_accept_pass)
            alog_var_matches "$case_dir" "HANDLED_RESULT" 'strict_accept' && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=1,valid=1,rejected=0'
            return $?
            ;;
        numeric_payload_pass)
            alog_var_matches "$case_dir" "HANDLED_NUMERIC" '42\.5'
            return $?
            ;;
        double_zero_payload_pass)
            alog_var_matches "$case_dir" "HANDLED_NUMERIC" 'HANDLED_NUMERIC.*[[:space:]]0(\.0+)?[[:space:]]*$'
            return $?
            ;;
        mixed_payload_invalid_pass)
            alog_var_absent "$case_dir" "HANDLED_RESULT" && \
            alog_var_absent "$case_dir" "HANDLED_NUMERIC" && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=1,valid=0,rejected=0'
            return $?
            ;;
        quoted_string_unquoted_pass)
            alog_var_matches "$case_dir" "HANDLED_RESULT" 'quotedok'
            return $?
            ;;
        src_app_forward_pass)
            alog_var_matches "$case_dir" "HANDLED_RESULT" 'src_app_ok' && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=1,valid=1,rejected=0'
            return $?
            ;;
        invalid_payload_reject_pass)
            alog_var_absent "$case_dir" "HANDLED_RESULT" && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=1,valid=0,rejected=0'
            return $?
            ;;
        invalid_no_bad_flag_pass)
            alog_var_absent "$case_dir" "HANDLED_RESULT" && \
            alog_var_absent "$case_dir" "UMH_BAD" && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=1,valid=0,rejected=0'
            return $?
            ;;
        missing_destination_invalid_pass)
            alog_var_absent "$case_dir" "HANDLED_RESULT" && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=1,valid=0,rejected=0'
            return $?
            ;;
        msg_flag_macro_pass)
            alog_var_matches "$case_dir" "HANDLED_RESULT" 'flagged' && \
            alog_var_matches "$case_dir" "UMH_GOOD" 'good_1_of_1'
            return $?
            ;;
        msg_flag_numeric_pass)
            alog_var_matches "$case_dir" "HANDLED_RESULT" 'flagged_num' && \
            alog_var_matches "$case_dir" "UMH_GOOD_NUM" '7'
            return $?
            ;;
        bad_msg_flag_macro_pass)
            alog_var_absent "$case_dir" "HANDLED_RESULT" && \
            alog_var_matches "$case_dir" "UMH_BAD" 'bad_1_of_1'
            return $?
            ;;
        bad_msg_flag_numeric_pass)
            alog_var_absent "$case_dir" "HANDLED_RESULT" && \
            alog_var_matches "$case_dir" "UMH_BAD_NUM" '11'
            return $?
            ;;
        multiple_bad_summary_pass)
            alog_var_absent "$case_dir" "HANDLED_RESULT" && \
            alog_var_matches "$case_dir" "UMH_BAD" 'bad_2_of_2' && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=2,valid=2,rejected=2'
            return $?
            ;;
        aux_info_node_app_forward_pass)
            alog_var_matches "$case_dir" "HANDLED_RESULT" 'aux_ok' && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=1,valid=1,rejected=0'
            return $?
            ;;
        dest_group_accept_pass)
            alog_var_matches "$case_dir" "HANDLED_RESULT" 'group_ok'
            return $?
            ;;
        dest_node_overrides_group_reject_pass)
            alog_var_absent "$case_dir" "HANDLED_RESULT" && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=1,valid=1,rejected=1'
            return $?
            ;;
        dest_all_mixedcase_reject_pass)
            alog_var_absent "$case_dir" "HANDLED_RESULT" && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=1,valid=1,rejected=1'
            return $?
            ;;
        multiple_good_summary_pass)
            alog_var_matches "$case_dir" "HANDLED_RESULT" 'second_ok' && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=2,valid=2,rejected=0'
            return $?
            ;;
        mixed_valid_rejected_summary_pass)
            alog_var_matches "$case_dir" "HANDLED_RESULT" 'first_ok' && \
            alog_var_absent "$case_dir" "HANDLED_REJECT" && \
            alog_var_matches "$case_dir" "UMH_SUMMARY_MSGS" 'total=2,valid=2,rejected=1'
            return $?
            ;;
        baseline_short_report_pass|allow_echo_type_accept_pass|allow_echo_type_multi_accept_ship_pass|sensor_arc_forward_accept_pass|sensor_arc_wrap_accept_pass|sensor_arc_full_accept_pass|crs_node_report_local_pass)
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*target=bravo' && \
            alog_var_matches "$case_dir" "VIEW_RANGE_PULSE" 'label=(alpha_ping|bravo_echo)'
            return $?
            ;;
        sensor_arc_multi_segment_aft_accept_pass)
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*target=bravo'
            return $?
            ;;
        report_vars_short_no_long_pass)
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*target=bravo' && \
            alog_var_absent "$case_dir" "CRS_RANGE_REPORT_ALPHA"
            return $?
            ;;
        report_vars_long_pass)
            alog_var_absent "$case_dir" "CRS_RANGE_REPORT" && \
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT_ALPHA" 'range=50(\.0+)?.*target=bravo'
            return $?
            ;;
        report_vars_both_pass)
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*target=bravo' && \
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT_ALPHA" 'range=50(\.0+)?.*target=bravo'
            return $?
            ;;
        ground_truth_uniform_zero_pass)
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*target=bravo' && \
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT_GT" 'vname=alpha.*range=50(\.0+)?.*target=bravo'
            return $?
            ;;
        ground_truth_gaussian_unsupported_no_gt_pass)
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*target=bravo' && \
            alog_var_absent "$case_dir" "CRS_RANGE_REPORT_GT"
            return $?
            ;;
        ground_truth_long_only_pass)
            alog_var_absent "$case_dir" "CRS_RANGE_REPORT" && \
            alog_var_absent "$case_dir" "CRS_RANGE_REPORT_GT" && \
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT_ALPHA" 'range=50(\.0+)?.*target=bravo' && \
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT_GT_ALPHA" 'range=50(\.0+)?.*target=bravo'
            return $?
            ;;
        ground_truth_false_no_gt_pass)
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*target=bravo' && \
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT_ALPHA" 'range=50(\.0+)?.*target=bravo' && \
            alog_var_absent "$case_dir" "CRS_RANGE_REPORT_GT"
            return $?
            ;;
        ground_truth_no_algorithm_absent_pass)
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*target=bravo' && \
            alog_var_absent "$case_dir" "CRS_RANGE_REPORT_GT"
            return $?
            ;;
        allow_echo_type_block_pass|sensor_arc_aft_block_pass|unknown_request_no_report_pass|missing_name_request_no_report_pass|default_short_blocks_far_pass|short_total_range_far_blocks_pass|named_push_short_blocks_far_pass)
            alog_var_absent "$case_dir" "CRS_RANGE_REPORT" && \
            alog_var_absent "$case_dir" "CRS_RANGE_REPORT_ALPHA"
            return $?
            ;;
        ping_wait_blocks_second_pass|named_ping_wait_blocks_second_pass)
            [ "$(alog_var_count "$case_dir" "CRS_RANGE_REPORT")" = "1" ]
            return $?
            ;;
        ping_wait_unlimited_allows_second_pass|named_ping_wait_unlimited_allows_second_pass|ping_wait_nolimit_alias_allows_second_pass)
            [ "$(alog_var_count "$case_dir" "CRS_RANGE_REPORT")" -ge "2" ]
            return $?
            ;;
        display_pulses_false_pass)
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*target=bravo' && \
            alog_var_absent "$case_dir" "VIEW_RANGE_PULSE"
            return $?
            ;;
        unlimited_far_range_pass|push_distance_alias_far_pass|named_pull_unlimited_far_pass|pull_distance_alias_far_pass)
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT" 'vname=alpha.*range=500(\.0+)?.*target=bravo'
            return $?
            ;;
        reciprocal_request_report_pass)
            alog_var_matches "$case_dir" "CRS_RANGE_REPORT" 'vname=bravo.*range=50(\.0+)?.*target=alpha'
            return $?
            ;;
        request_short_report_pass)
            alog_var_matches "$case_dir" "BRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*id=beacon' && \
            alog_var_matches "$case_dir" "VIEW_RANGE_PULSE" 'label=(alpha_range_req|beacon)'
            return $?
            ;;
        request_long_report_pass)
            alog_var_absent "$case_dir" "BRS_RANGE_REPORT" && \
            alog_var_matches "$case_dir" "BRS_RANGE_REPORT_ALPHA" 'range=50(\.0+)?.*id=beacon'
            return $?
            ;;
        request_both_report_pass)
            alog_var_matches "$case_dir" "BRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*id=beacon' && \
            alog_var_matches "$case_dir" "BRS_RANGE_REPORT_ALPHA" 'range=50(\.0+)?.*id=beacon'
            return $?
            ;;
        brs_ground_truth_uniform_zero_pass)
            if [ "$HARNESS_KIND" = "beacon_range_sensor" ]; then
                alog_var_matches "$case_dir" "BRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*id=beacon' && \
                alog_var_matches "$case_dir" "BRS_RANGE_REPORT_GT" 'vname=ALPHA.*range=50(\.0+)?.*id=beacon'
                return $?
            fi
            ;;
        brs_ground_truth_false_no_gt_pass)
            if [ "$HARNESS_KIND" = "beacon_range_sensor" ]; then
                alog_var_matches "$case_dir" "BRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*id=beacon' && \
                alog_var_absent "$case_dir" "BRS_RANGE_REPORT_GT"
                return $?
            fi
            ;;
        far_request_blocked_pass)
            alog_var_absent "$case_dir" "BRS_RANGE_REPORT" && \
            alog_var_absent "$case_dir" "BRS_RANGE_REPORT_ALPHA"
            return $?
            ;;
        node_push_unlimited_far_pass)
            alog_var_matches "$case_dir" "BRS_RANGE_REPORT" 'vname=alpha.*range=500(\.0+)?.*id=farbeacon'
            return $?
            ;;
        brs_ping_wait_blocks_second_pass)
            if [ "$HARNESS_KIND" = "beacon_range_sensor" ]; then
                [ "$(alog_var_count "$case_dir" "BRS_RANGE_REPORT")" = "1" ]
                return $?
            fi
            ;;
        brs_named_ping_wait_blocks_only_alpha_pass)
            [ "$(alog_var_count "$case_dir" "BRS_RANGE_REPORT")" = "2" ] && \
            alog_var_matches "$case_dir" "BRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*id=beacon' && \
            alog_var_matches "$case_dir" "BRS_RANGE_REPORT" 'vname=bravo.*range=50(\.0+)?.*id=beacon'
            return $?
            ;;
        ping_payment_request_blocks_retry_pass)
            alog_var_absent "$case_dir" "BRS_RANGE_REPORT"
            return $?
            ;;
        ping_payment_response_allows_retry_pass)
            alog_var_matches "$case_dir" "BRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*id=beacon'
            return $?
            ;;
        ping_payment_accept_blocks_retry_pass)
            alog_var_absent "$case_dir" "BRS_RANGE_REPORT" && \
            [ "$(alog_var_count "$case_dir" "VIEW_RANGE_PULSE")" = "1" ]
            return $?
            ;;
        brs_node_report_local_pass)
            alog_var_matches "$case_dir" "BRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*id=beacon'
            return $?
            ;;
        beacon_style_defaults_pass)
            alog_var_matches "$case_dir" "VIEW_MARKER" 'x=30.*y=40.*width=7.*primary_color=green.*type=square'
            return $?
            ;;
        beacon_reply_push_blocks_pass)
            alog_var_absent "$case_dir" "BRS_RANGE_REPORT" && \
            alog_var_matches "$case_dir" "VIEW_RANGE_PULSE" 'label=alpha_range_req'
            return $?
            ;;
        request_path_pull_blocks_pass)
            alog_var_absent "$case_dir" "BRS_RANGE_REPORT" && \
            alog_var_matches "$case_dir" "VIEW_RANGE_PULSE" 'label=alpha_range_req'
            return $?
            ;;
        unknown_request_no_report_pass|missing_name_request_no_report_pass)
            alog_var_absent "$case_dir" "BRS_RANGE_REPORT" && \
            alog_var_absent "$case_dir" "VIEW_RANGE_PULSE"
            return $?
            ;;
        beacon_visual_marker_pass)
            alog_var_matches "$case_dir" "VIEW_MARKER" 'x=30.*y=40'
            return $?
            ;;
        unsolicited_frequency_pass)
            alog_var_matches "$case_dir" "BRS_RANGE_REPORT" 'vname=alpha.*range=50(\.0+)?.*id=beacon'
            return $?
            ;;
        collision_event_pass)
            alog_var_matches "$case_dir" "COLLISION_TOTAL" 'COLLISION_TOTAL.*[[:space:]]1(\.0+)?[[:space:]]*$' && \
            alog_var_matches "$case_dir" "UCD_REPORT" 'cpa=2.*rank=collision' && \
            [ "$(alog_var_count "$case_dir" "VIEW_RANGE_PULSE")" -ge "1" ]
            return $?
            ;;
        near_miss_event_pass)
            alog_var_matches "$case_dir" "NEAR_MISS_TOTAL" 'NEAR_MISS_TOTAL.*[[:space:]]1(\.0+)?[[:space:]]*$' && \
            alog_var_matches "$case_dir" "UCD_REPORT" 'cpa=3.*rank=near_miss'
            return $?
            ;;
        clear_encounter_report_pass)
            alog_var_matches "$case_dir" "ENCOUNTER_TOTAL" 'ENCOUNTER_TOTAL.*[[:space:]]1(\.0+)?[[:space:]]*$' && \
            alog_var_matches "$case_dir" "UCD_REPORT" 'cpa=10.*rank=clear'
            return $?
            ;;
        closest_range_posts_pass)
            alog_var_matches "$case_dir" "UCD_CLOSEST_RANGE" 'UCD_CLOSEST_RANGE.*[[:space:]]8(\.0+)?[[:space:]]*$' && \
            alog_var_matches "$case_dir" "UCD_CLOSEST_RANGE_EVER" 'UCD_CLOSEST_RANGE_EVER.*[[:space:]]8(\.0+)?[[:space:]]*$'
            return $?
            ;;
        reset_closest_range_ever_pass)
            alog_var_matches "$case_dir" "UCD_CLOSEST_RANGE_EVER" 'UCD_CLOSEST_RANGE_EVER.*[[:space:]]12(\.0+)?[[:space:]]*$'
            return $?
            ;;
        pulse_render_false_pass)
            alog_var_matches "$case_dir" "UCD_REPORT" 'rank=collision' && \
            alog_var_absent "$case_dir" "VIEW_RANGE_PULSE"
            return $?
            ;;
        ignore_group_blocks_pass|reject_group_blocks_pass|condition_blocks_pass)
            alog_var_absent "$case_dir" "UCD_REPORT"
            return $?
            ;;
        condition_allows_pass)
            alog_var_matches "$case_dir" "UCD_REPORT" 'rank=collision' && \
            alog_var_matches "$case_dir" "COLLISION_TOTAL" 'COLLISION_TOTAL.*[[:space:]]1(\.0+)?[[:space:]]*$'
            return $?
            ;;
        collision_flag_macro_pass)
            alog_var_matches "$case_dir" "UCD_HIT" 'hit_alpha_2'
            return $?
            ;;
        near_miss_flag_macro_pass)
            alog_var_matches "$case_dir" "UCD_NEAR" 'near_alpha_3'
            return $?
            ;;
        encounter_flag_density_pass)
            alog_var_matches "$case_dir" "UCD_DENSITY" 'UCD_DENSITY.*[[:space:]]1(\.0+)?[[:space:]]*$'
            return $?
            ;;
        outside_encounter_range_blocks_pass)
            alog_var_absent "$case_dir" "UCD_REPORT"
            return $?
            ;;
        closest_posts_disabled_absent_pass)
            alog_var_matches "$case_dir" "UCD_REPORT" 'rank=collision' && \
            alog_var_absent "$case_dir" "UCD_CLOSEST_RANGE" && \
            alog_var_absent "$case_dir" "UCD_CLOSEST_RANGE_EVER"
            return $?
            ;;
        clear_encounter_default_suppressed_pass)
            alog_var_matches "$case_dir" "ENCOUNTER_TOTAL" 'ENCOUNTER_TOTAL.*[[:space:]]1(\.0+)?[[:space:]]*$' && \
            alog_var_absent "$case_dir" "UCD_REPORT"
            return $?
            ;;
        collision_flag_numeric_cpa_pass)
            alog_var_matches "$case_dir" "UCD_COLLISION_CPA" 'UCD_COLLISION_CPA.*[[:space:]]2(\.0+)?[[:space:]]*$'
            return $?
            ;;
        encounter_rings_true_posts_pass)
            alog_var_matches "$case_dir" "VIEW_CIRCLE" 'label=alpharng' && \
            alog_var_matches "$case_dir" "VIEW_CIRCLE" 'label=bravorng'
            return $?
            ;;
        encounter_rings_false_absent_pass)
            alog_var_absent "$case_dir" "VIEW_CIRCLE"
            return $?
            ;;
        range_normalization_params_pass)
            alog_var_matches "$case_dir" "COLLISION_DETECT_PARAMS" 'collision_range=6, near_miss_range=6, encounter_range=6'
            return $?
            ;;
        collision_flag_pass)
            alog_var_matches "$case_dir" "COD_COLLISION" 'alpha:obs:0\.5:1' && \
            alog_var_matches "$case_dir" "COD_ENCOUNTER" 'alpha:obs:0\.5:1'
            return $?
            ;;
        near_miss_flag_pass)
            alog_var_matches "$case_dir" "COD_NEAR" 'alpha:obs:3:1' && \
            alog_var_matches "$case_dir" "COD_ENCOUNTER" 'alpha:obs:3:1'
            return $?
            ;;
        encounter_only_flag_pass)
            alog_var_absent "$case_dir" "COD_NEAR" && \
            alog_var_absent "$case_dir" "COD_COLLISION" && \
            alog_var_matches "$case_dir" "COD_ENCOUNTER" 'alpha:obs:9:1'
            return $?
            ;;
        global_min_distance_pass)
            alog_var_matches "$case_dir" "OB_GLOBAL_MIN" 'OB_GLOBAL_MIN.*[[:space:]]7(\.0+)?[[:space:]]*$'
            return $?
            ;;
        invalid_obstacle_absent_pass|obstacle_clear_blocks_later_pass)
            alog_var_absent "$case_dir" "COD_ENCOUNTER" && \
            alog_var_absent "$case_dir" "COD_COLLISION" && \
            alog_var_absent "$case_dir" "OB_GLOBAL_MIN"
            return $?
            ;;
        upper_macro_flag_pass)
            alog_var_matches "$case_dir" "COD_COLLISION_ALPHA" 'obs:1'
            return $?
            ;;
        fresh_clear_keeps_obstacle_pass)
            alog_var_matches "$case_dir" "COD_COLLISION" 'alpha:obs:0\.5:1' && \
            alog_var_matches "$case_dir" "COD_ENCOUNTER" 'alpha:obs:0\.5:1'
            return $?
            ;;
        encounter_double_flag_pass)
            alog_var_matches "$case_dir" "COD_ENCOUNTER_DIST" 'COD_ENCOUNTER_DIST.*[[:space:]]9(\.0+)?[[:space:]]*$'
            return $?
            ;;
        near_miss_upper_macro_flag_pass)
            alog_var_matches "$case_dir" "COD_NEAR_ALPHA" 'obs:1'
            return $?
            ;;
        collision_boundary_counts_near_pass)
            alog_var_absent "$case_dir" "COD_COLLISION" && \
            alog_var_matches "$case_dir" "COD_NEAR" 'alpha:obs:1:1' && \
            alog_var_matches "$case_dir" "COD_ENCOUNTER" 'alpha:obs:1:1'
            return $?
            ;;
        near_boundary_counts_encounter_pass)
            alog_var_absent "$case_dir" "COD_NEAR" && \
            alog_var_absent "$case_dir" "COD_COLLISION" && \
            alog_var_matches "$case_dir" "COD_ENCOUNTER" 'alpha:obs:5:1'
            return $?
            ;;
        named_clear_only_removes_target_pass)
            alog_var_matches "$case_dir" "COD_COLLISION" 'alpha:obs2:0\.5:1' && \
            alog_var_matches "$case_dir" "COD_ENCOUNTER" 'alpha:obs2:0\.5:1' && \
            alog_var_not_matches "$case_dir" "COD_COLLISION" 'obs1'
            return $?
            ;;
        range_normalization_collision_pass)
            alog_var_matches "$case_dir" "COD_COLLISION" 'alpha:obs:5\.5:1' && \
            alog_var_matches "$case_dir" "COD_ENCOUNTER" 'alpha:obs:5\.5:1'
            return $?
            ;;
        appcast_table_pass)
            alog_scope_appcast_matches "$case_dir" 'alpha' && \
            alog_scope_appcast_matches "$case_dir" 'survey' && \
            alog_scope_appcast_matches "$case_dir" '2\.5' && \
            alog_scope_appcast_matches "$case_dir" '12'
            return $?
            ;;
        alias_headers_pass)
            alog_scope_appcast_matches "$case_dir" 'nav_mode' && \
            alog_scope_appcast_matches "$case_dir" 'speed' && \
            alog_scope_appcast_matches "$case_dir" 'trip' && \
            ! alog_scope_appcast_matches "$case_dir" 'trip_dist'
            return $?
            ;;
        update_replaces_same_key_pass)
            alog_scope_appcast_matches "$case_dir" 'return' && \
            ! alog_scope_appcast_matches "$case_dir" 'survey'
            return $?
            ;;
        multi_vehicle_rows_pass)
            alog_scope_appcast_matches "$case_dir" 'alpha' && \
            alog_scope_appcast_matches "$case_dir" 'bravo' && \
            alog_scope_appcast_matches "$case_dir" 'station'
            return $?
            ;;
        invalid_scope_ignored_pass)
            alog_scope_appcast_matches "$case_dir" 'MODE'
            return $?
            ;;
        missing_field_blank_cell_pass)
            alog_scope_appcast_matches "$case_dir" 'alpha' && \
            alog_scope_appcast_matches "$case_dir" 'MODE' && \
            ! alog_scope_appcast_matches "$case_dir" 'survey'
            return $?
            ;;
        missing_key_blank_row_pass)
            alog_scope_appcast_matches "$case_dir" 'MODE' && \
            alog_scope_appcast_matches "$case_dir" 'survey' && \
            ! alog_scope_appcast_matches "$case_dir" 'alpha'
            return $?
            ;;
        later_missing_field_replaces_value_pass)
            alog_scope_appcast_matches "$case_dir" 'alpha' && \
            ! alog_scope_appcast_matches "$case_dir" 'survey'
            return $?
            ;;
    esac

    echo "$ME: No evidence check for case: $CASE_NAME"
    return 1
}

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    local case_tag
    local case_dir
    local case_row_file
    local case_base
    local rc
    local status
    local line

    CASE_NAME="$case_name"
    get_case_config "$case_name" || return 1

    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    case_row_file="$CASE_ROW_DIR/${case_tag}.txt"
    mkdir -p "$case_dir"
    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  grade=fail  reason=prepare_error" > "$case_row_file"
        return 1
    }
    case_base=$((PORT_BASE + case_idx*PORT_STRIDE))

    (
        cd "$case_dir"
        : > results.txt
        ./launch.sh --just_make --xlaunched --nogui --mport="$case_base" --pshare="$((case_base + 2))" --mmod="$case_name" "$TIME_WARP" >/dev/null
        if [ "$JUST_MAKE" = "yes" ]; then
            exit 0
        fi
        pAntler targ_shoreside.moos >/dev/null 2>&1 &
        launcher_pid=$!
        uMayFinish --max_time="$CASE_MAX_TIME" --alias="uMayFinish_${case_idx}" targ_shoreside.moos >/dev/null
        rc=$?
        teardown_rc=0
        if ! moos_scoped_teardown_stop_root "$case_dir"; then
            echo "$ME: Scoped teardown failed for $case_dir" >&2
            teardown_rc=1
        fi
        kill "$launcher_pid" >/dev/null 2>&1 || true
        wait "$launcher_pid" >/dev/null 2>&1 || true
        if [ "$rc" -eq 0 ] && [ "$teardown_rc" -ne 0 ]; then
            exit "$teardown_rc"
        fi
        exit "$rc"
    )
    rc=$?

    if [ "$JUST_MAKE" = "yes" ]; then
        echo "case=$case_name  grade=pass  reason=just_make" > "$case_row_file"
        return 0
    fi

    line=$(latest_result_line "$case_dir")
    status="success"
    if [ "$rc" != "0" ]; then
        status="mayfinish_error"
    elif ! echo "$line" | rg -q 'grade=pass'; then
        status="mission_grade_mismatch"
    elif ! CASE_NAME="$case_name" case_evidence_ok "$case_dir" "$line"; then
        status="evidence_mismatch"
    fi

    format_case_row "$case_name" "$status" "$rc" "$line" > "$case_row_file"
    [ "$status" = "success" ]
}

if [ "${#ALL_CASES[@]}" -eq 0 ]; then
    echo "$ME: No ALL_CASES entries configured"
    exit 1
fi

RUN_CASES=("${ALL_CASES[@]}")
if [ "$CASE" != "" ]; then
    RUN_CASES=("$CASE")
fi

if [ ! -d "$MISSION_DIR" ]; then
    echo "$ME: Missing mission dir: $MISSION_DIR"
    exit 1
fi

: > "$RESULTS_FILE"
RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_${RUN_ROOT_PREFIX}_XXXXXX")
CASE_ROW_DIR="$RUN_ROOT/case_rows"
mkdir -p "$CASE_ROW_DIR"

idx=0
wave_count=0
for case_name in "${RUN_CASES[@]}"; do
    run_case_isolated "$idx" "$case_name" &
    wave_count=$((wave_count + 1))
    idx=$((idx + 1))
    if [ "$wave_count" -ge "$JOBS" ]; then
        wait || ALL_OK="no"
        wave_count=0
    fi
done
wait || ALL_OK="no"

for result_file in $(find "$CASE_ROW_DIR" -type f | sort); do
    cat "$result_file" >> "$RESULTS_FILE"
    if ! rg -q '^case=[^[:space:]]+[[:space:]]+grade=pass([[:space:]]|$)' "$result_file"; then
        ALL_OK="no"
    fi
done

cat "$RESULTS_FILE"

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
