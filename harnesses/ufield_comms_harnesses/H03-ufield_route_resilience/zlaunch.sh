#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H03-ufield_route_resilience
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
dead_first_tryhost_blocks_fail
secondary_dead_tryhost_ignored_pass
startup_invalid_bad_port_valid_route_pass
startup_invalid_bad_param_valid_route_pass
startup_ip_route_recover_pass
runtime_tryhost_recover_pass
runtime_dead_then_valid_blocks_fail
runtime_valid_then_dead_stays_pass
runtime_duplicate_tryhost_ignored_pass
startup_duplicate_runtime_ignored_pass
delayed_runtime_tryhost_recover_pass
late_runtime_tryhost_blocks_delivery_fail
runtime_one_node_missing_route_fail
runtime_staggered_nodes_recover_pass
invalid_runtime_tryhost_recover_pass
invalid_runtime_bad_port_recover_pass
invalid_runtime_bad_param_recover_pass
runtime_ip_route_recover_pass
shore_vnode_discovery_recover_pass
shore_vnode_one_node_only_fail
shore_vnode_extra_dead_ignored_pass
shore_vnode_default_port_no_listener_fail
shore_vnode_invalid_host_ignored_pass
shore_vnode_bad_port_ignored_pass
shore_vnode_duplicate_ignored_pass
)

case_list() {
    printf "%s\n" "${ALL_CASES[@]}"
}

get_case_config() {
    CASE_NAME="$1"
    ROUTE_MODE="stock"
    EXTRA_CHECK=""
    NEGATIVE_PROFILE=""

    case "$CASE_NAME" in
        dead_first_tryhost_blocks_fail)
            ROUTE_MODE="dead_first_tryhost"
            EXTRA_CHECK="dead_first_tryhost_blocks"
            NEGATIVE_PROFILE="blocked_no_nodes"
            ;;
        secondary_dead_tryhost_ignored_pass)
            ROUTE_MODE="secondary_dead_tryhost"
            EXTRA_CHECK="secondary_dead_tryhost_ignored"
            ;;
        startup_invalid_bad_port_valid_route_pass)
            ROUTE_MODE="startup_invalid_bad_port"
            EXTRA_CHECK="startup_invalid_bad_port_valid_route"
            ;;
        startup_invalid_bad_param_valid_route_pass)
            ROUTE_MODE="startup_invalid_bad_param"
            EXTRA_CHECK="startup_invalid_bad_param_valid_route"
            ;;
        startup_ip_route_recover_pass)
            ROUTE_MODE="startup_ip_route"
            EXTRA_CHECK="startup_ip_route_recover"
            ;;
        runtime_tryhost_recover_pass)
            ROUTE_MODE="runtime_tryhost"
            EXTRA_CHECK="runtime_tryhost_recover"
            ;;
        runtime_dead_then_valid_blocks_fail)
            ROUTE_MODE="runtime_dead_then_valid"
            EXTRA_CHECK="runtime_dead_then_valid_blocks"
            NEGATIVE_PROFILE="blocked_no_nodes"
            ;;
        runtime_valid_then_dead_stays_pass)
            ROUTE_MODE="runtime_valid_then_dead"
            EXTRA_CHECK="runtime_valid_then_dead_stays"
            ;;
        runtime_duplicate_tryhost_ignored_pass)
            ROUTE_MODE="runtime_duplicate_tryhost"
            EXTRA_CHECK="runtime_duplicate_tryhost_ignored"
            ;;
        startup_duplicate_runtime_ignored_pass)
            ROUTE_MODE="startup_duplicate_runtime"
            EXTRA_CHECK="startup_duplicate_runtime_ignored"
            ;;
        delayed_runtime_tryhost_recover_pass)
            ROUTE_MODE="delayed_runtime_tryhost"
            EXTRA_CHECK="delayed_runtime_tryhost_recover"
            ;;
        late_runtime_tryhost_blocks_delivery_fail)
            ROUTE_MODE="late_runtime_tryhost"
            EXTRA_CHECK="late_runtime_tryhost_blocks_delivery"
            NEGATIVE_PROFILE="late_blocks_delivery"
            ;;
        runtime_one_node_missing_route_fail)
            ROUTE_MODE="runtime_one_node_missing_route"
            EXTRA_CHECK="runtime_one_node_missing_route"
            NEGATIVE_PROFILE="one_node_only"
            ;;
        runtime_staggered_nodes_recover_pass)
            ROUTE_MODE="runtime_staggered_nodes"
            EXTRA_CHECK="runtime_staggered_nodes_recover"
            ;;
        invalid_runtime_tryhost_recover_pass)
            ROUTE_MODE="invalid_runtime_tryhost"
            EXTRA_CHECK="invalid_runtime_tryhost_recover"
            ;;
        invalid_runtime_bad_port_recover_pass)
            ROUTE_MODE="invalid_runtime_bad_port"
            EXTRA_CHECK="invalid_runtime_bad_port_recover"
            ;;
        invalid_runtime_bad_param_recover_pass)
            ROUTE_MODE="invalid_runtime_bad_param"
            EXTRA_CHECK="invalid_runtime_bad_param_recover"
            ;;
        runtime_ip_route_recover_pass)
            ROUTE_MODE="runtime_ip_route"
            EXTRA_CHECK="runtime_ip_route_recover"
            ;;
        shore_vnode_discovery_recover_pass)
            ROUTE_MODE="shore_vnode_discovery"
            EXTRA_CHECK="shore_vnode_discovery_recover"
            ;;
        shore_vnode_one_node_only_fail)
            ROUTE_MODE="shore_vnode_one_node_only"
            EXTRA_CHECK="shore_vnode_one_node_only"
            NEGATIVE_PROFILE="one_node_only"
            ;;
        shore_vnode_extra_dead_ignored_pass)
            ROUTE_MODE="shore_vnode_extra_dead"
            EXTRA_CHECK="shore_vnode_extra_dead_ignored"
            ;;
        shore_vnode_default_port_no_listener_fail)
            ROUTE_MODE="shore_vnode_default_port"
            EXTRA_CHECK="shore_vnode_default_port_no_listener"
            NEGATIVE_PROFILE="blocked_no_nodes"
            ;;
        shore_vnode_invalid_host_ignored_pass)
            ROUTE_MODE="shore_vnode_invalid_host"
            EXTRA_CHECK="shore_vnode_invalid_host_ignored"
            ;;
        shore_vnode_bad_port_ignored_pass)
            ROUTE_MODE="shore_vnode_bad_port"
            EXTRA_CHECK="shore_vnode_bad_port_ignored"
            ;;
        shore_vnode_duplicate_ignored_pass)
            ROUTE_MODE="shore_vnode_duplicate"
            EXTRA_CHECK="shore_vnode_duplicate_ignored"
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

