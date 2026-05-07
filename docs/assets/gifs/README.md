# GIF Asset Manifest

Place captured presentation clips in this directory using the filenames below.
Each harness page already references these paths.

Maintainer note: use the harness wrapper command for capture because it applies
case overlays before delegating to shared `xlaunch.sh`. Calling `xlaunch.sh`
directly is useful inside those wrappers, but it is not the case-selection
entry point for the harness pages.

Generated visual standard: app-level unit, classification, and variable-heavy motion pages may use 16:9,
map-style explanatory GIFs when a raw `pMarineViewer` capture would not make
the pass condition legible. Keep the look close to the PMV captures: dark chart
background, simple ownship/contact/obstacle geometry, compact labels, and one
small pass-condition callout. The callout should name the MOOS publication being
graded, because that is the evidence the harness is actually testing.

Generated unit visuals live in `docs/tools/render_unit_harness_gifs.py`.
Generated COLREGS classification visuals live in
`docs/tools/render_colregs_classification_gifs.py`.
Generated COLREGS threshold overlay visuals live in
`docs/tools/render_colregs_threshold_gifs.py`.
Generated COLREGS parameter comparison visuals live in
`docs/tools/render_colregs_parameter_gifs.py`.
Generated PID motion visuals live in `docs/tools/render_pid_motion_gifs.py`.
Headless harness runs remain the source of truth for the case behavior; the
generated GIFs are documentation views of that same geometry and variable-level
evidence.

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

## H01 Marine PID Unit

Capture from: `harnesses/pid_harnesses/H01-pid_unit`
Typical capture launch: `./zlaunch.sh --case=rudder_starboard_pass 10`

- `pid-unit-heading-wrap.gif` - Heading Wrap; representative case `heading_wrap_pass`
- `pid-unit-manual-override.gif` - Manual Override; representative case `manual_override_zero_pass`

## H02 Marine PID Motion

Capture from: `harnesses/pid_harnesses/H02-pid_motion`
Typical capture launch: `./zlaunch.sh --case=baseline_transit_pass --gui 10`

- `pid-motion-hard-turn-recover.gif` - Hard Turn Recovery; representative case `hard_turn_recover_pass`
- `pid-motion-depth-step.gif` - Depth Step Response; representative case `depth_step_pass`

## H01 Constant Depth Motion

Capture from: `harnesses/depth_behavior_harnesses/H01-constant_depth_motion`
Typical capture launch: `./zlaunch.sh --case=constant_depth_hold_pass --gui 10`

- `constant-depth-hold.gif` - Held Depth; representative case `constant_depth_hold_pass`
- `constant-depth-update.gif` - Runtime Update; representative case `constant_depth_update_pass`


## H02 GoToDepth Motion

Capture from: `harnesses/depth_behavior_harnesses/H02-goto_depth_motion`
Typical capture launch: `./zlaunch.sh --case=goto_depth_sequence_pass --gui 10`

- `goto-depth-sequence.gif` - Depth Sequence; representative case `goto_depth_sequence_pass`
- `goto-depth-crossing.gif` - Crossing Arrivals; representative case `goto_depth_crossing_pass`


## H03 Periodic Surface Motion

Capture from: `harnesses/depth_behavior_harnesses/H03-periodic_surface_motion`
Typical capture launch: `./zlaunch.sh --case=periodic_surface_pass --gui 10`

- `periodic-surface-cycle.gif` - Surfacing Cycle; representative case `periodic_surface_pass`
- `periodic-surface-wait-window.gif` - Wait Window; representative case `periodic_surface_wait_window_pass`


## H04 Max Depth Motion

Capture from: `harnesses/depth_behavior_harnesses/H04-max_depth_motion`
Typical capture launch: `./zlaunch.sh --case=max_depth_guard_pass --gui 10`

- `max-depth-guard.gif` - Max Depth Guard; representative case `max_depth_guard_pass`
- `max-depth-unconstrained.gif` - Unconstrained Shallow; representative case `max_depth_unconstrained_shallow_pass`


## H05 Min Altitude Motion

Capture from: `harnesses/depth_behavior_harnesses/H05-min_altitude_motion`
Typical capture launch: `./zlaunch.sh --case=min_altitude_guard_pass --gui 10`

