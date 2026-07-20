#!/usr/bin/env bash
#------------------------------------------------------------
#   Script: mission_utility_runner.sh
#  Purpose: Shared runner for mission utility harnesses
#   Author: Charles Benjamin
#   LastEd: Jul 2026
#------------------------------------------------------------
mission_utility_main() {
need_bash=5.1
if [ -z "${BASH_VERSION:-}" ]; then
    echo "zlaunch.sh: run this harness as ./zlaunch.sh with Bash >= $need_bash." >&2
    exit 2
fi

have_bash51() {
    (( BASH_VERSINFO[0] > 5 || (BASH_VERSINFO[0] == 5 && BASH_VERSINFO[1] >= 1) ))
}

if ! have_bash51; then
    if [ "${HARNESS_DISABLE_BASH_REEXEC:-}" != 1 ]; then
        for bash_candidate in "${HARNESS_BASH:-}" /opt/homebrew/bin/bash /usr/local/bin/bash /home/linuxbrew/.linuxbrew/bin/bash; do
            [ -n "$bash_candidate" ] && [ -x "$bash_candidate" ] || continue
            if "$bash_candidate" -c '(( BASH_VERSINFO[0] > 5 || (BASH_VERSINFO[0] == 5 && BASH_VERSINFO[1] >= 1) ))' 2>/dev/null; then
                echo "zlaunch.sh: re-running with $bash_candidate for Bash >= $need_bash" >&2
                exec "$bash_candidate" "$0" "$@"
            fi
        done
    fi
    echo "zlaunch.sh: Bash >= $need_bash is required for rolling --jobs scheduling." >&2
    echo "Detected Bash: $BASH_VERSION" >&2
    echo "On macOS, install Homebrew Bash or run: HARNESS_BASH=/opt/homebrew/bin/bash ./zlaunch.sh" >&2
    exit 2
fi

set -u

ME=$(basename "$0")
HARNESS_DIR=$(cd "$(dirname "$0")" && pwd)
HARNESS_GROUP_DIR=$(cd "$HARNESS_DIR/.." && pwd)
PATCH_DIR="$HARNESS_GROUP_DIR/patches"
REPO_DIR=$(cd "$HARNESS_DIR/../../.." && pwd)
MISSION_DIR="$REPO_DIR/missions/mission_utility_missions/mission_utility_unit"
TEARDOWN_HELPER="$REPO_DIR/scripts/moos_scoped_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
RUNS_DIR="$HARNESS_DIR/.harness_runs"
RUN_ROOT=""

TIME_WARP=10
MAX_TIME=80
JOBS=1
PORT_BASE=${PORT_BASE:-9000}
PORT_STRIDE=30
PSHARE_OFFSET=$((PORT_STRIDE / 2))
CASE=""
JUST_MAKE=no
LOG_MODE=minimal
KEEP_WORKDIRS=no
VERBOSE=no
DISPLAY_ARGS=(--nogui)
CLEANED=no
CLEANING=no
CLEANUP_FAILED=no
FINISH_FATAL_REASON=""

declare -a SELECTED_CASES CASE_RESULT
declare -A PID_CASE PID_RESULT PID_LOG PID_PORT_BASE

usage() {
    echo "$ME [OPTIONS] [time_warp]"
    echo ""
    echo "Options:"
    echo "  --help, -h         Show this help message"
    echo "  --case=<name>      Run one named case"
    echo "  --jobs=<n>         Run up to n cases with rolling scheduling"
    echo "  --port_base=<n>    Base MOOS port for isolated case blocks"
    echo "  --max_time=<secs>  Maximum real time for mission completion"
    echo "  --just_make, -j    Prepare each case without launching it"
    echo "  --log=<mode>       Logging mode: minimal (default) or full"
    echo "  --keep_workdirs    Keep this invocation's isolated case directories"
    echo "  --verbose, -v     Show scheduler events"
    echo "  --nogui, -ng       Headless launch (only supported display mode)"
    echo "  GUI support:       Unavailable (non-visual stem)"
}

die() {
    echo "$ME: $*" >&2
    exit 2
}

is_uint() {
    [[ "$1" =~ ^[0-9]+$ ]]
}

for arg in "$@"; do
    case "$arg" in
        --case=*)
            CASE="${arg#--case=}"
            [ -n "$CASE" ] || die "--case requires a nonempty case name"
            ;;
        --jobs=*) JOBS="${arg#--jobs=}" ;;
        --port_base=*) PORT_BASE="${arg#--port_base=}" ;;
        --max_time=*) MAX_TIME="${arg#--max_time=}" ;;
        --log=*) LOG_MODE="${arg#--log=}" ;;
        --keep_workdirs) KEEP_WORKDIRS=yes ;;
        --verbose|-v) VERBOSE=yes ;;
        --just_make|-j) JUST_MAKE=yes ;;
        --gui) die "--gui is unsupported: this non-visual stem has no pMarineViewer configuration" ;;
        --nogui|-ng) DISPLAY_ARGS=(--nogui) ;;
        --help|-h) usage; exit 0 ;;
        *[!0-9]*|'') die "bad argument: $arg" ;;
        *) TIME_WARP="$arg" ;;
    esac
done

case "$LOG_MODE" in
    minimal|full) ;;
    *) die "--log must be minimal or full" ;;
esac

is_uint "$TIME_WARP" && [ "${#TIME_WARP}" -le 9 ] || die "time warp must be a bounded positive integer"
is_uint "$JOBS" && [ "${#JOBS}" -le 9 ] || die "--jobs must be a bounded positive integer"
is_uint "$PORT_BASE" && [ "${#PORT_BASE}" -le 5 ] || die "--port_base must be an integer from 1 through 65535"
is_uint "$MAX_TIME" && [ "${#MAX_TIME}" -le 9 ] || die "--max_time must be a bounded positive integer"