grade_from_line() {
    echo "$1" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'
}

format_result_row() {
    local case_name="$1"
    local line="$2"
    local launch_rc="${3:-0}"
    local grade

    if [ "$launch_rc" != 0 ]; then
        echo "case=$case_name  grade=fail  reason=launch_error  launch_rc=$launch_rc"
        return
    fi

    line=$(echo "$line" | sed 's/^[[:space:]]*//')
    grade=$(grade_from_line "$line")
    if [ "$grade" = "" ]; then
        echo "case=$case_name  grade=fail  reason=missing_result"
        return
    fi

    line=$(echo "$line" | sed 's/grade=[^, ]*[[:space:]]*//')


    echo "case=$case_name  grade=$grade  $line"
}

result_row_passed() {
    local line="$1"
    local grade
    grade=$(grade_from_line "$line")
    [ "$grade" = "pass" ]
}

shore_alog() {
    find "$1" -path '*XLOG_SHORESIDE*/*.alog' -print | sort | tail -n 1
}

vehicle_alog() {
    local case_dir="$1"
    local vname="$2"
    find "$case_dir" -path "*LOG_${vname}*/*.alog" -print | sort | tail -n 1
}

alog_var_has_pattern() {
    local alog="$1"
    local var="$2"
    local pattern="$3"
    [ "$alog" != "" ] || return 1
    aloggrep "$alog" "$var" 2>/dev/null | rg "$pattern" >/dev/null
}

alog_var_lacks_pattern() {
    local alog="$1"
    local var="$2"
    local pattern="$3"
    local lines
    [ "$alog" != "" ] || return 1
    lines=`aloggrep "$alog" "$var" 2>/dev/null`
    [ "$lines" != "" ] || return 1
    ! printf "%s\n" "$lines" | rg "$pattern" >/dev/null
}

vehicle_has() {
    local case_dir="$1"
    local vname="$2"
    local var="$3"
    local pattern="$4"
    local alog
    alog=`vehicle_alog "$case_dir" "$vname"`
    alog_var_has_pattern "$alog" "$var" "$pattern"
}

vehicle_lacks() {
    local case_dir="$1"
    local vname="$2"
    local var="$3"
    local pattern="$4"
    local alog
    alog=`vehicle_alog "$case_dir" "$vname"`
    alog_var_lacks_pattern "$alog" "$var" "$pattern"
}

shore_has() {
    local case_dir="$1"
    local var="$2"
    local pattern="$3"
    local alog
    alog=`shore_alog "$case_dir"`
    alog_var_has_pattern "$alog" "$var" "$pattern"
}

shore_lacks() {
    local case_dir="$1"
    local var="$2"
    local pattern="$3"
    local alog
    alog=`shore_alog "$case_dir"`
    alog_var_lacks_pattern "$alog" "$var" "$pattern"
}

base_delivery_ok() {
    local line="$1"
    echo "$line" | rg -q 'node_count=2' && \
    echo "$line" | rg -q 'direct=true' && \
    echo "$line" | rg -q 'ack=true' && \
    echo "$line" | rg -q 'rpt_ab=true' && \
    echo "$line" | rg -q 'rpt_ba=true'
}

