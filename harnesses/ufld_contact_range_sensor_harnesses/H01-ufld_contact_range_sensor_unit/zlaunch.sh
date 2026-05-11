#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-ufld_contact_range_sensor_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

PORT_BASE=${PORT_BASE:-4500}
RUN_ROOT_PREFIX=ufld_contact_range_sensor
HARNESS_KIND=contact_range_sensor

ALL_CASES=(
    baseline_short_report_pass
    report_vars_short_no_long_pass
    report_vars_long_pass
    report_vars_both_pass
    ground_truth_uniform_zero_pass
    ground_truth_gaussian_unsupported_no_gt_pass
    ground_truth_long_only_pass
    ground_truth_no_algorithm_absent_pass
    allow_echo_type_block_pass
    allow_echo_type_accept_pass
    allow_echo_type_multi_accept_ship_pass
    sensor_arc_forward_accept_pass
    sensor_arc_wrap_accept_pass
    sensor_arc_multi_segment_aft_accept_pass
    sensor_arc_aft_block_pass
    sensor_arc_full_accept_pass
    ping_wait_blocks_second_pass
    named_ping_wait_blocks_second_pass
    ping_wait_unlimited_allows_second_pass
    named_ping_wait_unlimited_allows_second_pass
    ping_wait_nolimit_alias_allows_second_pass
    display_pulses_false_pass
    unknown_request_no_report_pass
    missing_name_request_no_report_pass
    ground_truth_false_no_gt_pass
    crs_node_report_local_pass
    unlimited_far_range_pass
    push_distance_alias_far_pass
    named_pull_unlimited_far_pass
    pull_distance_alias_far_pass
    default_short_blocks_far_pass
    short_total_range_far_blocks_pass
    named_push_short_blocks_far_pass
    reciprocal_request_report_pass
)

source "$(cd "$(dirname "$0")/../.." && pwd)/ufield_app_common/ufield_app_common.sh" "$@"
