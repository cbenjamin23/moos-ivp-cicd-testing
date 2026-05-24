#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-ufield_comms_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
ME=`basename "$0"`
HARNESS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(git -C "$HARNESS_DIR" rev-parse --show-toplevel)"
MISSION_DIR="$REPO_DIR/missions/ufield_comms_missions/ufield_comms_unit"
RESULTS_FILE="$HARNESS_DIR/results.txt"
TIME_WARP=20
MAX_TIME=4000
JOBS=1
PORT_BASE=4000
PORT_STRIDE=30
CASE=""
JUST_MAKE="no"
KEEP_WORKDIRS="no"
NOGUI="--nogui"
RUN_ROOT=""
ALL_OK="yes"

source "$REPO_DIR/scripts/harness_teardown.sh"

cleanup() {
    if [ "$RUN_ROOT" != "" ]; then
        harness_teardown_stop_root "$RUN_ROOT" >/dev/null 2>&1 || true
    fi
    if [ "$RUN_ROOT" != "" ] && [ "$KEEP_WORKDIRS" != "yes" ]; then
        rm -rf "$RUN_ROOT"
    fi
}
trap cleanup EXIT

usage() {
    echo "$ME [OPTIONS] [time_warp]"
    echo "  --case=<name>      Run one named case"
    echo "  --jobs=N           Run up to N cases per wave"
    echo "  --port_base=N      Base port; default 4000"
    echo "  --max_time=N       Max time passed to xlaunch"
    echo "  --just_make, -j    Generate target files only"
    echo "  --keep_workdirs    Keep isolated temp mission copies"
    echo "  --gui              Launch pMarineViewer"
}

for ARGI; do
    if [ "$ARGI" = "--help" ] || [ "$ARGI" = "-h" ]; then
        usage
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" ] && [ "$TIME_WARP" = 20 ]; then
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
    elif [ "$ARGI" = "--gui" ]; then
        NOGUI=""
    else
        echo "$ME: Bad arg: $ARGI"
        exit 1
    fi
done

ALL_CASES=(
baseline_broker_comms_pass
critical_range_override_pass
shared_node_reports_pass
pulse_disabled_pass
range_block_pass
zero_range_block_pass
drop_all_reports_pass
far_range_block_pass
no_limit_far_pass
max_msg_length_block_pass
dest_specific_message_pass
dest_group_message_pass
ack_requested_mediated_pass
double_val_message_pass
unknown_dest_block_pass
ghost_source_block_pass
invalid_ack_block_pass
msg_group_filter_pass
report_group_filter_pass
stealth_range_block_pass
earange_extend_pass
stale_message_block_pass
min_msg_interval_filter_pass
runtime_range_extend_pass
runtime_stealth_block_pass
runtime_earange_extend_pass
runtime_drop_all_pass
runtime_pulse_disabled_pass
runtime_shared_reports_pass
shared_node_reports_numeric_pass
min_rpt_interval_filter_pass
stale_time_unlimited_pass
max_msg_length_unlimited_pass
earrange_alias_extend_pass
critical_group_override_pass
)

case_list() {
    printf "%s\n" "${ALL_CASES[@]}"
}

