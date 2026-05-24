#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
vecho() { if [ "$VERBOSE" != "" ]; then echo "$ME: $1"; fi }
on_exit() { echo; echo "$ME: Halting all apps"; kill -- -$$; }
trap on_exit SIGINT
trap "echo zlaunch.sh has received sigterm" SIGTERM

ME=`basename "$0"`
CMD_ARGS=""
VERBOSE=""
JUST_MAKE=""
TIME_WARP="10"
MAX_TIME="65"
NOGUI="--nogui"
CASE=""
JOBS="1"
PORT_BASE="11000"
PORT_BASE_SET="no"
PORT_STRIDE="50"
KEEP_WORKDIRS="no"

HARNESS_DIR="${PWD}"
REPO_DIR="$(cd "$HARNESS_DIR/../../.." && pwd)"
MISSION_DIR="$REPO_DIR/missions/pshare_missions/pshare_topology"
TEARDOWN_HELPER="$REPO_DIR/scripts/harness_teardown.sh"
RESULTS_FILE="$HARNESS_DIR/results.txt"
ALL_OK="yes"
RUN_ROOT=""
CASE_ROW_DIR=""
SHORE_STEM="$MISSION_DIR/meta_shoreside.moos"
ALPHA_STEM="$MISSION_DIR/meta_alpha.moos"
BRAVO_STEM="$MISSION_DIR/meta_bravo.moos"
RELAY_STEM="$MISSION_DIR/meta_relay.moos"
SHORE_XFILE="$MISSION_DIR/meta_shoreside.moosx"
ALPHA_XFILE="$MISSION_DIR/meta_alpha.moosx"
BRAVO_XFILE="$MISSION_DIR/meta_bravo.moosx"
RELAY_XFILE="$MISSION_DIR/meta_relay.moosx"

if [ -f "$TEARDOWN_HELPER" ]; then
    . "$TEARDOWN_HELPER"
else
    echo "$ME: Missing teardown helper: $TEARDOWN_HELPER"
    exit 1
fi

for ARGI; do
    CMD_ARGS+="${ARGI} "
    if [ "${ARGI}" = "--help" -o "${ARGI}" = "-h" ]; then
        echo "$ME [OPTIONS] [time_warp]"
        echo "  --case=<name>      Run one named case"
        echo "  --jobs=<n>         Run up to n cases per wave"
        echo "  --port_base=<n>    Base port for per-case wave blocks"
        echo "  --max_time=<secs>  Max time passed to xlaunch"
        echo "  --just_make, -j    Only create targ files"
        echo "  --keep_workdirs    Keep temp mission copies in wave mode"
        echo "  --gui              Accepted for wrapper parity"
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

wait_for_result_line() {
    local results_path="$1"
    local attempts="${2:-80}"
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
    rm -f "$SHORE_XFILE" "$ALPHA_XFILE" "$BRAVO_XFILE" "$RELAY_XFILE"
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
    ALPHA_PATCH=""
    BRAVO_PATCH=""
    RELAY_PATCH=""

    case " ${ALL_CASES[*]} " in
        *" $CASE_NAME "*) ;;
        *)
            echo "$ME: Unknown case: [$CASE_NAME]"
            return 1
            ;;
    esac

    local patch_stem="${CASE_NAME//_/-}"
    if [ -f "$HARNESS_DIR/$patch_stem-shoreside.xmoos" ]; then
        SHORE_PATCH="$HARNESS_DIR/$patch_stem-shoreside.xmoos"
    fi
    if [ -f "$HARNESS_DIR/$patch_stem-alpha.xmoos" ]; then
        ALPHA_PATCH="$HARNESS_DIR/$patch_stem-alpha.xmoos"
    fi
    if [ -f "$HARNESS_DIR/$patch_stem-bravo.xmoos" ]; then
        BRAVO_PATCH="$HARNESS_DIR/$patch_stem-bravo.xmoos"
    fi
    if [ -f "$HARNESS_DIR/$patch_stem-relay.xmoos" ]; then
        RELAY_PATCH="$HARNESS_DIR/$patch_stem-relay.xmoos"
    fi

    return 0
}

apply_case_patches() {
    clear_xfiles

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$SHORE_STEM" "$SHORE_PATCH" --targ="$SHORE_XFILE"
    fi
    if [ "$ALPHA_PATCH" != "" ]; then
        nspatch --stem="$ALPHA_STEM" "$ALPHA_PATCH" --targ="$ALPHA_XFILE"
    fi
    if [ "$BRAVO_PATCH" != "" ]; then
        nspatch --stem="$BRAVO_STEM" "$BRAVO_PATCH" --targ="$BRAVO_XFILE"
    fi
    if [ "$RELAY_PATCH" != "" ]; then
        nspatch --stem="$RELAY_STEM" "$RELAY_PATCH" --targ="$RELAY_XFILE"
    fi
}