- `min-altitude-guard.gif` - Min Altitude Guard; representative case `min_altitude_guard_pass`
- `min-altitude-unconstrained.gif` - Deep Bottom; representative case `min_altitude_unconstrained_deep_bottom_pass`


## H01 COLREGS Classification

Capture from: `harnesses/colregs_harnesses/H01-colregs_classification`
Typical capture launch: `./zlaunch.sh --case=head_on_colregs_pass --gui 10`

- `colregs-classification-head-on.gif` - Head-on Classification; representative case `head_on_colregs_pass`
- `colregs-classification-crossing.gif` - Crossing Give-way; representative case `crossing_starboard_giveway_pass`

## H02 COLREGS Thresholds

Capture from: `harnesses/colregs_harnesses/H02-colregs_thresholds`
Typical capture launch: `./zlaunch.sh --group=headon --gui 10`

- `colregs-thresholds-giveway-bowdist.gif` - Give-way Bow Distance; representative case `giveway_bowdist_edge_pass`
- `colregs-thresholds-overtaking-triplet.gif` - Overtaking Threshold; representative case `overtaking_thresh_edge_pass`

## H03 COLREGS Execution

Capture from: `harnesses/colregs_harnesses/H03-colregs_execution`
Typical capture launch: `./zlaunch.sh --case=head_on_execution_pass --gui 10`

- `colregs-execution-head-on.gif` - Head-on Resolution; representative case `head_on_execution_pass`
- `colregs-execution-overtaken-midrange.gif` - Overtaken Midrange; representative case `overtaken_starboard_standon_midrange_execution_pass`
- `colregs-execution-crossing-giveway.gif` - Crossing Give-way; representative case `crossing_starboard_giveway_execution_pass`

## H04 COLREGS Parameters

Capture from: `harnesses/colregs_harnesses/H04-colregs_parameters`
Typical capture launch: `./zlaunch.sh --group=min_util_cpa_dist --gui 10`

- `colregs-parameters-bow-distance.gif` - Bow Distance Threshold; representative case `giveway_bow_dist_high_pass`
- `colregs-parameters-turn-radius.gif` - Turn Radius Branch; representative case `turn_radius_high_pass`

## H01 Avoid-Collision Behavior

Capture from: `harnesses/collision_behavior_harnesses/H01-collision_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=head_on_resolve_pass --gui 10`

- `collision-behavior-head-on-resolve.gif` - Head-on Resolve; representative case `head_on_resolve_pass`
- `collision-behavior-no-alert-fail.gif` - No Alert Request Fail; representative case `no_alert_request_fail`
- `collision-behavior-default-resolve.gif` - Baseline Resolve; representative case `default_resolve_pass`

## H01 Avoid-Obstacle Behavior

Capture from: `harnesses/obstacle_behavior_harnesses/H01-obstacle_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=two_obstacles_clean_pass --gui 10`

- `obstacle-behavior-default-auto-request.gif` - Default Auto Request; representative case `default_auto_request_pass`
- `obstacle-behavior-two-obstacles.gif` - Sequential Obstacle Alerts; representative case `two_obstacles_clean_pass`
- `obstacle-behavior-avoid-disabled.gif` - Avoid Disabled Fail; representative case `avoid_disabled_fail`

## H01 OpRegion Safety

Capture from: `harnesses/opregion_harnesses/H01-opregion_safety`
Typical capture launch: `./zlaunch.sh --case=save_recover_pass --gui 10`

- `opregion-safety-save-recover.gif` - Save Recovery; representative case `save_recover_pass`
- `opregion-safety-dynamic-update.gif` - Dynamic Region Update; representative case `dynamic_region_update_pass`
- `opregion-safety-entry-gate.gif` - Entry Gate; representative case `entry_gate_start_outside_pass`

## H01 Waypoint Behavior

Capture from: `harnesses/waypoint_behavior_harnesses/H01-waypoint_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=multi_point_sequence_pass --gui 10`

