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
Generated Timer behavior visuals live in `docs/tools/render_timer_behavior_gifs.py`.
Generated PeriodicSpeed behavior visuals live in `docs/tools/render_periodic_speed_behavior_gifs.py`.
Generated MemoryTurnLimit behavior visuals live in `docs/tools/render_memoryturnlimit_behavior_gifs.py`.
Generated utility infrastructure visuals live in `docs/tools/render_utility_watch_gifs.py`.
Generated simulator infrastructure visuals live in `docs/tools/render_simulator_infrastructure_gifs.py`.
Generated mission utility visuals live in `docs/tools/render_mission_utility_gifs.py`.
Generated uFldObstacleSim visuals live in `docs/tools/render_ufld_obstacle_sim_gifs.py`.
Generated uField app visuals live in `docs/tools/render_ufield_app_gifs.py`.
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

## H01 uSimMarineV22 Motion

Capture from: `harnesses/usim_marine_harnesses/H01-usim_marine_motion`
Typical capture launch: `./zlaunch.sh --case=thrust_forward_pass --port_base=30000 10`

- `usim-marine-forward-thrust.gif` - Forward Thrust; representative case `thrust_forward_pass`
- `usim-marine-runtime-drift.gif` - Runtime Drift; representative case `drift_x_pass`

## H01 pNodeReporter Unit

Capture from: `harnesses/pnodereporter_harnesses/H01-pnodereporter_unit`
Typical capture launch: `./zlaunch.sh --case=nav_report_baseline_pass --port_base=12400 10`

- `pnodereporter-baseline-report.gif` - Baseline Node Report; representative case `nav_report_baseline_pass`
- `pnodereporter-alt-nav-report.gif` - Alternate Nav Report; representative case `alt_nav_report_pass`

## H01 uPokeDB Unit

Capture from: `harnesses/upokedb_harnesses/H01-upokedb_unit`
Typical capture launch: `./zlaunch.sh --case=numeric_direct_pass --port_base=15000 10`

- `upokedb-direct-numeric.gif` - Direct Numeric Poke; representative case `numeric_direct_pass`
- `upokedb-cached-pokes.gif` - Cached Pokes; representative case `cache_string_numeric_pass`

## H01 uXMS Unit

Capture from: `harnesses/uxms_harnesses/H01-uxms_unit`
Typical capture launch: `./zlaunch.sh --case=scoped_var_pass --port_base=15200 10`

- `uxms-scoped-variable.gif` - Scoped Variable; representative case `scoped_var_pass`
- `uxms-history-view.gif` - History View; representative case `history_var_pass`

## H01 uQueryDB Unit

Capture from: `harnesses/uquerydb_harnesses/H01-uquerydb_unit`
Typical capture launch: `./zlaunch.sh --case=cli_numeric_pass --port_base=15600 10`

- `uquerydb-numeric-condition.gif` - Numeric Condition; representative case `cli_numeric_pass`
- `uquerydb-checkvar-formats.gif` - Checkvar Formats; representative case `checkvar_csv_pass`

## H01 pDeadManPost Unit

Capture from: `harnesses/pdeadmanpost_harnesses/H01-pdeadmanpost_unit`
Typical capture launch: `./zlaunch.sh --case=active_start_once_posts_pass --port_base=15800 10`

- `pdeadmanpost-deadman-trip.gif` - Deadman Trip; representative case `active_start_once_posts_pass`
- `pdeadmanpost-heartbeat-suppression.gif` - Heartbeat Suppression; representative case `heartbeat_before_dead_suppresses_pass`

## H01 pSpoofNode Unit

Capture from: `harnesses/pspoofnode_harnesses/H01-pspoofnode_unit`
Typical capture launch: `./zlaunch.sh --case=config_static_report_pass --port_base=16000 10`

- `pspoofnode-static-spoof.gif` - Static Spoof; representative case `config_static_report_pass`
- `pspoofnode-cancel-by-name.gif` - Cancel By Name; representative case `cancel_vname_pass`

