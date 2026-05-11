#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-ufld_obstacle_sim_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------
ME=`basename "$0"`
HARNESS_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(git -C "$HARNESS_DIR" rev-parse --show-toplevel)"
MISSION_DIR="$REPO_DIR/missions/ufld_obstacle_sim_missions/ufld_obstacle_sim_unit"
RESULTS_FILE="$HARNESS_DIR/results.txt"
TIME_WARP=10
MAX_TIME=30
JOBS=1
PORT_BASE=7600
PORT_STRIDE=30
CASE=""
JUST_MAKE="no"
KEEP_WORKDIRS="no"
NOGUI="--nogui"
RUN_ROOT=""
ALL_OK="yes"

source "$REPO_DIR/scripts/harness_teardown.sh"

cleanup() {
    if [ "$RUN_ROOT" != "" ]; then
        harness_teardown_stop_root "$RUN_ROOT" >/dev/null 2>&1 || true
    fi
    if [ "$RUN_ROOT" != "" ] && [ "$KEEP_WORKDIRS" != "yes" ]; then
        rm -rf "$RUN_ROOT"
    fi
}
trap cleanup EXIT

usage() {
    echo "$ME [OPTIONS] [time_warp]"
    echo "  --case=<name>      Run one named case"
    echo "  --jobs=N           Run up to N cases per wave"
    echo "  --port_base=N      Base port; default 7600"
    echo "  --max_time=N       Max time passed to xlaunch"
    echo "  --just_make, -j    Generate target files only"
    echo "  --keep_workdirs    Keep isolated temp mission copies"
    echo "  --gui              Launch pMarineViewer"
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
    elif [ "$ARGI" = "--gui" ]; then
        NOGUI=""
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

ALL_CASES=(
fixed_field_publish_pass
multi_field_labels_pass
duration_fixed_pass
duration_min_only_pass
duration_min_max_swap_pass
duration_disabled_zero_pass
pmv_connect_repeated_repost_pass
pmv_connect_absent_no_publish_pass
vehicle_connect_no_refresh_pass
post_visuals_false_pass
draw_region_false_pass
post_points_inside_pass
post_points_multi_field_pass
post_points_multi_vehicle_pass
invalid_node_report_no_refresh_pass
invalid_node_report_no_points_pass
node_report_mixed_case_vehicle_pass
node_report_color_point_pass
sensor_range_outside_pass
sensor_range_edge_pass
sensor_range_zero_suppresses_pass
point_size_bigger_pass
point_size_numeric_pass
point_size_smaller_pass
point_size_sequence_pass
point_size_invalid_ignored_pass
point_size_floor_ignored_pass
point_size_uppercase_pass
point_size_fraction_pass
point_size_zero_ignored_pass
rate_points_three_pass
rate_points_zero_pass
post_points_no_visuals_pass
node_report_refresh_pass
refresh_interval_repost_pass
refresh_interval_post_points_no_given_pass
reset_request_reposts_pass
reset_request_inside_blocked_pass
reset_reuse_ids_false_pass
reset_post_points_no_given_pass
reset_post_visuals_false_pass
reset_range_boundary_blocked_pass
reset_interval_exit_pass
reset_interval_wait_blocked_pass
sensor_range_broad_far_pass
obstacle_visual_style_pass
region_visual_style_pass
poly_visual_zero_sizes_pass
missing_obstacle_file_absent_pass
)

case_list() {
    printf "%s\n" "${ALL_CASES[@]}"
}

get_case_config() {
    CASE_NAME="$1"
    EXPECTED="pass"
    SHORE_PATCH=""
    EXTRA_CHECK="$CASE_NAME"

    case " ${ALL_CASES[*]} " in
        *" $CASE_NAME "*) ;;
        *)
            echo "$ME: Unknown case: $CASE_NAME"
            return 1
            ;;
    esac

    if [ "$CASE_NAME" != "fixed_field_publish_pass" ]; then
        local patch_name="${CASE_NAME//_/-}-shoreside.xmoos"
        SHORE_PATCH="$HARNESS_DIR/$patch_name"
    fi

    if [ "$SHORE_PATCH" != "" ] && [ ! -f "$SHORE_PATCH" ]; then
        echo "$ME: Missing case patch: $SHORE_PATCH"
        return 1
    fi
}

