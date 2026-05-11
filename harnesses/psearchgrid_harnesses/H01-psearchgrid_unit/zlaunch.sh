#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-psearchgrid_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
on_exit() { echo; echo "$ME: Halting all apps"; cleanup; exit 1; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

ME=`basename "$0"`
TIME_WARP="10"
MAX_TIME="30"
JOBS="1"
PORT_BASE="16400"
PORT_STRIDE="20"
CASE=""
JUST_MAKE="no"
KEEP_WORKDIRS="no"
VERBOSE=""

HARNESS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/psearchgrid_missions/psearchgrid_unit"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
RUN_ROOT=""
CASE_RESULT_DIR=""
ALL_OK="yes"

if [ -f "$TEARDOWN_HELPER" ]; then
    . "$TEARDOWN_HELPER"
else
    echo "$ME: Missing teardown helper: $TEARDOWN_HELPER"
    exit 1
fi

usage() {
    echo "$ME [OPTIONS] [time_warp]"
    echo ""
    echo "Options:"
    echo "  --help, -h           Show this help message"
    echo "  --case=<name>        Run one named case"
    echo "  --jobs=<n>           Run up to n cases per wave"
    echo "  --port_base=<n>      Base MOOSDB port for per-case blocks"
    echo "  --max_time=<secs>    Max time passed to uMayFinish"
    echo "  --just_make, -j      Generate target files only"
    echo "  --keep_workdirs      Keep temp mission copies"
    echo "  --verbose, -v        Verbose output"
}

for ARGI; do
    if [ "$ARGI" = "--help" ] || [ "$ARGI" = "-h" ]; then
        usage
        exit 0
    elif [ "${ARGI//[^0-9]/}" = "$ARGI" ] && [ "$TIME_WARP" = 10 ]; then
        TIME_WARP="$ARGI"
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
    elif [ "$ARGI" = "--gui" ] || [ "$ARGI" = "--nogui" ] || [ "$ARGI" = "-ng" ]; then
        true
    else
        echo "$ME: Bad arg: $ARGI"
        exit 1
    fi
done

if ! echo "$JOBS" | grep -Eq '^[0-9]+$' || [ "$JOBS" -lt 1 ]; then
    echo "$ME: Bad --jobs value: $JOBS"
    exit 1
fi
if ! echo "$PORT_BASE" | grep -Eq '^[0-9]+$'; then
    echo "$ME: Bad --port_base value: $PORT_BASE"
    exit 1
fi

ALL_CASES=(
    initial_grid_publish_pass
    node_local_delta_pass
    node_global_delta_pass
    boundary_origin_delta_pass
    outside_grid_no_delta_pass
    negative_outside_no_delta_pass
    full_grid_report_pass
    custom_var_label_pass
    lowercase_var_name_pass
    custom_full_grid_var_pass
    invalid_report_deltas_default_delta_pass
    reset_grid_clears_pass
    reset_false_payload_clears_pass
    two_cell_delta_pass
    repeated_cell_delta_pass
    sequential_delta_cleared_pass
    match_community_allows_pass
    match_list_allows_pass
    match_community_blocks_pass
    ignore_other_allows_pass
    ignore_community_blocks_pass
    ignore_list_blocks_pass
    malformed_report_absent_pass
)

cleanup() {
    if [ "$RUN_ROOT" != "" ]; then
        harness_teardown_stop_root "$RUN_ROOT"
    fi
    if [ "$KEEP_WORKDIRS" != "yes" ] && [ "$RUN_ROOT" != "" ] && [ -d "$RUN_ROOT" ]; then
        rm -rf "$RUN_ROOT"
    fi
}

case_known() {
    local wanted="$1"
    local one
    for one in "${ALL_CASES[@]}"; do
        if [ "$one" = "$wanted" ]; then
            return 0
        fi
    done
    return 1
}

base_node_report() {
    local name="$1"
    local x="$2"
    local y="$3"
    echo "NAME=$name,X=$x,Y=$y,SPD=0,HDG=90,TYPE=kayak,TIME=1,LENGTH=5"
}

get_case_config() {
    CASE_NAME="$1"
    grid_lines=("report_deltas = true" "grid_label = psg" "grid_var_name = VIEW_GRID")
    grid_config_lines=("grid_config = pts={0,0:20,0:20,20:0,20}" "grid_config = cell_size=10" "grid_config = cell_vars=x:0")
    timer_events=()
    required_var_tokens=()
    required_var_count_tokens=()
    absent_var_tokens=()
    absent_vars=()
    absent_after_marker=""
    absent_after_time="0.2"
    absent_after_var_tokens=()
    EVAL_TIME="1.8"

    if ! case_known "$CASE_NAME"; then
        echo "$ME: Unknown case: $CASE_NAME"
        return 1
    fi

    case "$CASE_NAME" in
        initial_grid_publish_pass)
            required_var_tokens=("VIEW_GRID:pts={" "VIEW_GRID:cell_size=10" "VIEW_GRID:cell_vars=x:0" "VIEW_GRID:label=psg")
            ;;
        node_local_delta_pass)
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report abe 5 5)\", time=0.4")
            required_var_tokens=("VIEW_GRID_DELTA:psg@" "VIEW_GRID_DELTA:,x,1")
            ;;
        node_global_delta_pass)
            timer_events=("event = var=NODE_REPORT, val=\"$(base_node_report ben 15 5)\", time=0.4")
            required_var_tokens=("VIEW_GRID_DELTA:psg@" "VIEW_GRID_DELTA:,x,1")
            ;;
        boundary_origin_delta_pass)
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report edge 0 0)\", time=0.4")
            required_var_tokens=("VIEW_GRID_DELTA:psg@" "VIEW_GRID_DELTA:,x,1")
            ;;
        outside_grid_no_delta_pass)
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report far 50 50)\", time=0.4")
            absent_vars=("VIEW_GRID_DELTA")
            ;;
        negative_outside_no_delta_pass)
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report neg -1 -1)\", time=0.4")
            absent_vars=("VIEW_GRID_DELTA")
            ;;
        full_grid_report_pass)
            grid_lines=("report_deltas = false" "grid_label = psg" "grid_var_name = VIEW_GRID")
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report full 5 5)\", time=0.4")
            required_var_tokens=("VIEW_GRID:cell=" "VIEW_GRID:cell=0:x:1")
            ;;
        custom_var_label_pass)
            grid_lines=("report_deltas = true" "grid_label = custom_psg" "grid_var_name = SEARCH_GRID")
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report custom 5 5)\", time=0.4")
            required_var_tokens=("SEARCH_GRID:label=psg" "SEARCH_GRID_DELTA:custom_psg@" "SEARCH_GRID_DELTA:,x,1")
            absent_vars=("VIEW_GRID_DELTA")
            ;;
        lowercase_var_name_pass)
            grid_lines=("report_deltas = true" "grid_label = lower_psg" "grid_var_name = search_grid")
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report lower 5 5)\", time=0.4")
            required_var_tokens=("SEARCH_GRID:label=psg" "SEARCH_GRID_DELTA:lower_psg@" "SEARCH_GRID_DELTA:,x,1")
            absent_vars=("VIEW_GRID_DELTA")
            ;;
        custom_full_grid_var_pass)
            grid_lines=("report_deltas = false" "grid_label = custom_full" "grid_var_name = search_grid")
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report customfull 5 5)\", time=0.4")
            required_var_tokens=("SEARCH_GRID:cell=0:x:1")
            absent_vars=("VIEW_GRID")
            ;;
        invalid_report_deltas_default_delta_pass)
            grid_lines=("report_deltas = maybe" "grid_label = psg" "grid_var_name = VIEW_GRID")
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report defaultdelta 5 5)\", time=0.4")
            required_var_tokens=("VIEW_GRID_DELTA:psg@" "VIEW_GRID_DELTA:,x,1")
            ;;
        reset_grid_clears_pass)
            grid_lines=("report_deltas = false" "grid_label = psg" "grid_var_name = VIEW_GRID")
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report reset 5 5)\", time=0.4" "event = var=PSG_RESET_GRID, val=true, time=0.9")
            required_var_tokens=("VIEW_GRID:cell=0:x:1")
            absent_after_marker="PSG_RESET_GRID"
            absent_after_var_tokens=("VIEW_GRID:cell=0:x:1")
            EVAL_TIME="1.6"
            ;;
        reset_false_payload_clears_pass)
            grid_lines=("report_deltas = false" "grid_label = psg" "grid_var_name = VIEW_GRID")
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report resetfalse 5 5)\", time=0.4" "event = var=PSG_RESET_GRID, val=false, time=0.9")
            required_var_tokens=("VIEW_GRID:cell=0:x:1")
            absent_after_marker="PSG_RESET_GRID"
            absent_after_var_tokens=("VIEW_GRID:cell=0:x:1")
            EVAL_TIME="1.6"
            ;;
        two_cell_delta_pass)
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report one 5 5)\", time=0.4" "event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report two 15 5)\", time=0.4")
            required_var_tokens=("VIEW_GRID_DELTA:psg@" "VIEW_GRID_DELTA:,x,1")
            ;;
        repeated_cell_delta_pass)
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report one 5 5)\", time=0.4" "event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report two 5 5)\", time=0.4")
            required_var_tokens=("VIEW_GRID_DELTA:psg@" "VIEW_GRID_DELTA:,x,2")
            ;;
        sequential_delta_cleared_pass)
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report one 5 5)\", time=0.4" "event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report two 5 5)\", time=1.0")
            required_var_count_tokens=("VIEW_GRID_DELTA:,x,1:2")
            absent_var_tokens=("VIEW_GRID_DELTA:,x,2")
            EVAL_TIME="1.8"
            ;;
        match_community_allows_pass)
            grid_lines=("report_deltas = true" "grid_label = psg" "grid_var_name = VIEW_GRID" "match_name = shoreside")
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report allowed 5 5)\", time=0.4")
            required_var_tokens=("VIEW_GRID_DELTA:psg@" "VIEW_GRID_DELTA:,x,1")
            ;;
        match_list_allows_pass)
            grid_lines=("report_deltas = true" "grid_label = psg" "grid_var_name = VIEW_GRID" "match_name = {vehicle,shoreside}")
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report listallowed 5 5)\", time=0.4")
            required_var_tokens=("VIEW_GRID_DELTA:psg@" "VIEW_GRID_DELTA:,x,1")
            ;;
        match_community_blocks_pass)
            grid_lines=("report_deltas = true" "grid_label = psg" "grid_var_name = VIEW_GRID" "match_name = vehicle")
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report blocked 5 5)\", time=0.4")
            absent_vars=("VIEW_GRID_DELTA")
            ;;
        ignore_other_allows_pass)
            grid_lines=("report_deltas = true" "grid_label = psg" "grid_var_name = VIEW_GRID" "ignore_name = vehicle")
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report notignored 5 5)\", time=0.4")
            required_var_tokens=("VIEW_GRID_DELTA:psg@" "VIEW_GRID_DELTA:,x,1")
            ;;
        ignore_community_blocks_pass)
            grid_lines=("report_deltas = true" "grid_label = psg" "grid_var_name = VIEW_GRID" "ignore_name = shoreside")
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report ignored 5 5)\", time=0.4")
            absent_vars=("VIEW_GRID_DELTA")
            ;;
        ignore_list_blocks_pass)
            grid_lines=("report_deltas = true" "grid_label = psg" "grid_var_name = VIEW_GRID" "ignore_name = {vehicle,shoreside}")
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"$(base_node_report listignored 5 5)\", time=0.4")
            absent_vars=("VIEW_GRID_DELTA")
            ;;
        malformed_report_absent_pass)
            timer_events=("event = var=NODE_REPORT_LOCAL, val=\"NAME=bad,X=5\", time=0.4")
            absent_vars=("VIEW_GRID_DELTA")
            ;;
    esac
}