get_case_config() {
    CASE_NAME="$1"
    EXPECTED="pass"
    SHORE_PATCH=""
    EXTRA_CHECK=""
    POSITION_MODE="near"
    GROUP_MODE="same"
    REPORT_MODE="fresh"
    MESSAGE_MODE="single"
    ACK_MODE="valid"

    case "$CASE_NAME" in
        baseline_broker_comms_pass)
            EXTRA_CHECK="baseline"
            ;;
        critical_range_override_pass)
            SHORE_PATCH="$HARNESS_DIR/critical-range-override-pass-shoreside.xmoos"
            EXTRA_CHECK="reports"
            ;;
        shared_node_reports_pass)
            SHORE_PATCH="$HARNESS_DIR/shared-node-reports-pass-shoreside.xmoos"
            EXTRA_CHECK="shared"
            ;;
        pulse_disabled_pass)
            SHORE_PATCH="$HARNESS_DIR/pulse-disabled-pass-shoreside.xmoos"
            EXTRA_CHECK="no_pulse"
            ;;
        range_block_pass)
            SHORE_PATCH="$HARNESS_DIR/range-block-pass-shoreside.xmoos"
            EXTRA_CHECK="range_block"
            ;;
        zero_range_block_pass)
            SHORE_PATCH="$HARNESS_DIR/zero-range-block-pass-shoreside.xmoos"
            EXTRA_CHECK="range_block"
            ;;
        drop_all_reports_pass)
            SHORE_PATCH="$HARNESS_DIR/drop-all-reports-pass-shoreside.xmoos"
            EXTRA_CHECK="range_block"
            ;;
        far_range_block_pass)
            SHORE_PATCH="$HARNESS_DIR/far-range-block-pass-shoreside.xmoos"
            EXTRA_CHECK="range_block"
            POSITION_MODE="far"
            ;;
        no_limit_far_pass)
            SHORE_PATCH="$HARNESS_DIR/no-limit-far-pass-shoreside.xmoos"
            EXTRA_CHECK="reports"
            POSITION_MODE="far"
            ;;
        max_msg_length_block_pass)
            SHORE_PATCH="$HARNESS_DIR/max-msg-length-block-pass-shoreside.xmoos"
            EXTRA_CHECK="direct_block_ack_pass"
            ;;
        dest_specific_message_pass)
            EXTRA_CHECK="dest_specific_message"
            MESSAGE_MODE="dest_specific"
            ;;
        dest_group_message_pass)
            EXTRA_CHECK="dest_group_message"
            MESSAGE_MODE="dest_group"
            ;;
        ack_requested_mediated_pass)
            SHORE_PATCH="$HARNESS_DIR/ack-requested-mediated-pass-shoreside.xmoos"
            EXTRA_CHECK="ack_requested_mediated"
            MESSAGE_MODE="ack_requested"
            ;;
        double_val_message_pass)
            EXTRA_CHECK="double_val_message"
            MESSAGE_MODE="double_val"
            ;;
        unknown_dest_block_pass)
            SHORE_PATCH="$HARNESS_DIR/unknown-dest-block-pass-shoreside.xmoos"
            EXTRA_CHECK="unknown_dest_block"
            MESSAGE_MODE="unknown_dest"
            ;;
        ghost_source_block_pass)
            SHORE_PATCH="$HARNESS_DIR/ghost-source-block-pass-shoreside.xmoos"
            EXTRA_CHECK="ghost_source_block"
            MESSAGE_MODE="ghost_source"
            ;;
        invalid_ack_block_pass)
            SHORE_PATCH="$HARNESS_DIR/invalid-ack-block-pass-shoreside.xmoos"
            EXTRA_CHECK="invalid_ack_block"
            ACK_MODE="invalid"
            ;;
        msg_group_filter_pass)
            SHORE_PATCH="$HARNESS_DIR/msg-group-filter-pass-shoreside.xmoos"
            EXTRA_CHECK="direct_block_ack_pass"
            GROUP_MODE="split"
            ;;
        report_group_filter_pass)
            SHORE_PATCH="$HARNESS_DIR/report-group-filter-pass-shoreside.xmoos"
            EXTRA_CHECK="report_block_msg_pass"
            GROUP_MODE="split"
            ;;
        stealth_range_block_pass)
            SHORE_PATCH="$HARNESS_DIR/stealth-range-block-pass-shoreside.xmoos"
            EXTRA_CHECK="stealth_asym_block"
            ;;
        earange_extend_pass)
            SHORE_PATCH="$HARNESS_DIR/earange-extend-pass-shoreside.xmoos"
            EXTRA_CHECK="earange_asym_extend"
            ;;
        stale_message_block_pass)
            SHORE_PATCH="$HARNESS_DIR/stale-message-block-pass-shoreside.xmoos"
            EXTRA_CHECK="stale_message_block"
            REPORT_MODE="stale"
            ;;
        min_msg_interval_filter_pass)
            SHORE_PATCH="$HARNESS_DIR/min-msg-interval-filter-pass-shoreside.xmoos"
            EXTRA_CHECK="min_msg_interval_filter"
            MESSAGE_MODE="double"
            ;;
        runtime_range_extend_pass)
            SHORE_PATCH="$HARNESS_DIR/runtime-range-extend-pass-shoreside.xmoos"
            EXTRA_CHECK="reports"
            ;;
        runtime_stealth_block_pass)
            SHORE_PATCH="$HARNESS_DIR/runtime-stealth-block-pass-shoreside.xmoos"
            EXTRA_CHECK="runtime_stealth_block"
            REPORT_MODE="late"
            MESSAGE_MODE="early_late"
            ;;
        runtime_earange_extend_pass)
            SHORE_PATCH="$HARNESS_DIR/runtime-earange-extend-pass-shoreside.xmoos"
            EXTRA_CHECK="runtime_earange_extend"
            REPORT_MODE="late"
            MESSAGE_MODE="early_late"
            ;;
        runtime_drop_all_pass)
            SHORE_PATCH="$HARNESS_DIR/runtime-drop-all-pass-shoreside.xmoos"
            EXTRA_CHECK="runtime_drop_all"
            REPORT_MODE="late"
            MESSAGE_MODE="early_late"
            ;;
        runtime_pulse_disabled_pass)
            SHORE_PATCH="$HARNESS_DIR/runtime-pulse-disabled-pass-shoreside.xmoos"
            EXTRA_CHECK="runtime_pulse_disabled"
            REPORT_MODE="late"
            ;;
        runtime_shared_reports_pass)
            SHORE_PATCH="$HARNESS_DIR/runtime-shared-reports-pass-shoreside.xmoos"
            EXTRA_CHECK="shared"
            ;;
        shared_node_reports_numeric_pass)
            SHORE_PATCH="$HARNESS_DIR/shared-node-reports-numeric-pass-shoreside.xmoos"
            EXTRA_CHECK="shared"
            ;;
        min_rpt_interval_filter_pass)
            SHORE_PATCH="$HARNESS_DIR/min-rpt-interval-filter-pass-shoreside.xmoos"
            EXTRA_CHECK="min_rpt_interval_filter"
            ;;
        stale_time_unlimited_pass)
            SHORE_PATCH="$HARNESS_DIR/stale-time-unlimited-pass-shoreside.xmoos"
            EXTRA_CHECK="stale_time_unlimited"
            REPORT_MODE="stale"
            ;;
        max_msg_length_unlimited_pass)
            SHORE_PATCH="$HARNESS_DIR/max-msg-length-unlimited-pass-shoreside.xmoos"
            EXTRA_CHECK="max_msg_length_unlimited"
            MESSAGE_MODE="long_string"
            ;;
        earrange_alias_extend_pass)
            SHORE_PATCH="$HARNESS_DIR/earrange-alias-extend-pass-shoreside.xmoos"
            EXTRA_CHECK="earange_asym_extend"
            ;;
        critical_group_override_pass)
            SHORE_PATCH="$HARNESS_DIR/critical-group-override-pass-shoreside.xmoos"
            EXTRA_CHECK="critical_group_override"
            GROUP_MODE="split"
            ;;
        *)
            echo "$ME: Unknown case: $CASE_NAME"
            return 1
            ;;
    esac
}