wait_for_result_line() {
    local file="$1"
    local tries=120
    local line
    local i=0
    while [ "$i" -lt "$tries" ]; do
        line=`tail -n 1 "$file" 2>/dev/null`
        if echo "$line" | grep -q "grade="; then
            echo "$line"
            return 0
        fi
        sleep 0.5
        i=$((i + 1))
    done
    echo ""
    return 1
}

find_shore_alog() {
    find "$1" -name '*.alog' -print | sort | tail -n 1
}

alog_var_has_pattern() {
    local case_dir="$1"
    local var="$2"
    local pattern="$3"
    local alog
    alog=`find_shore_alog "$case_dir"`
    [ "$alog" != "" ] || return 1
    aloggrep "$alog" "$var" 2>/dev/null | rg "$pattern" >/dev/null
}

alog_var_absent() {
    local case_dir="$1"
    local var="$2"
    local alog
    alog=`find_shore_alog "$case_dir"`
    [ "$alog" != "" ] || return 1
    ! aloggrep "$alog" "$var" 2>/dev/null | awk -v v="$var" '$2 == v {found=1} END {exit found ? 0 : 1}'
}

alog_var_pattern_absent() {
    local case_dir="$1"
    local var="$2"
    local pattern="$3"
    local alog
    alog=`find_shore_alog "$case_dir"`
    [ "$alog" != "" ] || return 1
    ! aloggrep "$alog" "$var" 2>/dev/null | rg "$pattern" >/dev/null
}

alog_var_count() {
    local case_dir="$1"
    local var="$2"
    local alog
    alog=`find_shore_alog "$case_dir"`
    [ "$alog" != "" ] || return 1
    aloggrep "$alog" "$var" 2>/dev/null | awk -v v="$var" '$2 == v {c++} END {print c + 0}'
}