TIME_WARP=$((10#$TIME_WARP))
JOBS=$((10#$JOBS))
PORT_BASE=$((10#$PORT_BASE))
MAX_TIME=$((10#$MAX_TIME))

[ "$TIME_WARP" -gt 0 ] || die "time warp must be a positive integer"
[ "$JOBS" -gt 0 ] || die "--jobs must be a positive integer"
[ "$PORT_BASE" -gt 0 ] && [ "$PORT_BASE" -le 65535 ] || die "--port_base must be an integer from 1 through 65535"
[ "$MAX_TIME" -gt 0 ] || die "--max_time must be a positive integer"

[ -d "$MISSION_DIR" ] || die "mission directory not found: $MISSION_DIR"
[ -f "$TEARDOWN_HELPER" ] || die "missing teardown helper: $TEARDOWN_HELPER"
# shellcheck source=/dev/null
. "$TEARDOWN_HELPER"

select_cases() {
    local case_name
    SELECTED_CASES=()
    if [ -n "$CASE" ]; then
        for case_name in "${ALL_CASES[@]}"; do
            if [ "$case_name" = "$CASE" ]; then
                SELECTED_CASES=("$case_name")
                return 0
            fi
        done
        die "unknown case: $CASE"
    fi
    SELECTED_CASES=("${ALL_CASES[@]}")
    [ "${#SELECTED_CASES[@]}" -gt 0 ] || die "no cases selected"
}

field_value() {
    local line="$1"
    local key="$2"
    local field
    local -a fields=()
    read -r -a fields <<< "$line"
    for field in "${fields[@]}"; do
        case "$field" in
            "$key"=*) printf '%s\n' "${field#*=}"; return 0 ;;
        esac
    done
    return 1
}

field_count() {
    local line="$1"
    local key="$2"
    local field
    local count=0
    local -a fields=()
    read -r -a fields <<< "$line"
    for field in "${fields[@]}"; do
        case "$field" in
            "$key"=*) count=$((count + 1)) ;;
        esac
    done
    printf '%s\n' "$count"
}

stop_root() {
    if ! moos_scoped_teardown_stop_root "$1"; then
        echo "$ME: scoped teardown failed for $1" >&2
        return 1
    fi
}

cleanup_runtime() {
    local pid
    local root_stopped=yes
    [ "$CLEANED" = no ] || return 0
    [ "$CLEANING" = no ] || return 0
    CLEANING=yes
    trap '' INT TERM PIPE
    for pid in "${!PID_CASE[@]}"; do
        kill "$pid" 2>/dev/null || true
    done
    wait 2>/dev/null || true
    if [ -n "$RUN_ROOT" ] && [ -d "$RUN_ROOT" ]; then
        if ! stop_root "$RUN_ROOT"; then
            root_stopped=no
            CLEANUP_FAILED=yes
        fi
        if [ "$KEEP_WORKDIRS" != yes ] && [ "$root_stopped" = yes ] &&
           [ "$CLEANUP_FAILED" = no ]; then
            rm -rf "$RUN_ROOT"
        fi
    fi
    rmdir "$RUNS_DIR" 2>/dev/null || true
    CLEANED=yes
    CLEANING=no
}

# Invoked indirectly by the signal traps installed below.
# shellcheck disable=SC2329
on_signal() {
    exit 130
}

trap cleanup_runtime EXIT
trap on_signal INT TERM PIPE

get_case_config() {
    CASE_NAME="$1"
    EXPECTED_SUBJECT="pass"
    SHORE_PATCH=""
    EXTRA_CHECK=""
    CASE_MAX_TIME="$MAX_TIME"
    MAYFINISH_ALIAS=""
    POKE_ARGS="PASS_INPUT=true CASE_VALUE:=alpha CASE_MAIL:=baseline TEST_EVAL_READY=true"
    PRE_EVAL_SLEEP="1"

    case "$CASE_NAME" in
        eval_baseline_pass)
            ;;
        eval_false_condition_fail)
            EXPECTED_SUBJECT="fail"
            SHORE_PATCH="$PATCH_DIR/eval-false-condition-shoreside.xmoos"
            POKE_ARGS="PASS_INPUT=false CASE_VALUE:=falsecase CASE_MAIL:=falsecase TEST_EVAL_READY=true"
            ;;
        mayfinish_default_exit_pass)
            ;;
        eval_builtin_finish_pass)
            SHORE_PATCH="$PATCH_DIR/eval-builtin-finish-shoreside.xmoos"
            EXTRA_CHECK="builtin_finish"
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=builtin CASE_MAIL:=builtin TEST_EVAL_READY=true"
            ;;
        eval_numeric_multi_pass)
            SHORE_PATCH="$PATCH_DIR/eval-numeric-multi-shoreside.xmoos"
            EXTRA_CHECK="numeric_multi"
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=numeric CASE_PHASE:=ready NUM_SCORE=42.5 CASE_MAIL:=numeric TEST_EVAL_READY=true"
            ;;
        eval_sequence_two_stage_pass)
            SHORE_PATCH="$PATCH_DIR/eval-sequence-two-stage-shoreside.xmoos"
            EXTRA_CHECK="sequence_two_stage"
            CASE_MAX_TIME="120"
            POKE_ARGS="STAGE1_READY=true STAGE1_PASS=true __SLEEP__=1 STAGE2_READY=true STAGE2_PASS=true"
            ;;
        eval_sequence_first_stage_fail)
            EXPECTED_SUBJECT="fail"
            SHORE_PATCH="$PATCH_DIR/eval-sequence-first-stage-fail-shoreside.xmoos"
            EXTRA_CHECK="sequence_first_fail"
            POKE_ARGS="STAGE1_READY=true STAGE1_PASS=false"
            ;;
        eval_sequence_second_stage_fail)
            EXPECTED_SUBJECT="fail"
            SHORE_PATCH="$PATCH_DIR/eval-sequence-second-stage-fail-shoreside.xmoos"
            EXTRA_CHECK="sequence_second_fail"
            CASE_MAX_TIME="120"
            POKE_ARGS="STAGE1_READY=true STAGE1_PASS=true __SLEEP__=1 STAGE2_READY=true STAGE2_PASS=false"
            ;;
        eval_numeric_literal_flag_pass)
            SHORE_PATCH="$PATCH_DIR/eval-numeric-literal-flag-shoreside.xmoos"
            EXTRA_CHECK="numeric_literal"
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=literalnum CASE_MAIL:=literalnum TEST_EVAL_READY=true"
            ;;
        eval_lead_only_pass)
            SHORE_PATCH="$PATCH_DIR/eval-lead-only-shoreside.xmoos"
            EXTRA_CHECK="lead_only"
            POKE_ARGS="CASE_VALUE:=leadonly CASE_MAIL:=leadonly TEST_EVAL_READY=true"
            ;;
        eval_numeric_partial_fail)
            EXPECTED_SUBJECT="fail"
            SHORE_PATCH="$PATCH_DIR/eval-numeric-partial-fail-shoreside.xmoos"
            EXTRA_CHECK="numeric_partial_fail"
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=numericpartial CASE_PHASE:=ready NUM_SCORE=39 CASE_MAIL:=numericpartial TEST_EVAL_READY=true"
            ;;
        eval_fail_only_condition_fail)
            EXPECTED_SUBJECT="fail"
            SHORE_PATCH="$PATCH_DIR/eval-fail-only-condition-shoreside.xmoos"
            EXTRA_CHECK="fail_only"
            POKE_ARGS="FORCE_FAIL=true CASE_VALUE:=failonly CASE_MAIL:=failonly TEST_EVAL_READY=true"
            ;;
        eval_fail_condition_fail)
            EXPECTED_SUBJECT="fail"
            SHORE_PATCH="$PATCH_DIR/eval-fail-condition-shoreside.xmoos"
            EXTRA_CHECK="fail_flag"
            POKE_ARGS="PASS_INPUT=true FORCE_FAIL=true CASE_VALUE:=failcondition CASE_MAIL:=failcondition TEST_EVAL_READY=true"
            ;;
        eval_flags_and_mail_pass)
            EXTRA_CHECK="flags"
            ;;
        eval_csp_report_pass)
            SHORE_PATCH="$PATCH_DIR/eval-csp-report-shoreside.xmoos"
            EXTRA_CHECK="csp"
            ;;
        hash_custom_vars_pass)
            SHORE_PATCH="$PATCH_DIR/hash-custom-vars-shoreside.xmoos"
            EXTRA_CHECK="custom_hash"
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=customhash CASE_MAIL:=customhash TEST_EVAL_READY=true"
            ;;
        eval_mhash_macros_pass)
            SHORE_PATCH="$PATCH_DIR/eval-mhash-macros-shoreside.xmoos"
            EXTRA_CHECK="mhash_macros"
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=mhashmacros CASE_MAIL:=mhashmacros TEST_EVAL_READY=true"
            ;;
        eval_clock_macros_pass)
            SHORE_PATCH="$PATCH_DIR/eval-clock-macros-shoreside.xmoos"
            EXTRA_CHECK="clock_macros"
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=clockmacros CASE_MAIL:=clockmacros TEST_EVAL_READY=true"
            ;;
        hash_short_off_pass)
            SHORE_PATCH="$PATCH_DIR/hash-short-off-shoreside.xmoos"
            EXTRA_CHECK="hash_short_off"
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=shortoff CASE_MAIL:=shortoff TEST_EVAL_READY=true"
            ;;
        hash_long_off_pass)
            SHORE_PATCH="$PATCH_DIR/hash-long-off-shoreside.xmoos"
            EXTRA_CHECK="hash_long_off"
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=longoff CASE_MAIL:=longoff TEST_EVAL_READY=true"
            ;;
        eval_no_hash_report_pass)
            SHORE_PATCH="$PATCH_DIR/eval-no-hash-report-shoreside.xmoos"
            EXTRA_CHECK="no_hash_report"
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=nohash CASE_MAIL:=nohash TEST_EVAL_READY=true"
            ;;
        hash_reset_changes_pass)
            SHORE_PATCH="$PATCH_DIR/hash-reset-changes-shoreside.xmoos"
            EXTRA_CHECK="hash_reset"
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=hashreset CASE_MAIL:=hashreset"
            ;;
        mayfinish_custom_finish_pass)
            SHORE_PATCH="$PATCH_DIR/mayfinish-custom-finish-shoreside.xmoos"
            EXTRA_CHECK="custom_finish"
            MAYFINISH_ALIAS="uMayFinish_custom_finish"
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=customfinish CASE_MAIL:=customfinish TEST_EVAL_READY=true"
            ;;
        mayfinish_custom_value_timeout_fail)
            EXPECTED_SUBJECT="timeout"
            SHORE_PATCH="$PATCH_DIR/mayfinish-custom-value-timeout-shoreside.xmoos"
            EXTRA_CHECK="custom_mismatch"
            MAYFINISH_ALIAS="uMayFinish_custom_mismatch"
            CASE_MAX_TIME="6"
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=customtimeout CASE_MAIL:=customtimeout TEST_EVAL_READY=true"
            ;;
        mayfinish_timeout_fail)
            EXPECTED_SUBJECT="timeout"
            SHORE_PATCH="$PATCH_DIR/mayfinish-timeout-shoreside.xmoos"
            CASE_MAX_TIME="2"
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=timeout CASE_MAIL:=timeout"
            ;;
        *)
            echo "$ME: Unknown case: [$CASE_NAME]"
            return 1
            ;;
    esac
}