## H01 uTermCommand Unit

Capture from: `harnesses/utermcommand_harnesses/H01-utermcommand_unit`
Typical capture launch: `./zlaunch.sh --case=numeric_command_pass --port_base=16200 10`

- `utermcommand-numeric-command.gif` - Numeric Command; representative case `numeric_command_pass`
- `utermcommand-arrow-syntax.gif` - Arrow Syntax; representative case `arrow_syntax_command_pass`

## H01 pSearchGrid Unit

Capture from: `harnesses/psearchgrid_harnesses/H01-psearchgrid_unit`
Typical capture launch: `./zlaunch.sh --case=node_local_delta_pass --port_base=16400 10`

- `psearchgrid-grid-delta.gif` - Grid Delta; representative case `node_local_delta_pass`
- `psearchgrid-full-grid-report.gif` - Full Grid Report; representative case `full_grid_report_pass`

## H01 uField Comms Unit

Capture from: `harnesses/ufield_comms_harnesses/H01-ufield_comms_unit`
Typical capture launch: `./zlaunch.sh --case=baseline_broker_comms_pass --port_base=4000 10`

- `ufield-comms-baseline-broker.gif` - Baseline Broker Comms; representative case `baseline_broker_comms_pass`
- `ufield-comms-runtime-range.gif` - Runtime Range Extend; representative case `runtime_range_extend_pass`

## H02 uField Broker Bridge

Capture from: `harnesses/ufield_comms_harnesses/H02-ufield_broker_bridge`
Typical capture launch: `./zlaunch.sh --case=broker_handshake_pass --port_base=4000 10`

- `ufield-broker-handshake.gif` - Broker Handshake; representative case `broker_handshake_pass`
- `ufield-broker-bridge-tokens.gif` - Bridge Tokens; representative case `shore_bridge_tokens_pass`

## H03 uField Route Resilience

Capture from: `harnesses/ufield_comms_harnesses/H03-ufield_route_resilience`
Typical capture launch: `./zlaunch.sh --case=runtime_tryhost_recover_pass --port_base=4000 10`

- `ufield-route-runtime-recovery.gif` - Runtime Route Recovery; representative case `runtime_tryhost_recover_pass`
- `ufield-route-vnode-discovery.gif` - Shore VNode Discovery; representative case `shore_vnode_discovery_recover_pass`

## H01 uFldObstacleSim Unit

Capture from: `harnesses/ufld_obstacle_sim_harnesses/H01-ufld_obstacle_sim_unit`
Typical capture launch: `./zlaunch.sh --case=fixed_field_publish_pass --port_base=7600 10`

- `ufld-obstacle-sim-fixed-field.gif` - Fixed Field Publish; representative case `fixed_field_publish_pass`
- `ufld-obstacle-sim-point-sensor.gif` - Point Sensor Mode; representative case `post_points_inside_pass`

## H01 uFldPathCheck Unit

Capture from: `harnesses/ufld_pathcheck_harnesses/H01-ufld_pathcheck_unit`
Typical capture launch: `./zlaunch.sh --case=odometry_baseline_pass --port_base=4300 10`

- `ufld-pathcheck-baseline-odometry.gif` - Baseline Odometry; representative case `odometry_baseline_pass`
- `ufld-pathcheck-trip-reset.gif` - Trip Reset; representative case `trip_reset_pass`

## H01 uFldMessageHandler Unit

Capture from: `harnesses/ufld_message_handler_harnesses/H01-ufld_message_handler_unit`
Typical capture launch: `./zlaunch.sh --case=dest_specific_self_pass --port_base=4400 10`

- `ufld-message-handler-accepted.gif` - Accepted Message; representative case `dest_specific_self_pass`
- `ufld-message-handler-strict-reject.gif` - Strict Reject; representative case `strict_all_reject_pass`

