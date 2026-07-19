# H01-waypoint_behavior_motion

Logging is minimal by default in both communities. Use `--log=full` for the
whole matrix or with `--case=NAME` for one diagnostic case.

Patch-driven moving correctness harness for
[`missions/waypoint_behavior_missions/waypoint_behavior_motion`](../../../missions/waypoint_behavior_missions/waypoint_behavior_motion).

This harness focuses on `BHV_Waypoint` as the behavior under test. The stem
mission keeps one simulated vehicle on a short corridor and grades on
behavior-owned waypoint outputs:

- completion through `WPT_DONE`
- end flags through `WAYPOINT_END`
- waypoint-hit flags through `WPT_HIT`
- repeat/cycle flags through `CYCLE_HIT`
- runtime updates through `WPT_UPDATES`
- clean behavior state through `BHV_ERROR_SEEN=false`

## Current Matrix

- `single_point_arrival_pass` Drives from `(-18,-121)` to the default point `(42,-121)` at speed `2`; passes on completion when `WPT_DONE`, `WPT_HIT`, `CYCLE_HIT`, and `WAYPOINT_END` are true and no `BHV_ERROR` occurred.
- `endflag_pass` Uses the same default single-point route to exercise the configured `WAYPOINT_END=true` end flag; passes on the same four completion flags and no behavior error.
- `multi_point_sequence_pass` Configures `(-3,-121):(18,-103):(42,-121)`, exercising a three-point route with a turn; passes when the route completes with all four waypoint/cycle/end flags true and no error.
- `wptflag_pass` Uses the same three-point route to exercise `wptflag=WPT_HIT=true`; passes when `WPT_DONE=true`, `WPT_HIT=true`, and no error occurred.
- `repeat_once_cycle_pass` Configures points `(4,-121):(26,-121)` with `repeat=1`, exercising one repeated cycle; passes when completion, cycle, and end flags are true and no error occurred.
- `repeat_forever_cycle_pass` Configures vertical points `(22,-121):(22,-99)` with `repeat=forever` and `perpetual=true`; passes at 38 seconds when a waypoint and cycle have been hit, `CYCLE_INDEX>=1`, neither done nor end flag has fired, and no error occurred.
- `dynamic_points_update_pass` Starts with `points=empty` and posts `points=12,-106:42,-121` at five seconds, exercising runtime route installation; passes when the new route completes with all four waypoint/cycle/end flags true and no error.
- `newpt_update_pass` Starts with `points=empty` and posts `newpt=x=42,y=-121` at two seconds, exercising runtime append of one waypoint; passes when the behavior completes with all four waypoint/cycle/end flags true and no error.
- `xpoints_update_pass` Starts with `(12,-121):(42,-121)` and posts same-length `xpoints=12,-121:42,-103` at 11 seconds, exercising in-place route replacement; passes when the route completes with all four waypoint/cycle/end flags true and no error.
- `capture_radius_large_pass` Places point `(12,-121)` 30 meters from the start, disables capture lines, and sets `capture_radius=40`; exercising large-radius capture, it passes when the route completes with all four waypoint/cycle/end flags true and no error.
- `capture_line_absolute_pass` Sets `capture_line=absolute` for point `(12,-121)` while omitting capture and slip radii, exercising absolute line-crossing capture; passes when all four completion flags are true and no error occurred.
- `slip_radius_pass` Disables capture lines and sets `capture_radius=0.1`, `slip_radius=8` for point `(12,-121)`, exercising closest-approach slip capture; passes when all four completion flags are true and no error occurred.
- `order_reverse_pass` Configures collinear points `(-3,-121):(18,-121):(42,-121)` with `order=reverse`, exercising reverse traversal order; passes when all four completion flags are true and no error occurred.
- `currix_start_pass` Uses the same three points with `currix=1`, exercising startup at the second waypoint; passes when all four completion flags are true and no error occurred.
- `lead_to_start_pass` Configures points `(10,-113):(42,-121)` with `lead_to_start=true`, `lead=12`, and `lead_damper=4`, exercising trackline lead into the route; passes when all four completion flags are true and no error occurred.
- `lead_condition_pass` Configures `lead_condition=ALLOW_LEAD=true`, posts that variable at two seconds, and uses a three-point route, exercising conditional lead enablement; passes when all four completion flags are true and no error occurred.
- `end_spd_slowdown_pass` Sets `end_spd=slow_dist=25,stop_dist=1` for point `(42,-121)`, exercising terminal speed tapering; passes when all four completion flags are true and no error occurred.
- `speed_alt_update_pass` Starts with `speed=0.35`, configures `speed_alt=2.2`, and posts `use_alt_speed=true` at three seconds, exercising runtime alternate-speed selection; passes when all four completion flags are true and no error occurred.
- `reset_on_idle_pass` Sets `reset_on_idle=true` for points `(10,-121):(42,-121)`, turns `DEPLOY` off at six seconds and back on at nine, exercising traversal reset across idle; passes when all four completion flags are true and no error occurred.
- `empty_start_then_update_pass` Starts with `points=empty` and posts `point=start` at two seconds, exercising a runtime waypoint at the current ownship position; passes when `WPT_DONE=true` and no error occurred.
- `wptflag_on_start_pass` Sets `wptflag_on_start=true` for points `(18,-121):(42,-121)`, exercising the activation-time waypoint flag; passes at three seconds when `WPT_HIT=true`, `WPT_DONE=false`, and no error occurred.
- `custom_status_vars_pass` Sets `post_suffix=CUSTOM` and custom previous/next-distance variable names on a three-point route, exercising suffixed status outputs; passes at five seconds while incomplete when the custom index is nonnegative, previous distance is nonnegative, next distance is positive, and no error occurred.
- `polygon_points_pass` Supplies `polygon=pts={-3,-121:18,-121:42,-121}` instead of `points`, exercising polygon-to-route parsing; passes when all four completion flags are true and no error occurred.
- `ipf_type_roc_pass` Sets `ipf_type=roc` for point `(36,-121)`, exercising the rate-of-closure objective; passes when all four completion flags are true and no error occurred.
- `ipf_type_zaic_spd_pass` Sets `ipf_type=zaic_spd` for point `(36,-121)`, exercising the alternate speed ZAIC; passes when all four completion flags are true and no error occurred.
- `greedy_tour_pass` Configures points `(42,-121):(8,-104):(24,-121)` with `greedy_tour=true`, exercising ownship-relative route reordering; passes when all four completion flags are true and no error occurred.
- `cycle_index_output_pass` Reuses the two-point `repeat=1` route to exercise `cycle_index_var`; passes when done, end, and cycle flags are true, `CYCLE_INDEX>=1`, and no error occurred.
- `efficiency_measure_pass` Sets `efficiency_measure=all` on route `(4,-121):(24,-103):(44,-121)`, exercising aggregate efficiency publication; passes on completion when both `WPT_EFF_DIST_ALL` and `WPT_EFF_TIME_ALL` are positive, `WAYPOINT_END=true`, and no error occurred.
- `slow_speed_no_arrival_pass` Sets speed `0.35`, disables capture lines, and targets `(42,-121)` with `capture_radius=2`; the harness passes at 10 seconds when no waypoint/cycle/end flag has fired, `WPT_DIST_TO_NEXT>=40`, and no error occurred.
- `no_points_timeout_fail` Starts with `points=empty` and sends no update, exercising the no-route idle state; the harness passes at eight seconds when no waypoint/cycle/end flag has fired and no error occurred.
- `malformed_update_fail` Starts with `points=empty` and posts `points=not_a_point` at two seconds, exercising malformed runtime route rejection; the harness passes at eight seconds when no waypoint/cycle/end flag has fired and no error occurred.
- `bad_xpoints_size_fail` Starts with two points `(80,-121):(105,-121)` and posts one-point `xpoints=30,-121` at two seconds, exercising size-mismatch rejection; the harness passes at eight seconds when no waypoint/cycle/end flag has fired and no error occurred.
- `bad_speed_fail` Sets `speed=-1`, exercising negative-speed rejection; the harness currently passes when any `IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_capture_line_fail` Sets `capture_line=diagonal`, exercising invalid capture-mode rejection; the harness currently passes when any `IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_capture_radius_fail` Sets `capture_radius=-1`, exercising negative capture-radius rejection; the harness currently passes when any `IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_slip_radius_fail` Sets `slip_radius=-1`, exercising negative slip-radius rejection; the harness currently passes when any `IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_order_fail` Sets `order=sideways`, exercising invalid route-order rejection; the harness currently passes when any `IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_repeat_fail` Sets `repeat=-1`, exercising negative repeat-count rejection; the harness currently passes when any `IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_lead_damper_fail` Sets `lead_damper=0`, exercising rejection of a nonpositive lead damper; the harness currently passes when any `IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_lead_condition_fail` Sets malformed `lead_condition=ALLOW_LEAD >`, exercising condition-parser rejection; the harness currently passes when any `IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_end_spd_fail` Sets `end_spd=slow_dist=10,stop_dist=20`, exercising rejection when stop distance exceeds slowdown distance; the harness currently passes when any `IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_patience_fail` Sets `patience=0`, exercising rejection outside the accepted course/speed ratio range; the harness currently passes when any `IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_efficiency_measure_fail` Sets `efficiency_measure=summary`, exercising rejection outside `all|off`; the harness currently passes when any `IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=repeat_once_cycle_pass 10
./zlaunch.sh --case=repeat_forever_cycle_pass --gui 1
./zlaunch.sh --just_make 10
./zlaunch.sh --jobs=4 --port_base=36000 10
```

Results are appended to `results.txt` as `case=<name>` followed by the
mission-owned row. Expected-negative cases still produce `grade=pass` when their
specific evidence is observed, using `expected=no_waypoint_completion` or
`expected=helm_malconfig` plus the supporting fields. Serial and rolling modes
both use isolated mission copies. Rolling mode starts the next pending case as
soon as an active slot finishes. Each case gets its own MOOSDB and pShare port
block using `case_base = port_base + case_idx*PORT_STRIDE`, with pShare ports
starting at `case_base + 10`. The default is serial `--jobs=1`; use
`--keep_workdirs` when preserving the case folders is useful for debugging.

Latest validation:

- July 16, 2026
- generated-file matrix: `43/43` cases completed with `--just_make --jobs=4`
- three full rolling matrices: `129/129` mission-owned verdicts passed
- full serial matrix: `43/43` mission-owned verdicts passed
- warp: `10`