apply_case_patch_in_dir() {
    local case_dir="$1"
    local patch_file
    local patches=()
    if [ "$LOG_MODE" = full ]; then
        patches+=("$case_dir/full-logging-shoreside.xmoos")
    fi
    if [ "$SHORE_PATCH" != "" ]; then
        patches+=("$SHORE_PATCH")
    fi
    patches+=("$PATCH_DIR/direct-timer-disabled-shoreside.xmoos")
    for patch_file in "${patches[@]}"; do
        [ -f "$patch_file" ] || {
            echo "$ME: missing case patch: $patch_file" >&2
            return 1
        }
    done
    nspatch --stem="$case_dir/meta_shoreside.moos" "${patches[@]}" --targ="$case_dir/meta_shoreside.moosx"
}

prepare_case_dir() {
    local case_dir="$1"
    rm -rf "$case_dir"
    mkdir -p "$case_dir" || return 1
    cp -R "$MISSION_DIR"/. "$case_dir"/ || return 1
    (
        cd "$case_dir" || exit 1
        ./clean.sh >/dev/null 2>&1 || true
    )
    apply_case_patch_in_dir "$case_dir"
}

find_alog() {
    local case_dir="$1"
    find "$case_dir" -path '*XLOG_SHORESIDE*/*.alog' -print | sort | tail -n 1
}

