#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-uquerydb_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; cleanup; exit 1; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

ME=`basename "$0"`
TIME_WARP="10"
MAX_TIME="30"
JOBS="1"
PORT_BASE="15600"
PORT_STRIDE="20"
CASE=""
JUST_MAKE="no"
KEEP_WORKDIRS="no"
VERBOSE=""

HARNESS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/uquerydb_missions/uquerydb_unit"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
RUN_ROOT=""
CASE_ROW_DIR=""
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
    cli_numeric_pass
    cli_string_pass
    cli_boolean_pass
    cli_negative_numeric_pass
    multiple_pass_conditions_pass
    fail_condition_false_pass
    fail_condition_true_fail
    missing_var_timeout_fail
    late_var_wait_pass
    mission_file_config_pass
    checkvar_esv_pass
    checkvar_csv_pass
    checkvar_wsv_pass
    checkvar_value_only_pass
    unique_name_pass
    compound_or_pass
    pass_condition_alias_pass
    compound_and_pass
    compound_or_fail
    config_halt_max_time_timeout_fail
    config_fail_condition_false_pass
    config_fail_condition_true_fail
    checkvar_multi_csv_pass
    checkvar_timeout_esv_pass
    case_config_checkvar_csv_pass
    fail_only_false_pass
    fail_only_true_fail
    numeric_greater_than_pass
    numeric_less_than_fail
    config_condition_alias_pass
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

