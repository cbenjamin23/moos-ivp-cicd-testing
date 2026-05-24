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
MAX_TIME="35"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="12400"
PORT_BASE_SET="no"
PORT_STRIDE="30"
KEEP_WORKDIRS="no"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/pnodereporter_missions/pnodereporter_unit"
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
        echo "  ./zlaunch.sh --case=nav_report_baseline_pass"
        echo "  ./zlaunch.sh --jobs=4 --port_base=12400"
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
#  Part 4: Define cleanup, patching, and payload-check helpers.
#------------------------------------------------------------
wait_for_result_line() {
    local results_path="$1"
    local attempts="${2:-120}"
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

payload_has() {
    local line="$1"
    local token="$2"
    echo "$line" | grep -Fq "$token"
}

check_payload() {
    local line="$1"
    local missing=""
    local unexpected=""
    local token

    for token in $CHECK_TOKENS; do
        if ! payload_has "$line" "$token"; then
            missing="${missing}${missing:+,}$token"
        fi
    done

    for token in $ABSENT_TOKENS; do
        if payload_has "$line" "$token"; then
            unexpected="${unexpected}${unexpected:+,}$token"
        fi
    done

    if [ "$missing" = "" ] && [ "$unexpected" = "" ]; then
        echo "pass"
        return 0
    fi

    echo "fail:missing=${missing:-none}:unexpected=${unexpected:-none}"
    return 1
}

get_case_config() {
    CASE_NAME="$1"
    SHORE_PATCH=""
    CHECK_TOKENS=""
    ABSENT_TOKENS=""

    case " ${ALL_CASES[*]} " in
        *" $CASE_NAME "*) ;;
        *)
            echo "$ME: Unknown case: [$CASE_NAME]"
            return 1
            ;;
    esac

    if [ "$CASE_NAME" = "nav_report_baseline_pass" ]; then
        CHECK_TOKENS="node_seen=true first_seen=true NAME=reporter TYPE=kayak COLOR=red X=10 Y=-5 SPD=2.5 HDG=123 DEP=7 LENGTH=4.2 BEAM=1.4 MODE=MODE@ACTIVE:TRANSIT first_report=NAME=reporter"
    elif [ "$CASE_NAME" = "platform_metadata_pass" ]; then
        CHECK_TOKENS="NAME=reporter TYPE=uuv COLOR=blue LENGTH=9.5 BEAM=2.1 GROUP=survey PAYLOAD=ctd ROLE=lead"
    elif [ "$CASE_NAME" = "helm_nohelm_mode_pass" ]; then
        CHECK_TOKENS="NAME=reporter MODE=NOHELM-EVER"
    elif [ "$CASE_NAME" = "helm_standby_switch_pass" ]; then
        CHECK_TOKENS="NAME=reporter MODE=MODE@ACTIVE:STANDBY_TRACK"
    elif [ "$CASE_NAME" = "helm_stale_threshold_pass" ]; then
        CHECK_TOKENS="NAME=reporter MODE=NOHELM-"
    elif [ "$CASE_NAME" = "color_change_blocked_pass" ]; then
        CHECK_TOKENS="NAME=reporter COLOR=red"
        ABSENT_TOKENS="COLOR=yellow"
    elif [ "$CASE_NAME" = "color_change_allowed_pass" ]; then
        CHECK_TOKENS="NAME=reporter COLOR=yellow"
    elif [ "$CASE_NAME" = "platform_report_pass" ]; then
        CHECK_TOKENS="platform_seen=true platform_report=platform=reporter batt=12.7 sats=9"
    elif [ "$CASE_NAME" = "platform_report_gap_pass" ]; then
        CHECK_TOKENS="platform_seen=true platform_report=platform=reporter batt=12.8"
        ABSENT_TOKENS="batt=12.7"
    elif [ "$CASE_NAME" = "rider_payload_pass" ]; then
        CHECK_TOKENS="NAME=reporter batt=12.7 payload=armed"
    elif [ "$CASE_NAME" = "custom_output_var_pass" ]; then
        CHECK_TOKENS="my_seen=true my_first_seen=true my_report=NAME=reporter"
        ABSENT_TOKENS="node_report=NAME=reporter"
    elif [ "$CASE_NAME" = "json_only_report_pass" ]; then
        CHECK_TOKENS='node_seen=true node_report={"NAME":"reporter" "TYPE":"kayak"'
        ABSENT_TOKENS="node_report=NAME=reporter"
    elif [ "$CASE_NAME" = "json_dual_report_pass" ]; then
        CHECK_TOKENS='node_seen=true json_seen=true node_report=NAME=reporter json_report={"NAME":"reporter"'
    elif [ "$CASE_NAME" = "alt_nav_report_pass" ]; then
        CHECK_TOKENS="node_seen=true NAME=reporter_GT GROUP=truth X=20 Y=-15 SPD=3.1 HDG=210"
    elif [ "$CASE_NAME" = "alt_nav_named_absolute_pass" ]; then
        CHECK_TOKENS="node_seen=true NAME=truth_node GROUP=truth_abs X=22 Y=-12 DEP=4 ALTITUDE=40"
    elif [ "$CASE_NAME" = "coord_policy_global_pass" ]; then
        CHECK_TOKENS="NAME=reporter LAT=43.82535 LON=-70.33045"
        ABSENT_TOKENS="X=10 Y=-5"
    elif [ "$CASE_NAME" = "heading_error_pass" ]; then
        CHECK_TOKENS="NAME=reporter HDG=128"
        ABSENT_TOKENS="HDG=123"
    elif [ "$CASE_NAME" = "crossfill_local_to_global_pass" ]; then
        CHECK_TOKENS="NAME=reporter X=12 Y=-8 LAT=43.825"
    elif [ "$CASE_NAME" = "crossfill_global_to_local_pass" ]; then
        CHECK_TOKENS="NAME=reporter LAT=43.82536 LON=-70.33046 X="
    elif [ "$CASE_NAME" = "crossfill_global_terse_pass" ]; then
        CHECK_TOKENS="NAME=reporter LAT=43.82536 LON=-70.33046 X="
    elif [ "$CASE_NAME" = "node_group_update_pass" ]; then
        CHECK_TOKENS="NAME=reporter GROUP=dynamic"
    elif [ "$CASE_NAME" = "platform_color_mail_pass" ]; then
        CHECK_TOKENS="NAME=reporter COLOR=orange"
    elif [ "$CASE_NAME" = "reverse_load_warning_pass" ]; then
        CHECK_TOKENS="NAME=reporter THRUST_MODE_REVERSE=true LOAD_WARNING=pHelmIvP:5"
    elif [ "$CASE_NAME" = "blackout_interval_reset_fail" ]; then
        CHECK_TOKENS="evidence=blackout_gap_below_threshold node_seen=true pnr_gap="
    elif [ "$CASE_NAME" = "mhash_odometer_pass" ]; then
        CHECK_TOKENS="pnr_mhash_seen=true pnr_mhash=mhash="
    elif [ "$CASE_NAME" = "report_cog_pass" ]; then
        CHECK_TOKENS="NAME=reporter COG="
    elif [ "$CASE_NAME" = "terse_reports_pass" ]; then
        CHECK_TOKENS="NAME=reporter X=10 Y=-5 SPD=2.5 HDG=123 TYPE=kayak COLOR=red"
        ABSENT_TOKENS="DEP= LAT= LON= YAW="
    elif [ "$CASE_NAME" = "vsource_aux_allstop_pass" ]; then
        CHECK_TOKENS="NAME=reporter VSOURCE=ais MODE_AUX=survey ALLSTOP=manual"
    elif [ "$CASE_NAME" = "pause_resume_pass" ]; then
        CHECK_TOKENS="NAME=reporter X=30 Y=-10"
        ABSENT_TOKENS="X=1,Y=1"
    elif [ "$CASE_NAME" = "crossfill_fill_empty_pass" ]; then
        CHECK_TOKENS="NAME=reporter LAT=43.82537 LON=-70.33047 X="
    elif [ "$CASE_NAME" = "crossfill_use_latest_local_pass" ]; then
        CHECK_TOKENS="NAME=reporter X=25 Y=5 LAT=43.825"
    elif [ "$CASE_NAME" = "extrap_gap_metric_pass" ]; then
        CHECK_TOKENS="node_seen=true extrap_pos= extrap_hdg="
    elif [ "$CASE_NAME" = "extrap_prune_pass" ]; then
        CHECK_TOKENS="node_seen=true X=0 Y=0 extrap_pos= extrap_hdg="
        ABSENT_TOKENS="X=12"
    fi

    if [ "$CASE_NAME" != "nav_report_baseline_pass" ]; then
        local patch_name="${CASE_NAME//_/-}.xmoos"
        SHORE_PATCH="$HARNESS_DIR/$patch_name"
    fi

    if [ "$SHORE_PATCH" != "" ] && [ ! -f "$SHORE_PATCH" ]; then
        echo "$ME: Missing case patch: [$SHORE_PATCH]"
        return 1
    fi

    return 0
}