alog_has() {
    local case_dir="$1"
    local pattern="$2"
    local alog
    alog=$(find_alog "$case_dir")
    [ "$alog" != "" ] || return 1
    aloggrep "$alog" "$pattern" 2>/dev/null | rg -q "$pattern"
}

alog_var_has() {
    local case_dir="$1"
    local var="$2"
    local alog
    alog=$(find_alog "$case_dir")
    [ "$alog" != "" ] || return 1
    aloggrep "$alog" "$var" 2>/dev/null | rg -q "^[[:space:]]*[0-9].*[[:space:]]${var}[[:space:]]"
}

alog_var_line_matches() {
    local case_dir="$1"
    local var="$2"
    local pattern="$3"
    local alog
    alog=$(find_alog "$case_dir")
    [ "$alog" != "" ] || return 1
    aloggrep "$alog" "$var" 2>/dev/null | rg "$var" | rg -q "$pattern"
}

alog_var_scalar_equals() {
    local case_dir="$1"
    local var="$2"
    local expected="$3"
    local alog
    alog=$(find_alog "$case_dir")
    [ "$alog" != "" ] || return 1
    awk -v var="$var" -v expected="$expected" '
        $2 == var && $4 == expected {
            found = 1
            exit
        }
        END { exit(found ? 0 : 1) }
    ' "$alog"
}

unique_hash_count() {
    local case_dir="$1"
    local alog
    alog=$(find_alog "$case_dir")
    [ "$alog" != "" ] || {
        echo 0
        return
    }
    awk '
        $2 == "MISSION_HASH" && $3 == "pMissionHash" {
            value = $4
            sub(/^mhash=/, "", value)
            sub(/,.*/, "", value)
            if(value != "")
                seen[value] = 1
        }
        END {
            count = 0
            for(value in seen)
                count++
            print count
        }
    ' "$alog"
}

grade_from_line() {
    echo "$1" | sed -n 's/.*grade=\([^, ]*\).*/\1/p'
}

subject_for_case() {
    local case_name="$1"
    case "$case_name" in
        hash_*)
            echo "pMissionHash"
            ;;
        mayfinish_*)
            echo "uMayFinish"
            ;;
        *)
            echo "pMissionEval"
            ;;
    esac
}

subject_result_line() {
    local line="$1"
    printf '%s\n' "${line/grade=/subject_grade=}"
}

extra_check_ok() {
    local check="$1"
    local case_dir="$2"
    local line="$3"
    case "$check" in
        "")
            return 0
            ;;
        flags)
            alog_var_scalar_equals "$case_dir" "EVAL_RESULT_FLAG" "result:alpha" && \
            alog_var_scalar_equals "$case_dir" "EVAL_PASS_FLAG" "pass:alpha" && \
            alog_var_scalar_equals "$case_dir" "EVAL_MAIL_SEEN" "true"
            return $?
            ;;
        fail_flag)
            alog_has "$case_dir" "EVAL_FAIL_FLAG" && \
            echo "$line" | rg -q 'grade=fail.*force_fail=true'
            return $?
            ;;
        builtin_finish)
            alog_var_has "$case_dir" "MISSION_EVALUATED" && \
            ! alog_var_line_matches "$case_dir" "EVAL_RESULT_FLAG" 'result:'
            return $?
            ;;
        numeric_multi)
            echo "$line" | rg -q 'grade=pass.*phase=ready.*score=42\.5' && \
            alog_var_line_matches "$case_dir" "EVAL_SCORE_FLAG" 'score:42\.5'
            return $?
            ;;
        sequence_two_stage)
            echo "$line" | rg -q 'grade=pass.*stage1=true.*stage2=true' && \
            alog_var_line_matches "$case_dir" "STAGE1_READY" 'true' && \
            alog_var_line_matches "$case_dir" "STAGE2_READY" 'true'
            return $?
            ;;
        sequence_first_fail)
            echo "$line" | rg -q 'grade=fail.*stage1=false' && \
            alog_var_line_matches "$case_dir" "EVAL_FAIL_FLAG" 'stage1_failed' && \
            ! alog_var_has "$case_dir" "STAGE2_READY"
            return $?
            ;;
        sequence_second_fail)
            echo "$line" | rg -q 'grade=fail.*stage1=true.*stage2=false' && \
            alog_var_line_matches "$case_dir" "EVAL_FAIL_FLAG" 'stage2_failed' && \
            alog_var_line_matches "$case_dir" "STAGE2_READY" 'true'
            return $?
            ;;
        numeric_literal)
            echo "$line" | rg -q 'grade=pass.*case_value=literalnum' && \
            alog_var_line_matches "$case_dir" "EVAL_LITERAL_NUM" '17\.25'
            return $?
            ;;
        lead_only)
            echo "$line" | rg -q 'grade=pass.*eval=true.*case_value=leadonly'
            return $?
            ;;
        numeric_partial_fail)
            echo "$line" | rg -q 'grade=fail.*phase=ready.*score=39' && \
            alog_var_line_matches "$case_dir" "EVAL_FAIL_FLAG" 'partial_numeric_fail'
            return $?
            ;;
        fail_only)
            echo "$line" | rg -q 'grade=fail.*eval=true.*force_fail=true' && \
            alog_var_line_matches "$case_dir" "EVAL_FAIL_FLAG" 'fail_only'
            return $?
            ;;
        csp)
            echo "$line" | rg -q 'form=mission_utility_unit, mmod=eval_csp_report_pass.*grade=pass, eval=true'
            return $?
            ;;
        custom_hash)
            echo "$line" | rg -q 'custom_hash=mhash=' && \
            echo "$line" | rg -q 'custom_short=[A-Z]+-[A-Z]+' && \
            ! echo "$line" | rg -Fq '$[CUSTOM_' && \
            ! alog_var_has "$case_dir" "MISSION_HASH" && \
            ! alog_var_has "$case_dir" "MHASH"
            return $?
            ;;
        hash_reset)
            local prehash
            local reset_hash
            prehash=$(field_value "$line" "prehash") || return 1
            reset_hash=$(field_value "$line" "reset_hash") || return 1
            [[ "$prehash" =~ ^\[[A-Z]+-[A-Z]+\]$ ]] && \
            [[ "$reset_hash" =~ ^\[[A-Z]+-[A-Z]+\]$ ]] && \
            [ "$prehash" != "$reset_hash" ] && \
            [ "$(unique_hash_count "$case_dir")" -ge 2 ]
            return $?
            ;;
        mhash_macros)
            echo "$line" | rg -q 'short=\[[A-Z]+-[A-Z]+\]' && \
            echo "$line" | rg -q 'lshort=[A-Z]+-[A-Z]+' && \
            echo "$line" | rg -q 'utc=[0-9]+\.[0-9]+' && \
            echo "$line" | rg -q 'full=mhash=[0-9]{6}-[0-9]{4}[A-Z]-[A-Z]+-[A-Z]+,utc=' && \
            echo "$line" | rg -q 'mission=mhash=[0-9]{6}-[0-9]{4}[A-Z]-[A-Z]+-[A-Z]+,utc='
            return $?
            ;;
        clock_macros)
            echo "$line" | rg -q 'grade=pass.*month=[0-9]{2}.*day=[0-9]{2}.*hour=[0-9]{2}.*min=[0-9]{2}.*sec=[0-9]{2}.*date=[0-9]{4}:[0-9]{2}:[0-9]{2}.*time=[0-9]{2}:[0-9]{2}:[0-9]{2}' && \
            ! echo "$line" | rg -Fq '$['
            return $?
            ;;
        hash_short_off)
            echo "$line" | rg -q 'computed_short=\[[A-Z]+-[A-Z]+\].*full_hash=mhash=' && \
            ! alog_var_has "$case_dir" "MHASH"
            return $?
            ;;
        hash_long_off)
            echo "$line" | rg -q 'grade=pass.*case_value=longoff' && \
            ! alog_var_has "$case_dir" "MISSION_HASH" && \
            ! alog_var_has "$case_dir" "MHASH"
            return $?
            ;;
        no_hash_report)
            echo "$line" | rg -q 'grade=pass.*eval=true.*case_value=nohash' && \
            ! alog_var_has "$case_dir" "MISSION_HASH"
            return $?
            ;;
        custom_finish)
            alog_var_line_matches "$case_dir" "CUSTOM_DONE" 'complete'
            return $?
            ;;
        custom_mismatch)
            alog_var_scalar_equals "$case_dir" "CUSTOM_DONE" "almost"
            return $?
            ;;
    esac
    return 1
}