extra_check_ok() {
    local check="$1"
    local case_dir="$2"
    local line="$3"

    case "$check" in
        dead_first_tryhost_blocks)
            echo "$line" | rg -q 'grade=pass' && \
            echo "$line" | rg -q 'expected=dead_first_tryhost_blocks' && \
            echo "$line" | rg -q 'node_count=0' && \
            echo "$line" | rg -q 'direct=false' && \
            echo "$line" | rg -q 'ack=false' && \
            vehicle_has "$case_dir" "ABE" "PSHARE_CMD" "src_name=NODE_BROKER_PING_0,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "PSHARE_CMD" "src_name=NODE_BROKER_PING_0,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "PSHARE_CMD" "src_name=NODE_BROKER_PING_1,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "PSHARE_CMD" "src_name=NODE_BROKER_PING_1,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            ! shore_has "$case_dir" "NODE_BROKER_PING" "key=1" && \
            ! vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok" && \
            ! vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok"
            ;;
        secondary_dead_tryhost_ignored)
            base_delivery_ok "$line" && \
            vehicle_has "$case_dir" "ABE" "PSHARE_CMD" "src_name=NODE_BROKER_PING_0,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "PSHARE_CMD" "src_name=NODE_BROKER_PING_0,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "PSHARE_CMD" "src_name=NODE_BROKER_PING_1,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "PSHARE_CMD" "src_name=NODE_BROKER_PING_1,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            shore_has "$case_dir" "NODE_BROKER_ACK_ABE" "status=ok,key=0" && \
            shore_has "$case_dir" "NODE_BROKER_ACK_BEN" "status=ok,key=0" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        startup_invalid_bad_port_valid_route)
            base_delivery_ok "$line" && \
            vehicle_lacks "$case_dir" "ABE" "PSHARE_CMD" "notaport" && \
            vehicle_lacks "$case_dir" "BEN" "PSHARE_CMD" "notaport" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        startup_invalid_bad_param_valid_route)
            base_delivery_ok "$line" && \
            vehicle_lacks "$case_dir" "ABE" "PSHARE_CMD" "shore_route" && \
            vehicle_lacks "$case_dir" "BEN" "PSHARE_CMD" "shore_route" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        startup_ip_route_recover)
            base_delivery_ok "$line" && \
            vehicle_has "$case_dir" "ABE" "PSHARE_CMD" "route=127.0.0.1:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "PSHARE_CMD" "route=127.0.0.1:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        runtime_tryhost_recover)
            base_delivery_ok "$line" && \
            vehicle_has "$case_dir" "ABE" "TRY_SHORE_HOST" "pshare_route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "TRY_SHORE_HOST" "pshare_route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "PSHARE_CMD" "src_name=NODE_BROKER_PING_0,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "PSHARE_CMD" "src_name=NODE_BROKER_PING_0,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        runtime_dead_then_valid_blocks)
            echo "$line" | rg -q 'grade=pass' && \
            echo "$line" | rg -q 'expected=runtime_dead_then_valid_blocks' && \
            echo "$line" | rg -q 'node_count=0' && \
            echo "$line" | rg -q 'direct=false' && \
            echo "$line" | rg -q 'ack=false' && \
            vehicle_has "$case_dir" "ABE" "PSHARE_CMD" "src_name=NODE_BROKER_PING_0,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "PSHARE_CMD" "src_name=NODE_BROKER_PING_0,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "PSHARE_CMD" "src_name=NODE_BROKER_PING_1,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "PSHARE_CMD" "src_name=NODE_BROKER_PING_1,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            ! vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok" && \
            ! vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok"
            ;;
        runtime_valid_then_dead_stays)
            base_delivery_ok "$line" && \
            vehicle_has "$case_dir" "ABE" "PSHARE_CMD" "src_name=NODE_BROKER_PING_1,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "PSHARE_CMD" "src_name=NODE_BROKER_PING_1,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        runtime_duplicate_tryhost_ignored)
            base_delivery_ok "$line" && \
            vehicle_has "$case_dir" "ABE" "TRY_SHORE_HOST" "pshare_route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "TRY_SHORE_HOST" "pshare_route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "PSHARE_CMD" "src_name=NODE_BROKER_PING_0,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "PSHARE_CMD" "src_name=NODE_BROKER_PING_0,dest_name=NODE_BROKER_PING,route=localhost:[0-9]+" && \
            vehicle_lacks "$case_dir" "ABE" "PSHARE_CMD" "src_name=NODE_BROKER_PING_1" && \
            vehicle_lacks "$case_dir" "BEN" "PSHARE_CMD" "src_name=NODE_BROKER_PING_1" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        startup_duplicate_runtime_ignored)
            base_delivery_ok "$line" && \
            vehicle_has "$case_dir" "ABE" "TRY_SHORE_HOST" "pshare_route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "TRY_SHORE_HOST" "pshare_route=localhost:[0-9]+" && \
            vehicle_lacks "$case_dir" "ABE" "PSHARE_CMD" "src_name=NODE_BROKER_PING_1" && \
            vehicle_lacks "$case_dir" "BEN" "PSHARE_CMD" "src_name=NODE_BROKER_PING_1" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        delayed_runtime_tryhost_recover)
            base_delivery_ok "$line" && \
            vehicle_has "$case_dir" "ABE" "TRY_SHORE_HOST" "pshare_route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "TRY_SHORE_HOST" "pshare_route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        late_runtime_tryhost_blocks_delivery)
            echo "$line" | rg -q 'grade=pass' && \
            echo "$line" | rg -q 'expected=late_runtime_tryhost_blocks_delivery' && \
            echo "$line" | rg -q 'node_count=2' && \
            ! base_delivery_ok "$line" && \
            vehicle_has "$case_dir" "ABE" "TRY_SHORE_HOST" "pshare_route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "TRY_SHORE_HOST" "pshare_route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        runtime_one_node_missing_route)
            echo "$line" | rg -q 'grade=pass' && \
            echo "$line" | rg -q 'expected=runtime_one_node_missing_route' && \
            echo "$line" | rg -q 'node_count=1' && \
            vehicle_has "$case_dir" "ABE" "TRY_SHORE_HOST" "pshare_route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            ! vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok"
            ;;
        runtime_staggered_nodes_recover)
            base_delivery_ok "$line" && \
            vehicle_has "$case_dir" "ABE" "TRY_SHORE_HOST" "pshare_route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "TRY_SHORE_HOST" "pshare_route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        invalid_runtime_tryhost_recover)
            base_delivery_ok "$line" && \
            vehicle_has "$case_dir" "ABE" "TRY_SHORE_HOST" "pshare_route=not_a_route" && \
            vehicle_has "$case_dir" "BEN" "TRY_SHORE_HOST" "pshare_route=not_a_route" && \
            vehicle_lacks "$case_dir" "ABE" "PSHARE_CMD" "not_a_route" && \
            vehicle_lacks "$case_dir" "BEN" "PSHARE_CMD" "not_a_route" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        invalid_runtime_bad_port_recover)
            base_delivery_ok "$line" && \
            vehicle_has "$case_dir" "ABE" "TRY_SHORE_HOST" "pshare_route=localhost:notaport" && \
            vehicle_has "$case_dir" "BEN" "TRY_SHORE_HOST" "pshare_route=localhost:notaport" && \
            vehicle_lacks "$case_dir" "ABE" "PSHARE_CMD" "notaport" && \
            vehicle_lacks "$case_dir" "BEN" "PSHARE_CMD" "notaport" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        invalid_runtime_bad_param_recover)
            base_delivery_ok "$line" && \
            vehicle_has "$case_dir" "ABE" "TRY_SHORE_HOST" "shore_route=localhost:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "TRY_SHORE_HOST" "shore_route=localhost:[0-9]+" && \
            vehicle_lacks "$case_dir" "ABE" "PSHARE_CMD" "shore_route" && \
            vehicle_lacks "$case_dir" "BEN" "PSHARE_CMD" "shore_route" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        runtime_ip_route_recover)
            base_delivery_ok "$line" && \
            vehicle_has "$case_dir" "ABE" "TRY_SHORE_HOST" "pshare_route=127.0.0.1:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "TRY_SHORE_HOST" "pshare_route=127.0.0.1:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "PSHARE_CMD" "route=127.0.0.1:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "PSHARE_CMD" "route=127.0.0.1:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        shore_vnode_discovery_recover)
            base_delivery_ok "$line" && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=TRY_SHORE_HOST,dest_name=TRY_SHORE_HOST,route=127.0.0.1:[0-9]+" && \
            shore_has "$case_dir" "TRY_SHORE_HOST" "pshare_route=.*:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "TRY_SHORE_HOST" "pshare_route=.*:[0-9]+" && \
            vehicle_has "$case_dir" "BEN" "TRY_SHORE_HOST" "pshare_route=.*:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        shore_vnode_one_node_only)
            echo "$line" | rg -q 'grade=pass' && \
            echo "$line" | rg -q 'expected=shore_vnode_one_node_only' && \
            echo "$line" | rg -q 'node_count=1' && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=TRY_SHORE_HOST,dest_name=TRY_SHORE_HOST,route=127.0.0.1:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "TRY_SHORE_HOST" "pshare_route=.*:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            ! vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok"
            ;;
        shore_vnode_extra_dead_ignored)
            base_delivery_ok "$line" && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=TRY_SHORE_HOST,dest_name=TRY_SHORE_HOST,route=127.0.0.1:[0-9]+" && \
            shore_has "$case_dir" "PSHARE_CMD" "route=127.0.0.1:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        shore_vnode_default_port_no_listener)
            echo "$line" | rg -q 'grade=pass' && \
            echo "$line" | rg -q 'expected=shore_vnode_default_port_no_listener' && \
            echo "$line" | rg -q 'node_count=0' && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=TRY_SHORE_HOST,dest_name=TRY_SHORE_HOST,route=127.0.0.1:9200" && \
            ! vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok" && \
            ! vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok"
            ;;
        shore_vnode_invalid_host_ignored)
            base_delivery_ok "$line" && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=TRY_SHORE_HOST,dest_name=TRY_SHORE_HOST,route=127.0.0.1:[0-9]+" && \
            shore_lacks "$case_dir" "PSHARE_CMD" "badhost" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        shore_vnode_bad_port_ignored)
            base_delivery_ok "$line" && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=TRY_SHORE_HOST,dest_name=TRY_SHORE_HOST,route=127.0.0.1:[0-9]+" && \
            shore_lacks "$case_dir" "PSHARE_CMD" "notaport" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        shore_vnode_duplicate_ignored)
            base_delivery_ok "$line" && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=TRY_SHORE_HOST,dest_name=TRY_SHORE_HOST,route=127.0.0.1:[0-9]+" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0"
            ;;
        *)
            return 0
            ;;
    esac
}