wait_for_result_line() {
    local file="$1"
    local tries=240
    local line
    local i=0
    while [ "$i" -lt "$tries" ]; do
        line=`tail -n 1 "$file" 2>/dev/null`
        if echo "$line" | grep -q "grade="; then
            echo "$line"
            return 0
        fi
        sleep 0.5
        i=$((i + 1))
    done
    echo ""
    return 1
}

find_shore_alog() {
    find "$1" -path '*XLOG_SHORESIDE*/*.alog' -print | sort | tail -n 1
}

alog_has_var() {
    local case_dir="$1"
    local var="$2"
    local alog
    alog=`find_shore_alog "$case_dir"`
    [ "$alog" != "" ] || return 1
    aloggrep "$alog" "$var" 2>/dev/null | rg -q "[[:space:]]$var[[:space:]]"
}

alog_var_has_pattern() {
    local case_dir="$1"
    local var="$2"
    local pattern="$3"
    local alog
    alog=`find_shore_alog "$case_dir"`
    [ "$alog" != "" ] || return 1
    aloggrep "$alog" "$var" 2>/dev/null | rg "$pattern" >/dev/null
}

alog_var_first_time() {
    local case_dir="$1"
    local var="$2"
    local alog
    alog=`find_shore_alog "$case_dir"`
    [ "$alog" != "" ] || return 1
    aloggrep "$alog" "$var" 2>/dev/null | awk -v v="$var" '$2 == v {print $1; exit}'
}

