#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-pspoofnode_unit
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
PORT_BASE="16000"
PORT_STRIDE="20"
CASE=""
JUST_MAKE="no"
KEEP_WORKDIRS="no"
VERBOSE=""

HARNESS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/pspoofnode_missions/pspoofnode_unit"
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
    config_static_report_pass
    mail_spoof_report_pass
    mail_group_vsource_pass
    default_fields_pass
    generated_name_pass
    moving_contact_advances_pass
    duration_expires_pass
    explicit_zero_duration_persists_pass
    duplicate_name_update_pass
    cancel_vname_pass
    cancel_group_pass
    cancel_contact_alias_pass
    runtime_defaults_pass
    default_after_spoof_order_pass
    runtime_length_ignored_pass
    default_group_vsource_not_inherited_pass
    default_duration_expires_pass
    invalid_default_vtype_fallback_pass
    invalid_default_color_fallback_pass
    mixed_case_request_normalized_pass
    runtime_color_unvalidated_pass
    unknown_alnum_type_preserved_pass
    negative_speed_accepted_pass
    invalid_request_absent_pass
    invalid_x_absent_pass
    invalid_name_absent_pass
    invalid_group_absent_pass
    invalid_type_absent_pass
    invalid_vsource_absent_pass
    invalid_heading_absent_pass
    invalid_speed_absent_pass
    invalid_length_absent_pass
    invalid_duration_absent_pass
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
    SPOOF_LINES=()
    TIMER_EVENTS=()
    REQUIRED_TOKENS=()
    ABSENT_TOKENS=()
    ABSENT_AFTER_TIME=""
    ABSENT_AFTER_MARKER=""
    ABSENT_AFTER_TOKENS=()
    REQUIRED_AFTER_TIME=""
    REQUIRED_AFTER_MARKER=""
    REQUIRED_AFTER_TOKENS=()
    ABSENT_FOR_REQUIRED_NAME=""
    ABSENT_FOR_REQUIRED_TOKENS=()
    MOVEMENT_NAME=""
    EVAL_TIME="1.8"

    if ! case_known "$CASE_NAME"; then
        echo "$ME: Unknown case: $CASE_NAME"
        return 1
    fi

    case "$CASE_NAME" in
        config_static_report_pass)
            SPOOF_LINES=("spoof = x=10,y=20,name=alpha,type=heron,group=red,color=green,vsource=ais,hdg=90,spd=0")
            REQUIRED_TOKENS=("NAME=alpha" "TYPE=heron" "GROUP=red" "COLOR=green" "VSOURCE=ais" "HDG=90")
            ;;
        mail_spoof_report_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=5,y=-3,name=bravo,type=kayak,color=yellow,hdg=180,spd=0\", time=0.3")
            REQUIRED_TOKENS=("NAME=bravo" "TYPE=kayak" "COLOR=yellow" "HDG=180")
            ;;
        mail_group_vsource_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=4,y=6,name=papa,group=team,type=ship,color=yellow,vsource=radar,hdg=270,spd=0\", time=0.3")
            REQUIRED_TOKENS=("NAME=papa" "GROUP=team" "TYPE=ship" "COLOR=yellow" "VSOURCE=radar" "HDG=270")
            ;;
        default_fields_pass)
            SPOOF_LINES=("default_vtype = ship" "default_group = fleet" "default_color = orange" "default_vsource = radar" "default_hdg = 135" "default_spd = 0.4" "default_length = 8" "spoof = x=0,y=0,name=charlie")
            REQUIRED_TOKENS=("NAME=charlie" "TYPE=ship" "COLOR=orange" "HDG=135" "SPD=0.4" "LENGTH=8")
            ;;
        generated_name_pass)
            SPOOF_LINES=("spoof = x=1,y=2,type=auv,color=blue")
            REQUIRED_TOKENS=("NAME=C" "TYPE=auv" "COLOR=blue")
            ;;
        moving_contact_advances_pass)
            SPOOF_LINES=("spoof = x=0,y=0,name=delta,hdg=90,spd=2")
            REQUIRED_TOKENS=("NAME=delta" "SPD=2" "HDG=90")
            MOVEMENT_NAME="delta"
            EVAL_TIME="2.2"
            ;;
        duration_expires_pass)
            SPOOF_LINES=("spoof = x=2,y=2,name=echo,dur=0.7")
            REQUIRED_TOKENS=("NAME=echo")
            ABSENT_AFTER_TIME="1.4"
            ABSENT_AFTER_TOKENS=("NAME=echo")
            EVAL_TIME="2.0"
            ;;
        explicit_zero_duration_persists_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=3,y=3,name=quebec,dur=0\", time=0.2")
            REQUIRED_TOKENS=("NAME=quebec")
            REQUIRED_AFTER_TIME="1.3"
            REQUIRED_AFTER_TOKENS=("NAME=quebec")
            EVAL_TIME="2.0"
            ;;
        duplicate_name_update_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=1,y=1,name=romeo\", time=0.2" "event = var=SPOOF, val=\"x=8,y=8,name=romeo\", time=0.9")
            REQUIRED_TOKENS=("NAME=romeo")
            REQUIRED_AFTER_TIME="1.3"
            REQUIRED_AFTER_TOKENS=("NAME=romeo" "X=8" "Y=8")
            ABSENT_AFTER_TIME="1.3"
            ABSENT_AFTER_TOKENS=("X=1" "Y=1")
            EVAL_TIME="2.0"
            ;;
        cancel_vname_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=2,y=2,name=foxtrot,group=red\", time=0.2" "event = var=SPOOF, val=\"x=4,y=4,name=golf,group=red\", time=0.2" "event = var=SPOOF_CANCEL, val=\"vname=foxtrot\", time=0.9")
            REQUIRED_TOKENS=("NAME=foxtrot" "NAME=golf")
            ABSENT_AFTER_TIME="1.3"
            ABSENT_AFTER_MARKER="SPOOF_CANCEL"
            ABSENT_AFTER_TOKENS=("NAME=foxtrot")
            ;;
        cancel_group_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=2,y=2,name=hotel,group=red\", time=0.2" "event = var=SPOOF, val=\"x=4,y=4,name=india,group=blue\", time=0.2" "event = var=SPOOF_CANCEL, val=\"group=red\", time=0.9")
            REQUIRED_TOKENS=("NAME=hotel" "NAME=india")
            ABSENT_AFTER_TIME="1.3"
            ABSENT_AFTER_MARKER="SPOOF_CANCEL"
            ABSENT_AFTER_TOKENS=("NAME=hotel")
            ;;
        cancel_contact_alias_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=2,y=2,name=kilo,group=red\", time=0.2" "event = var=SPOOF, val=\"x=4,y=4,name=lima,group=red\", time=0.2" "event = var=SPOOF_CANCEL, val=\"contact=kilo\", time=0.9")
            REQUIRED_TOKENS=("NAME=kilo" "NAME=lima")
            ABSENT_AFTER_TIME="1.3"
            ABSENT_AFTER_MARKER="SPOOF_CANCEL"
            ABSENT_AFTER_TOKENS=("NAME=kilo")
            ;;
        runtime_defaults_pass)
            SPOOF_LINES=("default_vtype = ship" "default_group = runtime" "default_color = orange" "default_vsource = radar" "default_hdg = 135" "default_spd = 0.4" "default_length = 8")
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=6,y=7,name=mike\", time=0.3")
            REQUIRED_TOKENS=("NAME=mike" "TYPE=ship" "COLOR=orange" "HDG=135" "SPD=0.4" "LENGTH=8")
            ;;
        default_after_spoof_order_pass)
            SPOOF_LINES=("spoof = x=4,y=5,name=orderly" "default_vtype = ship" "default_color = orange" "default_hdg = 222" "default_length = 9")
            REQUIRED_TOKENS=("NAME=orderly" "TYPE=ship" "COLOR=orange" "HDG=222" "LENGTH=9")
            ;;
        runtime_length_ignored_pass)
            SPOOF_LINES=("default_length = 8")
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=6,y=7,name=victor,len=12\", time=0.3")
            REQUIRED_TOKENS=("NAME=victor" "LENGTH=8")
            ABSENT_FOR_REQUIRED_NAME="victor"
            ABSENT_FOR_REQUIRED_TOKENS=("LENGTH=12")
            ;;
        default_group_vsource_not_inherited_pass)
            SPOOF_LINES=("default_group = fleet" "default_vsource = radar" "spoof = x=2,y=2,name=whiskey")
            REQUIRED_TOKENS=("NAME=whiskey")
            ABSENT_FOR_REQUIRED_NAME="whiskey"
            ABSENT_FOR_REQUIRED_TOKENS=("GROUP=fleet" "VSOURCE=radar")
            ;;
        default_duration_expires_pass)
            SPOOF_LINES=("default_duration = 0.7" "spoof = x=2,y=2,name=november")
            REQUIRED_TOKENS=("NAME=november")
            ABSENT_AFTER_TIME="1.4"
            ABSENT_AFTER_TOKENS=("NAME=november")
            EVAL_TIME="2.0"
            ;;
        invalid_default_vtype_fallback_pass)
            SPOOF_LINES=("default_vtype = dragon" "spoof = x=2,y=2,name=xray")
            REQUIRED_TOKENS=("NAME=xray" "TYPE=kayak")
            ABSENT_FOR_REQUIRED_NAME="xray"
            ABSENT_FOR_REQUIRED_TOKENS=("TYPE=dragon")
            ;;
        invalid_default_color_fallback_pass)
            SPOOF_LINES=("default_color = notacolor" "spoof = x=2,y=2,name=colorbad")
            REQUIRED_TOKENS=("NAME=colorbad" "COLOR=purple")
            ABSENT_FOR_REQUIRED_NAME="colorbad"
            ABSENT_FOR_REQUIRED_TOKENS=("COLOR=notacolor")
            ;;
        mixed_case_request_normalized_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=3,y=3,name=MiXeD,type=Heron,group=TeamA,color=Yellow,vsource=Radar\", time=0.2")
            REQUIRED_TOKENS=("NAME=mixed" "TYPE=heron" "GROUP=teama" "COLOR=yellow" "VSOURCE=radar")
            ;;
        runtime_color_unvalidated_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=3,y=3,name=colorfree,color=notacolor\", time=0.2")
            REQUIRED_TOKENS=("NAME=colorfree" "COLOR=notacolor")
            ;;
        unknown_alnum_type_preserved_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=3,y=3,name=yankee,type=dragon\", time=0.2")
            REQUIRED_TOKENS=("NAME=yankee" "TYPE=dragon")
            ;;
        negative_speed_accepted_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=3,y=3,name=backward,spd=-1,hdg=90\", time=0.2")
            REQUIRED_TOKENS=("NAME=backward" "SPD=-1" "HDG=90")
            ;;
        invalid_request_absent_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=9,name=juliet\", time=0.2")
            ABSENT_TOKENS=("NAME=juliet")
            ;;
        invalid_x_absent_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=west,y=9,name=zulu\", time=0.2")
            ABSENT_TOKENS=("NAME=zulu")
            ;;
        invalid_name_absent_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=9,y=9,name=bad-name\", time=0.2")
            ABSENT_TOKENS=("NAME=bad-name" "NAME=bad")
            ;;
        invalid_group_absent_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=9,y=9,name=sierra,group=bad-team\", time=0.2")
            ABSENT_TOKENS=("NAME=sierra")
            ;;
        invalid_type_absent_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=9,y=9,name=alpha2,type=bad-type\", time=0.2")
            ABSENT_TOKENS=("NAME=alpha2")
            ;;
        invalid_vsource_absent_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=9,y=9,name=bravo2,vsource=bad-source\", time=0.2")
            ABSENT_TOKENS=("NAME=bravo2")
            ;;
        invalid_heading_absent_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=9,y=9,name=charlie2,hdg=east\", time=0.2")
            ABSENT_TOKENS=("NAME=charlie2")
            ;;
        invalid_speed_absent_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=9,y=9,name=tango,spd=fast\", time=0.2")
            ABSENT_TOKENS=("NAME=tango")
            ;;
        invalid_length_absent_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=9,y=9,name=uniform,len=long\", time=0.2")
            ABSENT_TOKENS=("NAME=uniform")
            ;;
        invalid_duration_absent_pass)
            TIMER_EVENTS=("event = var=SPOOF, val=\"x=9,y=9,name=oscar,dur=soon\", time=0.2")
            ABSENT_TOKENS=("NAME=oscar")
            ;;
    esac
}

