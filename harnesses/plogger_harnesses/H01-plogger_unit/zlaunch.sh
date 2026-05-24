#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
#  Part 1: Set convenience functions for producing terminal
#          debugging output, and catching SIGINT (ctrl-c).
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

#------------------------------------------------------------
#  Part 2: Set global variable default values
#------------------------------------------------------------
ME=`basename "$0"`
CMD_ARGS=""
VERBOSE=""
JUST_MAKE=""
TIME_WARP="10"
MAX_TIME="20"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="12000"
PORT_BASE_SET="no"
PORT_STRIDE="30"
KEEP_WORKDIRS="no"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/plogger_missions/plogger_unit"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
RUN_ROOT=""
CASE_ROW_DIR=""
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"

if [ -f "$TEARDOWN_HELPER" ]; then
    . "$TEARDOWN_HELPER"
else
    echo "$ME: Missing teardown helper: $TEARDOWN_HELPER"
    exit 1
fi

#------------------------------------------------------------
#  Part 3: Check for and handle command-line arguments
#------------------------------------------------------------
for ARGI; do
    CMD_ARGS+="${ARGI} "
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo ""
        echo "Options:"
        echo "  --help, -h         Show this help message"
        echo "  --verbose, -v      Verbose, confirm launch"
        echo "  --just_make, -j    Only create targ files"
        echo "  --max_time=<secs>  Max time passed to xlaunch"
        echo "  --case=<name>      Run one named case"
        echo "  --jobs=<n>         Run up to n cases per wave"
        echo "  --port_base=<n>    Base port for per-case wave blocks"
        echo "  --keep_workdirs    Keep temp mission copies in wave mode"
        echo "  --gui              Accepted for wrapper parity"
        echo ""
        echo "Examples:"
        echo "  ./zlaunch.sh"
        echo "  ./zlaunch.sh --case=plogger_explicit_log_pass"
        echo "  ./zlaunch.sh --jobs=4 --port_base=12000"
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
    elif [ "${ARGI:0:7}" = "--jobs=" ]; then
        JOBS="${ARGI#--jobs=*}"
    elif [ "${ARGI:0:12}" = "--port_base=" ]; then
        PORT_BASE="${ARGI#--port_base=*}"
        PORT_BASE_SET="yes"
    elif [ "${ARGI}" = "--keep_workdirs" ]; then
        KEEP_WORKDIRS="yes"
    elif [ "${ARGI}" = "--gui" ]; then
        NOGUI=""
    else
        echo "$ME: Bad arg:" $ARGI "Exit Code 1."
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

