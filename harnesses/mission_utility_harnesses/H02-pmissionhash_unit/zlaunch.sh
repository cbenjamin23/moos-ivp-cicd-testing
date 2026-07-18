#!/usr/bin/env bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H02-pmissionhash_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

PORT_BASE=${PORT_BASE:-9000}

ALL_CASES=(
    hash_custom_vars_pass
    hash_short_off_pass
    hash_long_off_pass
    hash_reset_changes_pass
)

source "$(cd "$(dirname "$0")/.." && pwd)/mission_utility_common.sh" "$@"