grade_from_line() {
    echo "$1" | sed -n 's/.*grade=\([^, ]*\).*/\1/p'
}

mission_line_for_failure() {
    local line="$1"
    if [ "$line" = "" ]; then
        return 0
    fi
    echo "$line" | sed 's/grade=/mission_grade=/'
}

mission_line_without_grade() {
    local line="$1"
    echo "$line" | sed 's/grade=[^, ]*[[:space:]]*//'
}

case_failure_reason() {
    local launch_rc="$1"
    local mission_grade="$2"
    local token_status="$3"

    if [ "$launch_rc" != 0 ]; then
        echo "launch_error"
    elif [ "$mission_grade" = "missing" ]; then
        echo "missing_result"
    elif [ "$mission_grade" != "pass" ]; then
        echo "mission_grade_mismatch"
    elif [ "${token_status%%:*}" != "pass" ]; then
        echo "token_check_failed"
    else
        echo "unknown"
    fi
}

format_case_row() {
    local case_name="$1"
    local launch_rc="$2"
    local line="$3"
    local token_status="$4"
    local mission_grade
    local success_line
    local reason
    local failure_line

    mission_grade=$(grade_from_line "$line")
    if [ "$mission_grade" = "" ]; then
        mission_grade="missing"
    fi

    if [ "$launch_rc" = 0 ] && [ "$mission_grade" = "pass" ] && [ "${token_status%%:*}" = "pass" ]; then
        success_line=$(mission_line_without_grade "$line")
        echo "case=$case_name  grade=$mission_grade  $success_line  token_check=$token_status"
        return 0
    fi

    reason=$(case_failure_reason "$launch_rc" "$mission_grade" "$token_status")
    failure_line=$(mission_line_for_failure "$line")
    echo "case=$case_name  grade=fail  reason=$reason  launch_rc=$launch_rc  token_check=$token_status  $failure_line"
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
run_case() {
    local case_name="$1"
    local case_idx="${RUN_CASE_IDX:-0}"
    RUN_CASE_IDX=$((case_idx + 1))
    local shore_mport
    local shore_pshare
    local case_base
    local xargs
    local line
    local launch_rc
    local token_status
    local case_row

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
        if [ "$launch_rc" = 0 ]; then
            echo "case=$case_name  grade=pass  reason=just_make" >> "$RESULTS_FILE"
        else
            echo "case=$case_name  grade=fail  reason=launch_error  launch_rc=$launch_rc" >> "$RESULTS_FILE"
            ALL_OK="no"
        fi
        return 0
    fi

    line=$(wait_for_result_line results.txt 120)
    token_status=$(check_payload "$line")
    case_row=$(format_case_row "$case_name" "$launch_rc" "$line" "$token_status")
    if ! echo "$case_row" | grep -Eq '(^|[[:space:]])grade=pass([[:space:]]|$)'; then
        ALL_OK="no"
    fi

    echo "$case_row" >> "$RESULTS_FILE"
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
    local xargs
    local launch_rc
    local token_status
    local case_row

    get_case_config "$case_name" || return 1

    case_tag=$(printf "%03d_%s" "$case_idx" "$case_name")
    case_dir="$RUN_ROOT/$case_tag"
    case_row_file="$CASE_ROW_DIR/${case_tag}.txt"

    prepare_case_dir "$case_dir" || {
        echo "case=$case_name  grade=fail  reason=prepare_error" > "$case_row_file"
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
        echo "case=$case_name  grade=fail  reason=launch_error  launch_rc=$launch_rc" > "$case_row_file"
        return 1
    fi

    line=$(wait_for_result_line "$case_dir/results.txt" 120)
    token_status=$(check_payload "$line")
    case_row=$(format_case_row "$case_name" "$launch_rc" "$line" "$token_status")

    echo "$case_row" > "$case_row_file"

    if echo "$case_row" | grep -Eq '(^|[[:space:]])grade=pass([[:space:]]|$)'; then
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
    nav_report_baseline_pass
    platform_metadata_pass
    helm_nohelm_mode_pass
    helm_standby_switch_pass
    helm_stale_threshold_pass
    color_change_blocked_pass
    color_change_allowed_pass
    platform_report_pass
    platform_report_gap_pass
    rider_payload_pass
    custom_output_var_pass
    json_only_report_pass
    json_dual_report_pass
    alt_nav_report_pass
    alt_nav_named_absolute_pass
    coord_policy_global_pass
    heading_error_pass
    crossfill_local_to_global_pass
    crossfill_global_to_local_pass
    crossfill_global_terse_pass
    node_group_update_pass
    platform_color_mail_pass
    reverse_load_warning_pass
    blackout_interval_reset_fail
    mhash_odometer_pass
    report_cog_pass
    terse_reports_pass
    vsource_aux_allstop_pass
    pause_resume_pass
    crossfill_fill_empty_pass
    crossfill_use_latest_local_pass
    extrap_gap_metric_pass
    extrap_prune_pass
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
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_pnodereporter_unit_XXXXXX")
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
                if ! echo "$line" | grep -Eq '(^|[[:space:]])grade=pass([[:space:]]|$)'; then
                    ALL_OK="no"
                fi
            else
                ALL_OK="no"
                echo "case=${case_tag#???_}  grade=fail  reason=missing_case_row" >> "$RESULTS_FILE"
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