#------------------------------------------------------------
#  Part 4: Define cleanup and patch application helpers.
#------------------------------------------------------------
wait_for_result_line() {
    local results_path="$1"
    local attempts="${2:-60}"
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

remove_tree() {
    local targ="$1"
    if [ "$targ" != "" ] && [ -d "$targ" ]; then
        rm -rf "$targ"
    fi
}

clear_xfiles() {
    rm -f "$SHORE_XFILE"
}

stop_mission_apps() {
    local mission_root="${1:-$MISSION_DIR}"
    harness_teardown_stop_root "$mission_root"
}

cleanup() {
    local start_dir="$PWD"
    if [ -d "$MISSION_DIR" ]; then
        cd "$MISSION_DIR"
        ./clean.sh >/dev/null 2>&1 || true
        stop_mission_apps "$MISSION_DIR"
    fi
    cd "$start_dir"
    if [ "$RUN_ROOT" != "" ]; then
        stop_mission_apps "$RUN_ROOT"
    fi
    if [ "$KEEP_WORKDIRS" != "yes" ] && [ "$RUN_ROOT" != "" ]; then
        remove_tree "$RUN_ROOT"
    fi
}

get_case_config() {
    CASE_NAME="$1"
    EXPECTED="pass"
    SHORE_PATCH=""
    EXTRA_CHECK="explicit"

    case " ${ALL_CASES[*]} " in
        *" $CASE_NAME "*) ;;
        *)
            echo "$ME: Unknown case: [$CASE_NAME]"
            return 1
            ;;
    esac

    if [ "$SHORE_PATCH" = "" ]; then
        local patch_name="${CASE_NAME//_/-}-shoreside.xmoos"
        if [ -f "$HARNESS_DIR/$patch_name" ]; then
            SHORE_PATCH="$HARNESS_DIR/$patch_name"
        fi
    fi

    if [ "$SHORE_PATCH" != "" ] && [ ! -f "$SHORE_PATCH" ]; then
        echo "$ME: Missing case patch: [$SHORE_PATCH]"
        return 1
    fi

    case "$CASE_NAME" in
        plogger_wildcard_omit_pass)
            EXTRA_CHECK="wildcard_omit"
            ;;
        plogger_dynamic_log_request_pass)
            EXTRA_CHECK="dynamic"
            ;;
        plogger_double_log_pass)
            EXTRA_CHECK="double"
            ;;
        plogger_sync_log_pass)
            EXTRA_CHECK="sync"
            ;;
        plogger_wildcard_xlog_pass)
            EXTRA_CHECK="wildcard_xlog"
            ;;
        plogger_filetimestamp_false_pass)
            EXTRA_CHECK="filetimestamp"
            ;;
        plogger_datatype_precision_pass)
            EXTRA_CHECK="datatype_precision"
            ;;
        plogger_dynamic_sync_column_pass)
            EXTRA_CHECK="dynamic_sync"
            ;;
        plogger_copy_file_request_pass)
            EXTRA_CHECK="copy_file"
            ;;
    esac

    return 0
}

check_plogger_artifact() {
    local root="$1"
    local mode="$2"
    local alogs

    alogs=$(find "$root" -type f -name '*.alog' 2>/dev/null)
    if [ "$alogs" = "" ]; then
        echo "missing_alog"
        return 1
    fi

    case "$mode" in
        explicit)
            if find "$root" -type f -name '*.alog' -exec grep -q ' PLOGGER_ALPHA .* alpha' {} +; then
                echo "alog_explicit_ok"
                return 0
            fi
            ;;
        wildcard_omit)
            if find "$root" -type f -name '*.alog' -exec grep -q ' KEEP_ME .* keep' {} + &&
               ! find "$root" -type f -name '*.alog' -exec grep -q ' OMIT_ME .* omit' {} +; then
                echo "alog_wildcard_omit_ok"
                return 0
            fi
            ;;
        dynamic)
            if find "$root" -type f -name '*.alog' -exec grep -q ' DYN_LOG .* dynamic' {} +; then
                echo "alog_dynamic_ok"
                return 0
            fi
            ;;
        double)
            if find "$root" -type f -name '*.alog' -exec grep -q ' PLOGGER_NUM ' {} +; then
                echo "alog_double_ok"
                return 0
            fi
            ;;
        sync)
            if find "$root" -type f -name '*.slog' -exec grep -q 'SYNC_ALPHA' {} +; then
                echo "slog_sync_ok"
                return 0
            fi
            ;;
        wildcard_xlog)
            if find "$root" -type f -name '*.alog' -exec grep -q ' XLOG_KEEP .* keep' {} + &&
               find "$root" -type f -name '*.xlog' -exec grep -q ' XLOG_OMIT .* omit' {} + &&
               ! find "$root" -type f -name '*.alog' -exec grep -q ' XLOG_OMIT .* omit' {} +; then
                echo "xlog_exclusion_ok"
                return 0
            fi
            ;;
        filetimestamp)
            if [ -f "$root/LOG_plogger_filetimestamp_false_pass/LOG_plogger_filetimestamp_false_pass.alog" ] &&
               grep -q ' FIXED_NAME .* fixed' "$root/LOG_plogger_filetimestamp_false_pass/LOG_plogger_filetimestamp_false_pass.alog"; then
                echo "filetimestamp_false_ok"
                return 0
            fi
            ;;
        datatype_precision)
            if find "$root" -type f -name '*.alog' -exec grep -q ' DTYPE_NUM .* D:3.142' {} + &&
               find "$root" -type f -name '*.alog' -exec grep -q ' DTYPE_STR .* S:typed' {} +; then
                echo "datatype_precision_ok"
                return 0
            fi
            ;;
        dynamic_sync)
            if find "$root" -type f -name '*.slog' -exec grep -q 'DYN_SYNC' {} +; then
                echo "dynamic_sync_ok"
                return 0
            fi
            ;;
        copy_file)
            if find "$root" -type f -name 'logger_copy_source._txt' -exec grep -q 'pLogger copy-file source artifact' {} +; then
                echo "copy_file_ok"
                return 0
            fi
            ;;
    esac

    echo "alog_check_failed"
    return 1
}
apply_case_patches() {
    clear_xfiles

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$SHORE_STEM" "$SHORE_PATCH" --targ="$SHORE_XFILE"
    fi
}

