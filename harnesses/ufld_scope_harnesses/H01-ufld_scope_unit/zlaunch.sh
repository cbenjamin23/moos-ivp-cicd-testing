#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-ufld_scope_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

PORT_BASE=${PORT_BASE:-5000}
RUN_ROOT_PREFIX=ufld_scope
HARNESS_KIND=scope

ALL_CASES=(
    appcast_table_pass
    alias_headers_pass
    update_replaces_same_key_pass
    multi_vehicle_rows_pass
    invalid_scope_ignored_pass
    missing_field_blank_cell_pass
    missing_key_blank_row_pass
    later_missing_field_replaces_value_pass
)

source "$(cd "$(dirname "$0")/../.." && pwd)/ufield_app_common/ufield_app_common.sh" "$@"
