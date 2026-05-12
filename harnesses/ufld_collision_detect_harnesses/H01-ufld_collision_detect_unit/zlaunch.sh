#!/bin/bash
#------------------------------------------------------------
#   Script: zlaunch.sh
#  Harness: H01-ufld_collision_detect_unit
#   Author: Charles Benjamin
#   LastEd: May 2026
#------------------------------------------------------------

PORT_BASE=${PORT_BASE:-4800}
RUN_ROOT_PREFIX=ufld_collision_detect
HARNESS_KIND=collision_detect

ALL_CASES=(
    collision_event_pass
    near_miss_event_pass
    clear_encounter_report_pass
    closest_range_posts_pass
    reset_closest_range_ever_pass
    pulse_render_false_pass
    ignore_group_blocks_pass
    reject_group_blocks_pass
    condition_blocks_pass
    condition_allows_pass
    collision_flag_macro_pass
    near_miss_flag_macro_pass
    encounter_flag_density_pass
    outside_encounter_range_blocks_pass
    closest_posts_disabled_absent_pass
    clear_encounter_default_suppressed_pass
    collision_flag_numeric_cpa_pass
    encounter_rings_true_posts_pass
    encounter_rings_false_absent_pass
    range_normalization_params_pass
)

source "$(cd "$(dirname "$0")/../.." && pwd)/ufield_app_common/ufield_app_common.sh" "$@"
