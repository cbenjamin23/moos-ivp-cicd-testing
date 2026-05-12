#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-ufld_collob_detect_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

PORT_BASE=${PORT_BASE:-4900}
RUN_ROOT_PREFIX=ufld_collob_detect
HARNESS_KIND=collob_detect

ALL_CASES=(
    collision_flag_pass
    near_miss_flag_pass
    encounter_only_flag_pass
    global_min_distance_pass
    invalid_obstacle_absent_pass
    obstacle_clear_blocks_later_pass
    upper_macro_flag_pass
    fresh_clear_keeps_obstacle_pass
    encounter_double_flag_pass
    near_miss_upper_macro_flag_pass
    collision_boundary_counts_near_pass
    near_boundary_counts_encounter_pass
    named_clear_only_removes_target_pass
    range_normalization_collision_pass
)

source "$(cd "$(dirname "$0")/../.." && pwd)/ufield_app_common/ufield_app_common.sh" "$@"
