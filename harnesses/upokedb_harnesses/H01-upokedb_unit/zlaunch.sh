#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-upokedb_unit
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
PORT_BASE="15000"
PORT_STRIDE="20"
CASE=""
JUST_MAKE="no"
KEEP_WORKDIRS="no"
VERBOSE=""

HARNESS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/upokedb_missions/upokedb_unit"
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
    numeric_direct_pass
    string_forced_pass
    multiple_mixed_pass
    overwrite_existing_pass
    cache_config_pass
    mission_file_connection_pass
    quiet_mode_pass
    negative_double_pass
    boolean_true_pass
    repeated_batch_pass
    mixed_numeric_string_batch_pass
    cache_string_numeric_pass
    host_alias_args_pass
    server_alias_args_pass
    server_underscore_args_pass
    quiet_short_alias_pass
    cache_short_alias_pass
    zero_double_pass
    scientific_literal_pass
    large_double_pass
    cache_forced_string_pass
    cache_boolean_pass
    utc_macro_pass
    moostime_macro_pass
    forced_utc_string_pass
    duplicate_same_invocation_pass
    cache_duplicate_first_wins_pass
    cache_cli_mixed_pass
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
    EXPECTED="pass"
    POKE_MODE="host"
    POKE_BATCHES=()
    PASS_CONDITIONS=()
    REPORT_COLUMNS=()
    CACHE_POKES=()
    CHECK_TOKENS=()
    ABSENT_TOKENS=()
    NUMERIC_GT_CHECKS=()

    if ! case_known "$CASE_NAME"; then
        echo "$ME: Unknown case: $CASE_NAME"
        return 1
    fi

    case "$CASE_NAME" in
        numeric_direct_pass)
            POKE_BATCHES=("POKE_NUM=42.5 TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_NUM = 42.5")
            REPORT_COLUMNS=("poke_num=\$[POKE_NUM]")
            CHECK_TOKENS=("poke_num=42.5")
            ;;
        string_forced_pass)
            POKE_BATCHES=("POKE_STR:=0012 TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_STR = 0012")
            REPORT_COLUMNS=("poke_str=\$[POKE_STR]")
            CHECK_TOKENS=("poke_str=0012")
            ;;
        multiple_mixed_pass)
            POKE_BATCHES=("POKE_A=7 POKE_B:=bravo TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_A = 7" "POKE_B = bravo")
            REPORT_COLUMNS=("poke_a=\$[POKE_A]" "poke_b=\$[POKE_B]")
            CHECK_TOKENS=("poke_a=7" "poke_b=bravo")
            ;;
        overwrite_existing_pass)
            POKE_BATCHES=("POKE_OVER:=old" "__SLEEP__=0.5" "POKE_OVER:=new TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_OVER = new")
            REPORT_COLUMNS=("poke_over=\$[POKE_OVER]")
            CHECK_TOKENS=("poke_over=new")
            ABSENT_TOKENS=("poke_over=old")
            ;;
        cache_config_pass)
            POKE_MODE="cache"
            CACHE_POKES=("POKE_CACHE_NUM=88" "TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_CACHE_NUM = 88")
            REPORT_COLUMNS=("cache_num=\$[POKE_CACHE_NUM]")
            CHECK_TOKENS=("cache_num=88")
            ;;
        mission_file_connection_pass)
            POKE_MODE="file"
            POKE_BATCHES=("POKE_FILE:=from_file TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_FILE = from_file")
            REPORT_COLUMNS=("poke_file=\$[POKE_FILE]")
            CHECK_TOKENS=("poke_file=from_file")
            ;;
        quiet_mode_pass)
            POKE_BATCHES=("POKE_QUIET:=silent TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_QUIET = silent")
            REPORT_COLUMNS=("poke_quiet=\$[POKE_QUIET]")
            CHECK_TOKENS=("poke_quiet=silent")
            ;;
        negative_double_pass)
            POKE_BATCHES=("POKE_NEG=-13.25 TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_NEG = -13.25")
            REPORT_COLUMNS=("poke_neg=\$[POKE_NEG]")
            CHECK_TOKENS=("poke_neg=-13.25")
            ;;
        boolean_true_pass)
            POKE_BATCHES=("POKE_BOOL=true TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_BOOL = true")
            REPORT_COLUMNS=("poke_bool=\$[POKE_BOOL]")
            CHECK_TOKENS=("poke_bool=true")
            ;;
        repeated_batch_pass)
            POKE_BATCHES=("POKE_REPEAT:=first" "__SLEEP__=0.5" "POKE_REPEAT:=second TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_REPEAT = second")
            REPORT_COLUMNS=("poke_repeat=\$[POKE_REPEAT]")
            CHECK_TOKENS=("poke_repeat=second")
            ABSENT_TOKENS=("poke_repeat=first")
            ;;
        mixed_numeric_string_batch_pass)
            POKE_BATCHES=("POKE_MIXED_NUM=31 POKE_MIXED_STR:=031 TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_MIXED_NUM = 31" "POKE_MIXED_STR = 031")
            REPORT_COLUMNS=("mixed_num=\$[POKE_MIXED_NUM]" "mixed_str=\$[POKE_MIXED_STR]")
            CHECK_TOKENS=("mixed_num=31" "mixed_str=031")
            ;;
        cache_string_numeric_pass)
            POKE_MODE="cache"
            CACHE_POKES=("POKE_CACHE_NUM=77" "POKE_CACHE_STR:=seventy_seven" "TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_CACHE_NUM = 77" "POKE_CACHE_STR = seventy_seven")
            REPORT_COLUMNS=("cache_num=\$[POKE_CACHE_NUM]" "cache_str=\$[POKE_CACHE_STR]")
            CHECK_TOKENS=("cache_num=77" "cache_str=seventy_seven")
            ;;
        host_alias_args_pass)
            POKE_MODE="host_alias"
            POKE_BATCHES=("POKE_ALIAS:=alias_host TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_ALIAS = alias_host")
            REPORT_COLUMNS=("poke_alias=\$[POKE_ALIAS]")
            CHECK_TOKENS=("poke_alias=alias_host")
            ;;
        server_alias_args_pass)
            POKE_MODE="server_alias"
            POKE_BATCHES=("POKE_SERVER_ALIAS:=server_alias TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_SERVER_ALIAS = server_alias")
            REPORT_COLUMNS=("server_alias=\$[POKE_SERVER_ALIAS]")
            CHECK_TOKENS=("server_alias=server_alias")
            ;;
        server_underscore_args_pass)
            POKE_MODE="server_underscore"
            POKE_BATCHES=("POKE_SERVER_UNDERSCORE:=server_under TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_SERVER_UNDERSCORE = server_under")
            REPORT_COLUMNS=("server_under=\$[POKE_SERVER_UNDERSCORE]")
            CHECK_TOKENS=("server_under=server_under")
            ;;
        quiet_short_alias_pass)
            POKE_MODE="quiet_short"
            POKE_BATCHES=("POKE_QSHORT:=quiet_short TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_QSHORT = quiet_short")
            REPORT_COLUMNS=("qshort=\$[POKE_QSHORT]")
            CHECK_TOKENS=("qshort=quiet_short")
            ;;
        cache_short_alias_pass)
            POKE_MODE="cache_short"
            CACHE_POKES=("POKE_CACHE_SHORT:=cache_short" "TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_CACHE_SHORT = cache_short")
            REPORT_COLUMNS=("cache_short=\$[POKE_CACHE_SHORT]")
            CHECK_TOKENS=("cache_short=cache_short")
            ;;
        zero_double_pass)
            POKE_BATCHES=("POKE_ZERO=0 TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_ZERO = 0")
            REPORT_COLUMNS=("poke_zero=\$[POKE_ZERO]")
            CHECK_TOKENS=("poke_zero=0")
            ;;
        scientific_literal_pass)
            POKE_BATCHES=("POKE_SCI=1e-3 TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_SCI = 1e-3")
            REPORT_COLUMNS=("poke_sci=\$[POKE_SCI]")
            CHECK_TOKENS=("poke_sci=1e-3")
            ;;
        large_double_pass)
            POKE_BATCHES=("POKE_BIG=123456.75 TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_BIG = 123456.75")
            REPORT_COLUMNS=("poke_big=\$[POKE_BIG]")
            CHECK_TOKENS=("poke_big=123456.75")
            ;;
        cache_forced_string_pass)
            POKE_MODE="cache"
            CACHE_POKES=("POKE_CACHE_FORCED:=0007" "TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_CACHE_FORCED = 0007")
            REPORT_COLUMNS=("cache_forced=\$[POKE_CACHE_FORCED]")
            CHECK_TOKENS=("cache_forced=0007")
            ;;
        cache_boolean_pass)
            POKE_MODE="cache"
            CACHE_POKES=("POKE_CACHE_BOOL=true" "TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_CACHE_BOOL = true")
            REPORT_COLUMNS=("cache_bool=\$[POKE_CACHE_BOOL]")
            CHECK_TOKENS=("cache_bool=true")
            ;;
        utc_macro_pass)
            POKE_BATCHES=("POKE_UTC=@UTC TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_UTC > 0")
            REPORT_COLUMNS=("poke_utc=\$[POKE_UTC]")
            CHECK_TOKENS=("poke_utc=")
            NUMERIC_GT_CHECKS=("poke_utc:0")
            ;;
        moostime_macro_pass)
            POKE_BATCHES=("POKE_MOOSTIME=@MOOSTIME TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_MOOSTIME > 0")
            REPORT_COLUMNS=("poke_moostime=\$[POKE_MOOSTIME]")
            CHECK_TOKENS=("poke_moostime=")
            NUMERIC_GT_CHECKS=("poke_moostime:0")
            ;;
        forced_utc_string_pass)
            POKE_BATCHES=("POKE_UTC_STRING:=@UTC TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_UTC_STRING = @UTC")
            REPORT_COLUMNS=("utc_string=\$[POKE_UTC_STRING]")
            CHECK_TOKENS=("utc_string=@UTC")
            ;;
        duplicate_same_invocation_pass)
            POKE_BATCHES=("POKE_DUP:=first POKE_DUP:=second TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_DUP = second")
            REPORT_COLUMNS=("poke_dup=\$[POKE_DUP]")
            CHECK_TOKENS=("poke_dup=second")
            ABSENT_TOKENS=("poke_dup=first")
            ;;
        cache_duplicate_first_wins_pass)
            POKE_MODE="cache"
            CACHE_POKES=("POKE_CACHE_DUP:=first" "POKE_CACHE_DUP:=second" "TEST_EVAL_READY=true")
            PASS_CONDITIONS=("POKE_CACHE_DUP = first")
            REPORT_COLUMNS=("cache_dup=\$[POKE_CACHE_DUP]")
            CHECK_TOKENS=("cache_dup=first")
            ABSENT_TOKENS=("cache_dup=second")
            ;;
        cache_cli_mixed_pass)
            POKE_MODE="cache_cli"
            CACHE_POKES=("POKE_CACHE_MIX:=from_cache" "TEST_EVAL_READY=true")
            POKE_BATCHES=("POKE_CLI_MIX:=from_cli")
            PASS_CONDITIONS=("POKE_CACHE_MIX = from_cache" "POKE_CLI_MIX = from_cli")
            REPORT_COLUMNS=("cache_mix=\$[POKE_CACHE_MIX]" "cli_mix=\$[POKE_CLI_MIX]")
            CHECK_TOKENS=("cache_mix=from_cache" "cli_mix=from_cli")
            ;;
    esac
}

