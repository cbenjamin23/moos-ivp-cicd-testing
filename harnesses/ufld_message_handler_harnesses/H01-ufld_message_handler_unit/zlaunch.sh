#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-ufld_message_handler_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

PORT_BASE=${PORT_BASE:-4400}
RUN_ROOT_PREFIX=ufld_message_handler
HARNESS_KIND=message_handler

ALL_CASES=(
    dest_all_string_pass
    dest_all_uppercase_pass
    dest_specific_self_pass
    dest_mismatch_reject_pass
    strict_all_reject_pass
    strict_group_reject_pass
    strict_specific_accept_pass
    numeric_payload_pass
    double_zero_payload_pass
    mixed_payload_invalid_pass
    quoted_string_unquoted_pass
    src_app_forward_pass
    invalid_payload_reject_pass
    missing_destination_invalid_pass
    invalid_no_bad_flag_pass
    msg_flag_macro_pass
    msg_flag_numeric_pass
    bad_msg_flag_macro_pass
    bad_msg_flag_numeric_pass
    multiple_bad_summary_pass
    aux_info_node_app_forward_pass
    dest_group_accept_pass
    dest_node_overrides_group_reject_pass
    dest_all_mixedcase_reject_pass
    multiple_good_summary_pass
    mixed_valid_rejected_summary_pass
)

source "$(cd "$(dirname "$0")/../.." && pwd)/ufield_app_common/ufield_app_common.sh" "$@"