write_case_files() {
    local case_dir="$1"
    local item
    {
        echo "ProcessConfig = pSearchGrid"
        echo "{"
        echo "  AppTick   = 10"
        echo "  CommsTick = 10"
        echo ""
        for item in "${grid_lines[@]}"; do
            echo "  $item"
        done
        for item in "${grid_config_lines[@]}"; do
            echo "  $item"
        done
        echo "}"
    } > "$case_dir/case_searchgrid.moos"

    {
        echo "ProcessConfig = uTimerScript"
        echo "{"
        echo "  AppTick   = 10"
        echo "  CommsTick = 10"
        echo ""
        for item in "${timer_events[@]}"; do
            echo "  $item"
        done
        echo "  event = var=TEST_EVAL_READY, val=true, time=$EVAL_TIME"
        echo "}"
    } > "$case_dir/case_timer.moos"

    {
        echo "ProcessConfig = pMissionEval"
        echo "{"
        echo "  AppTick   = 4"
        echo "  CommsTick = 4"
        echo ""
        echo "  lead_condition = TEST_EVAL_READY = true"
        echo "  pass_condition = TEST_EVAL_READY = true"
        echo ""
        echo "  result_flag  = MISSION_EVALUATED = true"
        echo "  mission_form = psearchgrid_unit"
        echo "  mission_mod  = $CASE_NAME"
        echo ""
        echo "  prereport_column = form=\$[MISSION_FORM]"
        echo "  prereport_column = mmod=\$[MMOD]"
        echo "  report_column    = grade=\$[GRADE]"
        echo "  report_column    = eval=\$[TEST_EVAL_READY]"
        echo "  report_column    = mhash=\$[MHASH_SHORT]"
        echo "  report_file      = results.txt"
        echo "}"
    } > "$case_dir/case_eval.moos"
}

