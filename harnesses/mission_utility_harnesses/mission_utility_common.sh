#!/bin/bash
#------------------------------------------------------------
#   Script: mission_utility_common.sh
#  Purpose: Shared runner for mission utility harnesses
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }

ME=`basename "$0"`
HARNESS_DIR="$(cd "$(dirname "$0")" && pwd)"
HARNESS_GROUP_DIR="$(cd "$HARNESS_DIR/.." && pwd)"
PATCH_DIR="$HARNESS_GROUP_DIR/patches"
REPO_DIR="$(git -C "$HARNESS_DIR" rev-parse --show-toplevel)"
MISSION_DIR="$REPO_DIR/missions/mission_utility_missions/mission_utility_unit"
RESULTS_FILE="$HARNESS_DIR/results.txt"
TIME_WARP=10
MAX_TIME=80
JOBS=1
PORT_BASE=${PORT_BASE:-7100}
PORT_STRIDE=10
CASE=""
JUST_MAKE="no"
KEEP_WORKDIRS="no"
VERBOSE=""
CASE_ROW_DIR=""
RUN_ROOT=""
RUN_ROOT_PREFIX=${RUN_ROOT_PREFIX:-mission_utility}
ALL_OK="yes"

source "$REPO_DIR/scripts/harness_teardown.sh"

cleanup() {
    if [ "$RUN_ROOT" != "" ] && [ "$KEEP_WORKDIRS" != "yes" ]; then
        rm -rf "$RUN_ROOT"
    fi
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

wait_for_result_line() {
    local file="$1"
    local attempts="${2:-80}"
    local i
    for ((i=0; i<attempts; i++)); do
        if [ -s "$file" ]; then
            tail -n 1 "$file"
            return 0
        fi
        sleep 0.25
    done
    return 1
}

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
            POKE_ARGS="PASS_INPUT=true CASE_VALUE:=hashreset CASE_MAIL:=hashreset RESET_MHASH=true __SLEEP__=4 TEST_EVAL_READY=true"
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
    local patches=()
    if [ "$SHORE_PATCH" != "" ]; then
        patches+=("$SHORE_PATCH")
    fi
    patches+=("$PATCH_DIR/direct-timer-disabled-shoreside.xmoos")
    nspatch --stem="$case_dir/meta_shoreside.moos" "${patches[@]}" --targ="$case_dir/meta_shoreside.moosx"
}