poke_case_mail() {
    local case_base="$1"
    local item
    local -a batch=()

    sleep "$PRE_EVAL_SLEEP"
    for item in $POKE_ARGS; do
        if [[ "$item" == __SLEEP__=* ]]; then
            if [ "${#batch[@]}" -gt 0 ]; then
                uPokeDB --quiet --host=localhost --port="$case_base" \
                    "${batch[@]}" >/dev/null || return 1
                batch=()
            fi
            sleep "${item#__SLEEP__=}"
        else
            batch+=("$item")
        fi
    done
    if [ "${#batch[@]}" -gt 0 ]; then
        uPokeDB --quiet --host=localhost --port="$case_base" \
            "${batch[@]}" >/dev/null || return 1
    fi
}

subject_row() {
    local results_file="$1"
    local attempt
    local row_count
    local line
    for ((attempt = 0; attempt < 60; attempt++)); do
        row_count=$(awk 'NF {count++} END {print count+0}' "$results_file" 2>/dev/null)
        if [ "$row_count" -gt 1 ]; then
            return 2
        fi
        if [ "$row_count" -eq 1 ]; then
            line=$(awk 'NF {print; exit}' "$results_file")
            if [ -n "$(grade_from_line "$line")" ]; then
                printf '%s\n' "$line"
                return 0
            fi
        fi
        sleep 0.05
    done
    return 1
}

write_just_make_result() {
    local case_name="$1"
    local result_file="$2"
    local launch_rc="$3"
    local subject="$4"
    if [ "$launch_rc" -eq 0 ]; then
        printf 'case=%s grade=pass mode=just_make subject=%s\n' \
            "$case_name" "$subject" > "$result_file"
    else
        printf 'case=%s grade=fail reason=launch_error launch_rc=%s mode=just_make subject=%s\n' \
            "$case_name" "$launch_rc" "$subject" > "$result_file"
    fi
}