- `waypoint-behavior-multipoint.gif` - Multi-point Sequence; representative case `multi_point_sequence_pass`
- `waypoint-behavior-dynamic-update.gif` - Dynamic Route Update; representative case `xpoints_update_pass`
- `waypoint-behavior-repeat-forever.gif` - Repeat Forever Cycle; representative case `repeat_forever_cycle_pass`

## H01 Loiter Behavior

Capture from: `harnesses/loiter_behavior_harnesses/H01-loiter_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=radial_clockwise_pass --gui 10`

- `loiter-behavior-radial.gif` - Radial Clockwise; representative case `radial_clockwise_pass`
- `loiter-behavior-radius-expand.gif` - Runtime Radius Expand; representative case `mod_poly_rad_expand_pass`
- `loiter-behavior-center-assign-pair.gif` - Center Assign Pair; representative case `center_assign_pair_pass`

## H01 StationKeep Behavior

Capture from: `harnesses/stationkeep_behavior_harnesses/H01-stationkeep_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=static_station_pass --gui 10`

- `stationkeep-behavior-static.gif` - Static Station; representative case `static_station_pass`
- `stationkeep-behavior-hibernation.gif` - Hibernation Settle; representative case `hibernation_settle_pass`
- `stationkeep-behavior-radius-expand.gif` - Runtime Radius Expand; representative case `radius_update_expand_pass`

## H01 Trail Behavior

Capture from: `harnesses/trail_behavior_harnesses/H01-trail_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=static_trail_pass --gui 10`

- `trail-behavior-relative-port.gif` - Relative Port Trail; representative case `relative_port_pass`
- `trail-behavior-lead-turn-angle-update.gif` - Lead Turn Angle Update; representative case `lead_turn_angle_update_pass`
- `trail-behavior-runtime-range-extend.gif` - Runtime Range Extend; representative case `runtime_range_extend_pass`

## H01 Convoy Behavior

Capture from: `harnesses/convoy_behavior_harnesses/H01-convoy_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=static_convoy_pass --gui 10`

- `convoy-behavior-static.gif` - Static Convoy; representative case `static_convoy_pass`
- `convoy-behavior-angled-entry.gif` - Angled Entry; representative case `angled_entry_pass`
- `convoy-behavior-lead-s-turn.gif` - Lead S-turn; representative case `lead_s_turn_pass`

## H01 CutRange Behavior

Capture from: `harnesses/cutrange_behavior_harnesses/H01-cutrange_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=static_cutrange_pass --gui 10`

- `cutrange-behavior-headon.gif` - Head-on Closure; representative case `headon_cutrange_pass`
- `cutrange-behavior-crossing.gif` - Crossing Closure; representative case `crossing_cutrange_pass`
- `cutrange-behavior-s-turn.gif` - S-turn Target; representative case `s_turn_target_pass`

## H01 FixedTurn Behavior

Capture from: `harnesses/fixedturn_behavior_harnesses/H01-fixedturn_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=starboard_90_pass --gui 10`

- `fixedturn-behavior-starboard-90.gif` - Starboard 90; representative case `starboard_90_pass`
- `fixedturn-behavior-port-360.gif` - Port 360; representative case `port_360_pass`
- `fixedturn-behavior-turn-spec-sequence.gif` - Turn Spec Sequence; representative case `turn_spec_sequence_pass`

## H01 LegRun Behavior

Capture from: `harnesses/legrun_behavior_harnesses/H01-legrun_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=baseline_cycle_pass --gui --port_base=4000 10`

- `legrun-baseline-cycle.gif` - Baseline Cycle; representative case `baseline_cycle_pass`
- `legrun-far-turn-init.gif` - Far Turn Init; representative case `init_far_turn_pass`
- `legrun-individual-turn-params.gif` - Individual Turn Params; representative case `individual_turn_params_pass`

## H01 Shadow Behavior

Capture from: `harnesses/shadow_behavior_harnesses/H01-shadow_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=static_shadow_pass --gui 10`

- `shadow-behavior-static.gif` - Static Shadow; representative case `static_shadow_pass`
- `shadow-behavior-turn-north.gif` - Turn North Shadow; representative case `turn_north_shadow_pass`
- `shadow-behavior-runtime-pwt-on.gif` - Runtime PWT On; representative case `runtime_pwt_outer_on_pass`

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