#------------------------------------------------------------
#  Part 5: Run one case in the shared stem mission directory.
#------------------------------------------------------------

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

run_case() {
    local case_name="$1"
    local case_idx="${RUN_CASE_IDX:-0}"
    RUN_CASE_IDX=$((case_idx + 1))
    local shore_mport
    local shore_pshare
    local case_base
    local xargs
    local line
    local actual
    local status
    local launch_rc

    get_case_config "$case_name" || return 1

    vecho "Preparing case: $case_name"

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    stop_mission_apps "$MISSION_DIR"
    apply_case_patches || return 1
    : > results.txt

    xargs="--max_time=$MAX_TIME --mmod=$case_name $TIME_WARP"
    if [ "$PORT_BASE_SET" = "yes" ]; then
        case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
        shore_mport=$((case_base + 0))
        shore_pshare=$((case_base + 10))
        xargs="$xargs --shore_mport=$shore_mport --shore_pshare=$shore_pshare"
    fi
    if [ "$NOGUI" != "" ]; then
        xargs="$xargs $NOGUI"
    fi
    if [ "$JUST_MAKE" = "yes" ]; then
        xargs="$xargs --just_make"
    fi

    vecho "Running case [$case_name] with xlaunch args: $xargs"
    xlaunch.sh $xargs
    launch_rc=$?

    if [ "$JUST_MAKE" = "yes" ]; then
        cd "$HARNESS_DIR"
        return "$launch_rc"
    fi

    line=$(wait_for_result_line results.txt 120)
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="success"
    if [ "$launch_rc" != 0 ] || [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
        ALL_OK="no"
    fi
    artifact_status=$(check_plogger_artifact "$MISSION_DIR" "$EXTRA_CHECK")
    if [ "$artifact_status" = "" ]; then
        artifact_status="alog_check_failed"
    fi
    if ! echo "$artifact_status" | grep -q '_ok$'; then
        status="mismatch"
        ALL_OK="no"
    fi

    emit_case_row "$case_name" "$status" "$EXPECTED" "$actual" "$line" "artifact=$artifact_status" >> "$RESULTS_FILE"
    cd "$HARNESS_DIR"
}

#------------------------------------------------------------
#  Part 6: Prepare and run one isolated case copy.
#------------------------------------------------------------
prepare_case_dir() {
    local case_dir="$1"
    local shore_stem="$case_dir/meta_shoreside.moos"
    local shore_xfile="$case_dir/meta_shoreside.moosx"

    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
    )

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$shore_stem" "$SHORE_PATCH" --targ="$shore_xfile"
    fi
}

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    local case_tag
    local case_dir
    local case_row_file
    local shore_mport
    local shore_pshare
    local case_base
    local line
    local actual
    local status
    local xargs
    local launch_rc

    get_case_config "$case_name" || return 1

    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    case_row_file="$CASE_ROW_DIR/${case_tag}.txt"

    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  grade=fail  reason=script_error" > "$case_row_file"
        return 1
    }

    case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    shore_mport=$((case_base + 0))
    shore_pshare=$((case_base + 10))

    (
        cd "$case_dir"
        : > results.txt
        xargs="--max_time=$MAX_TIME --mmod=$case_name --shore_mport=$shore_mport --shore_pshare=$shore_pshare $TIME_WARP"
        if [ "$NOGUI" != "" ]; then
            xargs="$xargs $NOGUI"
        fi
        if [ "$JUST_MAKE" = "yes" ]; then
            xargs="$xargs --just_make"
        fi
        xlaunch.sh $xargs
    )
    launch_rc=$?

    if [ "$JUST_MAKE" = "yes" ]; then
        if [ "$launch_rc" = 0 ]; then
            echo "case=$case_name  grade=pass  reason=just_make" > "$case_row_file"
            return 0
        fi
        echo "case=$case_name  grade=fail  reason=script_error" > "$case_row_file"
        return 1
    fi

    line=$(wait_for_result_line "$case_dir/results.txt" 120)
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="success"
    if [ "$launch_rc" != 0 ] || [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
    fi
    artifact_status=$(check_plogger_artifact "$case_dir" "$EXTRA_CHECK")
    if [ "$artifact_status" = "" ]; then
        artifact_status="alog_check_failed"
    fi
    if ! echo "$artifact_status" | grep -q '_ok$'; then
        status="mismatch"
    fi

    emit_case_row "$case_name" "$status" "$EXPECTED" "$actual" "$line" "artifact=$artifact_status" > "$case_row_file"

    if [ "$status" = "success" ]; then
        return 0
    fi
    return 1
}

