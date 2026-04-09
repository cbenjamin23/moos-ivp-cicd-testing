#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: Mar 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

ME=`basename "$0"`
TIME_WARP="10"
VERBOSE=""
JUST_MAKE=""
MAX_TIME="160"
CASE=""
GROUP=""
JOBS="1"
PORT_BASE="9900"
KEEP_WORKDIRS="no"
RESULTS_FILE="$PWD/results.txt"
HARNESS_DIR="$PWD"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/colregs_missions/colregs_unit"
ALL_OK="yes"
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
MIN_UTIL_CPA=""
MAX_UTIL_CPA=""
RUN_ROOT=""
CASE_RESULT_DIR=""
WAVE_SERIAL_CASES="giveway_turngap_edge_pass giveway_turngap_above_pass giveway_turngap_edge_mirror_pass giveway_turngap_above_mirror_pass"

for ARGI; do
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo ""
        echo "Options:"
        echo "  --help, -h         Show this help message"
        echo "  --verbose, -v      Verbose, confirm launch"
        echo "  --just_make, -j    Only create targ files"
        echo "  --max_time=<secs>  Max time passed to xlaunch"
        echo "  --case=<name>      Run one named case"
        echo "  --group=<name>     Run one named case group"
        echo "  --jobs=<n>         Run up to n cases per wave"
        echo "  --port_base=<n>    Base shoreside MOOSDB port for wave mode"
        echo "  --keep_workdirs    Keep temp mission copies in wave mode"
        echo ""
        echo "Groups:"
        echo "  all"
        echo "  headon"
        echo "  overtaking"
        echo "  giveway"
        echo "  standon"
        echo "  outer_dist"
        echo "  overtaken"
        echo "  overtaken_mirror"
        echo "  inextremis"
        echo "                     head_on_thresh_below_pass"
        echo "                     head_on_thresh_edge_pass"
        echo "                     head_on_thresh_above_pass"
        echo "                     head_on_thresh_below_mirror_pass"
        echo "                     head_on_thresh_edge_mirror_pass"
        echo "                     head_on_thresh_above_mirror_pass"
        echo "                     overtaking_thresh_below_pass"
        echo "                     overtaking_thresh_edge_pass"
        echo "                     overtaking_thresh_above_pass"
        echo "                     overtaking_thresh_below_mirror_pass"
        echo "                     overtaking_thresh_edge_mirror_pass"
        echo "                     overtaking_thresh_above_mirror_pass"
        echo "                     overtaken_thresh_below_pass"
        echo "                     overtaken_thresh_edge_pass"
        echo "                     overtaken_thresh_above_pass"
        echo "                     overtaken_thresh_below_mirror_pass"
        echo "                     overtaken_thresh_edge_mirror_pass"
        echo "                     overtaken_thresh_above_mirror_pass"
        echo "                     giveway_bowdist_below_pass"
        echo "                     giveway_bowdist_edge_pass"
        echo "                     giveway_bowdist_above_pass"
        echo "                     giveway_bowdist_below_mirror_pass"
        echo "                     giveway_bowdist_edge_mirror_pass"
        echo "                     giveway_bowdist_above_mirror_pass"
        echo "                     standon_unsurebow_below_pass"
        echo "                     standon_unsurebow_edge_pass"
        echo "                     standon_unsurebow_above_pass"
        echo "                     standon_band270_stern_pass"
        echo "                     standon_band350_unsurebow_pass"
        echo "                     standon_band350_unsurebow_alt_pass"
        echo "                     standon_band350_bow_pass"
        echo "                     standon_band315_unsure_pass"
        echo "                     standon_band315_unsure_bow_pass"
        echo "                     standon_band315_bow_pass"
        echo "                     standon_southwest_unsurebow_pass"
        echo "                     standon_southwest_unsure_pass"
        echo "                     standon_southwest_stern_pass"
        echo "                     standon_neither_* (exploratory/manual only)"
        echo "                     standon_neither_below_pass"
        echo "                     standon_neither_edge_pass"
        echo "                     standon_neither_above_pass"
        echo "                     outer_dist_below_pass"
        echo "                     outer_dist_edge_pass"
        echo "                     outer_dist_above_pass"
        echo "                     standon_inextremis_range_below_pass"
        echo "                     standon_inextremis_range_edge_pass"
        echo "                     standon_inextremis_range_above_pass"
        echo "                     standon_inextremis_cpa_below_pass"
        echo "                     standon_inextremis_cpa_edge_pass"
        echo "                     standon_inextremis_cpa_above_pass"
        echo "                     standon_ot_inextremis_range_below_pass"
        echo "                     standon_ot_inextremis_range_edge_pass"
        echo "                     standon_ot_inextremis_range_above_pass"
        echo "                     standon_ot_inextremis_cpa_below_pass"
        echo "                     standon_ot_inextremis_cpa_edge_pass"
        echo "                     standon_ot_inextremis_cpa_above_pass"
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" -a "$TIME_WARP" = 10 ]; then
        TIME_WARP=$ARGI
    elif [ "${ARGI}" = "--verbose" -o "${ARGI}" = "-v" ]; then
        VERBOSE="yes"
    elif [ "${ARGI}" = "--just_make" -o "${ARGI}" = "-j" ]; then
        JUST_MAKE="yes"
    elif [ "${ARGI:0:11}" = "--max_time=" ]; then
        MAX_TIME="${ARGI#--max_time=*}"
    elif [ "${ARGI:0:7}" = "--case=" ]; then
        CASE="${ARGI#--case=*}"
    elif [ "${ARGI:0:8}" = "--group=" ]; then
        GROUP="${ARGI#--group=*}"
    elif [ "${ARGI:0:7}" = "--jobs=" ]; then
        JOBS="${ARGI#--jobs=*}"
    elif [ "${ARGI:0:12}" = "--port_base=" ]; then
        PORT_BASE="${ARGI#--port_base=*}"
    elif [ "${ARGI}" = "--keep_workdirs" ]; then
        KEEP_WORKDIRS="yes"
    else
        echo "$ME: Bad arg: $ARGI"
        exit 1
    fi