prepare_case_dir() {
    local case_dir="$1"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    (
        cd "$case_dir"
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
    aloggrep "$alog" "$var" 2>/dev/null | rg -q "^[[:space:]]*[0-9].*[[:space:]]$var[[:space:]]"
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

unique_hash_count() {
    local case_dir="$1"
    local alog
    alog=$(find_alog "$case_dir")
    [ "$alog" != "" ] || {
        echo 0
        return
    }
    aloggrep "$alog" MISSION_HASH 2>/dev/null | sed -n 's/.*mhash=\([^, ]*\).*/\1/p' | sort -u | wc -l | tr -d ' '
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
    echo "$line" | sed 's/grade=/subject_grade=/'
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
            alog_has "$case_dir" "EVAL_RESULT_FLAG" && \
            alog_has "$case_dir" "EVAL_PASS_FLAG" && \
            alog_has "$case_dir" "EVAL_MAIL_SEEN"
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
            ! echo "$line" | rg -Fq '$[CUSTOM_'
            return $?
            ;;
        hash_reset)
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
    esac
    return 1
}

poke_case_mail() {
    local case_base="$1"
    local item
    local batch=""
    sleep "$PRE_EVAL_SLEEP"
    for item in $POKE_ARGS; do
        if [[ "$item" == __SLEEP__=* ]]; then
            if [ "$batch" != "" ]; then
                uPokeDB --quiet --host=localhost --port="$case_base" $batch >/dev/null
                batch=""
            fi
            sleep "${item#__SLEEP__=}"
        else
            batch="$batch $item"
        fi
    done
    if [ "$batch" != "" ]; then
        uPokeDB --quiet --host=localhost --port="$case_base" $batch >/dev/null
    fi
}

run_direct_mayfinish_case() {
    local case_idx="$1"
    local case_name="$2"
    local case_tag
    local case_dir
    local case_row_file
    local case_base
    local rc
    local expected_rc
    local line
    local subject
    local subject_grade
    local subject_line
    local mayfinish_alias

    get_case_config "$case_name" || return 1
    subject=$(subject_for_case "$case_name")
    mayfinish_alias="$MAYFINISH_ALIAS"
    if [ "$mayfinish_alias" = "" ]; then
        mayfinish_alias="uMayFinish_${case_idx}"
    fi
    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    case_row_file="$CASE_ROW_DIR/${case_tag}.txt"
    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  grade=fail  reason=prepare_error  subject=$subject" > "$case_row_file"
        return 1
    }
    case_base=$((PORT_BASE + case_idx*PORT_STRIDE))

    (
        cd "$case_dir"
        : > results.txt
        ./launch.sh --just_make --xlaunched --nogui --mport="$case_base" --pshare="$((case_base + 2))" --mmod="$case_name" "$TIME_WARP" >/dev/null
        pAntler targ_shoreside.moos >/dev/null 2>&1 &
        launcher_pid=$!
        sleep 1
        poke_case_mail "$case_base" &
        poker_pid=$!
        uMayFinish --max_time="$CASE_MAX_TIME" --alias="$mayfinish_alias" targ_shoreside.moos
        rc=$?
        wait "$poker_pid" >/dev/null 2>&1 || true
        "$REPO_DIR/scripts/harness_teardown.sh" "$case_dir" >/dev/null 2>&1 || true
        kill "$launcher_pid" >/dev/null 2>&1 || true
        wait "$launcher_pid" >/dev/null 2>&1 || true
        exit "$rc"
    )
    rc=$?

    expected_rc=0
    if [ "$EXPECTED_SUBJECT" = "timeout" ]; then
        expected_rc=1
    fi

    if [ "$rc" != "$expected_rc" ]; then
        echo "case=$case_name  grade=fail  reason=subject_rc_mismatch  subject=$subject  expected_subject=$EXPECTED_SUBJECT  subject_rc=$rc" > "$case_row_file"
        return 1
    fi

    if [ "$EXPECTED_SUBJECT" = "timeout" ]; then
        echo "case=$case_name  grade=pass  subject=$subject  expected_subject=timeout  subject_rc=$rc" > "$case_row_file"
        return 0
    fi

    line=$(wait_for_result_line "$case_dir/results.txt" 80)
    subject_grade=$(grade_from_line "$line")
    if [ "$subject_grade" = "" ]; then
        subject_grade="missing"
    fi
    if [ "$subject_grade" != "$EXPECTED_SUBJECT" ]; then
        subject_line=$(subject_result_line "$line")
        echo "case=$case_name  grade=fail  reason=subject_grade_mismatch  subject=$subject  expected_subject=$EXPECTED_SUBJECT  subject_rc=$rc  $subject_line" > "$case_row_file"
        return 1
    fi

    if ! extra_check_ok "$EXTRA_CHECK" "$case_dir" "$line"; then
        subject_line=$(subject_result_line "$line")
        echo "case=$case_name  grade=fail  reason=evidence_mismatch  subject=$subject  expected_subject=$EXPECTED_SUBJECT  subject_rc=$rc  $subject_line" > "$case_row_file"
        return 1
    fi

    subject_line=$(subject_result_line "$line")
    echo "case=$case_name  grade=pass  subject=$subject  expected_subject=$EXPECTED_SUBJECT  subject_rc=$rc  $subject_line" > "$case_row_file"
    return 0
}

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    get_case_config "$case_name" || return 1
    if [ "$JUST_MAKE" = "yes" ]; then
        local case_tag
        local case_dir
        local case_row_file
        local case_base
        case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
        case_dir="$RUN_ROOT/$case_tag"
        case_row_file="$CASE_ROW_DIR/${case_tag}.txt"
        prepare_case_dir "$case_dir" || return 1
        case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
        (
            cd "$case_dir"
            ./launch.sh --just_make --xlaunched --nogui --mport="$case_base" --pshare="$((case_base + 2))" --mmod="$case_name" "$TIME_WARP" >/dev/null
        )
        echo "case=$case_name  grade=pass  reason=just_make" > "$case_row_file"
        return 0
    fi

    run_direct_mayfinish_case "$case_idx" "$case_name"
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
    if ! rg -q '(^|[[:space:]])grade=pass([[:space:]]|$)' "$result_file"; then
        ALL_OK="no"
    fi
done

cat "$RESULTS_FILE"

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