get_case_config() {
    CASE_NAME="$1"
    local compound_condition
    EXPECTED_RC="0"
    QUERY_FILE_MODE="connect"
    QUERY_ARGS=()
    CONFIG_LINES=()
    CHECKVAR_TOKENS=()
    ABSENT_TOKENS=()

    if ! case_known "$CASE_NAME"; then
        echo "$ME: Unknown case: $CASE_NAME"
        return 1
    fi

    case "$CASE_NAME" in
        cli_numeric_pass)
            QUERY_ARGS=("--condition=QUERY_NUM = 42" "--wait=3")
            ;;
        cli_string_pass)
            QUERY_ARGS=("--condition=QUERY_STR = ready" "--wait=3")
            ;;
        cli_boolean_pass)
            QUERY_ARGS=("--condition=QUERY_BOOL = true" "--wait=3")
            ;;
        cli_negative_numeric_pass)
            QUERY_ARGS=("--condition=QUERY_NEG = -7.5" "--wait=3")
            ;;
        multiple_pass_conditions_pass)
            QUERY_ARGS=("--condition=QUERY_NUM = 42" "--condition=QUERY_STR = ready" "--wait=3")
            ;;
        fail_condition_false_pass)
            QUERY_ARGS=("--condition=QUERY_NUM = 42" "--fail_condition=QUERY_FAIL = true" "--wait=3")
            ;;
        fail_condition_true_fail)
            EXPECTED_RC="1"
            QUERY_ARGS=("--condition=QUERY_NUM = 42" "--fail_condition=QUERY_FAIL = false" "--wait=3")
            ;;
        missing_var_timeout_fail)
            EXPECTED_RC="1"
            QUERY_ARGS=("--condition=QUERY_MISSING = ready" "--wait=1")
            ;;
        late_var_wait_pass)
            QUERY_ARGS=("--condition=QUERY_LATE = arrived" "--wait=3")
            ;;
        mission_file_config_pass)
            QUERY_FILE_MODE="mission_config"
            QUERY_ARGS=()
            CHECKVAR_TOKENS=("QUERY_NUM=42")
            ;;
        checkvar_esv_pass)
            QUERY_ARGS=("--condition=QUERY_NUM = 42" "--check_var=QUERY_NUM" "--esv" "--wait=3")
            CHECKVAR_TOKENS=("QUERY_NUM=42")
            ;;
        checkvar_csv_pass)
            QUERY_ARGS=("--condition=QUERY_STR = ready" "--check_var=QUERY_STR" "--csv" "--wait=3")
            CHECKVAR_TOKENS=("QUERY_STR,ready")
            ;;
        checkvar_wsv_pass)
            QUERY_ARGS=("--condition=QUERY_OTHER = bravo" "--check_var=QUERY_OTHER" "--wsv" "--wait=3")
            CHECKVAR_TOKENS=("QUERY_OTHER bravo")
            ;;
        checkvar_value_only_pass)
            QUERY_FILE_MODE="case_config"
            CONFIG_LINES=("pass_condition = QUERY_BOOL = true" "check_var = QUERY_BOOL" "check_var_format = vo" "wait = 3")
            CHECKVAR_TOKENS=("true")
            ABSENT_TOKENS=("QUERY_BOOL=" "QUERY_BOOL,true" "QUERY_BOOL true")
            ;;
        unique_name_pass)
            QUERY_ARGS=("--unique" "--condition=QUERY_NUM = 42" "--wait=3")
            ;;
        compound_or_pass)
            compound_condition="--condition=(QUERY_STR = not_ready) or (QUERY_OTHER = bravo)"
            QUERY_ARGS=("$compound_condition" "--wait=3")
            ;;
        pass_condition_alias_pass)
            QUERY_ARGS=("--pass_condition=QUERY_STR = ready" "--wait=3")
            ;;
        compound_and_pass)
            compound_condition="--condition=(QUERY_NUM = 42) and (QUERY_STR = ready)"
            QUERY_ARGS=("$compound_condition" "--wait=3")
            ;;
        compound_or_fail)
            EXPECTED_RC="1"
            compound_condition="--condition=(QUERY_STR = not_ready) or (QUERY_OTHER = not_bravo)"
            QUERY_ARGS=("$compound_condition" "--wait=1")
            ;;
        config_halt_max_time_timeout_fail)
            EXPECTED_RC="1"
            QUERY_FILE_MODE="case_config"
            CONFIG_LINES=("halt_max_time = 1" "pass_condition = QUERY_MISSING = ready")
            ;;
        config_fail_condition_false_pass)
            QUERY_FILE_MODE="case_config"
            CONFIG_LINES=("wait = 3" "pass_condition = QUERY_NUM = 42" "fail_condition = QUERY_FAIL = true")
            ;;
        config_fail_condition_true_fail)
            EXPECTED_RC="1"
            QUERY_FILE_MODE="case_config"
            CONFIG_LINES=("wait = 3" "pass_condition = QUERY_NUM = 42" "fail_condition = QUERY_FAIL = false")
            ;;
        checkvar_multi_csv_pass)
            QUERY_ARGS=("--condition=QUERY_NUM = 42" "--check_var=QUERY_NUM" "--check_var=QUERY_STR" "--csv" "--wait=3")
            CHECKVAR_TOKENS=("QUERY_NUM,42" "QUERY_STR,ready")
            ;;
        checkvar_timeout_esv_pass)
            EXPECTED_RC="1"
            QUERY_ARGS=("--condition=QUERY_MISSING = ready" "--check_var=QUERY_NUM" "--esv" "--wait=1")
            CHECKVAR_TOKENS=("QUERY_NUM=42")
            ;;
        case_config_checkvar_csv_pass)
            QUERY_FILE_MODE="case_config"
            CONFIG_LINES=("wait = 3" "pass_condition = QUERY_OTHER = bravo" "check_var = QUERY_OTHER" "check_var_format = csv")
            CHECKVAR_TOKENS=("QUERY_OTHER,bravo")
            ;;
        fail_only_false_pass)
            QUERY_ARGS=("--fail_condition=QUERY_FAIL = true" "--wait=3")
            ;;
        fail_only_true_fail)
            EXPECTED_RC="1"
            QUERY_ARGS=("--fail_condition=QUERY_FAIL = false" "--wait=3")
            ;;
        numeric_greater_than_pass)
            QUERY_ARGS=("--condition=QUERY_NUM > 41" "--wait=3")
            ;;
        numeric_less_than_fail)
            EXPECTED_RC="1"
            QUERY_ARGS=("--condition=QUERY_NUM < 41" "--wait=1")
            ;;
        config_condition_alias_pass)
            QUERY_FILE_MODE="case_config"
            CONFIG_LINES=("wait = 3" "condition = QUERY_BOOL = true")
            ;;
    esac
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

run_querydb() {
    local case_dir="$1"
    local port="$2"
    local output="$case_dir/querydb.out"
    local connect_file="$case_dir/query_connect.moos"
    local config_file="$case_dir/query_config.moos"
    local query_pid
    local deadline
    local line
    local cmd=()

    if [ "$QUERY_FILE_MODE" = "mission_config" ]; then
        cmd=("uQueryDB" "$case_dir/targ_shoreside.moos" "${QUERY_ARGS[@]}")
    elif [ "$QUERY_FILE_MODE" = "case_config" ]; then
        {
            echo "ServerHost = localhost"
            echo "ServerPort = $port"
            echo "Community  = query"
            echo ""
            echo "ProcessConfig = uQueryDB"
            echo "{"
            echo "  AppTick   = 4"
            echo "  CommsTick = 4"
            for line in "${CONFIG_LINES[@]}"; do
                echo "  $line"
            done
            echo "}"
        } > "$config_file"
        cmd=("uQueryDB" "$config_file" "${QUERY_ARGS[@]}")
    else
        {
            echo "ServerHost = localhost"
            echo "ServerPort = $port"
            echo "Community  = query"
        } > "$connect_file"
        cmd=("uQueryDB" "$connect_file" "${QUERY_ARGS[@]}")
    fi

    (
        cd "$case_dir"
        "${cmd[@]}" > "$output" 2>&1 &
        query_pid=$!
        deadline=$((SECONDS + 8))
        while kill -0 "$query_pid" 2>/dev/null; do
            if [ "$SECONDS" -ge "$deadline" ]; then
                kill "$query_pid" 2>/dev/null || true
                wait "$query_pid" 2>/dev/null || true
                exit 124
            fi
            sleep 0.1
        done
        wait "$query_pid"
    )
}