write_eval_config() {
    local case_dir="$1"
    local eval_file="$case_dir/case_eval.moos"
    {
        echo "//-------------------------------------------------"
        echo "// FILE: case_eval.moos"
        echo "// NAME: Charles Benjamin"
        echo "//-------------------------------------------------"
        echo ""
        echo "ProcessConfig = pMissionEval"
        echo "{"
        echo "  AppTick   = 4"
        echo "  CommsTick = 4"
        echo ""
        echo "  lead_condition = TEST_EVAL_READY = true"
        for cond in "${PASS_CONDITIONS[@]}"; do
            echo "  pass_condition = $cond"
        done
        echo ""
        echo "  result_flag  = MISSION_EVALUATED = true"
        echo "  mission_form = upokedb_unit"
        echo "  mission_mod  = $CASE_NAME"
        echo ""
        echo "  prereport_column = form=\$[MISSION_FORM]"
        echo "  prereport_column = mmod=\$[MMOD]"
        echo "  report_column    = grade=\$[GRADE]"
        echo "  report_column    = eval=\$[TEST_EVAL_READY]"
        for col in "${REPORT_COLUMNS[@]}"; do
            echo "  report_column    = $col"
        done
        echo "  report_column    = mhash=\$[MHASH_SHORT]"
        echo "  report_file      = results.txt"
        echo "}"
    } > "$eval_file"
}