alog_var_count_after() {
    local case_dir="$1"
    local var="$2"
    local after="$3"
    local alog
    alog=`find_shore_alog "$case_dir"`
    [ "$alog" != "" ] || return 1
    aloggrep "$alog" "$var" 2>/dev/null | awk -v v="$var" -v a="$after" '$2 == v && ($1 + 0) > (a + 0) {c++} END {print c + 0}'
}

alog_var_count_after_pattern() {
    local case_dir="$1"
    local var="$2"
    local after="$3"
    local pattern="$4"
    local alog
    alog=`find_shore_alog "$case_dir"`
    [ "$alog" != "" ] || return 1
    aloggrep "$alog" "$var" 2>/dev/null | awk -v v="$var" -v a="$after" -v pat="$pattern" '$2 == v && ($1 + 0) > (a + 0) && $0 ~ pat {c++} END {print c + 0}'
}

alog_var_count() {
    local case_dir="$1"
    local var="$2"
    local alog
    alog=`find_shore_alog "$case_dir"`
    [ "$alog" != "" ] || return 1
    aloggrep "$alog" "$var" 2>/dev/null | awk -v v="$var" '$2 == v {c++} END {print c + 0}'
}

extra_check_ok() {
    local check="$1"
    local case_dir="$2"
    local line="$3"
    case "$check" in
        baseline|reports)
            echo "$line" | rg -q 'node_count=2' && \
            echo "$line" | rg -q 'rpt_ab=true' && \
            echo "$line" | rg -q 'rpt_ba=true' && \
            echo "$line" | rg -q 'pulse=true'
            ;;
        shared)
            echo "$line" | rg -q 'shared=true' && alog_has_var "$case_dir" "NODE_REPORT_UNC"
            ;;
        no_pulse)
            echo "$line" | rg -q 'pulse=false' && ! alog_has_var "$case_dir" "VIEW_COMMS_PULSE"
            ;;
        range_block)
            echo "$line" | rg -q 'rpt_ab=false' && \
            echo "$line" | rg -q 'rpt_ba=false' && \
            echo "$line" | rg -q 'direct=false' && \
            echo "$line" | rg -q 'ack=false' && \
            echo "$line" | rg -q 'pulse=false'
            ;;
        direct_block_ack_pass)
            echo "$line" | rg -q 'direct=false' && \
            echo "$line" | rg -q 'ack=true' && \
            echo "$line" | rg -q 'rpt_ab=true' && \
            echo "$line" | rg -q 'rpt_ba=true'
            ;;
        dest_specific_message)
            echo "$line" | rg -q 'direct=true' && \
            alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "dest_node=ben" && \
            alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_DEST"
            ;;
        dest_group_message)
            echo "$line" | rg -q 'direct=true' && \
            alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "dest_group=red" && \
            alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_GROUP"
            ;;
        ack_requested_mediated)
            echo "$line" | rg -q 'mediated=true' && \
            echo "$line" | rg -q 'ack=true' && \
            alog_var_has_pattern "$case_dir" "MEDIATED_MESSAGE_BEN" "ack=true" && \
            alog_var_has_pattern "$case_dir" "MEDIATED_MESSAGE_BEN" "UFC_ACKREQ" && \
            ! alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_ACKREQ"
            ;;
        double_val_message)
            echo "$line" | rg -q 'direct=true' && \
            alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_DOUBLE" && \
            alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "double_val=3.1415"
            ;;
        unknown_dest_block)
            echo "$line" | rg -q 'direct=false' && \
            echo "$line" | rg -q 'ack=true' && \
            ! alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_UNKNOWN"
            ;;
        ghost_source_block)
            echo "$line" | rg -q 'direct=false' && \
            echo "$line" | rg -q 'ack=true' && \
            ! alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_GHOST"
            ;;
        invalid_ack_block)
            echo "$line" | rg -q 'direct=true' && \
            echo "$line" | rg -q 'ack=false' && \
            ! alog_var_has_pattern "$case_dir" "ACK_MESSAGE_abe" "src=ben"
            ;;
        report_block_msg_pass)
            echo "$line" | rg -q 'direct=true' && \
            echo "$line" | rg -q 'ack=true' && \
            echo "$line" | rg -q 'rpt_ab=false' && \
            echo "$line" | rg -q 'rpt_ba=false'
            ;;
        stealth_asym_block)
            echo "$line" | rg -q 'direct=false' && \
            echo "$line" | rg -q 'ack=true' && \
            echo "$line" | rg -q 'rpt_ab=false' && \
            echo "$line" | rg -q 'rpt_ba=true'
            ;;
        earange_asym_extend)
            echo "$line" | rg -q 'direct=true' && \
            echo "$line" | rg -q 'ack=false' && \
            echo "$line" | rg -q 'rpt_ab=true' && \
            echo "$line" | rg -q 'rpt_ba=false'
            ;;
        direct_ack_block_reports_pass)
            echo "$line" | rg -q 'direct=false' && \
            echo "$line" | rg -q 'ack=false' && \
            echo "$line" | rg -q 'rpt_ab=true' && \
            echo "$line" | rg -q 'rpt_ba=true' && \
            echo "$line" | rg -q 'pulse=true'
            ;;
        stale_message_block)
            echo "$line" | rg -q 'direct=false' && \
            echo "$line" | rg -q 'ack=false' && \
            echo "$line" | rg -q 'rpt_ab=true' && \
            echo "$line" | rg -q 'rpt_ba=true' && \
            echo "$line" | rg -q 'pulse=true' && \
            alog_var_has_pattern "$case_dir" "NODE_REPORT_BEN" "TIME=1"
            ;;
        min_msg_interval_filter)
            echo "$line" | rg -q 'direct=true' && \
            echo "$line" | rg -q 'ack=true' && \
            echo "$line" | rg -q 'rpt_ab=true' && \
            echo "$line" | rg -q 'rpt_ba=true' && \
            alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_DIRECT" && \
            ! alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_SECOND"
            ;;
        runtime_stealth_block)
            echo "$line" | rg -q 'direct=true' && \
            echo "$line" | rg -q 'ack=true' && \
            echo "$line" | rg -q 'rpt_ab=true' && \
            echo "$line" | rg -q 'rpt_ba=true' && \
            alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_BEFORE" && \
            ! alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_AFTER" && \
            alog_var_has_pattern "$case_dir" "ACK_MESSAGE_abe" "id=after_1"
            ;;
        runtime_earange_extend)
            echo "$line" | rg -q 'direct=true' && \
            echo "$line" | rg -q 'ack=false' && \
            echo "$line" | rg -q 'rpt_ab=true' && \
            echo "$line" | rg -q 'rpt_ba=false' && \
            ! alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_BEFORE" && \
            alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_AFTER"
            ;;
        runtime_drop_all)
            echo "$line" | rg -q 'direct=true' && \
            echo "$line" | rg -q 'ack=true' && \
            echo "$line" | rg -q 'rpt_ab=true' && \
            echo "$line" | rg -q 'rpt_ba=true' && \
            alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_BEFORE" && \
            ! alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_AFTER" && \
            alog_var_has_pattern "$case_dir" "ACK_MESSAGE_abe" "id=before_1" && \
            ! alog_var_has_pattern "$case_dir" "ACK_MESSAGE_abe" "id=after_1"
            ;;
        runtime_pulse_disabled)
            local t
            local after_grace
            t=`alog_var_first_time "$case_dir" "UNC_VIEW_NODE_RPT_PULSES"`
            after_grace=`awk -v t="$t" 'BEGIN {printf "%.6f", t + 2.0}'`
            [ "$t" != "" ] && \
            echo "$line" | rg -q 'direct=true' && \
            echo "$line" | rg -q 'ack=true' && \
            echo "$line" | rg -q 'rpt_ab=true' && \
            echo "$line" | rg -q 'rpt_ba=true' && \
            [ "`alog_var_count_after_pattern "$case_dir" "VIEW_COMMS_PULSE" "$after_grace" "ptype=nrep"`" = "0" ]
            ;;
        min_rpt_interval_filter)
            echo "$line" | rg -q 'direct=true' && \
            echo "$line" | rg -q 'ack=true' && \
            echo "$line" | rg -q 'rpt_ab=true' && \
            echo "$line" | rg -q 'rpt_ba=true' && \
            [ "`alog_var_count "$case_dir" "NODE_REPORT_BEN"`" -le 2 ] && \
            [ "`alog_var_count "$case_dir" "NODE_REPORT_ABE"`" -le 2 ]
            ;;
        stale_time_unlimited)
            echo "$line" | rg -q 'direct=true' && \
            echo "$line" | rg -q 'ack=true' && \
            echo "$line" | rg -q 'rpt_ab=true' && \
            echo "$line" | rg -q 'rpt_ba=true' && \
            alog_var_has_pattern "$case_dir" "NODE_REPORT_BEN" "TIME=1"
            ;;
        max_msg_length_unlimited)
            echo "$line" | rg -q 'direct=true' && \
            echo "$line" | rg -q 'ack=true' && \
            alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "UFC_LONG" && \
            alog_var_has_pattern "$case_dir" "NODE_MESSAGE_BEN" "long_payload_segment_05"
            ;;
        critical_group_override)
            echo "$line" | rg -q 'direct=false' && \
            echo "$line" | rg -q 'ack=false' && \
            echo "$line" | rg -q 'rpt_ab=true' && \
            echo "$line" | rg -q 'rpt_ba=true' && \
            echo "$line" | rg -q 'pulse=true'
            ;;
        *)
            return 0
            ;;
    esac
}