remove_startup_tryhost() {
    local case_dir="$1"
    perl -0pi -e 's/\n  try_shore_host = pshare_route=localhost:\$\(SHORE_PSHARE\)\n/\n/' "$case_dir/meta_vehicle.moos"
}

insert_vehicle_event_before_messages() {
    local case_dir="$1"
    local event_text="$2"
    EVENT_TEXT="$event_text" perl -0pi -e 's/(#ifdef VNAME abe)/$ENV{EVENT_TEXT} . "\n\n" . $1/e' "$case_dir/meta_vehicle.moos"
}

insert_abe_event() {
    local case_dir="$1"
    local event_text="$2"
    EVENT_TEXT="$event_text" perl -0pi -e 's/(#ifdef VNAME abe\n)/$1 . $ENV{EVENT_TEXT} . "\n"/e' "$case_dir/meta_vehicle.moos"
}

insert_ben_event() {
    local case_dir="$1"
    local event_text="$2"
    EVENT_TEXT="$event_text" perl -0pi -e 's/(#ifdef VNAME ben\n)/$1 . $ENV{EVENT_TEXT} . "\n"/e' "$case_dir/meta_vehicle.moos"
}

patch_shore_broker_block() {
    local case_dir="$1"
    local block="$2"
    BLOCK="$block" perl -0pi -e 's/ProcessConfig = uFldShoreBroker\n\{\n.*?\n\}/$ENV{BLOCK}/s or die "failed to patch uFldShoreBroker block\n";' "$case_dir/meta_shoreside.moos"
}