## H01 uFldContactRangeSensor Unit

Capture from: `harnesses/ufld_contact_range_sensor_harnesses/H01-ufld_contact_range_sensor_unit`
Typical capture launch: `./zlaunch.sh --case=baseline_short_report_pass --port_base=4500 10`

- `ufld-contact-range-sensor-baseline.gif` - Baseline Range; representative case `baseline_short_report_pass`
- `ufld-contact-range-sensor-arc-block.gif` - Arc Block; representative case `sensor_arc_aft_block_pass`

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

## H01 PeriodicSpeed Behavior

Capture from: `harnesses/periodic_speed_behavior_harnesses/H01-periodic_speed_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=baseline_cycle_pass --gui --port_base=15000 10`

- `periodic-speed-baseline-cycle.gif` - Baseline Lazy/Busy Cycle; representative case `baseline_cycle_pass`
- `periodic-speed-reset-false-reenable.gif` - Reset False Re-Enable; representative case `reset_false_visual_pass`

## H01 MemoryTurnLimit Behavior

Capture from: `harnesses/memoryturnlimit_behavior_harnesses/H01-memoryturnlimit_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=baseline_memory_pass --gui --port_base=15000 10`

- `memoryturnlimit-behavior-tight-window.gif` - Tight Turn Window; representative case `tight_window_pass`
- `memoryturnlimit-behavior-runtime-widen.gif` - Runtime Range Widen; representative case `runtime_widen_range_pass`

## H01 Timer Behavior

Capture from: `harnesses/timer_behavior_harnesses/H01-timer_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=baseline_idle_running_pass --gui --port_base=15000 10`

- `timer-behavior-pause-resume.gif` - Pause/Resume Counters; representative case `pause_resume_pass`
- `timer-behavior-runtime-update.gif` - Runtime Status Update; representative case `runtime_update_pass`

## H01 TestFailure Behavior

Capture from: `harnesses/testfailure_behavior_harnesses/H01-testfailure_behavior_unit`
Typical capture launch: `./zlaunch.sh --case=burn_gap_detected_pass --port_base=15000 10`

- `testfailure-burn-gap.gif` - Burn Gap Detection; representative case `burn_gap_detected_pass`
- `testfailure-crash-process-loss.gif` - Crash Process Loss; representative case `crash_on_complete_fail`

## H01 pHostInfo Unit

Capture from: `harnesses/hostinfo_harnesses/H01-hostinfo_unit`
Typical capture launch: `./zlaunch.sh --jobs=3 --port_base=11000 10`

- `hostinfo-pshare-route-payload.gif` - pShare Route Payload; representative case `hostinfo_pshare_route_pass`
- `hostinfo-invalid-route-omitted.gif` - Invalid Route Omitted; representative case `hostinfo_invalid_pshare_route_pass`

## H01 uLoadWatch Unit

Capture from: `harnesses/loadwatch_harnesses/H01-loadwatch_unit`
Typical capture launch: `./zlaunch.sh --jobs=3 --port_base=11200 10`

- `loadwatch-near-breach-band.gif` - Near-Breach Band; representative case `loadwatch_near_breach_pass`
- `loadwatch-trigger-holdoff.gif` - Trigger Holdoff; representative case `loadwatch_trigger_second_breaches_pass`

## H01 uProcessWatch Unit

Capture from: `harnesses/processwatch_harnesses/H01-processwatch_unit`
Typical capture launch: `./zlaunch.sh --jobs=3 --port_base=11600 10`

- `processwatch-mapped-summary.gif` - Mapped Summary; representative case `processwatch_post_mapping_pass`
- `processwatch-awol-detection.gif` - AWOL Detection; representative case `processwatch_missing_awol_fail`

## H01 pShare Unit

Capture from: `harnesses/pshare_harnesses/H01-pshare_unit`
Typical capture launch: `./zlaunch.sh --jobs=2 --port_base=11000 10`

