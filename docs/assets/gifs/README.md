# GIF Asset Manifest

Place captured presentation clips in this directory using the filenames below.
Each harness page already references these paths.

Maintainer note: use the harness wrapper command for capture because it applies
case overlays before delegating to shared `xlaunch.sh`. Calling `xlaunch.sh`
directly is useful inside those wrappers, but it is not the case-selection
entry point for the harness pages.

## H01 Contact Manager Unit

Capture from: `harnesses/cmgr_harnesses/H01-cmgr_unit`
Typical capture launch: `./zlaunch.sh --case=detect_baseline_pass 10`

- `cmgr-unit-runtime-alert.gif` - Runtime Alert Path; representative case `runtime_alert_add_pass`
- `cmgr-unit-cpa-detection.gif` - CPA-only Detection; representative case `detect_cpa_only_pass`

## H02 Contact Manager Motion

Capture from: `harnesses/cmgr_harnesses/H02-cmgr_motion`
Typical capture launch: `./zlaunch.sh --case=baseline_crossing_pass --gui 10`

- `cmgr-motion-baseline-crossing.gif` - Baseline Crossing Pass; representative case `baseline_crossing_pass`
- `cmgr-motion-head-on-pass.gif` - Head-on Pass; representative case `head_on_pass`
- `cmgr-motion-two-contact-pmarineviewer.gif` - Two Contact Pass; representative case `two_contact_pass`

## H01 Obstacle Manager Unit

Capture from: `harnesses/obmgr_harnesses/H01-obmgr_unit`
Typical capture launch: `./zlaunch.sh --case=given_baseline_pass 10`

- `obmgr-unit-given-baseline.gif` - Given Obstacle Alert; representative case `given_baseline_pass`
- `obmgr-unit-point-cluster.gif` - Point Cluster Hull; representative case `points_cluster_dist_pass`

## H02 Obstacle Manager Motion

Capture from: `harnesses/obmgr_harnesses/H02-obmgr_motion`
Typical capture launch: `./zlaunch.sh --case=baseline_center_pass --gui 10`

- `obmgr-motion-baseline-center.gif` - Baseline Center Pass; representative case `baseline_center_pass`
- `obmgr-motion-offset-clear.gif` - Offset Clear Pass; representative case `offset_clear_pass`
- `obmgr-motion-two-sequential.gif` - Two Sequential Fail; representative case `two_sequential_fail`

## H01 COLREGS Classification

Capture from: `harnesses/colregs_harnesses/H01-colregs_classification`
Typical capture launch: `./zlaunch.sh --case=head_on_colregs_pass --gui 10`

- `colregs-classification-head-on.gif` - Head-on Classification; representative case `head_on_colregs_pass`
- `colregs-classification-crossing.gif` - Crossing Give-way; representative case `crossing_starboard_giveway_pass`

## H02 COLREGS Thresholds

Capture from: `harnesses/colregs_harnesses/H02-colregs_thresholds`
Typical capture launch: `./zlaunch.sh --group=headon --gui 10`

- `colregs-thresholds-headon-triplet.gif` - Below/Edge/Above Triplet; representative case `group=headon`
- `colregs-thresholds-mirror-boundary.gif` - Mirrored Boundary; representative case `group=standon`

## H03 COLREGS Execution

Capture from: `harnesses/colregs_harnesses/H03-colregs_execution`
Typical capture launch: `./zlaunch.sh --case=head_on_execution_pass --gui 10`

- `colregs-execution-head-on.gif` - Head-on Resolution; representative case `head_on_execution_pass`
- `colregs-execution-overtaken-midrange.gif` - Overtaken Midrange; representative case `overtaken_starboard_standon_midrange_execution_pass`

## H04 COLREGS Parameters

Capture from: `harnesses/colregs_harnesses/H04-colregs_parameters`
Typical capture launch: `./zlaunch.sh --group=min_util_cpa_dist --gui 10`

- `colregs-parameters-cpa-compare.gif` - Default vs High CPA; representative case `min_util_cpa_default/high`
- `colregs-parameters-headon-only.gif` - Head-on Only Toggle; representative case `headon_only_true/false`

## H01 Avoid-Collision Behavior

Capture from: `harnesses/collision_behavior_harnesses/H01-collision_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=head_on_resolve_pass --gui 10`