patch_pmission_eval_block() {
    local case_dir="$1"
    local block="$2"
    BLOCK="$block" perl -0pi -e 's/ProcessConfig = pMissionEval\n\{\n.*?\n\}/$ENV{BLOCK}/s or die "failed to patch pMissionEval block\n";' "$case_dir/meta_shoreside.moos"
}

patch_negative_eval_block() {
    local case_dir="$1"
    local profile="$2"
    local expected="$3"
    local conditions

    if [ "$profile" = "blocked_no_nodes" ]; then
        conditions="  pass_condition = UFSB_NODE_COUNT = 0
  pass_condition = DIRECT_MSG_DELIVERED = false
  pass_condition = ACK_DELIVERED = false
  pass_condition = REPORT_ABE_TO_BEN = false
  pass_condition = REPORT_BEN_TO_ABE = false
  pass_condition = SHARED_REPORT_SEEN = false
  pass_condition = MISSION_TIMEOUT = false"
    elif [ "$profile" = "late_blocks_delivery" ]; then
        conditions="  pass_condition = UFSB_NODE_COUNT >= 2
  pass_condition = MISSION_TIMEOUT = false"
    elif [ "$profile" = "one_node_only" ]; then
        conditions="  pass_condition = UFSB_NODE_COUNT = 1
  pass_condition = DIRECT_MSG_DELIVERED = false
  pass_condition = MISSION_TIMEOUT = false"
    else
        return 0
    fi

    patch_pmission_eval_block "$case_dir" "ProcessConfig = pMissionEval
{
  AppTick   = 10
  CommsTick = 10

  mailflag = @NODE_MESSAGE_BEN#DIRECT_MSG_DELIVERED=true
  mailflag = @ACK_MESSAGE_abe#ACK_DELIVERED=true
  mailflag = @NODE_REPORT_BEN#REPORT_ABE_TO_BEN=true
  mailflag = @NODE_REPORT_ABE#REPORT_BEN_TO_ABE=true
  mailflag = @VIEW_COMMS_PULSE#VIEW_COMMS_PULSE_SEEN=true
  mailflag = @NODE_REPORT_UNC#SHARED_REPORT_SEEN=true

  lead_condition = MISSION_DONE = true
$conditions

  result_flag = MISSION_EVALUATED = true
  pass_flag   = SAY_MOOS = pass
  fail_flag   = SAY_MOOS = fail

  mission_form = ufield_comms_tests
  mission_mod  = \$(MMOD:=route_resilience_expected_negative_pass)

  report_file   = results.txt
  report_column = grade=\$[GRADE]
  report_column = form=\$[MISSION_FORM]
  report_column = mmod=\$[MMOD]
  report_column = expected=$expected
  report_column = node_count=\$[UFSB_NODE_COUNT]
  report_column = direct=\$[DIRECT_MSG_DELIVERED]
  report_column = ack=\$[ACK_DELIVERED]
  report_column = rpt_ab=\$[REPORT_ABE_TO_BEN]
  report_column = rpt_ba=\$[REPORT_BEN_TO_ABE]
  report_column = pulse=\$[VIEW_COMMS_PULSE_SEEN]
  report_column = shared=\$[SHARED_REPORT_SEEN]
  report_column = timeout=\$[MISSION_TIMEOUT]
  report_column = mhash=\$[MHASH_SHORT]
}"
}

