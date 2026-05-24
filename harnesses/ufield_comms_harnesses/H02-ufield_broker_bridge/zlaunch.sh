#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H02-ufield_broker_bridge
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
broker_handshake_pass
shore_qbridge_expansion_pass
shore_custom_bridge_pass
node_custom_bridge_pass
keyword_mismatch_status_pass
shore_bridge_tokens_pass
node_mediated_bridge_pass
shore_try_vnode_pass
shore_minimal_autobridge_pass
node_minimal_autobridge_pass
shore_default_alias_pass
shore_try_vnode_default_port_pass
)

case_list() {
    printf "%s\n" "${ALL_CASES[@]}"
}

get_case_config() {
    CASE_NAME="$1"
    EXPECTED="pass"
    SHORE_PATCH=""
    NODE_MODE="stock"
    EXTRA_CHECK=""

    case "$CASE_NAME" in
        broker_handshake_pass)
            EXTRA_CHECK="broker_handshake"
            ;;
        shore_qbridge_expansion_pass)
            EXTRA_CHECK="shore_qbridge_expansion"
            ;;
        shore_custom_bridge_pass)
            SHORE_PATCH="$HARNESS_DIR/shore-custom-bridge-shoreside.xmoos"
            EXTRA_CHECK="shore_custom_bridge"
            ;;
        node_custom_bridge_pass)
            NODE_MODE="custom_bridge"
            EXTRA_CHECK="node_custom_bridge"
            ;;
        keyword_mismatch_status_pass)
            SHORE_PATCH="$HARNESS_DIR/keyword-mismatch-shoreside.xmoos"
            EXTRA_CHECK="keyword_mismatch_status"
            ;;
        shore_bridge_tokens_pass)
            SHORE_PATCH="$HARNESS_DIR/shore-bridge-tokens-shoreside.xmoos"
            EXTRA_CHECK="shore_bridge_tokens"
            ;;
        node_mediated_bridge_pass)
            NODE_MODE="mediated_bridge"
            SHORE_PATCH="$HARNESS_DIR/mediated-eval-shoreside.xmoos"
            EXTRA_CHECK="node_mediated_bridge"
            ;;
        shore_try_vnode_pass)
            SHORE_PATCH="$HARNESS_DIR/try-vnode-shoreside.xmoos"
            EXTRA_CHECK="shore_try_vnode"
            ;;
        shore_minimal_autobridge_pass)
            SHORE_PATCH="$HARNESS_DIR/minimal-autobridge-shoreside.xmoos"
            EXTRA_CHECK="shore_minimal_autobridge"
            ;;
        node_minimal_autobridge_pass)
            NODE_MODE="minimal_autobridge"
            EXTRA_CHECK="node_minimal_autobridge"
            ;;
        shore_default_alias_pass)
            SHORE_PATCH="$HARNESS_DIR/shore-default-alias-shoreside.xmoos"
            EXTRA_CHECK="shore_default_alias"
            ;;
        shore_try_vnode_default_port_pass)
            SHORE_PATCH="$HARNESS_DIR/try-vnode-default-port-shoreside.xmoos"
            EXTRA_CHECK="shore_try_vnode_default_port"
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