wait_for_results() {
    local file="$1"
    local attempts="${2:-120}"
    local line=""
    local i
    for ((i=0; i<attempts; i++)); do
        line=$(tail -n 1 "$file" 2>/dev/null)
        if echo "$line" | grep -q 'grade='; then
            echo "$line"
            return 0
        fi
        sleep 0.25
    done
    echo "$line"
    return 1
}

alog_lines_for_var() {
    local case_dir="$1"
    local var="$2"
    local alog
    alog=$(find "$case_dir" -name '*.alog' | head -n 1)
    if [ "$alog" = "" ]; then
        return 1
    fi
    aloggrep "$var" "$alog" | grep "$var" | grep -v '^%' || true
}

check_payloads() {
    local case_dir="$1"
    local status="success"
    local item
    local var
    local token
    local count
    local needed
    local lines
    local alog
    local base_time
    local threshold
    local after_lines

    for item in "${required_var_tokens[@]}"; do
        var="${item%%:*}"
        token="${item#*:}"
        lines=$(alog_lines_for_var "$case_dir" "$var")
        if ! echo "$lines" | grep -q "$token"; then
            status="mismatch"
        fi
    done
    for item in "${required_var_count_tokens[@]}"; do
        var="${item%%:*}"
        token="${item#*:}"
        token="${token%:*}"
        needed="${item##*:}"
        lines=$(alog_lines_for_var "$case_dir" "$var")
        count=$(echo "$lines" | grep -c "$token" || true)
        if [ "$count" -lt "$needed" ]; then
            status="mismatch"
        fi
    done
    for item in "${absent_var_tokens[@]}"; do
        var="${item%%:*}"
        token="${item#*:}"
        lines=$(alog_lines_for_var "$case_dir" "$var")
        if echo "$lines" | grep -q "$token"; then
            status="mismatch"
        fi
    done
    for var in "${absent_vars[@]}"; do
        lines=$(alog_lines_for_var "$case_dir" "$var")
        if [ "$lines" != "" ]; then
            status="mismatch"
        fi
    done
    if [ "$absent_after_marker" != "" ]; then
        alog=$(find "$case_dir" -name '*.alog' | head -n 1)
        base_time=$(aloggrep "$absent_after_marker" "$alog" | grep "$absent_after_marker" | grep -v '^%' | head -n 1 | awk '{print $1}')
        if [ "$base_time" = "" ]; then
            status="mismatch"
        else
            threshold=$(awk -v base="$base_time" -v offset="$absent_after_time" 'BEGIN { print base + offset }')
            for item in "${absent_after_var_tokens[@]}"; do
                var="${item%%:*}"
                token="${item#*:}"
                after_lines=$(alog_lines_for_var "$case_dir" "$var" | awk -v min_time="$threshold" '$1+0 > min_time {print}')
                if echo "$after_lines" | grep -q "$token"; then
                    status="mismatch"
                fi
            done
        fi
    fi
    echo "$status"
}

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    local case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    local shore_mport=$((case_base + 0))
    local shore_pshare=$((case_base + 10))
    local tag
    local case_dir
    local case_result_file
    local line
    local actual
    local payload_status
    local status="success"

    get_case_config "$case_name" || return 1

    tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$tag"
    case_result_file="$CASE_RESULT_DIR/$tag.txt"
    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    write_case_files "$case_dir"

    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
        ./launch.sh --just_make --xlaunched --nogui --shore_mport="$shore_mport" --shore_pshare="$shore_pshare" --mmod="$case_name" "$TIME_WARP" >/dev/null
    )
    if [ "$JUST_MAKE" = "yes" ]; then
        echo "case=$case_name  case_result=success  expected=just_make  actual=just_make" > "$case_result_file"
        return 0
    fi

    (
        cd "$case_dir"
        : > results.txt
        ./launch.sh --xlaunched --nogui --shore_mport="$shore_mport" --shore_pshare="$shore_pshare" --mmod="$case_name" "$TIME_WARP" > launch.out 2>&1
        uMayFinish --max_time="$MAX_TIME" targ_shoreside.moos > mayfinish.out 2>&1 || true
    )
    line=$(wait_for_results "$case_dir/results.txt" 120)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    payload_status=$(check_payloads "$case_dir")
    if [ "$actual" != "pass" ] || [ "$payload_status" != "success" ]; then
        status="mismatch"
    fi

    echo "case=$case_name  case_result=$status  expected=pass  actual=$actual  payload=$payload_status  $line" > "$case_result_file"
    harness_teardown_stop_root "$case_dir"

    if [ "$status" = "success" ]; then
        return 0
    fi
    return 1
}

