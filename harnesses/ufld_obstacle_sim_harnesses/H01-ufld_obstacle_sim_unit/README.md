# H01 uFldObstacleSim Unit Harness

This is an intentionally non-visual unit harness. `--gui` is unsupported
because the stem has no `pMarineViewer` configuration.

This harness validates `uFldObstacleSim` as the obstacle-field source rather
than as an obstacle consumer. The stem mission is a single shoreside community
that starts the simulator, prompts refresh/reset events with `uTimerScript`,
and lets the case's `pMissionEval` block write the authoritative `grade=` row.
The shell launcher only prepares, schedules, and aggregates cases; it does not
inspect `.alog` files or reinterpret ordinary mission failures.

## Execution And Grading

- Every case, including serial and single-case runs, executes in its own copy
  beneath `.harness_runs/`.
- Logging defaults to `--log=minimal`, which launches no pLogger because all
  grading is mission-owned. `--log=full` restores the stem's original logger
  for every selected case and can be combined with `--case=<name>`.
- Bash 5.1 or newer provides rolling `--jobs` scheduling. On macOS the launcher
  re-executes Homebrew Bash when available and otherwise prints a clear error.
- Each case gets a 30-port block. The generated shoreside target uses the first
  port; the midpoint pShare offsets are reserved consistently with other
  migrated harnesses. The launcher rejects a selected matrix whose final block
  would exceed port 65535.
- Each overlay owns its scripted inputs, subject configuration, and substantive
  `pMissionEval` conditions. Plain scalars, presence, absence, and counts use
  existing mission variables directly. Publication counters exist only when
  multiplicity is the behavior under test.
- The condition parser does not accept a structured literal containing a
  relation character such as `=`. Cases needing exact structured equality
  therefore post a case-owned expected value and use the parser-safe right-
  variable form, for example
  `KNOWN_OBSTACLE = $ (UFOS_EXPECT_OBSTACLE)`.
- One stock `pEchoVar` process provides narrowly filtered field or history
  adapters only where a current raw value is insufficient. `pMissionEval`
  normally checks those outputs as plain components or nonempty presence
  sentinels. The reset-ID case compares one exact inactive composite against a
  runtime expected variable; no `key=value` payload is embedded as a condition
  literal. No custom grading application or repository app is added.
- `uTimerScript` waits for the evaluator, adapter, and subject to appear in the
  MOOSDB client list before starting a case's event clock.
- The harness forwards `--max_time` unchanged to each stem wrapper as the
  mission's `uMayFinish` ceiling. It does not add a second wall-clock watchdog
  or process-tree supervisor.
- `pMissionEval` writes exactly one result row. After a clean stem exit, the
  harness prepends `case=` and preserves that row. Missing/malformed results,
  or a nonzero stem exit become runner-owned `reason=` failures; all usable
  mission fields remain as provenance, with reserved names rewritten (for
  example, `grade=` becomes `mission_grade=`).
- Results are aggregated in the declared case order after all selected cases
  finish. Ordinary failures do not stop later cases from running.
- Teardown is scoped to the case or invocation root. `--keep_workdirs` retains
  generated targets, logs, intermediate rows, and patch sidecars for review.
  A scoped-cleanup failure also retains its run root and safety lock;
  inspect the printed path, clear any surviving scoped processes, and remove
  `.harness_runs.lock` only after the root is safe.

## Current Matrix