extra_check_ok() {
    local check="$1"
    local case_dir="$2"
    local line="$3"

    case "$check" in
        broker_handshake)
            echo "$line" | rg -q 'node_count=2' && \
            shore_has "$case_dir" "NODE_BROKER_PING" "community=abe" && \
            shore_has "$case_dir" "NODE_BROKER_PING" "community=ben" && \
            shore_has "$case_dir" "NODE_BROKER_ACK_ABE" "status=ok,key=0" && \
            shore_has "$case_dir" "NODE_BROKER_ACK_BEN" "status=ok,key=0" && \
            shore_has "$case_dir" "NODE_BROKER_VACK" "abe" && \
            shore_has "$case_dir" "NODE_BROKER_VACK" "ben" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=ok,key=0" && \
            vehicle_has "$case_dir" "ABE" "NODE_PSHARE_VARS" "NODE_REPORT" && \
            vehicle_has "$case_dir" "BEN" "NODE_PSHARE_VARS" "NODE_REPORT"
            ;;
        shore_qbridge_expansion)
            shore_has "$case_dir" "UFSB_QBRIDGE_VARS" "NODE_REPORT" && \
            shore_has "$case_dir" "UFSB_QBRIDGE_VARS" "NODE_MESSAGE" && \
            shore_has "$case_dir" "UFSB_QBRIDGE_VARS" "ACK_MESSAGE" && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=NODE_REPORT_ALL,dest_name=NODE_REPORT" && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=NODE_REPORT_ABE,dest_name=NODE_REPORT" && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=NODE_REPORT_BEN,dest_name=NODE_REPORT" && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=NODE_MESSAGE_BEN,dest_name=NODE_MESSAGE" && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=ACK_MESSAGE_ABE,dest_name=ACK_MESSAGE"
            ;;
        shore_custom_bridge)
            shore_has "$case_dir" "PSHARE_CMD" "src_name=DEPLOY_ABE,dest_name=DEPLOY" && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=DEPLOY_BEN,dest_name=DEPLOY" && \
            shore_has "$case_dir" "UFSB_BRIDGE_VARS" 'DEPLOY_\$V' && \
            shore_has "$case_dir" "UFSB_BRIDGE_VARS" "DEPLOY_ABE" && \
            shore_has "$case_dir" "UFSB_BRIDGE_VARS" "DEPLOY_BEN"
            ;;
        node_custom_bridge)
            vehicle_has "$case_dir" "ABE" "PSHARE_CMD" "src_name=CUSTOM_LOCAL,dest_name=CUSTOM_SHARED" && \
            vehicle_has "$case_dir" "BEN" "PSHARE_CMD" "src_name=CUSTOM_LOCAL,dest_name=CUSTOM_SHARED" && \
            vehicle_has "$case_dir" "ABE" "NODE_PSHARE_VARS" "CUSTOM_SHARED" && \
            vehicle_has "$case_dir" "BEN" "NODE_PSHARE_VARS" "CUSTOM_SHARED"
            ;;
        keyword_mismatch_status)
            echo "$line" | rg -q 'node_count=2' && \
            shore_has "$case_dir" "NODE_BROKER_ACK_ABE" "status=keyword_mismatch" && \
            shore_has "$case_dir" "NODE_BROKER_ACK_BEN" "status=keyword_mismatch" && \
            vehicle_has "$case_dir" "ABE" "NODE_BROKER_ACK" "status=keyword_mismatch" && \
            vehicle_has "$case_dir" "BEN" "NODE_BROKER_ACK" "status=keyword_mismatch"
            ;;
        shore_bridge_tokens)
            shore_has "$case_dir" "PSHARE_CMD" "src_name=STATUS_abe,dest_name=STATUS" && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=STATUS_ben,dest_name=STATUS" && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=INDEX_1,dest_name=INDEXED_STATUS" && \
            shore_has "$case_dir" "PSHARE_CMD" "src_name=INDEX_2,dest_name=INDEXED_STATUS" && \
            shore_has "$case_dir" "UFSB_BRIDGE_VARS" 'STATUS_\$v' && \
            shore_has "$case_dir" "UFSB_BRIDGE_VARS" 'INDEX_\$N'
            ;;
        node_mediated_bridge)
            vehicle_has "$case_dir" "ABE" "PSHARE_CMD" "src_name=MEDIATED_MESSAGE_LOCAL,dest_name=MEDIATED_MESSAGE" && \
            vehicle_has "$case_dir" "BEN" "PSHARE_CMD" "src_name=MEDIATED_MESSAGE_LOCAL,dest_name=MEDIATED_MESSAGE" && \
            vehicle_has "$case_dir" "ABE" "NODE_PSHARE_VARS" "MEDIATED_MESSAGE" && \
            vehicle_has "$case_dir" "BEN" "NODE_PSHARE_VARS" "MEDIATED_MESSAGE" && \
            vehicle_lacks "$case_dir" "ABE" "PSHARE_CMD" "src_name=NODE_MESSAGE_LOCAL,dest_name=NODE_MESSAGE" && \
            vehicle_lacks "$case_dir" "BEN" "PSHARE_CMD" "src_name=NODE_MESSAGE_LOCAL,dest_name=NODE_MESSAGE"
            ;;
        shore_try_vnode)
            shore_has "$case_dir" "PSHARE_CMD" "src_name=TRY_SHORE_HOST,dest_name=TRY_SHORE_HOST,route=127.0.0.1:4999" && \
            shore_has "$case_dir" "TRY_SHORE_HOST" "pshare_route=.*:[0-9]+"
            ;;
        shore_minimal_autobridge)
            shore_has "$case_dir" "UFSB_QBRIDGE_VARS" "NODE_REPORT" && \
            shore_has "$case_dir" "UFSB_QBRIDGE_VARS" "NODE_MESSAGE" && \
            shore_has "$case_dir" "UFSB_QBRIDGE_VARS" "ACK_MESSAGE" && \
            shore_lacks "$case_dir" "UFSB_QBRIDGE_VARS" "REALMCAST_REQ" && \
            shore_lacks "$case_dir" "UFSB_QBRIDGE_VARS" "APPCAST_REQ" && \
            shore_lacks "$case_dir" "UFSB_BRIDGE_VARS" "MISSION_HASH"
            ;;
        node_minimal_autobridge)
            vehicle_has "$case_dir" "ABE" "NODE_PSHARE_VARS" "ACK_MESSAGE" && \
            vehicle_has "$case_dir" "ABE" "NODE_PSHARE_VARS" "NODE_MESSAGE" && \
            vehicle_has "$case_dir" "ABE" "NODE_PSHARE_VARS" "NODE_REPORT" && \
            vehicle_lacks "$case_dir" "ABE" "NODE_PSHARE_VARS" "REALMCAST" && \
            vehicle_lacks "$case_dir" "ABE" "NODE_PSHARE_VARS" "APPCAST" && \
            vehicle_lacks "$case_dir" "ABE" "PSHARE_CMD" "src_name=NODE_PSHARE_VARS,dest_name=NODE_PSHARE_VARS" && \
            vehicle_lacks "$case_dir" "BEN" "PSHARE_CMD" "src_name=NODE_PSHARE_VARS,dest_name=NODE_PSHARE_VARS"
            ;;
        shore_default_alias)
            shore_has "$case_dir" "PSHARE_CMD" "src_name=FLEET_ALERT,dest_name=FLEET_ALERT,route=localhost:[0-9]+" && \
            shore_has "$case_dir" "UFSB_BRIDGE_VARS" "FLEET_ALERT"
            ;;
        shore_try_vnode_default_port)
            shore_has "$case_dir" "PSHARE_CMD" "src_name=TRY_SHORE_HOST,dest_name=TRY_SHORE_HOST,route=127.0.0.1:9200" && \
            shore_has "$case_dir" "TRY_SHORE_HOST" "pshare_route=.*:[0-9]+"
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
    if [ "$NODE_MODE" = "custom_bridge" ]; then
        perl -0pi -e 's/(bridge = src=ACK_MESSAGE_LOCAL,  alias=ACK_MESSAGE)/$1\n  bridge = src=CUSTOM_LOCAL,      alias=CUSTOM_SHARED/' "$case_dir/meta_vehicle.moos"
    elif [ "$NODE_MODE" = "mediated_bridge" ]; then
        perl -0pi -e 's/bridge = src=NODE_MESSAGE_LOCAL, alias=NODE_MESSAGE/bridge = src=MEDIATED_MESSAGE_LOCAL, alias=MEDIATED_MESSAGE/' "$case_dir/meta_vehicle.moos"
        perl -0pi -e 's/(Run = uFldNodeBroker \@ NewConsole = false)/$1\n  Run = pMediator      \@ NewConsole = false/' "$case_dir/meta_vehicle.moos"
        perl -0pi -e 's/(ProcessConfig = uTimerScript)/ProcessConfig = pMediator\n{\n  AppTick      = 4\n  CommsTick    = 4\n  resend_thresh = 3\n  max_tries     = 20\n}\n\n\/\/----------------------------------------------------\n$1/' "$case_dir/meta_vehicle.moos"
        perl -0pi -e 's/dest_node=all,var_name=UFC_DIRECT/dest_node=ben,var_name=UFC_DIRECT/' "$case_dir/meta_vehicle.moos"
    elif [ "$NODE_MODE" = "minimal_autobridge" ]; then
        perl -0pi -e 's/(CommsTick = 2\n\n  try_shore_host)/CommsTick = 2\n\n  auto_bridge_realmcast   = false\n  auto_bridge_appcast     = false\n  auto_bridge_pshare_vars = false\n\n  try_shore_host/' "$case_dir/meta_vehicle.moos"
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

RUN_ROOT=`mktemp -d "${TMPDIR:-/tmp}/ufield_broker_harness.XXXXXX"`
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
