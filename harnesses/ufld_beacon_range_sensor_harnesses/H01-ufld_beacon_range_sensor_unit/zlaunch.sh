#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-ufld_beacon_range_sensor_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

PORT_BASE=${PORT_BASE:-4700}
RUN_ROOT_PREFIX=ufld_beacon_range_sensor
HARNESS_KIND=beacon_range_sensor

ALL_CASES=(
    request_short_report_pass
    request_long_report_pass
    request_both_report_pass
    brs_ground_truth_uniform_zero_pass
    brs_ground_truth_false_no_gt_pass
    far_request_blocked_pass
    node_push_unlimited_far_pass
    brs_ping_wait_blocks_second_pass
    brs_named_ping_wait_blocks_only_alpha_pass
    ping_payment_request_blocks_retry_pass
    ping_payment_response_allows_retry_pass
    ping_payment_accept_blocks_retry_pass
    brs_node_report_local_pass
    beacon_style_defaults_pass
    beacon_reply_push_blocks_pass
    request_path_pull_blocks_pass
    unknown_request_no_report_pass
    missing_name_request_no_report_pass
    beacon_visual_marker_pass
    unsolicited_frequency_pass
)

source "$(cd "$(dirname "$0")/../.." && pwd)/ufield_app_common/ufield_app_common.sh" "$@"