prepare_case_dir() {
    local case_dir="$1"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    (cd "$case_dir" && ./clean.sh >/dev/null 2>&1 || true)
    if [ "$POSITION_MODE" = "far" ]; then
        perl -0pi -e 's/x=60,y=0,heading=270/x=600,y=0,heading=270/' "$case_dir/init_field.sh"
    fi
    if [ "$GROUP_MODE" = "split" ]; then
        perl -0pi -e 's/echo "red"(\s*)>> vgroups\.txt/echo "blue"$1>> vgroups.txt/' "$case_dir/init_field.sh"
    fi
    if [ "$REPORT_MODE" = "stale" ]; then
        perl -0pi -e 's/TIME=\$\[UTC\]/TIME=1/g' "$case_dir/meta_vehicle.moos"
    fi
    if [ "$REPORT_MODE" = "late" ]; then
        perl -0pi -e 's/(#ifdef VNAME abe)/  event = var=NODE_REPORT_LOCAL, val="NAME=\$(VNAME),TYPE=KAYAK,TIME=\$[UTC],X=\$(XPOS),Y=\$(YPOS),LAT=43.825300,LON=-70.330400,SPD=0,HDG=\$(HDG),YAW=\$(HDG),DEPTH=0,LENGTH=4,GROUP=\$(VGROUP),MODE=MODE@ACTIVE:COMMS", time=1500\n  event = var=NODE_REPORT_LOCAL, val="NAME=\$(VNAME),TYPE=KAYAK,TIME=\$[UTC],X=\$(XPOS),Y=\$(YPOS),LAT=43.825300,LON=-70.330400,SPD=0,HDG=\$(HDG),YAW=\$(HDG),DEPTH=0,LENGTH=4,GROUP=\$(VGROUP),MODE=MODE@ACTIVE:COMMS", time=1540\n\n$1/' "$case_dir/meta_vehicle.moos"
    fi
    if [ "$MESSAGE_MODE" = "double" ]; then
        perl -0pi -e 's/(event = var=NODE_MESSAGE_LOCAL, val="src_node=abe,dest_node=all,var_name=UFC_DIRECT,string_val=hello_ben,ack_id=direct_1", time=1040)/$1\n  event = var=NODE_MESSAGE_LOCAL, val="src_node=abe,dest_node=all,var_name=UFC_SECOND,string_val=second_fast,ack_id=direct_2", time=1045/' "$case_dir/meta_vehicle.moos"
    fi
    if [ "$MESSAGE_MODE" = "dest_specific" ]; then
        perl -0pi -e 's/src_node=abe,dest_node=all,var_name=UFC_DIRECT,string_val=hello_ben,ack_id=direct_1/src_node=abe,dest_node=ben,var_name=UFC_DEST,string_val=hello_ben,ack_id=direct_1/' "$case_dir/meta_vehicle.moos"
    fi
    if [ "$MESSAGE_MODE" = "dest_group" ]; then
        perl -0pi -e 's/src_node=abe,dest_node=all,var_name=UFC_DIRECT,string_val=hello_ben,ack_id=direct_1/src_node=abe,dest_group=red,var_name=UFC_GROUP,string_val=hello_group,ack_id=direct_1/' "$case_dir/meta_vehicle.moos"
    fi
    if [ "$MESSAGE_MODE" = "ack_requested" ]; then
        perl -0pi -e 's/src_node=abe,dest_node=all,var_name=UFC_DIRECT,string_val=hello_ben,ack_id=direct_1/src_node=abe,dest_node=ben,var_name=UFC_ACKREQ,string_val=needs_ack,ack_id=ack_req_1,ack=true/' "$case_dir/meta_vehicle.moos"
    fi
    if [ "$MESSAGE_MODE" = "double_val" ]; then
        perl -0pi -e 's/src_node=abe,dest_node=all,var_name=UFC_DIRECT,string_val=hello_ben,ack_id=direct_1/src_node=abe,dest_node=ben,var_name=UFC_DOUBLE,double_val=3.1415,ack_id=direct_1/' "$case_dir/meta_vehicle.moos"
    fi
    if [ "$MESSAGE_MODE" = "long_string" ]; then
        perl -0pi -e 's/src_node=abe,dest_node=all,var_name=UFC_DIRECT,string_val=hello_ben,ack_id=direct_1/src_node=abe,dest_node=ben,var_name=UFC_LONG,string_val=long_payload_segment_01_long_payload_segment_02_long_payload_segment_03_long_payload_segment_04_long_payload_segment_05,ack_id=direct_1/' "$case_dir/meta_vehicle.moos"
    fi
    if [ "$MESSAGE_MODE" = "unknown_dest" ]; then
        perl -0pi -e 's/src_node=abe,dest_node=all,var_name=UFC_DIRECT,string_val=hello_ben,ack_id=direct_1/src_node=abe,dest_node=zed,var_name=UFC_UNKNOWN,string_val=lost,ack_id=direct_1/' "$case_dir/meta_vehicle.moos"
    fi
    if [ "$MESSAGE_MODE" = "ghost_source" ]; then
        perl -0pi -e 's/src_node=abe,dest_node=all,var_name=UFC_DIRECT,string_val=hello_ben,ack_id=direct_1/src_node=ghost,dest_node=ben,var_name=UFC_GHOST,string_val=lost,ack_id=direct_1/' "$case_dir/meta_vehicle.moos"
    fi
    if [ "$MESSAGE_MODE" = "early_late" ]; then
        perl -0pi -e 's/event = var=NODE_MESSAGE_LOCAL, val="src_node=abe,dest_node=all,var_name=UFC_DIRECT,string_val=hello_ben,ack_id=direct_1", time=1040/event = var=NODE_MESSAGE_LOCAL, val="src_node=abe,dest_node=all,var_name=UFC_BEFORE,string_val=before_runtime,ack_id=before_1", time=650\n  event = var=NODE_MESSAGE_LOCAL, val="src_node=abe,dest_node=all,var_name=UFC_AFTER,string_val=after_runtime,ack_id=after_1", time=1040/' "$case_dir/meta_vehicle.moos"
        perl -0pi -e 's/event = var=ACK_MESSAGE_LOCAL, val="src=ben,dest=abe,id=direct_1", time=1120/event = var=ACK_MESSAGE_LOCAL, val="src=ben,dest=abe,id=before_1", time=760\n  event = var=ACK_MESSAGE_LOCAL, val="src=ben,dest=abe,id=after_1", time=1120/' "$case_dir/meta_vehicle.moos"
    fi
    if [ "$ACK_MODE" = "invalid" ]; then
        perl -0pi -e 's/src=ben,dest=abe,id=direct_1/src=ben,dest=abe/' "$case_dir/meta_vehicle.moos"
    fi
    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$case_dir/meta_shoreside.moos" "$SHORE_PATCH" --targ="$case_dir/meta_shoreside.moosx"
    fi
}