- `fixed_field_publish_pass` Loads one square `ufos_a` obstacle at `(10,-10)` to `(14,-6)` inside the configured field and triggers refresh with `PMV_CONNECT`; passes when `KNOWN_OBSTACLE` and `GIVEN_OBSTACLE` exactly match that polygon, `UFOS_MIN_RNG=8`, the obstacle visual is observed, the final region visual is exact, and exactly two polygons are posted.
- `multi_field_labels_pass` Loads `ufos_a` and `ufos_b` from `multi_field.txt`; passes when filtered history contains both labels on both truth channels and each channel publishes exactly two obstacles.
- `duration_fixed_pass` Sets `min_duration=max_duration=12`; passes when both exact obstacle payloads append `duration=12`.
- `duration_min_only_pass` Sets only `min_duration=7`; passes when both exact obstacle payloads append `duration=7`.
- `duration_min_max_swap_pass` Sets `min_duration=10` and the smaller `max_duration=6`; passes when both exact payloads use normalized `duration=10`.
- `duration_disabled_zero_pass` Sets both duration bounds to zero; passes when each truth channel publishes exactly once and both exact payloads omit the duration field.
- `pmv_connect_repeated_repost_pass` Posts `PMV_CONNECT` at times 1, 2, and 3; passes when the connect, `KNOWN_OBSTACLE`, and `GIVEN_OBSTACLE` counters are all exactly three.
- `pmv_connect_absent_no_publish_pass` Loads the fixed field but provides no refresh trigger or interval; passes when `UFOS_MIN_RNG=8` is available while no `PMV_CONNECT`, truth, or polygon publication occurs.
- `vehicle_connect_no_refresh_pass` Posts `VEHICLE_CONNECT=true` instead of `PMV_CONNECT`; passes when the input and `UFOS_MIN_RNG=8` are observed but no truth or polygon publication occurs.
- `post_visuals_false_pass` Sets `post_visuals=false` and triggers the fixed field; passes when filtered history contains both truth outputs and no `VIEW_POLYGON` is ever published.
- `draw_region_false_pass` Sets `draw_region=false` with obstacle visuals enabled; passes when both truth outputs and the `ufos_a` obstacle polygon appear but no polygon labeled `obs_region` is observed.
- `post_points_inside_pass` Enables one point per cycle and places Alpha at `(8,-8)`, 2 meters from `ufos_a`; passes when known truth, a tracked feature keyed `ufos_a`, and an Alpha point labeled for `ufos_a` appear while `GIVEN_OBSTACLE` remains absent.
- `post_points_multi_field_pass` Enables point mode on the two-obstacle field, places Alpha at `(22,0)`, and uses `sensor_range=80`; passes when known history contains both obstacles and `TRACKED_FEATURE_ALPHA` history contains keys `ufos_a` and `ufos_b`, with no `GIVEN_OBSTACLE`.
- `post_points_multi_vehicle_pass` Places Alpha at `(8,-8)` and Beta at `(8,-7)` in one-obstacle point mode; passes when both per-vehicle tracked-feature variables carry key `ufos_a`, point history contains labels for Alpha and Beta, and no `GIVEN_OBSTACLE` appears.
- `invalid_node_report_no_refresh_pass` Posts only `NAME=alpha,TYPE=KAYAK` without a refresh trigger; passes when the malformed mail and loaded `UFOS_MIN_RNG=8` are visible but no truth, tracked feature, or polygon is published.
- `invalid_node_report_no_points_pass` First refreshes known truth in point mode and then posts the same incomplete node report; passes when no tracked feature, point visual, or `GIVEN_OBSTACLE` is produced.
- `node_report_mixed_case_vehicle_pass` Posts a valid report with `NAME=Alpha`; passes when the feature is published on `TRACKED_FEATURE_ALPHA`, its key is `ufos_a`, and the normalized point record retains the exact mixed-case label `Alpha`, with no `GIVEN_OBSTACLE`.
- `node_report_color_point_pass` Posts Alpha with `COLOR=red` in one-point mode; passes when one tracked feature keyed `ufos_a` and a size-2 Alpha/`ufos_a` point are observed, including a point whose vertex color is red.
- `sensor_range_outside_pass` Places Alpha at `(100,100)` with `sensor_range=5`; passes when known truth is present but no tracked feature, point visual, or `GIVEN_OBSTACLE` appears.
- `sensor_range_edge_pass` Places Alpha at `(5,-8)`, exactly 5 meters from the left edge of `ufos_a`, with `sensor_range=5`; passes when a feature keyed `ufos_a` and its point visual appear while `GIVEN_OBSTACLE` stays absent.
- `sensor_range_zero_suppresses_pass` Places Alpha at `(8,-8)`, 2 meters from the obstacle, with `sensor_range=0`; passes when known truth is present but no tracked feature, point visual, or `GIVEN_OBSTACLE` appears.
- `point_size_bigger_pass` Starts with `point_size=2`, posts `UFOS_POINT_SIZE=bigger`, and then reports Alpha in range; passes when the input is observed and point history contains size `3`, with no `GIVEN_OBSTACLE`.
- `point_size_numeric_pass` Starts at size 2 and posts `UFOS_POINT_SIZE=5pts`; passes when the numeric prefix produces a size-5 point and no `GIVEN_OBSTACLE`.
- `point_size_smaller_pass` Starts at size 2 and posts `UFOS_POINT_SIZE=smaller`; passes when subsequent point history contains size `1` and no `GIVEN_OBSTACLE`.
- `point_size_sequence_pass` Posts `bigger`, samples a point, then posts `smaller` and samples again; passes when point history proves size `3` occurred before the final Alpha/`ufos_a` point returns to size `2`, with no `GIVEN_OBSTACLE`.
- `point_size_invalid_ignored_pass` Starts at size 2 and posts `UFOS_POINT_SIZE=invalid`; passes when the input is observed but the subsequent point remains size `2`, with no `GIVEN_OBSTACLE`.
- `point_size_floor_ignored_pass` Starts at the minimum size 1 and posts `smaller`; passes when the subsequent point remains size `1`, with no `GIVEN_OBSTACLE`.
- `point_size_uppercase_pass` Starts at size 2 and posts uppercase `UFOS_POINT_SIZE=BIGGER`; passes when the subsequent point is size `3`, demonstrating case-insensitive keyword handling.
- `point_size_fraction_pass` Posts `UFOS_POINT_SIZE=2.5pts`; passes when the numeric prefix produces a size-2.5 point and no `GIVEN_OBSTACLE`.
- `point_size_zero_ignored_pass` Starts at size 2 and posts numeric zero; passes when the input is observed but the subsequent point remains size `2`, with no `GIVEN_OBSTACLE`.
- `rate_points_three_pass` Sets `AppTick=0.2` to leave one bounded publication iteration after Alpha's report and configures `rate_points=3`; passes when exactly three `TRACKED_FEATURE_ALPHA` and three `VIEW_POINT` posts occur and a feature keyed `ufos_a` is present.
- `rate_points_zero_pass` Sets `rate_points=0` with Alpha in range; passes when known truth is present but no tracked feature, point visual, or `GIVEN_OBSTACLE` appears.
- `post_points_no_visuals_pass` Sets `post_points=true` and `post_visuals=false` with Alpha in range; passes when a tracked feature keyed `ufos_a` is published while neither `VIEW_POINT` nor `GIVEN_OBSTACLE` appears.
- `node_report_refresh_pass` Provides no `PMV_CONNECT` and posts the first valid Alpha report at `(12,-8)`; passes when both known and given obstacle outputs appear and no `PMV_CONNECT` was observed.
- `refresh_interval_repost_pass` Sets `refresh_interval=0.6`, triggers once at time 1, and evaluates at time 6; passes when both truth channels contain `ufos_a` and each has published at least three times.
- `refresh_interval_post_points_no_given_pass` Uses the same 0.6-second interval with `post_points=true` and `rate_points=0`; passes when `KNOWN_OBSTACLE` publishes at least three times and `GIVEN_OBSTACLE` never appears.
- `reset_request_reposts_pass` Uses an outside Alpha report at `(100,100)` to create the initial field, then posts `UFOS_RESET=true`; passes when `KNOWN_OBSTACLE_CLEAR=all` appears and known-obstacle count reaches at least two.
- `reset_request_inside_blocked_pass` Refreshes the field, places Alpha at `(20,0)` inside the field region, and posts `UFOS_RESET=true` with `reset_range=10`; passes when the reset input and current known/given obstacle remain present but no clear publication occurs.
- `reset_reuse_ids_false_pass` Refreshes `ufos_a`, places Alpha outside, and resets with `reuse_ids=false`; passes when the field clears, exact history records `active=false:label=ufos_a`, and the replacement appears with generated label `ob_1`.
- `reset_post_points_no_given_pass` Creates and resets a point-mode field with Alpha outside and `rate_points=0`; passes when `KNOWN_OBSTACLE_CLEAR=all` appears, known-obstacle count reaches at least two, and no `GIVEN_OBSTACLE` is published.
- `reset_post_visuals_false_pass` Creates and resets the field with `post_visuals=false`; passes when the clear appears and known-obstacle count reaches at least two while no `VIEW_POLYGON` is ever published.
- `reset_range_boundary_blocked_pass` Places Alpha at `(70,0)`, exactly 10 meters outside the field-region boundary, and posts reset with `reset_range=10`; passes when the input and current truth remain present but no clear publication occurs.
- `reset_interval_exit_pass` Places Alpha inside the region at `(20,0)`, moves it to `(100,100)` after the 0.5-second reset interval, and never posts `UFOS_RESET`; passes when an automatic `KNOWN_OBSTACLE_CLEAR=all` occurs and known-obstacle count reaches at least two.
- `reset_interval_wait_blocked_pass` Refreshes the field, moves Alpha from inside to outside after 0.9 seconds, but sets `reset_interval=100`; passes when current truth remains present and neither `UFOS_RESET` nor a clear publication appears.
- `sensor_range_broad_far_pass` Places Alpha at `(100,100)` with `sensor_range=200`; passes when a tracked feature keyed `ufos_a` and an `ufos_a` point visual appear while `GIVEN_OBSTACLE` stays absent.
- `obstacle_visual_style_pass` Disables the region visual and sets obstacle fill green, edge red, vertices blue, label white, vertex size 3, edge size 2, and transparency 0.4; passes when `VIEW_POLYGON` exactly matches the styled `ufos_a` payload.
- `region_visual_style_pass` Sets the field-region edge orange and vertices yellow; passes when the exact `obs_region` polygon payload contains those colors while obstacle truth remains present.
- `poly_visual_zero_sizes_pass` Sets obstacle vertex size, edge size, and fill transparency to zero; passes when the exact `ufos_a` visual retains all three zero-valued fields.
- `missing_obstacle_file_absent_pass` Points `obstacle_file` at nonexistent `missing_field.txt`; passes only while `UFOS_MIN_RNG`, known/given truth, and polygon visuals all remain unpublished.

Typical runs:

```bash
./zlaunch.sh --jobs=2 --port_base=28000 10
./zlaunch.sh --case=post_points_inside_pass --port_base=30000 10
./zlaunch.sh --log=full --case=post_points_inside_pass --port_base=30000 10
./zlaunch.sh --jobs=2 --port_base=24000 --keep_workdirs 10
```
