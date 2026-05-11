# H01 uFldObstacleSim Unit Harness

This harness validates `uFldObstacleSim` as the obstacle-field source rather than as an obstacle consumer. The stem mission is a single shoreside community that starts the simulator, prompts refresh/reset events with `uTimerScript`, lets `pMissionEval` produce a mission grade, and then checks the resulting `.alog` for source-side publication details that are awkward to express as one current MOOS value.

## Current Matrix

- `fixed_field_publish_pass` Confirms a fixed obstacle file produces `KNOWN_OBSTACLE`, `GIVEN_OBSTACLE`, obstacle visuals, region visuals, and the configured minimum range.
- `multi_field_labels_pass` Confirms a two-obstacle file publishes both configured obstacle labels on the ground-truth and vehicle-facing outputs.
- `duration_fixed_pass` Confirms fixed `min_duration` and `max_duration` are propagated into obstacle payloads.
- `duration_min_only_pass` Confirms `min_duration` alone clamps duration publication to that configured value.
- `duration_min_max_swap_pass` Confirms `max_duration` below an earlier `min_duration` is normalized to the configured minimum value.
- `duration_disabled_zero_pass` Confirms zero duration bounds leave obstacle payloads without duration fields.
- `pmv_connect_repeated_repost_pass` Confirms repeated `PMV_CONNECT` mail triggers repeated obstacle truth and visual refreshes.
- `pmv_connect_absent_no_publish_pass` Confirms the simulator does not publish obstacles before a refresh trigger or refresh interval.
- `vehicle_connect_no_refresh_pass` Confirms current `VEHICLE_CONNECT` mail does not trigger obstacle refresh output.
- `post_visuals_false_pass` Confirms `post_visuals=false` suppresses `VIEW_POLYGON` output while still publishing obstacle truth.
- `draw_region_false_pass` Confirms `draw_region=false` suppresses only the region visual while leaving obstacle visuals enabled.
- `post_points_inside_pass` Confirms point-sensor mode suppresses `GIVEN_OBSTACLE` and emits `TRACKED_FEATURE_ALPHA` and `VIEW_POINT` for an in-range vehicle.
- `post_points_multi_field_pass` Confirms point-sensor mode can report features from two configured obstacles in the same field.
- `post_points_multi_vehicle_pass` Confirms point-sensor mode publishes per-vehicle tracked-feature variables and point labels.
- `invalid_node_report_no_refresh_pass` Confirms malformed `NODE_REPORT` mail does not create ledger state or trigger an obstacle refresh.
- `invalid_node_report_no_points_pass` Confirms malformed `NODE_REPORT` mail does not produce tracked features in point-sensor mode.
- `node_report_mixed_case_vehicle_pass` Confirms mixed-case vehicle names map to the upper-case tracked-feature variable while preserving the point label name.
- `node_report_color_point_pass` Confirms point visuals use the color supplied by the reporting vehicle.
- `sensor_range_outside_pass` Confirms an out-of-range vehicle receives no tracked feature points in point-sensor mode.
- `sensor_range_edge_pass` Confirms a vehicle exactly at the sensor-range boundary still receives tracked feature points.
- `sensor_range_zero_suppresses_pass` Confirms `sensor_range=0` suppresses point output for a vehicle just outside the obstacle boundary.
- `point_size_bigger_pass` Confirms runtime `UFOS_POINT_SIZE=bigger` changes subsequent point visuals.
- `point_size_numeric_pass` Confirms a runtime numeric-prefixed `UFOS_POINT_SIZE` string changes subsequent point visuals.
- `point_size_smaller_pass` Confirms runtime `UFOS_POINT_SIZE=smaller` decrements subsequent point visuals.
- `point_size_sequence_pass` Confirms sequential bigger/smaller point-size mail composes back to the original visual size.
- `point_size_invalid_ignored_pass` Confirms invalid point-size mail leaves subsequent point visuals at the configured size.
- `point_size_floor_ignored_pass` Confirms decrement mail at the minimum configured point size does not reduce point visuals below one.
- `point_size_uppercase_pass` Confirms point-size keyword mail is case-insensitive.
- `point_size_fraction_pass` Confirms numeric-prefix point-size mail accepts fractional values.
- `point_size_zero_ignored_pass` Confirms zero point-size mail is ignored and leaves the configured visual size intact.
- `rate_points_three_pass` Confirms `rate_points=3` produces multiple tracked features and matching point visuals per in-range iteration.
- `rate_points_zero_pass` Confirms `rate_points=0` suppresses tracked feature and point visual output while still publishing source truth.
- `post_points_no_visuals_pass` Confirms point-sensor mode can publish tracked features while `post_visuals=false` suppresses `VIEW_POINT`.
- `node_report_refresh_pass` Confirms a first `NODE_REPORT` can request an obstacle refresh without `PMV_CONNECT`.
- `refresh_interval_repost_pass` Confirms `refresh_interval` causes repeated obstacle truth publication.
- `refresh_interval_post_points_no_given_pass` Confirms refresh intervals still republish `KNOWN_OBSTACLE` without `GIVEN_OBSTACLE` in point-sensor mode.
- `reset_request_reposts_pass` Confirms `UFOS_RESET` clears and republishes obstacles when the known vehicle is outside the reset range.
- `reset_request_inside_blocked_pass` Confirms `UFOS_RESET` is blocked while the known vehicle remains inside the obstacle region.
- `reset_reuse_ids_false_pass` Confirms reset with `reuse_ids=false` retires the original obstacle label and republishes a generated replacement label.
- `reset_post_points_no_given_pass` Confirms reset in point-sensor mode clears and republishes truth without emitting `GIVEN_OBSTACLE`.
- `reset_post_visuals_false_pass` Confirms reset with `post_visuals=false` clears and republishes truth without emitting `VIEW_POLYGON`.
- `reset_range_boundary_blocked_pass` Confirms a reset request is blocked when the nearest vehicle is exactly at `reset_range`.
- `reset_interval_exit_pass` Confirms `reset_interval` can reset the field when a tracked vehicle exits the region after the interval has elapsed.
- `reset_interval_wait_blocked_pass` Confirms `reset_interval` blocks region-exit resets until the configured wait has elapsed.
- `sensor_range_broad_far_pass` Confirms a large `sensor_range` can produce tracked features for a far vehicle.
- `obstacle_visual_style_pass` Confirms obstacle polygon color, size, and transparency settings are reflected in `VIEW_POLYGON`.
- `region_visual_style_pass` Confirms region-specific visual color settings are reflected in the `obs_region` `VIEW_POLYGON`.
- `poly_visual_zero_sizes_pass` Confirms zero-valued polygon visual size and transparency settings are accepted in `VIEW_POLYGON`.
- `missing_obstacle_file_absent_pass` Confirms a missing obstacle file produces no minimum-range, obstacle truth, or visual publications.

Typical runs:

```bash
./zlaunch.sh --jobs=4 --port_base=7600 10
./zlaunch.sh --case=post_points_inside_pass --port_base=7900 10
```
