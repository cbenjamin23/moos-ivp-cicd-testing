# H01-waypoint_behavior_motion

Patch-driven moving-correctness harness for
[`missions/waypoint_behavior_missions/waypoint_behavior_motion`](../../../missions/waypoint_behavior_missions/waypoint_behavior_motion).
The live matrix keeps MOOS update, helm-integration, and vehicle-motion
contracts; deterministic configuration math is checked directly by
`BHVWaypointTest`.

Logging is minimal by default. Use `--log=full --case=NAME --keep_workdirs`
when a failure needs mission logs.

## Cases

- `single_point_arrival_pass` Drives from `(-18,-121)` to `(42,-121)` and requires waypoint, cycle, and both configured end flags when the route completes.
- `multi_point_sequence_pass` Traverses `(-3,-121):(18,-103):(42,-121)` and requires all three waypoint/cycle/end publications with no behavior error.
- `repeat_once_cycle_pass` Traverses `(4,-121):(26,-121)` twice with `repeat=1`; completion must report `CYCLE_INDEX=2`, not the default single cycle.
- `repeat_forever_cycle_pass` Uses `repeat=forever` on `(22,-121):(22,-99)`; at 38 seconds it must have cycled while `WPT_DONE` and `WAYPOINT_END` remain false.
- `dynamic_points_update_pass` Starts with `points=empty`, posts `points=12,-106:42,-121` at five seconds, and requires that runtime-installed route to complete.
- `newpt_update_pass` Starts with `points=empty`, appends `newpt=x=42,y=-121` at two seconds, and requires the new single-point route to complete.
- `xpoints_update_pass` Replaces `(12,-121):(42,-121)` with the same-length route `(12,-121):(42,-103)` at 11 seconds; completion must occur with `-110<ABE_NAV_Y<-96`, distinguishing the replacement endpoint from the original one.
- `empty_start_then_update_pass` Starts with `points=empty`, posts `point=start`, and requires immediate completion at the captured ownship position.
- `wptflag_on_start_pass` Sets `wptflag_on_start=true`; at three seconds `WPT_HIT` must be true while route completion remains false.
- `polygon_points_pass` Supplies `polygon=pts={-3,-121:18,-121:42,-121}` and requires the exported three-point route to complete normally.
- `slow_speed_no_arrival_pass` Commands `speed=0.35` toward a point 60 meters ahead; at ten seconds it requires `50<WPT_DIST_TO_NEXT<60`, `-18<ABE_NAV_X<-10`, and `0.2<ABE_NAV_SPEED<0.5`, proving bounded progress rather than inactivity.
- `no_points_timeout_fail` Leaves `points=empty` without an update and requires no waypoint, cycle, or end publication at eight seconds.
- `malformed_update_fail` Posts `points=not_a_point`; the live update result must be `param_error` before the deadline, while the empty route remains incomplete.
- `bad_xpoints_size_fail` Posts one replacement point against an active two-point route; the live update must return `param_error`, index zero and a distance above 70 must remain, and direct CTest requires the original two points to be unchanged.
- `bad_speed_fail` Sets `speed=-1`; the armed evaluator requires literal helm state `MALCONFIG` before its deadline, and direct CTest requires rejection of this exact value.
- `bad_capture_line_fail` Sets `capture_line=diagonal`; the armed evaluator requires literal `MALCONFIG`, and direct CTest requires rejection of this exact mode.
- `bad_capture_radius_fail` Sets `capture_radius=-1`; the armed evaluator requires literal `MALCONFIG`, and direct CTest requires rejection of this exact radius.
- `bad_slip_radius_fail` Sets `slip_radius=-1`; the armed evaluator requires literal `MALCONFIG`, and direct CTest requires rejection of this exact radius.
- `bad_order_fail` Sets `order=sideways`; the armed evaluator requires literal `MALCONFIG`, and direct CTest requires rejection of this exact order.
- `bad_repeat_fail` Sets `repeat=-1`; the armed evaluator requires literal `MALCONFIG`, and direct CTest requires rejection of this exact count.
- `bad_lead_damper_fail` Sets `lead_damper=0`; the armed evaluator requires literal `MALCONFIG`, and direct CTest requires rejection of this exact nonpositive damper.
- `bad_lead_condition_fail` Sets `lead_condition=ALLOW_LEAD >`; the armed evaluator requires literal `MALCONFIG`, and direct CTest requires rejection of this exact malformed expression.
- `bad_end_spd_fail` Sets `end_spd=slow_dist=10,stop_dist=20`; the armed evaluator requires literal `MALCONFIG`, and direct CTest requires rejection because stop distance exceeds slowdown distance.
- `bad_patience_fail` Sets `patience=0`; the armed evaluator requires literal `MALCONFIG`, and direct CTest requires rejection outside the accepted `[1,99]` ratio.
- `bad_efficiency_measure_fail` Sets `efficiency_measure=summary`; the armed evaluator requires literal `MALCONFIG`, and direct CTest requires rejection outside `all|off`.

## Direct CTest coverage

`BHVWaypointTest` fixes ownship state and inspects the behavior or its
`WaypointEngine` directly. It now requires:

- distinct capture-radius, absolute-line, and slip-radius advancement, with
  exact capture counters;
- reverse order, `currix=1`, and greedy-tour selection of their exact first
  route point;
- exact track points for `lead_to_start` and a false-to-true lead condition;
- normal versus alternate speed objectives, terminal taper and stop branches,
  and non-null `roc` and `zaic_spd` objectives;
- reset-on-idle to rewind index one to zero while the default retains index
  one;
- exact suffixed status, waypoint index, cycle index, and distance values;
- distance efficiency `1.0` and time efficiency `5/6` for a deterministic
  ten-meter, six-second traversal; and
- rejection of both malformed route updates and all eleven malformed live
  startup values.

The 18 removed missions either duplicated an existing case or graded only
eventual route completion. Their claimed capture mode, route order, lead point,
speed objective, reset state, suffix output, or efficiency math is now required
at the deterministic layer where ignoring the setting fails immediately.

## Running

From this harness directory:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=xpoints_update_pass 10
./zlaunch.sh --case=malformed_update_fail 10
./zlaunch.sh --jobs=4 --port_base=36000 10
```

The matrix contains 25 cases. Each case uses an isolated mission copy and
dedicated MOOSDB/pShare port block. Do not overlap this harness with another
MOOS harness using the same ports.

## Latest validation

- July 20, 2026
- generated-file matrix: `25/25` completed
- minimal-logging matrix: `25/25` passed
- full-logging matrix: `25/25` passed
- focused `BHVWaypointTest`: `13/13` passed
- focused `behaviors-marine` CTest family: `234/234` passed
- fourteen deliberate mutations failed at their intended evidence: capture
  radius, route order, lead-to-start, alternate speed, idle reset, suffix,
  efficiency speed, direct malformed-speed acceptance, xpoints endpoint,
  repeat count, zero-speed motion, malformed points update, valid-size xpoints
  update, and repaired startup speed
- warp: `10`