evaluate_mission_subject() {
    local case_name="$1"
    local workdir="$2"
    local result_file="$3"
    local case_base="$4"
    local launch_rc=0
    local poke_rc=0
    local stem_pid
    local line=""
    local row_rc=0
    local row_count
    local subject
    local subject_grade
    local subject_line
    local -a launch_args

    subject=$(subject_for_case "$case_name")
    (
        cd "$workdir" || exit 1
        : > results.txt
        launch_args=(
            --max_time="$CASE_MAX_TIME"
            "${DISPLAY_ARGS[@]}"
            --shore_mport="$case_base"
            --shore_pshare="$((case_base + PSHARE_OFFSET))"
            "$TIME_WARP"
        )
        LOG_MODE_PREPARED=yes ./zlaunch.sh --log="$LOG_MODE" "${launch_args[@]}"
    ) &
    stem_pid=$!
    poke_case_mail "$case_base" || poke_rc=$?
    wait "$stem_pid" || launch_rc=$?

    if [ "$poke_rc" -ne 0 ]; then
        printf 'case=%s grade=fail reason=stimulus_error subject=%s stimulus_rc=%s launch_rc=%s\n' \
            "$case_name" "$subject" "$poke_rc" "$launch_rc" > "$result_file"
        return 1
    fi

    line=$(subject_row "$workdir/results.txt") || row_rc=$?
    if [ "$row_rc" -ne 0 ]; then
        row_count=$(awk 'NF {count++} END {print count+0}' "$workdir/results.txt" 2>/dev/null)
        if [ "$row_rc" -eq 2 ]; then
            printf 'case=%s grade=fail reason=duplicate_results subject=%s result_rows=%s launch_rc=%s\n' \
                "$case_name" "$subject" "$row_count" "$launch_rc" > "$result_file"
        else
            printf 'case=%s grade=fail reason=missing_result subject=%s result_rows=%s launch_rc=%s\n' \
                "$case_name" "$subject" "$row_count" "$launch_rc" > "$result_file"
        fi
        return 1
    fi

    subject_grade=$(grade_from_line "$line")
    subject_line=$(subject_result_line "$line")
    if [ "$launch_rc" -ne 0 ]; then
        printf 'case=%s grade=fail reason=launch_error subject=%s expected_subject=%s launch_rc=%s %s\n' \
            "$case_name" "$subject" "$EXPECTED_SUBJECT" "$launch_rc" "$subject_line" > "$result_file"
        return 1
    fi
    if [ "$subject_grade" != "$EXPECTED_SUBJECT" ]; then
        printf 'case=%s grade=fail reason=subject_grade_mismatch subject=%s expected_subject=%s %s\n' \
            "$case_name" "$subject" "$EXPECTED_SUBJECT" "$subject_line" > "$result_file"
        return 1
    fi
    if ! extra_check_ok "$EXTRA_CHECK" "$workdir" "$line"; then
        printf 'case=%s grade=fail reason=evidence_mismatch subject=%s expected_subject=%s %s\n' \
            "$case_name" "$subject" "$EXPECTED_SUBJECT" "$subject_line" > "$result_file"
        return 1
    fi

    printf 'case=%s grade=pass subject=%s expected_subject=%s %s\n' \
        "$case_name" "$subject" "$EXPECTED_SUBJECT" "$subject_line" > "$result_file"
    return 0
}

evaluate_mayfinish_subject() {
    local case_name="$1"
    local workdir="$2"
    local result_file="$3"
    local case_base="$4"
    local launch_rc=0
    local poke_rc=0
    local subject_rc=0
    local expected_rc
    local line=""
    local row_rc=0
    local row_count
    local subject_grade
    local subject_line
    local mayfinish_alias
    local antler_pid
    local poker_pid

    mayfinish_alias="$MAYFINISH_ALIAS"
    if [ "$mayfinish_alias" = "" ]; then
        mayfinish_alias="uMayFinish_${case_name}"
    fi

    (
        cd "$workdir" || exit 1
        : > results.txt
        LOG_MODE_PREPARED=yes ./launch.sh --log="$LOG_MODE" \
            --just_make --xlaunched --nogui \
            --shore_mport="$case_base" \
            --shore_pshare="$((case_base + PSHARE_OFFSET))" \
            "$TIME_WARP" >/dev/null || exit $?
        pAntler targ_shoreside.moos >/dev/null 2>&1 &
        antler_pid=$!
        sleep 0.75
        poke_case_mail "$case_base" &
        poker_pid=$!
        uMayFinish targ_shoreside.moos --max_time="$CASE_MAX_TIME" \
            --alias="$mayfinish_alias" || subject_rc=$?
        wait "$poker_pid" || poke_rc=$?
        # Let pLogger commit the finish mail before stopping the community.
        sleep 0.5
        kill "$antler_pid" 2>/dev/null || true
        wait "$antler_pid" 2>/dev/null || true
        printf '%s %s\n' "$subject_rc" "$poke_rc" > direct_status.txt
    ) || launch_rc=$?

    if [ -f "$workdir/direct_status.txt" ]; then
        read -r subject_rc poke_rc < "$workdir/direct_status.txt"
    fi

    if [ "$launch_rc" -ne 0 ]; then
        printf 'case=%s grade=fail reason=launch_error subject=uMayFinish launch_rc=%s\n' \
            "$case_name" "$launch_rc" > "$result_file"
        return 1
    fi
    if [ "$poke_rc" -ne 0 ]; then
        printf 'case=%s grade=fail reason=stimulus_error subject=uMayFinish stimulus_rc=%s subject_rc=%s\n' \
            "$case_name" "$poke_rc" "$subject_rc" > "$result_file"
        return 1
    fi

    expected_rc=0
    if [ "$EXPECTED_SUBJECT" = "timeout" ]; then
        expected_rc=1
    fi

    if [ "$subject_rc" -ne "$expected_rc" ]; then
        printf 'case=%s grade=fail reason=subject_rc_mismatch subject=uMayFinish expected_subject=%s subject_rc=%s\n' \
            "$case_name" "$EXPECTED_SUBJECT" "$subject_rc" > "$result_file"
        return 1
    fi

    if [ "$EXPECTED_SUBJECT" = "timeout" ]; then
        if ! extra_check_ok "$EXTRA_CHECK" "$workdir" ""; then
            printf 'case=%s grade=fail reason=evidence_mismatch subject=uMayFinish expected_subject=timeout subject_rc=%s\n' \
                "$case_name" "$subject_rc" > "$result_file"
            return 1
        fi
        printf 'case=%s grade=pass subject=uMayFinish expected_subject=timeout subject_rc=%s\n' \
            "$case_name" "$subject_rc" > "$result_file"
        return 0
    fi

    line=$(subject_row "$workdir/results.txt") || row_rc=$?
    if [ "$row_rc" -ne 0 ]; then
        row_count=$(awk 'NF {count++} END {print count+0}' "$workdir/results.txt" 2>/dev/null)
        printf 'case=%s grade=fail reason=missing_or_duplicate_result subject=uMayFinish result_rows=%s subject_rc=%s\n' \
            "$case_name" "$row_count" "$subject_rc" > "$result_file"
        return 1
    fi
    subject_grade=$(grade_from_line "$line")
    if [ "$subject_grade" != pass ]; then
        subject_line=$(subject_result_line "$line")
        printf 'case=%s grade=fail reason=completion_grade_mismatch subject=uMayFinish expected_subject=%s subject_rc=%s %s\n' \
            "$case_name" "$EXPECTED_SUBJECT" "$subject_rc" "$subject_line" > "$result_file"
        return 1
    fi

    if ! extra_check_ok "$EXTRA_CHECK" "$workdir" "$line"; then
        subject_line=$(subject_result_line "$line")
        printf 'case=%s grade=fail reason=evidence_mismatch subject=uMayFinish expected_subject=%s subject_rc=%s %s\n' \
            "$case_name" "$EXPECTED_SUBJECT" "$subject_rc" "$subject_line" > "$result_file"
        return 1
    fi

    subject_line=$(subject_result_line "$line")
    printf 'case=%s grade=pass subject=uMayFinish expected_subject=%s subject_rc=%s %s\n' \
        "$case_name" "$EXPECTED_SUBJECT" "$subject_rc" "$subject_line" > "$result_file"
    return 0
}