extra_check_ok() {
    local check="$1"
    local case_dir="$2"
    local line="$3"

    case "$check" in
        fixed_field_publish_pass)
            echo "$line" | rg -q 'known=pts=\{10,-10:14,-10:14,-6:10,-6\},label=ufos_a' && \
            echo "$line" | rg -q 'given=pts=\{10,-10:14,-10:14,-6:10,-6\},label=ufos_a' && \
            alog_var_has_pattern "$case_dir" "VIEW_POLYGON" "label=ufos_a" && \
            alog_var_has_pattern "$case_dir" "VIEW_POLYGON" "label=obs_region" && \
            alog_var_has_pattern "$case_dir" "UFOS_MIN_RNG" "8"
            ;;
        multi_field_labels_pass)
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_a" && \
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_b" && \
            alog_var_has_pattern "$case_dir" "GIVEN_OBSTACLE" "label=ufos_a" && \
            alog_var_has_pattern "$case_dir" "GIVEN_OBSTACLE" "label=ufos_b"
            ;;
        duration_fixed_pass)
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "duration=12" && \
            alog_var_has_pattern "$case_dir" "GIVEN_OBSTACLE" "duration=12"
            ;;
        duration_min_only_pass)
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "duration=7" && \
            alog_var_has_pattern "$case_dir" "GIVEN_OBSTACLE" "duration=7"
            ;;
        duration_min_max_swap_pass)
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "duration=10" && \
            alog_var_has_pattern "$case_dir" "GIVEN_OBSTACLE" "duration=10"
            ;;
        duration_disabled_zero_pass)
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_a" && \
            alog_var_has_pattern "$case_dir" "GIVEN_OBSTACLE" "label=ufos_a" && \
            alog_var_pattern_absent "$case_dir" "KNOWN_OBSTACLE" "duration=" && \
            alog_var_pattern_absent "$case_dir" "GIVEN_OBSTACLE" "duration="
            ;;
        pmv_connect_repeated_repost_pass)
            [ "`alog_var_count "$case_dir" "PMV_CONNECT"`" -ge 2 ] && \
            [ "`alog_var_count "$case_dir" "KNOWN_OBSTACLE"`" -ge 3 ] && \
            [ "`alog_var_count "$case_dir" "GIVEN_OBSTACLE"`" -ge 3 ]
            ;;
        pmv_connect_absent_no_publish_pass)
            alog_var_absent "$case_dir" "PMV_CONNECT" && \
            alog_var_absent "$case_dir" "KNOWN_OBSTACLE" && \
            alog_var_absent "$case_dir" "GIVEN_OBSTACLE" && \
            alog_var_absent "$case_dir" "VIEW_POLYGON"
            ;;
        vehicle_connect_no_refresh_pass)
            alog_var_has_pattern "$case_dir" "VEHICLE_CONNECT" "true" && \
            alog_var_absent "$case_dir" "KNOWN_OBSTACLE" && \
            alog_var_absent "$case_dir" "GIVEN_OBSTACLE"
            ;;
        post_visuals_false_pass)
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_a" && \
            alog_var_has_pattern "$case_dir" "GIVEN_OBSTACLE" "label=ufos_a" && \
            alog_var_absent "$case_dir" "VIEW_POLYGON"
            ;;
        draw_region_false_pass)
            alog_var_has_pattern "$case_dir" "VIEW_POLYGON" "label=ufos_a" && \
            ! alog_var_has_pattern "$case_dir" "VIEW_POLYGON" "label=obs_region"
            ;;
        post_points_inside_pass)
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_a" && \
            alog_var_has_pattern "$case_dir" "TRACKED_FEATURE_ALPHA" "key=ufos_a" && \
            alog_var_has_pattern "$case_dir" "VIEW_POINT" "label=alpha:ufos_a" && \
            alog_var_absent "$case_dir" "GIVEN_OBSTACLE"
            ;;
        post_points_multi_field_pass)
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_a" && \
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_b" && \
            alog_var_has_pattern "$case_dir" "TRACKED_FEATURE_ALPHA" "key=ufos_a" && \
            alog_var_has_pattern "$case_dir" "TRACKED_FEATURE_ALPHA" "key=ufos_b" && \
            alog_var_absent "$case_dir" "GIVEN_OBSTACLE"
            ;;
        post_points_multi_vehicle_pass)
            alog_var_has_pattern "$case_dir" "TRACKED_FEATURE_ALPHA" "key=ufos_a" && \
            alog_var_has_pattern "$case_dir" "TRACKED_FEATURE_BETA" "key=ufos_a" && \
            alog_var_has_pattern "$case_dir" "VIEW_POINT" "label=alpha:ufos_a" && \
            alog_var_has_pattern "$case_dir" "VIEW_POINT" "label=beta:ufos_a"
            ;;
        invalid_node_report_no_refresh_pass)
            alog_var_has_pattern "$case_dir" "NODE_REPORT" "NAME=alpha" && \
            alog_var_absent "$case_dir" "KNOWN_OBSTACLE" && \
            alog_var_absent "$case_dir" "GIVEN_OBSTACLE" && \
            alog_var_absent "$case_dir" "TRACKED_FEATURE_ALPHA"
            ;;
        invalid_node_report_no_points_pass)
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_a" && \
            alog_var_has_pattern "$case_dir" "NODE_REPORT" "NAME=alpha" && \
            alog_var_absent "$case_dir" "TRACKED_FEATURE_ALPHA" && \
            alog_var_absent "$case_dir" "VIEW_POINT" && \
            alog_var_absent "$case_dir" "GIVEN_OBSTACLE"
            ;;
        node_report_mixed_case_vehicle_pass)
            alog_var_has_pattern "$case_dir" "NODE_REPORT" "NAME=Alpha" && \
            alog_var_has_pattern "$case_dir" "TRACKED_FEATURE_ALPHA" "key=ufos_a" && \
            alog_var_has_pattern "$case_dir" "VIEW_POINT" "label=Alpha:ufos_a"
            ;;
        node_report_color_point_pass)
            alog_var_has_pattern "$case_dir" "NODE_REPORT" "COLOR=red" && \
            alog_var_has_pattern "$case_dir" "VIEW_POINT" "label=alpha:ufos_a.*vertex_color=red"
            ;;
        sensor_range_outside_pass)
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_a" && \
            alog_var_absent "$case_dir" "TRACKED_FEATURE_ALPHA" && \
            alog_var_absent "$case_dir" "VIEW_POINT" && \
            alog_var_absent "$case_dir" "GIVEN_OBSTACLE"
            ;;
        sensor_range_edge_pass)
            alog_var_has_pattern "$case_dir" "TRACKED_FEATURE_ALPHA" "key=ufos_a"
            ;;
        sensor_range_zero_suppresses_pass)
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_a" && \
            alog_var_absent "$case_dir" "TRACKED_FEATURE_ALPHA" && \
            alog_var_absent "$case_dir" "VIEW_POINT" && \
            alog_var_absent "$case_dir" "GIVEN_OBSTACLE"
            ;;
        point_size_bigger_pass)
            alog_var_has_pattern "$case_dir" "UFOS_POINT_SIZE" "bigger" && \
            alog_var_has_pattern "$case_dir" "VIEW_POINT" "vertex_size=3"
            ;;
        point_size_numeric_pass)
            alog_var_has_pattern "$case_dir" "UFOS_POINT_SIZE" "5pts" && \
            alog_var_has_pattern "$case_dir" "VIEW_POINT" "vertex_size=5"
            ;;
        point_size_smaller_pass)
            alog_var_has_pattern "$case_dir" "UFOS_POINT_SIZE" "smaller" && \
            alog_var_has_pattern "$case_dir" "VIEW_POINT" "vertex_size=1"
            ;;
        point_size_sequence_pass)
            alog_var_has_pattern "$case_dir" "UFOS_POINT_SIZE" "bigger" && \
            alog_var_has_pattern "$case_dir" "UFOS_POINT_SIZE" "smaller" && \
            alog_var_has_pattern "$case_dir" "VIEW_POINT" "vertex_size=2"
            ;;
        point_size_invalid_ignored_pass)
            alog_var_has_pattern "$case_dir" "UFOS_POINT_SIZE" "invalid" && \
            alog_var_has_pattern "$case_dir" "VIEW_POINT" "vertex_size=2"
            ;;
        point_size_floor_ignored_pass)
            alog_var_has_pattern "$case_dir" "UFOS_POINT_SIZE" "smaller" && \
            alog_var_has_pattern "$case_dir" "VIEW_POINT" "vertex_size=1"
            ;;
        point_size_uppercase_pass)
            alog_var_has_pattern "$case_dir" "UFOS_POINT_SIZE" "BIGGER" && \
            alog_var_has_pattern "$case_dir" "VIEW_POINT" "vertex_size=3"
            ;;
        point_size_fraction_pass)
            alog_var_has_pattern "$case_dir" "UFOS_POINT_SIZE" "2.5pts" && \
            alog_var_has_pattern "$case_dir" "VIEW_POINT" "vertex_size=2.5"
            ;;
        point_size_zero_ignored_pass)
            alog_var_has_pattern "$case_dir" "UFOS_POINT_SIZE" "0" && \
            alog_var_has_pattern "$case_dir" "VIEW_POINT" "vertex_size=2"
            ;;
        rate_points_three_pass)
            [ "`alog_var_count "$case_dir" "TRACKED_FEATURE_ALPHA"`" -ge 3 ] && \
            [ "`alog_var_count "$case_dir" "VIEW_POINT"`" -ge 3 ]
            ;;
        rate_points_zero_pass)
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_a" && \
            alog_var_absent "$case_dir" "TRACKED_FEATURE_ALPHA" && \
            alog_var_absent "$case_dir" "VIEW_POINT" && \
            alog_var_absent "$case_dir" "GIVEN_OBSTACLE"
            ;;
        post_points_no_visuals_pass)
            alog_var_has_pattern "$case_dir" "TRACKED_FEATURE_ALPHA" "key=ufos_a" && \
            alog_var_absent "$case_dir" "VIEW_POINT" && \
            alog_var_absent "$case_dir" "GIVEN_OBSTACLE"
            ;;
        node_report_refresh_pass)
            alog_var_absent "$case_dir" "PMV_CONNECT" && \
            alog_var_has_pattern "$case_dir" "NODE_REPORT" "NAME=alpha" && \
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_a" && \
            alog_var_has_pattern "$case_dir" "GIVEN_OBSTACLE" "label=ufos_a"
            ;;
        refresh_interval_repost_pass)
            [ "`alog_var_count "$case_dir" "KNOWN_OBSTACLE"`" -ge 3 ] && \
            [ "`alog_var_count "$case_dir" "GIVEN_OBSTACLE"`" -ge 3 ]
            ;;
        refresh_interval_post_points_no_given_pass)
            [ "`alog_var_count "$case_dir" "KNOWN_OBSTACLE"`" -ge 3 ] && \
            alog_var_absent "$case_dir" "GIVEN_OBSTACLE"
            ;;
        reset_request_reposts_pass)
            alog_var_has_pattern "$case_dir" "UFOS_RESET" "true" && \
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE_CLEAR" "all" && \
            [ "`alog_var_count "$case_dir" "KNOWN_OBSTACLE"`" -ge 2 ]
            ;;
        reset_request_inside_blocked_pass)
            alog_var_has_pattern "$case_dir" "UFOS_RESET" "true" && \
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_a" && \
            alog_var_absent "$case_dir" "KNOWN_OBSTACLE_CLEAR"
            ;;
        reset_reuse_ids_false_pass)
            alog_var_has_pattern "$case_dir" "UFOS_RESET" "true" && \
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE_CLEAR" "all" && \
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "active=false.*label=ufos_a" && \
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ob_1"
            ;;
        reset_post_points_no_given_pass)
            alog_var_has_pattern "$case_dir" "UFOS_RESET" "true" && \
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE_CLEAR" "all" && \
            [ "`alog_var_count "$case_dir" "KNOWN_OBSTACLE"`" -ge 2 ] && \
            alog_var_absent "$case_dir" "GIVEN_OBSTACLE"
            ;;
        reset_post_visuals_false_pass)
            alog_var_has_pattern "$case_dir" "UFOS_RESET" "true" && \
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE_CLEAR" "all" && \
            [ "`alog_var_count "$case_dir" "KNOWN_OBSTACLE"`" -ge 2 ] && \
            alog_var_absent "$case_dir" "VIEW_POLYGON"
            ;;
        reset_range_boundary_blocked_pass)
            alog_var_has_pattern "$case_dir" "UFOS_RESET" "true" && \
            alog_var_has_pattern "$case_dir" "NODE_REPORT" "X=70,Y=0" && \
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_a" && \
            alog_var_absent "$case_dir" "KNOWN_OBSTACLE_CLEAR"
            ;;
        reset_interval_exit_pass)
            alog_var_absent "$case_dir" "UFOS_RESET" && \
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE_CLEAR" "all" && \
            [ "`alog_var_count "$case_dir" "KNOWN_OBSTACLE"`" -ge 2 ]
            ;;
        reset_interval_wait_blocked_pass)
            alog_var_absent "$case_dir" "UFOS_RESET" && \
            alog_var_has_pattern "$case_dir" "KNOWN_OBSTACLE" "label=ufos_a" && \
            alog_var_absent "$case_dir" "KNOWN_OBSTACLE_CLEAR"
            ;;
        sensor_range_broad_far_pass)
            alog_var_has_pattern "$case_dir" "TRACKED_FEATURE_ALPHA" "key=ufos_a"
            ;;
        obstacle_visual_style_pass)
            alog_var_has_pattern "$case_dir" "VIEW_POLYGON" "label=ufos_a.*edge_color=red.*vertex_color=blue.*fill_color=green.*vertex_size=3.*edge_size=2.*fill_transparency=0.4"
            ;;
        region_visual_style_pass)
            alog_var_has_pattern "$case_dir" "VIEW_POLYGON" "label=obs_region.*edge_color=orange.*vertex_color=yellow"
            ;;
        poly_visual_zero_sizes_pass)
            alog_var_has_pattern "$case_dir" "VIEW_POLYGON" "label=ufos_a.*vertex_size=0.*edge_size=0.*fill_transparency=0"
            ;;
        missing_obstacle_file_absent_pass)
            alog_var_absent "$case_dir" "UFOS_MIN_RNG" && \
            alog_var_absent "$case_dir" "KNOWN_OBSTACLE" && \
            alog_var_absent "$case_dir" "GIVEN_OBSTACLE" && \
            alog_var_absent "$case_dir" "VIEW_POLYGON"
            ;;
        *)
            return 0
            ;;
    esac
}