run_case() {
    local case_name="$1"
    local case_index="$2"
    get_case_config "$case_name" || return 1

    local case_dir="$RUN_ROOT/$case_name"
    local case_rows="$case_dir/results.txt"
    local pbase=$((PORT_BASE + case_index * PORT_STRIDE))
    mkdir -p "$case_dir"
    prepare_case_dir "$case_dir"
    : > "$case_rows"

    (
        cd "$case_dir"
        xlaunch.sh --max_time="$MAX_TIME" $NOGUI \
            --mmod="$case_name" \
            --shore_mport="$pbase" \
            --veh1_mport="$((pbase+1))" \
            --veh2_mport="$((pbase+2))" \
            --shore_pshare="$((pbase+10))" \
            --veh1_pshare="$((pbase+11))" \
            --veh2_pshare="$((pbase+12))" \
            "$TIME_WARP" >"$case_dir/xlaunch.out" 2>&1
    )
    harness_teardown_stop_root "$case_dir" >/dev/null 2>&1 || true

    local line
    line=`wait_for_result_line "$case_rows"`
    if [ "$line" = "" ]; then
        echo "$case_name: no-result"
        return 1
    fi

    echo "$case_name $line" >> "$RESULTS_FILE"
    if ! echo "$line" | rg -q "grade=$EXPECTED"; then
        echo "$case_name: expected $EXPECTED but got: $line"
        return 1
    fi
    if ! extra_check_ok "$EXTRA_CHECK" "$case_dir" "$line"; then
        echo "$case_name: extra check failed: $line"
        return 1
    fi
    echo "$case_name: ok"
}

if [ "$CASE" != "" ]; then
    CASES="$CASE"
else
    CASES=`case_list`
fi

RUN_ROOT=`mktemp -d "${TMPDIR:-/tmp}/ufield_comms_harness.XXXXXX"`
: > "$RESULTS_FILE"

INDEX=0
PIDS=""
for C in $CASES; do
    run_case "$C" "$INDEX" &
    PIDS="$PIDS $!"
    INDEX=$((INDEX + 1))
    if [ "$JUST_MAKE" = "yes" ]; then
        wait
    elif [ $((INDEX % JOBS)) -eq 0 ]; then
        for P in $PIDS; do
            wait "$P" || ALL_OK="no"
        done
        PIDS=""
    fi
done

for P in $PIDS; do
    wait "$P" || ALL_OK="no"
done

[ "$ALL_OK" = "yes" ]