done

if ! echo "$JOBS" | grep -Eq '^[0-9]+$' || [ "$JOBS" -lt 1 ]; then
    echo "$ME: Bad value for --jobs: [$JOBS]"
    exit 1
fi

if ! echo "$PORT_BASE" | grep -Eq '^[0-9]+$'; then
    echo "$ME: Bad value for --port_base: [$PORT_BASE]"
    exit 1
fi

wait_for_result_line() {
    local results_path="$1"
    local attempts="${2:-24}"
    local line=""
    local attempt

    for attempt in $(seq 1 "$attempts"); do
        line=$(tail -n 1 "$results_path" 2>/dev/null)
        if echo "$line" | grep -q 'grade='; then
            echo "$line"
            return 0
        fi
        sleep 0.25
    done

    echo "$line"
    return 1
}

wait_for_result_or_exit() {
    local results_path="$1"
    local root_pid="$2"
    local line=""

    while true; do
        line=$(tail -n 1 "$results_path" 2>/dev/null)
        if echo "$line" | grep -q 'grade='; then
            echo "$line"
            return 0
        fi
        if ! kill -0 "$root_pid" >/dev/null 2>&1; then
            break
        fi
        sleep 0.25
    done

    wait_for_result_line "$results_path" 32
}

collect_descendant_pids() {
    local root_pid="$1"
    local child_pid

    [ "$root_pid" = "" ] && return 0
    echo "$root_pid"
    for child_pid in $(pgrep -P "$root_pid" 2>/dev/null); do
        collect_descendant_pids "$child_pid"
    done
}

list_case_runtime_pids() {
    local port
    local listener_pid
    local parent_pid

    for port in "$@"; do
        for listener_pid in $(lsof -nP -t -iTCP:"$port" -sTCP:LISTEN 2>/dev/null); do
            echo "$listener_pid"
            parent_pid=$(ps -o ppid= -p "$listener_pid" 2>/dev/null | tr -d ' ')
            if [ "$parent_pid" != "" ]; then
                echo "$parent_pid"
            fi
        done
    done | awk 'NF' | sort -u
}

