#!/usr/bin/env bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H03-umayfinish_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

PORT_BASE=${PORT_BASE:-9000}

ALL_CASES=(
    mayfinish_default_exit_pass
    mayfinish_custom_finish_pass
    mayfinish_custom_value_timeout_fail
    mayfinish_timeout_fail
)

source "$(cd "$(dirname "$0")/.." && pwd)/mission_utility_common.sh" "$@"