- `collision-behavior-head-on-resolve.gif` - Head-on Resolve; representative case `head_on_resolve_pass`
- `collision-behavior-no-alert-fail.gif` - No Alert Request Fail; representative case `no_alert_request_fail`

## H01 Avoid-Obstacle Behavior

Capture from: `harnesses/obstacle_behavior_harnesses/H01-obstacle_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=two_obstacles_clean_pass --gui 10`

- `obstacle-behavior-two-obstacles.gif` - Two Obstacles Clean; representative case `two_obstacles_clean_pass`
- `obstacle-behavior-avoid-disabled.gif` - Avoid Disabled Fail; representative case `avoid_disabled_fail`

## H01 OpRegion Safety

Capture from: `harnesses/opregion_harnesses/H01-opregion_safety`
Typical capture launch: `./zlaunch.sh --case=save_recover_pass --gui 10`

- `opregion-safety-save-recover.gif` - Save Recovery; representative case `save_recover_pass`
- `opregion-safety-dynamic-update.gif` - Dynamic Region Update; representative case `dynamic_region_update_pass`

## H01 Waypoint Behavior

Capture from: `harnesses/waypoint_behavior_harnesses/H01-waypoint_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=multi_point_sequence_pass --gui 10`

- `waypoint-behavior-multipoint.gif` - Multi-point Sequence; representative case `multi_point_sequence_pass`
- `waypoint-behavior-dynamic-update.gif` - Dynamic Points Update; representative case `dynamic_points_update_pass`

## H01 Loiter Behavior

Capture from: `harnesses/loiter_behavior_harnesses/H01-loiter_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=radial_clockwise_pass --gui 10`

- `loiter-behavior-radial.gif` - Radial Clockwise; representative case `radial_clockwise_pass`
- `loiter-behavior-center-activate.gif` - Center Activate; representative case `center_activate_pass`

## H01 StationKeep Behavior

Capture from: `harnesses/stationkeep_behavior_harnesses/H01-stationkeep_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=static_station_pass --gui 10`

- `stationkeep-behavior-static.gif` - Static Station; representative case `static_station_pass`
- `stationkeep-behavior-hibernation.gif` - Hibernation Settle; representative case `hibernation_settle_pass`

## H01 Trail Behavior

Capture from: `harnesses/trail_behavior_harnesses/H01-trail_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=static_trail_pass --gui 10`

- `trail-behavior-static.gif` - Static Trail; representative case `static_trail_pass`
- `trail-behavior-runtime-angle.gif` - Runtime Angle Update; representative case `runtime_angle_update_pass`

## H01 Convoy Behavior

Capture from: `harnesses/convoy_behavior_harnesses/H01-convoy_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=static_convoy_pass --gui 10`

- `convoy-behavior-static.gif` - Static Convoy; representative case `static_convoy_pass`
- `convoy-behavior-angled-entry.gif` - Angled Entry; representative case `angled_entry_pass`

## P01 Obstacle Gauntlet

Capture from: `harnesses/performance_harnesses/P01-obstacle_gauntlet`
Typical capture launch: `./zlaunch.sh --case=baseline_field_pass --gui 10`

- `performance-obstacle-gauntlet-baseline.gif` - Baseline Field; representative case `baseline_field_pass`
- `performance-obstacle-gauntlet-dense.gif` - Dense Field; representative case `dense_field_pass`
- `performance-obstacle-gauntlet-endurance.gif` - Endurance Random; representative case `endurance_random_pass`

## P02 COLREGS Joust

Capture from: `harnesses/performance_harnesses/P02-colregs_joust`
Typical capture launch: `./zlaunch.sh --case=dense_colregs_pass --gui 10`

- `performance-colregs-joust-baseline.gif` - Baseline Joust; representative case `baseline_colregs_pass`
- `performance-colregs-joust-dense.gif` - Dense Joust; representative case `dense_colregs_pass`

## P03 COLREGS Traffic Ring

Capture from: `harnesses/performance_harnesses/P03-colregs_traffic_ring`
Typical capture launch: `./zlaunch.sh --case=baseline_circle_pass --gui 10`

- `performance-traffic-ring-baseline.gif` - Baseline Traffic Ring; representative case `baseline_circle_pass`
- `performance-traffic-ring-mixed-speed.gif` - Mixed-Speed Traffic Ring; representative case `mixed_speed_circle_pass`
- `performance-traffic-ring-noncoop.gif` - Non-Cooperative Traffic Ring; representative case `noncoop_circle_pass`