list_case_pids() {
    local root_pid="$1"
    shift

    {
        if [ "$root_pid" != "" ] && kill -0 "$root_pid" >/dev/null 2>&1; then
            collect_descendant_pids "$root_pid"
        fi
        list_case_runtime_pids "$@"
    } | awk 'NF' | sort -u
}

case_ports_quiet() {
    local port
    for port in "$@"; do
        if lsof -nP -t -iTCP:"$port" -sTCP:LISTEN >/dev/null 2>&1; then
            return 1
        fi
    done
    return 0
}

case_runtime_quiet() {
    local root_pid="$1"
    shift

    if [ "$root_pid" != "" ] && kill -0 "$root_pid" >/dev/null 2>&1; then
        return 1
    fi
    case_ports_quiet "$@"
}

stop_case_runtime() {
    local root_pid="$1"
    local attempts="${2:-24}"
    shift 2
    local runtime_pids=""
    local attempt

    runtime_pids=$(list_case_pids "$root_pid" "$@")
    if [ "$runtime_pids" != "" ]; then
        kill -INT $runtime_pids >/dev/null 2>&1 || true
    fi

    for attempt in $(seq 1 "$attempts"); do
        if case_runtime_quiet "$root_pid" "$@"; then
            return 0
        fi
        sleep 0.25
    done

    runtime_pids=$(list_case_pids "$root_pid" "$@")
    if [ "$runtime_pids" != "" ]; then
        kill -TERM $runtime_pids >/dev/null 2>&1 || true
    fi

    for attempt in $(seq 1 "$attempts"); do
        if case_runtime_quiet "$root_pid" "$@"; then
            return 0
        fi
        sleep 0.25
    done

    runtime_pids=$(list_case_pids "$root_pid" "$@")
    if [ "$runtime_pids" != "" ]; then
        kill -KILL $runtime_pids >/dev/null 2>&1 || true
    fi

    for attempt in $(seq 1 "$attempts"); do
        if case_runtime_quiet "$root_pid" "$@"; then
            return 0
        fi
        sleep 0.25
    done

    return 1
}

case_requires_solo_wave() {
    local case_name="$1"
    case " $WAVE_SERIAL_CASES " in
        *" $case_name "*) return 0 ;;
    esac
    return 1
}

clear_xfiles() {
    rm -f "$SHORE_XFILE"
}

cleanup() {
    local start_dir="$PWD"
    if [ -d "$MISSION_DIR" ]; then
        cd "$MISSION_DIR"
        clear_xfiles
        ./clean.sh >/dev/null 2>&1 || true
        ktm >/dev/null 2>&1 || true
    fi
    cd "$start_dir"
    if [ "$KEEP_WORKDIRS" != "yes" ] && [ "$RUN_ROOT" != "" ]; then
        rm -rf "$RUN_ROOT"
    fi
}

prepare_case_dir() {
    local case_dir="$1"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
    )

    local shore_stem="$case_dir/meta_shoreside.moos"
    local shore_xfile="$case_dir/meta_shoreside.moosx"
    local bhv_stem="$case_dir/meta_vehicle.bhv"
    local bhv_xfile="$case_dir/meta_vehicle.bhvx"
    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$shore_stem" "$SHORE_PATCH" --targ="$shore_xfile"
    fi
}

