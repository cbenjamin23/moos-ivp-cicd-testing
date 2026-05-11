#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-pmissioneval_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

PORT_BASE=${PORT_BASE:-7100}
RUN_ROOT_PREFIX=pmissioneval

ALL_CASES=(
    eval_baseline_pass
    eval_false_condition_fail
    eval_builtin_finish_pass
    eval_numeric_multi_pass
    eval_sequence_two_stage_pass
    eval_sequence_first_stage_fail
    eval_sequence_second_stage_fail
    eval_numeric_literal_flag_pass
    eval_lead_only_pass
    eval_numeric_partial_fail
    eval_fail_only_condition_fail
    eval_fail_condition_fail
    eval_flags_and_mail_pass
    eval_csp_report_pass
    eval_mhash_macros_pass
    eval_clock_macros_pass
    eval_no_hash_report_pass
)

source "$(cd "$(dirname "$0")/.." && pwd)/mission_utility_common.sh" "$@"