case_args() {
    local case_idx="$1"
    local case_base=$((PORT_BASE + case_idx*PORT_STRIDE))
    local shore_mport=$((case_base + 0))
    local alpha_mport=$((case_base + 1))
    local bravo_mport=$((case_base + 2))
    local relay_mport=$((case_base + 3))
    local shore_pshare=$((case_base + 10))
    local alpha_pshare=$((case_base + 11))
    local bravo_pshare=$((case_base + 12))
    local relay_pshare=$((case_base + 13))

    echo "--shore_mport=$shore_mport --alpha_mport=$alpha_mport --bravo_mport=$bravo_mport --relay_mport=$relay_mport --shore_pshare=$shore_pshare --alpha_pshare=$alpha_pshare --bravo_pshare=$bravo_pshare --relay_pshare=$relay_pshare"
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

run_case() {
    local case_name="$1"
    local case_idx="${RUN_CASE_IDX:-0}"
    RUN_CASE_IDX=$((case_idx + 1))
    local xargs
    local line
    local actual
    local status
    local launch_rc

    get_case_config "$case_name" || return 1

    cd "$MISSION_DIR"
    ./clean.sh >/dev/null 2>&1
    stop_mission_apps "$MISSION_DIR"
    apply_case_patches || return 1
    : > results.txt

    xargs="--max_time=$MAX_TIME --mmod=$case_name $TIME_WARP"
    if [ "$PORT_BASE_SET" = "yes" ]; then
        xargs="$xargs $(case_args "$case_idx")"
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

    line=$(wait_for_result_line results.txt 160)
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="success"
    if [ "$launch_rc" != 0 ] || [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
        ALL_OK="no"
    fi

    emit_case_row "$case_name" "$status" "$EXPECTED" "$actual" "$line" >> "$RESULTS_FILE"
    cd "$HARNESS_DIR"
}

prepare_case_dir() {
    local case_dir="$1"
    mkdir -p "$case_dir"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    (
        cd "$case_dir"
        ./clean.sh >/dev/null 2>&1 || true
    )

    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$case_dir/meta_shoreside.moos" "$SHORE_PATCH" --targ="$case_dir/meta_shoreside.moosx"
    fi
    if [ "$ALPHA_PATCH" != "" ]; then
        nspatch --stem="$case_dir/meta_alpha.moos" "$ALPHA_PATCH" --targ="$case_dir/meta_alpha.moosx"
    fi
    if [ "$BRAVO_PATCH" != "" ]; then
        nspatch --stem="$case_dir/meta_bravo.moos" "$BRAVO_PATCH" --targ="$case_dir/meta_bravo.moosx"
    fi
    if [ "$RELAY_PATCH" != "" ]; then
        nspatch --stem="$case_dir/meta_relay.moos" "$RELAY_PATCH" --targ="$case_dir/meta_relay.moosx"
    fi
}

run_case_isolated() {
    local case_idx="$1"
    local case_name="$2"
    local case_tag
    local case_dir
    local case_row_file
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

    (
        cd "$case_dir"
        : > results.txt
        xargs="--max_time=$MAX_TIME --mmod=$case_name $(case_args "$case_idx") $TIME_WARP"
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

    line=$(wait_for_result_line "$case_dir/results.txt" 160)
    actual=`echo "$line" | sed -n 's/.*grade=\([^ ]*\).*/\1/p'`
    if [ "$actual" = "" ]; then
        actual="missing"
    fi

    status="success"
    if [ "$launch_rc" != 0 ] || [ "$actual" != "$EXPECTED" ]; then
        status="mismatch"
    fi

    emit_case_row "$case_name" "$status" "$EXPECTED" "$actual" "$line" > "$case_row_file"

    if [ "$status" = "success" ]; then
        return 0
    fi
    return 1
}

if [ ! -d "$MISSION_DIR" ]; then
    echo "$ME: Mission dir not found: [$MISSION_DIR]"
    exit 1
fi

trap cleanup EXIT

ALL_CASES=(
    pshare_topology_fanin_pass
    pshare_topology_competing_update_pass
    pshare_topology_multi_input_port_pass
    pshare_topology_input_route_list_pass
    pshare_topology_multicast_multi_listener_pass
    pshare_topology_dynamic_input_route_pass
    pshare_topology_dynamic_multicast_input_pass
    pshare_topology_custom_multicast_base_pass
    pshare_topology_unicast_relay_branch_pass
    pshare_topology_route_list_branch_pass
    pshare_topology_alias_cmd_route_pass
    pshare_topology_runtime_route_pass
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
    RUN_ROOT=$(mktemp -d "$HARNESS_DIR/.parallel_pshare_topology_XXXXXX")
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