#------------------------------------------------------------
#  Part 7: Validate the mission path, select the case set,
#          run the matrix, and report.
#------------------------------------------------------------
if [ ! -d "$MISSION_DIR" ]; then
    echo "$ME: Mission dir not found: [$MISSION_DIR]"
    exit 1
fi

trap cleanup EXIT

ALL_CASES=(
    plogger_explicit_log_pass
    plogger_wildcard_omit_pass
    plogger_dynamic_log_request_pass
    plogger_double_log_pass
    plogger_sync_log_pass
    plogger_wildcard_xlog_pass
    plogger_filetimestamp_false_pass
    plogger_datatype_precision_pass
    plogger_dynamic_sync_column_pass
    plogger_copy_file_request_pass
)
RUN_CASES=("${ALL_CASES[@]}")
if [ "$CASE" != "" ]; then
    RUN_CASES=("$CASE")
fi

: > "$RESULTS_FILE"

if [ "$JOBS" -le 1 ] || [ "$CASE" != "" ]; then
    for case_name in "${RUN_CASES[@]}"; do
        run_case "$case_name" || {
            echo "case=$case_name  grade=fail  reason=script_error" >> "$RESULTS_FILE"
            ALL_OK="no"
            if [ "$JUST_MAKE" != "yes" ]; then
                break
            fi
        }
    done
else
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_plogger_unit_XXXXXX")
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
            if [ -f "$CASE_ROW_DIR/${case_tag}.txt" ]; then
                cat "$CASE_ROW_DIR/${case_tag}.txt" >> "$RESULTS_FILE"
                echo >> "$RESULTS_FILE"
                line=`tail -n 2 "$RESULTS_FILE" | head -n 1`
                case_grade=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
                if [ "$case_grade" != "pass" ]; then
                    ALL_OK="no"
                fi
            else
                ALL_OK="no"
            fi
        done

        stop_mission_apps "$RUN_ROOT"
        remaining_cases="$next_remaining"
    done
fi

cat "$RESULTS_FILE"

if [ "$ALL_OK" = "yes" ]; then
    exit 0
fi
exit 1