write_case_files() {
    local case_dir="$1"
    local item
    {
        echo "ProcessConfig = pSpoofNode"
        echo "{"
        echo "  AppTick   = 10"
        echo "  CommsTick = 10"
        echo ""
        echo "  refresh_interval = 0.2"
        for item in "${SPOOF_LINES[@]}"; do
            echo "  $item"
        done
        echo "}"
    } > "$case_dir/case_spoof.moos"

    {
        echo "ProcessConfig = uTimerScript"
        echo "{"
        echo "  AppTick   = 10"
        echo "  CommsTick = 10"
        echo ""
        for item in "${TIMER_EVENTS[@]}"; do
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
        echo "  mission_form = pspoofnode_unit"
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

node_reports_for_case() {
    local case_dir="$1"
    local alog
    alog=$(find "$case_dir" -name '*.alog' | head -n 1)
    if [ "$alog" = "" ]; then
        return 1
    fi
    aloggrep NODE_REPORT "$alog" | grep 'NODE_REPORT' | grep -v '^%'
}

field_from_report() {
    local report="$1"
    local key="$2"
    echo "$report" | sed -n "s/.*$key=\\([^, ]*\\).*/\\1/p"
}

check_payloads() {
    local case_dir="$1"
    local reports="$2"
    local status="success"
    local token
    local base_time
    local threshold
    local alog
    local after_reports
    local first_report
    local last_report
    local first_x
    local last_x

    for token in "${REQUIRED_TOKENS[@]}"; do
        if ! echo "$reports" | grep -q "$token"; then
            status="mismatch"
        fi
    done
    for token in "${ABSENT_TOKENS[@]}"; do
        if echo "$reports" | grep -q "$token"; then
            status="mismatch"
        fi
    done
    if [ "$ABSENT_FOR_REQUIRED_NAME" != "" ]; then
        local named_reports
        named_reports=$(echo "$reports" | grep "NAME=$ABSENT_FOR_REQUIRED_NAME" || true)
        for token in "${ABSENT_FOR_REQUIRED_TOKENS[@]}"; do
            if echo "$named_reports" | grep -q "$token"; then
                status="mismatch"
            fi
        done
    fi
    if [ "$ABSENT_AFTER_TIME" != "" ]; then
        if [ "$ABSENT_AFTER_MARKER" != "" ]; then
            alog=$(find "$case_dir" -name '*.alog' | head -n 1)
            base_time=$(aloggrep "$ABSENT_AFTER_MARKER" "$alog" | grep "$ABSENT_AFTER_MARKER" | grep -v '^%' | head -n 1 | awk '{print $1}')
        else
            base_time=$(echo "$reports" | head -n 1 | awk '{print $1}')
        fi
        threshold=$(awk -v base="$base_time" -v offset="$ABSENT_AFTER_TIME" 'BEGIN { print base + offset }')
        after_reports=$(echo "$reports" | awk -v min_time="$threshold" '$1+0 > min_time {print}')
        for token in "${ABSENT_AFTER_TOKENS[@]}"; do
            if echo "$after_reports" | grep -q "$token"; then
                status="mismatch"
            fi
        done
    fi
    if [ "$REQUIRED_AFTER_TIME" != "" ]; then
        if [ "$REQUIRED_AFTER_MARKER" != "" ]; then
            alog=$(find "$case_dir" -name '*.alog' | head -n 1)
            base_time=$(aloggrep "$REQUIRED_AFTER_MARKER" "$alog" | grep "$REQUIRED_AFTER_MARKER" | grep -v '^%' | head -n 1 | awk '{print $1}')
        else
            base_time=$(echo "$reports" | head -n 1 | awk '{print $1}')
        fi
        threshold=$(awk -v base="$base_time" -v offset="$REQUIRED_AFTER_TIME" 'BEGIN { print base + offset }')
        after_reports=$(echo "$reports" | awk -v min_time="$threshold" '$1+0 > min_time {print}')
        for token in "${REQUIRED_AFTER_TOKENS[@]}"; do
            if ! echo "$after_reports" | grep -q "$token"; then
                status="mismatch"
            fi
        done
    fi
    if [ "$MOVEMENT_NAME" != "" ]; then
        first_report=$(echo "$reports" | grep "NAME=$MOVEMENT_NAME" | head -n 1)
        last_report=$(echo "$reports" | grep "NAME=$MOVEMENT_NAME" | tail -n 1)
        first_x=$(field_from_report "$first_report" "X")
        last_x=$(field_from_report "$last_report" "X")
        if ! awk -v first="$first_x" -v last="$last_x" 'BEGIN { exit !(last > first + 0.1) }'; then
            status="mismatch"
        fi
    fi
    echo "$status"
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
    local reports=""
    local payload_status

    get_case_config "$case_name" || return 1

    tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$tag"
    case_row_file="$CASE_ROW_DIR/$tag.txt"
    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    write_case_files "$case_dir"

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
        uMayFinish --max_time="$MAX_TIME" targ_shoreside.moos > mayfinish.out 2>&1 || true
    )
    line=$(wait_for_results "$case_dir/results.txt" 120)
    actual=$(echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p')
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    reports=$(node_reports_for_case "$case_dir")
    payload_status=$(check_payloads "$case_dir" "$reports")
    if [ "$actual" != "pass" ] || [ "$payload_status" != "success" ]; then
        status="mismatch"
    fi

    emit_case_row "$case_name" "$status" "pass" "$actual" "$line" "payload=$payload_status" "reports=$(echo "$reports" | wc -l | tr -d ' ')" > "$case_row_file"
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
RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_pspoofnode_unit_XXXXXX")
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
