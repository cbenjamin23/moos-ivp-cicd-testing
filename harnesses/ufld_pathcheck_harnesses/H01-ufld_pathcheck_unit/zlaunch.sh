#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-ufld_pathcheck_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

PORT_BASE=${PORT_BASE:-4300}
RUN_ROOT_PREFIX=ufld_pathcheck
HARNESS_KIND=pathcheck

ALL_CASES=(
    odometry_baseline_pass
    two_reports_speed_only_pass
    trip_reset_pass
    node_report_local_pass
    mixed_report_sources_pass
    multi_node_independent_pass
    variable_speed_average_pass
    history_length_three_average_pass
    invalid_history_length_default_pass
    reverse_leg_accumulates_pass
    history_length_two_speed_pass
    invalid_report_rejected_pass
    stationary_zero_distance_pass
    diagonal_chain_distance_pass
    trip_reset_unknown_ignored_pass
    trip_reset_twice_pass
    bad_then_good_recovery_pass
)

source "$(cd "$(dirname "$0")/../.." && pwd)/ufield_app_common/ufield_app_common.sh" "$@"