if [ ! -d "$MISSION_DIR" ]; then
    echo "$ME: Mission dir not found: $MISSION_DIR"
    exit 1
fi

RUN_CASES=("${ALL_CASES[@]}")
if [ "$CASE" != "" ]; then
    if ! case_known "$CASE"; then
        echo "$ME: Unknown case: $CASE"
        exit 1
    fi
    RUN_CASES=("$CASE")
fi

trap cleanup EXIT
: > "$RESULTS_FILE"
RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_psearchgrid_unit_XXXXXX")
CASE_RESULT_DIR="$RUN_ROOT/case_results"
mkdir -p "$CASE_RESULT_DIR"

case_index=0
remaining_cases="${RUN_CASES[*]}"
while [ "$remaining_cases" != "" ]; do
    launched=0
    pids=""
    wave_cases=""
    next_remaining=""

    for one_case in $remaining_cases; do
        if [ "$launched" -lt "$JOBS" ]; then
            run_case_isolated "$case_index" "$one_case" &
            pids="$pids $!"
            wave_cases="$wave_cases $(printf "%03d_%s" "$case_index" "$one_case")"
            case_index=$((case_index + 1))
            launched=$((launched + 1))
        else
            next_remaining="$next_remaining $one_case"
        fi
    done

    for pid in $pids; do
        wait "$pid" || ALL_OK="no"
    done

    for case_tag in $wave_cases; do
        if [ -f "$CASE_RESULT_DIR/$case_tag.txt" ]; then
            cat "$CASE_RESULT_DIR/$case_tag.txt" >> "$RESULTS_FILE"
        else
            echo "case=${case_tag#*_}  case_result=error  expected=unknown  actual=missing_result" >> "$RESULTS_FILE"
            ALL_OK="no"
        fi
    done

    remaining_cases="$next_remaining"
done

if grep -q 'case_result=mismatch\|case_result=error' "$RESULTS_FILE"; then
    ALL_OK="no"
fi

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