write_cache_config() {
    local case_dir="$1"
    local port="$2"
    local cache_file="$case_dir/poke_cache.moos"
    {
        echo "ServerHost = localhost"
        echo "ServerPort = $port"
        echo "Community  = cache"
        echo ""
        echo "ProcessConfig = uPokeDB"
        echo "{"
        echo "  AppTick   = 4"
        echo "  CommsTick = 4"
        for poke in "${CACHE_POKES[@]}"; do
            echo "  poke = $poke"
        done
        echo "}"
    } > "$cache_file"
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

run_pokes() {
    local case_dir="$1"
    local port="$2"
    local batch

    if [ "$POKE_MODE" = "cache" ] || [ "$POKE_MODE" = "cache_short" ] || [ "$POKE_MODE" = "cache_cli" ]; then
        write_cache_config "$case_dir" "$port"
        if [ "$POKE_MODE" = "cache_short" ]; then
            uPokeDB "$case_dir/poke_cache.moos" -c -q > "$case_dir/upokedb.out" 2>&1
        elif [ "$POKE_MODE" = "cache_cli" ]; then
            uPokeDB "$case_dir/poke_cache.moos" --cache --quiet "${POKE_BATCHES[@]}" > "$case_dir/upokedb.out" 2>&1
        else
            uPokeDB "$case_dir/poke_cache.moos" --cache --quiet > "$case_dir/upokedb.out" 2>&1
        fi
        return $?
    fi

    for batch in "${POKE_BATCHES[@]}"; do
        if echo "$batch" | grep -q '^__SLEEP__='; then
            sleep "${batch#__SLEEP__=}"
            continue
        fi
        if [ "$POKE_MODE" = "file" ]; then
            uPokeDB "$case_dir/targ_shoreside.moos" --quiet $batch >> "$case_dir/upokedb.out" 2>&1 || return 1
        elif [ "$POKE_MODE" = "host_alias" ]; then
            uPokeDB --quiet host=localhost port="$port" $batch >> "$case_dir/upokedb.out" 2>&1 || return 1
        elif [ "$POKE_MODE" = "server_alias" ]; then
            uPokeDB --quiet serverhost=localhost serverport="$port" $batch >> "$case_dir/upokedb.out" 2>&1 || return 1
        elif [ "$POKE_MODE" = "server_underscore" ]; then
            uPokeDB --quiet server_host=localhost server_port="$port" $batch >> "$case_dir/upokedb.out" 2>&1 || return 1
        elif [ "$POKE_MODE" = "quiet_short" ]; then
            uPokeDB -q --host=localhost --port="$port" $batch >> "$case_dir/upokedb.out" 2>&1 || return 1
        else
            uPokeDB --quiet --host=localhost --port="$port" $batch >> "$case_dir/upokedb.out" 2>&1 || return 1
        fi
    done
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
    local actual
    local status="success"
    local launch_rc=0
    local poke_rc=0
    local check
    local key
    local threshold
    local value

    get_case_config "$case_name" || return 1

    tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$tag"
    case_row_file="$CASE_ROW_DIR/$tag.txt"
    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR"/. "$case_dir"/

    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
    )
    write_eval_config "$case_dir"

    (
        cd "$case_dir"
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
    launch_rc=$?
    sleep 0.75
    run_pokes "$case_dir" "$shore_mport"
    poke_rc=$?
    (
        cd "$case_dir"
        uMayFinish --max_time="$MAX_TIME" targ_shoreside.moos > mayfinish.out 2>&1 || true
    )
    line=$(wait_for_results "$case_dir/results.txt" 120)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    if [ "$launch_rc" != 0 ] || [ "$poke_rc" != 0 ] || [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
    fi
    for check in "${CHECK_TOKENS[@]}"; do
        if ! echo "$line" | grep -q "$check"; then
            status="mismatch"
        fi
    done
    for check in "${ABSENT_TOKENS[@]}"; do
        if echo "$line" | grep -q "$check"; then
            status="mismatch"
        fi
    done
    for check in "${NUMERIC_GT_CHECKS[@]}"; do
        key="${check%%:*}"
        threshold="${check#*:}"
        value=$(echo "$line" | sed -n "s/.*$key=\\([^ ]*\\).*/\\1/p")
        if [ "$value" = "" ] || ! awk -v value="$value" -v threshold="$threshold" 'BEGIN { exit !(value > threshold) }'; then
            status="mismatch"
        fi
    done

    emit_case_row "$case_name" "$status" "$EXPECTED" "$actual" "$line" > "$case_row_file"
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
RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_upokedb_unit_XXXXXX")
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