run_case() {
    local case_name="$1"
    local workdir="$2"
    local result_file="$3"
    local case_base="$4"
    local launch_rc=0
    local subject
    local line
    local grade
    local -a launch_args

    get_case_config "$case_name" || {
        printf 'case=%s grade=fail reason=case_config_error\n' "$case_name" > "$result_file"
        return 1
    }
    subject=$(subject_for_case "$case_name")
    prepare_case_dir "$workdir" || {
        printf 'case=%s grade=fail reason=prepare_error subject=%s\n' \
            "$case_name" "$subject" > "$result_file"
        return 1
    }

    if [ "$JUST_MAKE" = yes ]; then
        (
            cd "$workdir" || exit 1
            launch_args=(
                --just_make
                --max_time="$CASE_MAX_TIME"
                "${DISPLAY_ARGS[@]}"
                --shore_mport="$case_base"
                --shore_pshare="$((case_base + PSHARE_OFFSET))"
                "$TIME_WARP"
            )
            LOG_MODE_PREPARED=yes ./zlaunch.sh --log="$LOG_MODE" "${launch_args[@]}"
        ) || launch_rc=$?
        write_just_make_result "$case_name" "$result_file" "$launch_rc" "$subject"
    elif [ "$subject" = uMayFinish ]; then
        evaluate_mayfinish_subject "$case_name" "$workdir" "$result_file" "$case_base"
    else
        evaluate_mission_subject "$case_name" "$workdir" "$result_file" "$case_base"
    fi

    if ! stop_root "$workdir"; then
        printf 'case=%s grade=fail reason=teardown_error subject=%s\n' \
            "$case_name" "$subject" > "$result_file"
    fi

    line=$(awk 'NF {print; exit}' "$result_file" 2>/dev/null)
    grade=$(field_value "$line" grade || true)
    [ "$grade" = pass ]
}

start_case() {
    local case_idx="$1"
    local case_name="${SELECTED_CASES[$case_idx]}"
    local case_dir
    local workdir
    local result_file
    local log_file
    local case_base
    local pid

    case_base=$((PORT_BASE + case_idx * PORT_STRIDE))
    case_dir="$RUN_ROOT/case_$(printf '%03d' "$case_idx")_$case_name"
    workdir="$case_dir/mission"
    result_file="$case_dir/result.row"
    log_file="$case_dir/run.log"

    mkdir -p "$case_dir"
    CASE_RESULT[case_idx]="$result_file"

    (
        local rc=0
        set +e
        run_case "$case_name" "$workdir" "$result_file" "$case_base" > "$log_file" 2>&1
        rc=$?
        if [ ! -s "$result_file" ]; then
            printf 'case=%s grade=fail reason=missing_result worker_rc=%s\n' \
                "$case_name" "$rc" > "$result_file"
        fi
        exit "$rc"
    ) &

    pid=$!
    PID_CASE[$pid]="$case_name"
    PID_RESULT[$pid]="$result_file"
    PID_LOG[$pid]="$log_file"
    PID_PORT_BASE[$pid]="$case_base"

    if [ "$VERBOSE" = yes ]; then
        printf 'event=start epoch=%s pid=%s case=%s port_base=%s workdir=%s\n' \
            "$(date +%s)" "$pid" "$case_name" "$case_base" "$workdir"
    fi
}

finish_one() {
    local done_pid=""
    local wait_rc=0
    local case_name
    local line
    local grade
    local reason

    FINISH_FATAL_REASON=""
    wait -p done_pid -n || wait_rc=$?
    if [ -z "${done_pid:-}" ]; then
        echo "$ME: wait returned without a completed pid rc=$wait_rc" >&2
        FINISH_FATAL_REASON=scheduler_state_error
        return 2
    fi

    case_name="${PID_CASE[$done_pid]:-}"
    if [ -z "$case_name" ]; then
        echo "$ME: unknown completed pid $done_pid rc=$wait_rc" >&2
        FINISH_FATAL_REASON=scheduler_state_error
        return 2
    fi

    line=$(awk 'NF {print; exit}' "${PID_RESULT[$done_pid]}" 2>/dev/null)
    grade=$(field_value "$line" grade || true)
    reason=$(field_value "$line" reason || true)
    if [ "$wait_rc" -ne 0 ] && [ "$grade" = pass ]; then
        printf 'case=%s grade=fail reason=worker_error worker_rc=%s\n' \
            "$case_name" "$wait_rc" > "${PID_RESULT[$done_pid]}"
        grade=fail
    fi
    if [ "$VERBOSE" = yes ]; then
        printf 'event=finish epoch=%s pid=%s case=%s rc=%s grade=%s port_base=%s log=%s\n' \
            "$(date +%s)" "$done_pid" "$case_name" "$wait_rc" "${grade:-missing}" \
            "${PID_PORT_BASE[$done_pid]}" "${PID_LOG[$done_pid]}"
    fi

    unset 'PID_CASE[$done_pid]' 'PID_RESULT[$done_pid]' 'PID_LOG[$done_pid]'
    unset 'PID_PORT_BASE[$done_pid]'
    if [ "$reason" = teardown_error ]; then
        FINISH_FATAL_REASON=teardown_error
        return 2
    fi
    [ "$grade" = pass ]
}