prepare_case_dir() {
    local case_dir="$1"
    local pbase="$2"
    local bad_port=$((pbase + 29))
    local veh1_pshare=$((pbase + 11))
    local veh2_pshare=$((pbase + 12))

    cp -R "$MISSION_DIR"/. "$case_dir"/
    (cd "$case_dir" && ./clean.sh >/dev/null 2>&1 || true)

    if [ "$ROUTE_MODE" = "dead_first_tryhost" ]; then
        BAD_PORT="$bad_port" perl -0pi -e 's/  try_shore_host = pshare_route=localhost:\$\(SHORE_PSHARE\)/  try_shore_host = pshare_route=localhost:\$(SHORE_PSHARE)\n  try_shore_host = pshare_route=localhost:$ENV{BAD_PORT}/' "$case_dir/meta_vehicle.moos"
    elif [ "$ROUTE_MODE" = "secondary_dead_tryhost" ]; then
        BAD_PORT="$bad_port" perl -0pi -e 's/  try_shore_host = pshare_route=localhost:\$\(SHORE_PSHARE\)/  try_shore_host = pshare_route=localhost:$ENV{BAD_PORT}\n  try_shore_host = pshare_route=localhost:\$(SHORE_PSHARE)/' "$case_dir/meta_vehicle.moos"
    elif [ "$ROUTE_MODE" = "startup_invalid_bad_port" ]; then
        perl -0pi -e 's/  try_shore_host = pshare_route=localhost:\$\(SHORE_PSHARE\)/  try_shore_host = pshare_route=localhost:notaport\n  try_shore_host = pshare_route=localhost:\$(SHORE_PSHARE)/' "$case_dir/meta_vehicle.moos"
    elif [ "$ROUTE_MODE" = "startup_invalid_bad_param" ]; then
        perl -0pi -e 's/  try_shore_host = pshare_route=localhost:\$\(SHORE_PSHARE\)/  try_shore_host = shore_route=localhost:\$(SHORE_PSHARE)\n  try_shore_host = pshare_route=localhost:\$(SHORE_PSHARE)/' "$case_dir/meta_vehicle.moos"
    elif [ "$ROUTE_MODE" = "startup_ip_route" ]; then
        perl -0pi -e 's/  try_shore_host = pshare_route=localhost:\$\(SHORE_PSHARE\)/  try_shore_host = pshare_route=127.0.0.1:\$(SHORE_PSHARE)/' "$case_dir/meta_vehicle.moos"
    elif [ "$ROUTE_MODE" = "runtime_tryhost" ]; then
        remove_startup_tryhost "$case_dir"
        insert_vehicle_event_before_messages "$case_dir" '  event = var=TRY_SHORE_HOST, val="pshare_route=localhost:$(SHORE_PSHARE)", time=220'
    elif [ "$ROUTE_MODE" = "runtime_dead_then_valid" ]; then
        remove_startup_tryhost "$case_dir"
        BAD_PORT="$bad_port" insert_vehicle_event_before_messages "$case_dir" "  event = var=TRY_SHORE_HOST, val=\"pshare_route=localhost:$bad_port\", time=80
  event = var=TRY_SHORE_HOST, val=\"pshare_route=localhost:\$(SHORE_PSHARE)\", time=220"
    elif [ "$ROUTE_MODE" = "runtime_valid_then_dead" ]; then
        remove_startup_tryhost "$case_dir"
        insert_vehicle_event_before_messages "$case_dir" "  event = var=TRY_SHORE_HOST, val=\"pshare_route=localhost:\$(SHORE_PSHARE)\", time=120
  event = var=TRY_SHORE_HOST, val=\"pshare_route=localhost:$bad_port\", time=620"
    elif [ "$ROUTE_MODE" = "runtime_duplicate_tryhost" ]; then
        remove_startup_tryhost "$case_dir"
        insert_vehicle_event_before_messages "$case_dir" '  event = var=TRY_SHORE_HOST, val="pshare_route=localhost:$(SHORE_PSHARE)", time=220
  event = var=TRY_SHORE_HOST, val="pshare_route=localhost:$(SHORE_PSHARE)", time=260'
    elif [ "$ROUTE_MODE" = "startup_duplicate_runtime" ]; then
        insert_vehicle_event_before_messages "$case_dir" '  event = var=TRY_SHORE_HOST, val="pshare_route=localhost:$(SHORE_PSHARE)", time=160'
    elif [ "$ROUTE_MODE" = "delayed_runtime_tryhost" ]; then
        remove_startup_tryhost "$case_dir"
        insert_vehicle_event_before_messages "$case_dir" '  event = var=TRY_SHORE_HOST, val="pshare_route=localhost:$(SHORE_PSHARE)", time=700'
    elif [ "$ROUTE_MODE" = "late_runtime_tryhost" ]; then
        remove_startup_tryhost "$case_dir"
        insert_vehicle_event_before_messages "$case_dir" '  event = var=TRY_SHORE_HOST, val="pshare_route=localhost:$(SHORE_PSHARE)", time=1700'
    elif [ "$ROUTE_MODE" = "runtime_one_node_missing_route" ]; then
        remove_startup_tryhost "$case_dir"
        insert_abe_event "$case_dir" '  event = var=TRY_SHORE_HOST, val="pshare_route=localhost:$(SHORE_PSHARE)", time=220'
    elif [ "$ROUTE_MODE" = "runtime_staggered_nodes" ]; then
        remove_startup_tryhost "$case_dir"
        insert_abe_event "$case_dir" '  event = var=TRY_SHORE_HOST, val="pshare_route=localhost:$(SHORE_PSHARE)", time=220'
        insert_ben_event "$case_dir" '  event = var=TRY_SHORE_HOST, val="pshare_route=localhost:$(SHORE_PSHARE)", time=620'
    elif [ "$ROUTE_MODE" = "invalid_runtime_tryhost" ]; then
        remove_startup_tryhost "$case_dir"
        insert_vehicle_event_before_messages "$case_dir" '  event = var=TRY_SHORE_HOST, val="pshare_route=not_a_route", time=120
  event = var=TRY_SHORE_HOST, val="pshare_route=localhost:$(SHORE_PSHARE)", time=260'
    elif [ "$ROUTE_MODE" = "invalid_runtime_bad_port" ]; then
        remove_startup_tryhost "$case_dir"
        insert_vehicle_event_before_messages "$case_dir" '  event = var=TRY_SHORE_HOST, val="pshare_route=localhost:notaport", time=120
  event = var=TRY_SHORE_HOST, val="pshare_route=localhost:$(SHORE_PSHARE)", time=260'
    elif [ "$ROUTE_MODE" = "invalid_runtime_bad_param" ]; then
        remove_startup_tryhost "$case_dir"
        insert_vehicle_event_before_messages "$case_dir" '  event = var=TRY_SHORE_HOST, val="shore_route=localhost:$(SHORE_PSHARE)", time=120
  event = var=TRY_SHORE_HOST, val="pshare_route=localhost:$(SHORE_PSHARE)", time=260'
    elif [ "$ROUTE_MODE" = "runtime_ip_route" ]; then
        remove_startup_tryhost "$case_dir"
        insert_vehicle_event_before_messages "$case_dir" '  event = var=TRY_SHORE_HOST, val="pshare_route=127.0.0.1:$(SHORE_PSHARE)", time=220'
    elif [ "$ROUTE_MODE" = "shore_vnode_discovery" ]; then
        remove_startup_tryhost "$case_dir"
        patch_shore_broker_block "$case_dir" "ProcessConfig = uFldShoreBroker
{
  AppTick   = 2
  CommsTick = 2

  qbridge = NODE_REPORT, NODE_MESSAGE, ACK_MESSAGE

  try_vnode = route=127.0.0.1:$veh1_pshare
  try_vnode = route=127.0.0.1:$veh2_pshare
}"
    elif [ "$ROUTE_MODE" = "shore_vnode_one_node_only" ]; then
        remove_startup_tryhost "$case_dir"
        patch_shore_broker_block "$case_dir" "ProcessConfig = uFldShoreBroker
{
  AppTick   = 2
  CommsTick = 2

  qbridge = NODE_REPORT, NODE_MESSAGE, ACK_MESSAGE

  try_vnode = route=127.0.0.1:$veh1_pshare
}"
    elif [ "$ROUTE_MODE" = "shore_vnode_extra_dead" ]; then
        remove_startup_tryhost "$case_dir"
        patch_shore_broker_block "$case_dir" "ProcessConfig = uFldShoreBroker
{
  AppTick   = 2
  CommsTick = 2

  qbridge = NODE_REPORT, NODE_MESSAGE, ACK_MESSAGE

  try_vnode = route=127.0.0.1:$veh1_pshare
  try_vnode = route=127.0.0.1:$veh2_pshare
  try_vnode = route=127.0.0.1:$bad_port
}"
    elif [ "$ROUTE_MODE" = "shore_vnode_default_port" ]; then
        remove_startup_tryhost "$case_dir"
        patch_shore_broker_block "$case_dir" "ProcessConfig = uFldShoreBroker
{
  AppTick   = 2
  CommsTick = 2

  qbridge = NODE_REPORT, NODE_MESSAGE, ACK_MESSAGE

  try_vnode = route=127.0.0.1
}"
    elif [ "$ROUTE_MODE" = "shore_vnode_invalid_host" ]; then
        remove_startup_tryhost "$case_dir"
        patch_shore_broker_block "$case_dir" "ProcessConfig = uFldShoreBroker
{
  AppTick   = 2
  CommsTick = 2

  qbridge = NODE_REPORT, NODE_MESSAGE, ACK_MESSAGE

  try_vnode = route=badhost:$veh1_pshare
  try_vnode = route=127.0.0.1:$veh1_pshare
  try_vnode = route=127.0.0.1:$veh2_pshare
}"
    elif [ "$ROUTE_MODE" = "shore_vnode_bad_port" ]; then
        remove_startup_tryhost "$case_dir"
        patch_shore_broker_block "$case_dir" "ProcessConfig = uFldShoreBroker
{
  AppTick   = 2
  CommsTick = 2

  qbridge = NODE_REPORT, NODE_MESSAGE, ACK_MESSAGE

  try_vnode = route=127.0.0.1:notaport
  try_vnode = route=127.0.0.1:$veh1_pshare
  try_vnode = route=127.0.0.1:$veh2_pshare
}"
    elif [ "$ROUTE_MODE" = "shore_vnode_duplicate" ]; then
        remove_startup_tryhost "$case_dir"
        patch_shore_broker_block "$case_dir" "ProcessConfig = uFldShoreBroker
{
  AppTick   = 2
  CommsTick = 2

  qbridge = NODE_REPORT, NODE_MESSAGE, ACK_MESSAGE

  try_vnode = route=127.0.0.1:$veh1_pshare
  try_vnode = route=127.0.0.1:$veh1_pshare
  try_vnode = route=127.0.0.1:$veh2_pshare
}"
    fi

    if [ "$NEGATIVE_PROFILE" != "" ]; then
        patch_negative_eval_block "$case_dir" "$NEGATIVE_PROFILE" "$EXTRA_CHECK"
    fi
}