get_case_config() {
    CASE_NAME="$1"
    EXPECTED="pass"
    SHORE_PATCH=""
    MIN_UTIL_CPA=""
    MAX_UTIL_CPA=""
    MMOD="$CASE_NAME"

    if [ "$CASE_NAME" = "head_on_thresh_below_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/head-on-thresh-below-shoreside.xmoos"
    elif [ "$CASE_NAME" = "head_on_thresh_edge_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/head-on-thresh-edge-shoreside.xmoos"
    elif [ "$CASE_NAME" = "head_on_thresh_above_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/head-on-thresh-above-shoreside.xmoos"
    elif [ "$CASE_NAME" = "head_on_thresh_below_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/head-on-thresh-below-shoreside.xmoos"
    elif [ "$CASE_NAME" = "head_on_thresh_edge_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/head-on-thresh-edge-shoreside.xmoos"
    elif [ "$CASE_NAME" = "head_on_thresh_above_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/head-on-thresh-above-shoreside.xmoos"
    elif [ "$CASE_NAME" = "overtaking_thresh_below_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaking-thresh-below-port-shoreside.xmoos"
    elif [ "$CASE_NAME" = "overtaking_thresh_edge_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaking-thresh-giveway-bow-shoreside.xmoos"
    elif [ "$CASE_NAME" = "overtaking_thresh_above_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaking-thresh-giveway-bow-shoreside.xmoos"
    elif [ "$CASE_NAME" = "overtaking_thresh_below_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaking-thresh-edge-shoreside.xmoos"
    elif [ "$CASE_NAME" = "overtaking_thresh_edge_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaking-thresh-edge-shoreside.xmoos"
    elif [ "$CASE_NAME" = "overtaking_thresh_above_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaking-thresh-below-shoreside.xmoos"
    elif [ "$CASE_NAME" = "overtaken_thresh_below_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaken-thresh-cpa-shoreside.xmoos"
        MMOD="overtaken_port_standon_gate_below_pass"
    elif [ "$CASE_NAME" = "overtaken_thresh_edge_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaken-thresh-standonot-port-shoreside.xmoos"
        MMOD="overtaken_port_standon_gate_edge_pass"
    elif [ "$CASE_NAME" = "overtaken_thresh_above_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaken-thresh-standonot-port-shoreside.xmoos"
        MMOD="overtaken_port_standon_gate_above_pass"
    elif [ "$CASE_NAME" = "overtaken_thresh_below_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaken-thresh-cpa-shoreside.xmoos"
        MMOD="overtaken_starboard_standon_gate_below_pass"
    elif [ "$CASE_NAME" = "overtaken_thresh_edge_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaken-thresh-standonot-starboard-shoreside.xmoos"
        MMOD="overtaken_starboard_standon_gate_edge_pass"
    elif [ "$CASE_NAME" = "overtaken_thresh_above_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/overtaken-thresh-standonot-starboard-shoreside.xmoos"
        MMOD="overtaken_starboard_standon_gate_above_pass"
    elif [ "$CASE_NAME" = "giveway_bowdist_below_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/giveway-bowdist-stern-shoreside.xmoos"
    elif [ "$CASE_NAME" = "giveway_bowdist_edge_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/giveway-bowdist-stern-shoreside.xmoos"
    elif [ "$CASE_NAME" = "giveway_bowdist_above_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/giveway-bowdist-bow-shoreside.xmoos"
    elif [ "$CASE_NAME" = "giveway_bowdist_below_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/giveway-bowdist-stern-shoreside.xmoos"
    elif [ "$CASE_NAME" = "giveway_bowdist_edge_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/giveway-bowdist-stern-shoreside.xmoos"
    elif [ "$CASE_NAME" = "giveway_bowdist_above_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/giveway-bowdist-bow-shoreside.xmoos"
    elif [ "$CASE_NAME" = "giveway_turngap_below_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/giveway-turngap-stern-shoreside.xmoos"
    elif [ "$CASE_NAME" = "giveway_turngap_edge_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/giveway-turngap-stern-shoreside.xmoos"
    elif [ "$CASE_NAME" = "giveway_turngap_above_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/giveway-turngap-bow-shoreside.xmoos"
    elif [ "$CASE_NAME" = "giveway_turngap_below_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/giveway-turngap-stern-shoreside.xmoos"
    elif [ "$CASE_NAME" = "giveway_turngap_edge_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/giveway-turngap-stern-shoreside.xmoos"
    elif [ "$CASE_NAME" = "giveway_turngap_above_mirror_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/giveway-turngap-bow-shoreside.xmoos"
    elif [ "$CASE_NAME" = "standon_unsurebow_below_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-unsurebow-unsure-shoreside.xmoos"
        MMOD="crossing_port_standon_unsure_pass"
    elif [ "$CASE_NAME" = "standon_unsurebow_edge_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-unsurebow-unsure-bow-shoreside.xmoos"
        MMOD="crossing_port_standon_close_pass"
    elif [ "$CASE_NAME" = "standon_unsurebow_above_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-unsurebow-bow-shoreside.xmoos"
        MMOD="crossing_port_standon_far_pass"
    elif [ "$CASE_NAME" = "standon_band270_stern_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-stern-shoreside.xmoos"
        MMOD="crossing_port_standon_stern_pass"
    elif [ "$CASE_NAME" = "standon_band350_unsurebow_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-unsurebow-unsure-bow-shoreside.xmoos"
        MMOD="crossing_port_standon_band350_unsure_pass"
    elif [ "$CASE_NAME" = "standon_band350_unsurebow_alt_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-unsurebow-unsure-bow-shoreside.xmoos"
        MMOD="crossing_port_standon_band350_unsure_bow_pass"
    elif [ "$CASE_NAME" = "standon_band350_bow_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-unsurebow-bow-shoreside.xmoos"
        MMOD="crossing_port_standon_band350_bow_pass"
    elif [ "$CASE_NAME" = "standon_band315_unsure_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-unsurebow-unsure-shoreside.xmoos"
        MMOD="crossing_port_standon_band315_unsure_pass"
    elif [ "$CASE_NAME" = "standon_band315_unsure_bow_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-unsurebow-unsure-bow-shoreside.xmoos"
        MMOD="crossing_port_standon_band315_unsure_bow_pass"
    elif [ "$CASE_NAME" = "standon_band315_bow_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-unsurebow-bow-shoreside.xmoos"
        MMOD="crossing_port_standon_band315_bow_pass"
    elif [ "$CASE_NAME" = "standon_southwest_unsurebow_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-unsurebow-unsure-bow-shoreside.xmoos"
        MMOD="crossing_port_standon_southwest_unsure_bow_pass"
    elif [ "$CASE_NAME" = "standon_southwest_unsure_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-unsurebow-unsure-shoreside.xmoos"
        MMOD="crossing_port_standon_southwest_unsure_pass"
    elif [ "$CASE_NAME" = "standon_southwest_stern_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-stern-shoreside.xmoos"
        MMOD="crossing_port_standon_southwest_stern_pass"
    elif [ "$CASE_NAME" = "outer_dist_below_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/outer-dist-below-shoreside.xmoos"
        MMOD="crossing_port_standon_southwest_outer_below_pass"
    elif [ "$CASE_NAME" = "outer_dist_edge_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/outer-dist-below-shoreside.xmoos"
        MMOD="crossing_port_standon_southwest_outer_edge_pass"
    elif [ "$CASE_NAME" = "outer_dist_above_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/outer-dist-above-shoreside.xmoos"
        MMOD="crossing_port_standon_southwest_outer_above_pass"
    elif [ "$CASE_NAME" = "standon_neither_below_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-neither-below-shoreside.xmoos"
        MMOD="crossing_port_standon_southwest_neither_pass"
    elif [ "$CASE_NAME" = "standon_neither_edge_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-neither-edge-shoreside.xmoos"
        MMOD="crossing_port_standon_neither_heading135_edge_pass"
    elif [ "$CASE_NAME" = "standon_neither_above_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-neither-above-shoreside.xmoos"
        MMOD="crossing_port_standon_neither_heading135_above_pass"
    elif [ "$CASE_NAME" = "standon_inextremis_range_below_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-inextremis-below-shoreside.xmoos"
        MMOD="crossing_port_standon_stern_pass"
    elif [ "$CASE_NAME" = "standon_inextremis_range_edge_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-inextremis-edge-shoreside.xmoos"
        MMOD="crossing_port_standon_turn_unsure_stern_edge_pass"
    elif [ "$CASE_NAME" = "standon_inextremis_range_above_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-inextremis-edge-shoreside.xmoos"
        MMOD="crossing_port_standon_turn_unsure_stern_above_pass"
    elif [ "$CASE_NAME" = "standon_inextremis_cpa_below_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-inextremis-below-shoreside.xmoos"
        MMOD="crossing_port_standon_stern_pass"
    elif [ "$CASE_NAME" = "standon_inextremis_cpa_edge_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-inextremis-edge-shoreside.xmoos"
        MMOD="crossing_port_standon_cpa_edge_pass"
    elif [ "$CASE_NAME" = "standon_inextremis_cpa_above_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standon-inextremis-edge-shoreside.xmoos"
        MMOD="crossing_port_standon_cpa_above_pass"
    elif [ "$CASE_NAME" = "standon_ot_inextremis_range_below_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standonot-inextremis-below-shoreside.xmoos"
        MMOD="overtaken_port_standon_range_far_pass"
    elif [ "$CASE_NAME" = "standon_ot_inextremis_range_edge_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standonot-inextremis-edge-shoreside.xmoos"
        MMOD="overtaken_port_standon_range_edge_pass"
    elif [ "$CASE_NAME" = "standon_ot_inextremis_range_above_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standonot-inextremis-edge-shoreside.xmoos"
        MMOD="overtaken_port_standon_range_close_pass"
    elif [ "$CASE_NAME" = "standon_ot_inextremis_cpa_below_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standonot-inextremis-below-shoreside.xmoos"
        MMOD="overtaken_port_standon_cpa_wide_pass"
    elif [ "$CASE_NAME" = "standon_ot_inextremis_cpa_edge_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standonot-inextremis-edge-shoreside.xmoos"
        MMOD="overtaken_port_standon_cpa_edge_pass"
    elif [ "$CASE_NAME" = "standon_ot_inextremis_cpa_above_pass" ]; then
        SHORE_PATCH="$HARNESS_DIR/standonot-inextremis-edge-shoreside.xmoos"
        MMOD="overtaken_port_standon_cpa_above_pass"
    else
        echo "$ME: Unknown case [$CASE_NAME]"
        exit 2
    fi
}

run_case() {
    local case_name="$1"
    local line actual status
    local launch_rc
    local cleanup_note=""

    get_case_config "$case_name"

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1 || true
    clear_xfiles
    nspatch --stem="$SHORE_STEM" "$SHORE_PATCH" --targ="$SHORE_XFILE"
    : > results.txt
    xlaunch.sh --max_time=$MAX_TIME --mmod=$MMOD ${MIN_UTIL_CPA:+--min_util_cpa=$MIN_UTIL_CPA} ${MAX_UTIL_CPA:+--max_util_cpa=$MAX_UTIL_CPA} --nogui ${JUST_MAKE:+--just_make} ${VERBOSE:+--verbose} $TIME_WARP
    launch_rc=$?

    if [ "$JUST_MAKE" = "yes" ]; then
        clear_xfiles
        cd "$HARNESS_DIR"
        return 0
    fi

    line=$(wait_for_result_line results.txt 32)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    if [ "$actual" = "" ]; then
        actual="missing"
    fi
    status="ok"
    if [ "$actual" = "missing" ]; then
        status="error"
        if [ "$launch_rc" != "0" ]; then
            actual="script_error"
        fi
        ALL_OK="no"
    elif [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
        ALL_OK="no"
    fi

    if ! stop_case_runtime "" 24 9000 9001 9002 9200 9201 9202; then
        cleanup_note=" cleanup=failed"
        ALL_OK="no"
    fi

    if [ "$launch_rc" != "0" ]; then
        echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  launch_rc=$launch_rc  $line$cleanup_note" >> "$RESULTS_FILE"
    else
        echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  $line$cleanup_note" >> "$RESULTS_FILE"
    fi
    clear_xfiles
    cd "$HARNESS_DIR"
}

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    local case_tag
    local case_dir
    local case_result_file
    local shore_mport
    local veh_mport
    local shore_pshare
    local veh_pshare
    local line
    local actual
    local status
    local launch_rc
    local cleanup_note=""

    get_case_config "$case_name"
    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    case_result_file="$CASE_RESULT_DIR/${case_tag}.txt"

    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  expected=$EXPECTED  actual=script_error  status=error" > "$case_result_file"
        return 1
    }

    shore_mport=$((PORT_BASE + case_idx*20))
    veh_mport=$((shore_mport + 1))
    shore_pshare=$((PORT_BASE + 200 + case_idx*20))
    veh_pshare=$((shore_pshare + 1))

    (
        cd "$case_dir"
        : > results.txt
        xlaunch.sh --max_time=$MAX_TIME --mmod=$MMOD ${MIN_UTIL_CPA:+--min_util_cpa=$MIN_UTIL_CPA} ${MAX_UTIL_CPA:+--max_util_cpa=$MAX_UTIL_CPA} --shore_mport=$shore_mport --veh_mport=$veh_mport --shore_pshare=$shore_pshare --veh_pshare=$veh_pshare --nogui ${JUST_MAKE:+--just_make} ${VERBOSE:+--verbose} $TIME_WARP
    ) &
    runtime_pid=$!

    if [ "$JUST_MAKE" = "yes" ]; then
        wait "$runtime_pid"
        launch_rc=$?
        if [ "$launch_rc" = "0" ]; then
            echo "case=$case_name  expected=just_make  actual=just_make  status=ok" > "$case_result_file"
            return 0
        fi
        echo "case=$case_name  expected=just_make  actual=script_error  status=error" > "$case_result_file"
        return 1
    fi

    line=$(wait_for_result_or_exit "$case_dir/results.txt" "$runtime_pid")
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    if [ "$actual" = "" ]; then
        actual="missing"
    fi
    status="ok"
    if [ "$actual" = "missing" ]; then
        status="error"
        if [ "$launch_rc" != "0" ]; then
            actual="script_error"
        fi
    elif [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
    fi

    forced_stop="no"
    if kill -0 "$runtime_pid" >/dev/null 2>&1; then
        forced_stop="yes"
    fi

    if ! stop_case_runtime "$runtime_pid" 24 "$shore_mport" "$veh_mport" "$((veh_mport+1))" \
                           "$shore_pshare" "$veh_pshare" "$((veh_pshare+1))"; then
        cleanup_note=" cleanup=failed"
    fi
    wait "$runtime_pid"
    launch_rc=$?
    if [ "$forced_stop" = "yes" ] && [ "$actual" != "missing" ]; then
        launch_rc=0
    fi

    if [ "$launch_rc" != 0 ]; then
        echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  launch_rc=$launch_rc  $line$cleanup_note" > "$case_result_file"
    else
        echo "case=$case_name  expected=$EXPECTED  actual=$actual  status=$status  $line$cleanup_note" > "$case_result_file"
    fi

    [ "$status" = "ok" ] && [ "$cleanup_note" = "" ]
}

HEADON_CASES="head_on_thresh_below_pass head_on_thresh_edge_pass head_on_thresh_above_pass head_on_thresh_below_mirror_pass head_on_thresh_edge_mirror_pass head_on_thresh_above_mirror_pass"
OVERTAKING_CASES="overtaking_thresh_below_pass overtaking_thresh_edge_pass overtaking_thresh_above_pass overtaking_thresh_below_mirror_pass overtaking_thresh_edge_mirror_pass overtaking_thresh_above_mirror_pass"
OVERTAKEN_CASES="overtaken_thresh_below_pass overtaken_thresh_edge_pass overtaken_thresh_above_pass"
OVERTAKEN_MIRROR_CASES="overtaken_thresh_below_mirror_pass overtaken_thresh_edge_mirror_pass overtaken_thresh_above_mirror_pass"
GIVEWAY_CASES="giveway_bowdist_below_pass giveway_bowdist_edge_pass giveway_bowdist_above_pass giveway_bowdist_below_mirror_pass giveway_bowdist_edge_mirror_pass giveway_bowdist_above_mirror_pass"
TURNGAP_CASES="giveway_turngap_below_pass giveway_turngap_edge_pass giveway_turngap_above_pass giveway_turngap_below_mirror_pass giveway_turngap_edge_mirror_pass giveway_turngap_above_mirror_pass"
STANDON_CASES="standon_unsurebow_below_pass standon_unsurebow_edge_pass standon_unsurebow_above_pass standon_band270_stern_pass standon_band350_unsurebow_pass standon_band350_unsurebow_alt_pass standon_band350_bow_pass standon_band315_unsure_pass standon_band315_unsure_bow_pass standon_band315_bow_pass standon_southwest_unsurebow_pass standon_southwest_unsure_pass standon_southwest_stern_pass"
OUTER_DIST_CASES="outer_dist_below_pass outer_dist_edge_pass outer_dist_above_pass"
INEXTREMIS_CASES="standon_inextremis_range_below_pass standon_inextremis_range_edge_pass standon_inextremis_range_above_pass standon_inextremis_cpa_below_pass standon_inextremis_cpa_edge_pass standon_inextremis_cpa_above_pass standon_ot_inextremis_range_below_pass standon_ot_inextremis_range_edge_pass standon_ot_inextremis_range_above_pass standon_ot_inextremis_cpa_below_pass standon_ot_inextremis_cpa_edge_pass standon_ot_inextremis_cpa_above_pass"

if [ "$CASE" != "" ]; then
    CASES="$CASE"
else
    if [ "$GROUP" = "" ] || [ "$GROUP" = "all" ]; then
        CASES="$HEADON_CASES $OVERTAKING_CASES $OVERTAKEN_CASES $OVERTAKEN_MIRROR_CASES $GIVEWAY_CASES $TURNGAP_CASES $STANDON_CASES $OUTER_DIST_CASES $INEXTREMIS_CASES"
    elif [ "$GROUP" = "headon" ]; then
        CASES="$HEADON_CASES"
    elif [ "$GROUP" = "overtaking" ]; then
        CASES="$OVERTAKING_CASES"
    elif [ "$GROUP" = "overtaken" ]; then
        CASES="$OVERTAKEN_CASES"
    elif [ "$GROUP" = "overtaken_mirror" ]; then
        CASES="$OVERTAKEN_MIRROR_CASES"
    elif [ "$GROUP" = "giveway" ]; then
        CASES="$GIVEWAY_CASES"
    elif [ "$GROUP" = "turngap" ]; then
        CASES="$TURNGAP_CASES"
    elif [ "$GROUP" = "standon" ]; then
        CASES="$STANDON_CASES"
    elif [ "$GROUP" = "outer_dist" ]; then
        CASES="$OUTER_DIST_CASES"
    elif [ "$GROUP" = "inextremis" ]; then
        CASES="$INEXTREMIS_CASES"
    else
        echo "$ME: Unknown group [$GROUP]"
        exit 1
    fi
fi

: > "$RESULTS_FILE"
trap cleanup EXIT

ktm >/dev/null 2>&1 || true

if [ "$JOBS" -le 1 ]; then
    for ONE_CASE in $CASES; do
        run_case "$ONE_CASE" || ALL_OK="no"
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_colregs_thresholds_XXXXXX")
    CASE_RESULT_DIR="$RUN_ROOT/case_results"
    mkdir -p "$CASE_RESULT_DIR"

    case_idx=0
    wave_pids=""
    wave_count=0
    for ONE_CASE in $CASES; do
        if case_requires_solo_wave "$ONE_CASE"; then
	            if [ "$wave_pids" != "" ]; then
	                for pid in $wave_pids; do
	                    wait "$pid" || ALL_OK="no"
	                done
	                wave_pids=""
	                wave_count=0
	            fi
	
	            run_case_isolated "$case_idx" "$ONE_CASE" || ALL_OK="no"
	            case_idx=$((case_idx + 1))
	            continue
	        fi

        run_case_isolated "$case_idx" "$ONE_CASE" &
        wave_pids="$wave_pids $!"
        wave_count=$((wave_count + 1))
        case_idx=$((case_idx + 1))

	        if [ "$wave_count" -ge "$JOBS" ]; then
	            for pid in $wave_pids; do
	                wait "$pid" || ALL_OK="no"
	            done
	            wave_pids=""
	            wave_count=0
	        fi
	    done
	
	    if [ "$wave_pids" != "" ]; then
	        for pid in $wave_pids; do
	            wait "$pid" || ALL_OK="no"
	        done
	    fi

    for ONE_CASE in $CASES; do
        case_file=$(find "$CASE_RESULT_DIR" -maxdepth 1 -type f -name "*_${ONE_CASE}.txt" | sort | head -n 1)
        if [ "$case_file" = "" ]; then
            echo "case=$ONE_CASE  expected=unknown  actual=missing  status=error" >> "$RESULTS_FILE"
            ALL_OK="no"
        else
            cat "$case_file" >> "$RESULTS_FILE"
            grep -q " status=ok " "$case_file" || ALL_OK="no"
        fi
    done
fi

[ "$ALL_OK" = "yes" ]