prepare_case_dir() {
    local case_dir="$1"
    cp -R "$MISSION_DIR"/. "$case_dir"/
    (cd "$case_dir" && ./clean.sh >/dev/null 2>&1 || true)
    if [ "$SHORE_PATCH" != "" ]; then
        nspatch --stem="$case_dir/meta_shoreside.moos" "$SHORE_PATCH" --targ="$case_dir/meta_shoreside.moosx"
    fi
}

run_case() {
    local case_name="$1"
    local case_index="$2"
    get_case_config "$case_name" || return 1

    local case_dir="$RUN_ROOT/$case_name"
    local case_results="$case_dir/results.txt"
    local pbase=$((PORT_BASE + case_index * PORT_STRIDE))
    local just_make_arg=""
    mkdir -p "$case_dir"
    prepare_case_dir "$case_dir"
    : > "$case_results"

    if [ "$JUST_MAKE" = "yes" ]; then
        just_make_arg="--just_make"
    fi

    (
        cd "$case_dir"
        xlaunch.sh --max_time="$MAX_TIME" $NOGUI $just_make_arg \
            --mmod="$case_name" \
            --shore_mport="$pbase" \
            --shore_pshare="$((pbase+10))" \
            "$TIME_WARP" >"$case_dir/xlaunch.out" 2>&1
    )
    harness_teardown_stop_root "$case_dir" >/dev/null 2>&1 || true

    if [ "$JUST_MAKE" = "yes" ]; then
        echo "$case_name: targ files made"
        return 0
    fi

    local line
    line=`wait_for_result_line "$case_results"`
    if [ "$line" = "" ]; then
        echo "$case_name: no-result"
        return 1
    fi

    echo "$case_name $line" >> "$RESULTS_FILE"
    if ! echo "$line" | rg -q "grade=$EXPECTED"; then
        echo "$case_name: expected $EXPECTED but got: $line"
        return 1
    fi
    if ! extra_check_ok "$EXTRA_CHECK" "$case_dir" "$line"; then
        echo "$case_name: extra check failed: $line"
        return 1
    fi
    echo "$case_name: ok"
}

if [ "$CASE" != "" ]; then
    CASES="$CASE"
else
    CASES=`case_list`
fi

RUN_ROOT=`mktemp -d "${TMPDIR:-/tmp}/ufld_obstacle_sim_harness.XXXXXX"`
: > "$RESULTS_FILE"

INDEX=0
PIDS=""
for C in $CASES; do
    run_case "$C" "$INDEX" &
    PIDS="$PIDS $!"
    INDEX=$((INDEX + 1))
    if [ "$JUST_MAKE" = "yes" ]; then
        wait || ALL_OK="no"
        PIDS=""
    elif [ $((INDEX % JOBS)) -eq 0 ]; then
        for P in $PIDS; do
            wait "$P" || ALL_OK="no"
        done
        PIDS=""
    fi
done

for P in $PIDS; do
    wait "$P" || ALL_OK="no"
done

[ "$ALL_OK" = "yes" ]