run_case() {
    local case_name="$1"
    local case_index="$2"
    get_case_config "$case_name" || {
        echo "case=$case_name  grade=fail  reason=case_config_error" >> "$RESULTS_FILE"
        return 1
    }

    local case_dir="$RUN_ROOT/$case_name"
    local case_rows="$case_dir/results.txt"
    local pbase=$((PORT_BASE + case_index * PORT_STRIDE))
    mkdir -p "$case_dir"
    prepare_case_dir "$case_dir" "$pbase" || {
        echo "case=$case_name  grade=fail  reason=prepare_error" >> "$RESULTS_FILE"
        return 1
    }
    : > "$case_rows"

    local just_make_arg=""
    if [ "$JUST_MAKE" = "yes" ]; then
        just_make_arg="--just_make"
    fi

    (
        cd "$case_dir"
        xlaunch.sh --max_time="$MAX_TIME" $NOGUI $just_make_arg \
            --mmod="$case_name" \
            --shore_mport="$pbase" \
            --veh1_mport="$((pbase+1))" \
            --veh2_mport="$((pbase+2))" \
            --shore_pshare="$((pbase+10))" \
            --veh1_pshare="$((pbase+11))" \
            --veh2_pshare="$((pbase+12))" \
            "$TIME_WARP" >"$case_dir/xlaunch.out" 2>&1
    )
    local launch_rc=$?
    harness_teardown_stop_root "$case_dir" >/dev/null 2>&1 || true

    if [ "$JUST_MAKE" = "yes" ]; then
        if [ "$launch_rc" = 0 ]; then
            echo "case=$case_name  grade=pass  reason=just_make" >> "$RESULTS_FILE"
            echo "$case_name: just_make ok"
            return 0
        fi
        echo "case=$case_name  grade=fail  reason=launch_error  launch_rc=$launch_rc" >> "$RESULTS_FILE"
        echo "$case_name: launch failed in just_make"
        return 1
    fi

    local line
    local result_line
    if [ "$launch_rc" = 0 ]; then
        line=`wait_for_result_line "$case_rows"`
    else
        line=""
    fi

    result_line=$(format_result_row "$case_name" "$line" "$launch_rc")
    echo "$result_line" >> "$RESULTS_FILE"
    if ! result_row_passed "$result_line"; then
        echo "$case_name: result failed: $result_line"
        return 1
    fi
    if ! extra_check_ok "$EXTRA_CHECK" "$case_dir" "$result_line"; then
        echo "$case_name: extra check failed: $result_line"
        return 1
    fi
    echo "$case_name: ok"
}

if [ "$CASE" != "" ]; then
    CASES="$CASE"
else
    CASES=`case_list`
fi

RUN_ROOT=`mktemp -d "${TMPDIR:-/tmp}/ufield_route_resilience_harness.XXXXXX"`
: > "$RESULTS_FILE"

INDEX=0
PIDS=""
for C in $CASES; do
    run_case "$C" "$INDEX" &
    PIDS="$PIDS $!"
    INDEX=$((INDEX + 1))
    if [ "$JUST_MAKE" = "yes" ]; then
        wait || ALL_OK="no"
        PIDS=""
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