emit_case_row() {
    local case_name="$1"
    local status="$2"
    local expected="$3"
    local actual="$4"
    shift 4
    local line="$1"
    shift || true
    local grade

    grade=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    line=$(echo "$line" | sed 's/grade=[^, ]*[[:space:]]*//')

    if [ "$grade" != "" ]; then
        echo "case=$case_name  grade=$grade  $line  $*"
    elif [ "$status" = "success" ]; then
        echo "case=$case_name  grade=fail  reason=missing_result  $line  $*"
    else
        echo "case=$case_name  grade=fail  reason=$status  $line  $*"
    fi
}

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    local case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    local shore_mport=$((case_base + 0))
    local shore_pshare=$((case_base + 10))
    local tag
    local case_dir
    local case_row_file
    local line
    local readiness
    local query_rc=0
    local status="success"
    local check

    get_case_config "$case_name" || return 1

    tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$tag"
    case_row_file="$CASE_ROW_DIR/$tag.txt"
    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR"/. "$case_dir"/

    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
        ./launch.sh --just_make --xlaunched --nogui --shore_mport="$shore_mport" --shore_pshare="$shore_pshare" --mmod="$case_name" "$TIME_WARP" >/dev/null
    )
    if [ "$JUST_MAKE" = "yes" ]; then
        echo "case=$case_name  grade=pass  reason=just_make" > "$case_row_file"
        return 0
    fi

    (
        cd "$case_dir"
        : > results.txt
        ./launch.sh --xlaunched --nogui --shore_mport="$shore_mport" --shore_pshare="$shore_pshare" --mmod="$case_name" "$TIME_WARP" > launch.out 2>&1
    )
    sleep 0.25
    run_querydb "$case_dir" "$shore_mport"
    query_rc=$?
    (
        cd "$case_dir"
        uMayFinish --max_time="$MAX_TIME" targ_shoreside.moos > mayfinish.out 2>&1 || true
    )
    line=$(wait_for_results "$case_dir/results.txt" 120)
    readiness=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    if [ "$readiness" = "" ]; then
        readiness="missing"
    fi

    if [ "$query_rc" != "$EXPECTED_RC" ] || [ "$readiness" != "pass" ]; then
        status="mismatch"
    fi
    for check in "${CHECKVAR_TOKENS[@]}"; do
        if ! grep -q "$check" "$case_dir/.checkvars" 2>/dev/null; then
            status="mismatch"
        fi
    done
    for check in "${ABSENT_TOKENS[@]}"; do
        if grep -q "$check" "$case_dir/.checkvars" 2>/dev/null; then
            status="mismatch"
        fi
    done

    emit_case_row "$case_name" "$status" "rc_$EXPECTED_RC" "$query_rc" "$line" "expected_rc=$EXPECTED_RC" "actual_rc=$query_rc" "readiness=$readiness" "checkvars=${#CHECKVAR_TOKENS[@]}" "absent=${#ABSENT_TOKENS[@]}" > "$case_row_file"
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
RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_uquerydb_unit_XXXXXX")
CASE_ROW_DIR="$RUN_ROOT/case_rows"
mkdir -p "$CASE_ROW_DIR"

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
        if [ -f "$CASE_ROW_DIR/$case_tag.txt" ]; then
            cat "$CASE_ROW_DIR/$case_tag.txt" >> "$RESULTS_FILE"
        else
            echo "case=${case_tag#*_}  grade=fail  reason=missing_result" >> "$RESULTS_FILE"
            ALL_OK="no"
        fi
    done

    remaining_cases="$next_remaining"
done

if grep -Eq '(^|[[:space:]])grade=fail([[:space:]]|$)' "$RESULTS_FILE"; then
    ALL_OK="no"
fi

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