stop_refill_after_infrastructure_error() {
    local next_idx="$1"
    local total="$2"
    local fatal_reason="$3"
    local case_idx
    local case_name
    local case_dir
    local result_file

    CLEANUP_FAILED=yes
    echo "$ME: stopping scheduler refill after $fatal_reason" >&2
    for ((case_idx = next_idx; case_idx < total; case_idx++)); do
        case_name="${SELECTED_CASES[$case_idx]}"
        case_dir="$RUN_ROOT/case_$(printf '%03d' "$case_idx")_$case_name"
        result_file="$case_dir/result.row"
        CASE_RESULT[case_idx]="$result_file"
        if mkdir -p "$case_dir"; then
            printf 'case=%s grade=fail reason=scheduler_aborted_after_%s\n' \
                "$case_name" "$fatal_reason" > "$result_file"
        fi
    done
}

aggregate_results() {
    local total="${#SELECTED_CASES[@]}"
    local case_idx
    local case_name
    local result_file
    local row_count
    local line
    local row_case
    local grade_count
    local grade

    FINAL_FAILURES=0
    RESULT_ROWS=0
    : > "$RESULTS_FILE"

    for ((case_idx = 0; case_idx < total; case_idx++)); do
        case_name="${SELECTED_CASES[$case_idx]}"
        result_file="${CASE_RESULT[$case_idx]:-}"
        line=""
        if [ -n "$result_file" ] && [ -f "$result_file" ]; then
            row_count=$(awk 'NF {count++} END {print count+0}' "$result_file")
            if [ "$row_count" -eq 1 ]; then
                line=$(awk 'NF {print; exit}' "$result_file")
            else
                line="case=$case_name grade=fail reason=invalid_row_count result_rows=$row_count"
            fi
        else
            line="case=$case_name grade=fail reason=missing_result_file"
        fi

        row_case=$(field_value "$line" case || true)
        grade_count=$(field_count "$line" grade)
        if [ "$row_case" != "$case_name" ]; then
            line="case=$case_name grade=fail reason=result_case_mismatch"
        elif [ "$grade_count" -ne 1 ]; then
            line="case=$case_name grade=fail reason=malformed_result grade_fields=$grade_count"
        else
            grade=$(field_value "$line" grade || true)
            if [ "$grade" != pass ] && [ "$grade" != fail ]; then
                line="case=$case_name grade=fail reason=malformed_result harness_grade=${grade:-missing}"
            fi
        fi

        printf '%s\n' "$line" >> "$RESULTS_FILE"
        RESULT_ROWS=$((RESULT_ROWS + 1))
        grade=$(field_value "$line" grade || true)
        [ "$grade" = pass ] || FINAL_FAILURES=$((FINAL_FAILURES + 1))
    done
}

[ "${#ALL_CASES[@]}" -gt 0 ] || die "no ALL_CASES entries configured"
select_cases
total=${#SELECTED_CASES[@]}
last_port=$((PORT_BASE + (total - 1) * PORT_STRIDE + PSHARE_OFFSET))
[ "$last_port" -le 65535 ] || die "selected cases require ports through $last_port"

mkdir -p "$RUNS_DIR" || die "unable to create run directory: $RUNS_DIR"
RUN_ROOT="$RUNS_DIR/run_$(date +%Y%m%dT%H%M%S)_$$"
mkdir -p "$RUN_ROOT" || die "unable to create run root: $RUN_ROOT"
: > "$RESULTS_FILE"

SECONDS=0
active=0
next=0
scheduler_broken=no

while [ "$next" -lt "$total" ] || [ "$active" -gt 0 ]; do
    while [ "$next" -lt "$total" ] && [ "$active" -lt "$JOBS" ]; do
        start_case "$next"
        next=$((next + 1))
        active=$((active + 1))
    done
    if [ "$active" -gt 0 ]; then
        finish_rc=0
        finish_one || finish_rc=$?
        if [ "$finish_rc" -eq 2 ] && [ "$FINISH_FATAL_REASON" = scheduler_state_error ]; then
            stop_refill_after_infrastructure_error "$next" "$total" "$FINISH_FATAL_REASON"
            next=$total
            scheduler_broken=yes
            break
        fi
        active=$((active - 1))
        if [ "$finish_rc" -eq 2 ]; then
            stop_refill_after_infrastructure_error "$next" "$total" "$FINISH_FATAL_REASON"
            next=$total
        fi
    fi
done

if [ "$scheduler_broken" = yes ]; then
    cleanup_runtime
fi

aggregate_results
if [ "$RESULT_ROWS" -ne "$total" ] || { [ "$total" -gt 0 ] && [ "$RESULT_ROWS" -eq 0 ]; }; then
    echo "$ME: expected $total result rows but wrote $RESULT_ROWS" >&2
    FINAL_FAILURES=$((FINAL_FAILURES + 1))
fi

[ "$FINAL_FAILURES" -eq 0 ] || KEEP_WORKDIRS=yes
cleanup_runtime
elapsed_seconds=$SECONDS
if [ -d "$RUN_ROOT" ]; then
    kept_root="$RUN_ROOT"
else
    kept_root=none
fi
if [ "$CLEANUP_FAILED" = yes ]; then
    FINAL_FAILURES=$((FINAL_FAILURES + 1))
fi
trap - EXIT INT TERM PIPE

cat "$RESULTS_FILE"
printf 'results=%s failures=%s total=%s jobs=%s log_mode=%s elapsed_seconds=%s bash=%s workdirs=%s\n' \
    "$RESULTS_FILE" "$FINAL_FAILURES" "$total" "$JOBS" "$LOG_MODE" "$elapsed_seconds" "$BASH_VERSION" "$kept_root"

[ "$FINAL_FAILURES" -eq 0 ]
}