- `pshare-direct-route.gif` - Direct Route; representative case `pshare_direct_route_pass`
- `pshare-wildcard-route.gif` - Wildcard Route; representative case `pshare_wildcard_route_pass`

## H02 pShare Topology

Capture from: `harnesses/pshare_harnesses/H02-pshare_topology`
Typical capture launch: `./zlaunch.sh --jobs=2 --port_base=11000 --max_time=65 10`

- `pshare-topology-fanin.gif` - Fan-in; representative case `pshare_topology_fanin_pass`
- `pshare-topology-multicast-relay-proof.gif` - Multicast Relay Proof; representative case `pshare_topology_multicast_multi_listener_pass`

## H01 pLogger Unit

Capture from: `harnesses/plogger_harnesses/H01-plogger_unit`
Typical capture launch: `./zlaunch.sh --jobs=2 --port_base=12000 10`

- `plogger-explicit-log-capture.gif` - Explicit Log Capture; representative case `plogger_explicit_log_pass`
- `plogger-wildcard-omit.gif` - Wildcard Omit; representative case `plogger_wildcard_omit_pass`

## H01 pAntler Unit

Capture from: `harnesses/pantler_harnesses/H01-pantler_unit`
Typical capture launch: `./zlaunch.sh --jobs=2 --port_base=12200 10`

- `pantler-alias-launch.gif` - Alias Launch; representative case `pantler_alias_launch_pass`
- `pantler-multi-alias-launch.gif` - Multi-alias Launch; representative case `pantler_multi_alias_launch_pass`

## H01 pMissionEval Unit

Capture from: `harnesses/mission_utility_harnesses/H01-pmissioneval_unit`
Typical capture launch: `./zlaunch.sh --jobs=3 --port_base=7100 10`

- `pmissioneval-baseline-evaluation.gif` - Baseline Evaluation; representative case `eval_baseline_pass`
- `pmissioneval-two-stage-logic.gif` - Two-stage Logic; representative case `eval_sequence_two_stage_pass`

## H02 pMissionHash Unit

Capture from: `harnesses/mission_utility_harnesses/H02-pmissionhash_unit`
Typical capture launch: `./zlaunch.sh --jobs=3 --port_base=7300 10`

- `pmissionhash-custom-vars.gif` - Custom Hash Vars; representative case `hash_custom_vars_pass`
- `pmissionhash-reset.gif` - Hash Reset; representative case `hash_reset_changes_pass`

## H03 uMayFinish Unit

Capture from: `harnesses/mission_utility_harnesses/H03-umayfinish_unit`
Typical capture launch: `./zlaunch.sh --jobs=3 --port_base=7400 10`

- `umayfinish-default-exit.gif` - Default Finish Exit; representative case `mayfinish_default_exit_pass`
- `umayfinish-custom-value-timeout.gif` - Custom Value Timeout; representative case `mayfinish_custom_value_timeout_fail`

## H01 ZigZag Behavior

Capture from: `harnesses/zigzag_behavior_harnesses/H01-zigzag_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=baseline_port_first_pass --gui --port_base=15000 10`

- `zigzag-behavior-baseline-port.gif` - Baseline Port First; representative case `baseline_port_first_pass`
- `zigzag-behavior-wide-angle.gif` - Wide Angle; representative case `wide_angle_pass`
- `zigzag-behavior-fierce-zigging.gif` - Fierce Zigging; representative case `fierce_zigging_pass`

## H01 LegRun Behavior

Capture from: `harnesses/legrun_behavior_harnesses/H01-legrun_behavior_motion`
Typical capture launch: `./zlaunch.sh --case=baseline_cycle_pass --gui --port_base=4000 10`

- GIF pending - Baseline Cycle; representative case `baseline_cycle_pass`
- GIF pending - Far Turn Init; representative case `init_far_turn_pass`
- GIF pending - Individual Turn Params; representative case `individual_turn_params_pass`

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
