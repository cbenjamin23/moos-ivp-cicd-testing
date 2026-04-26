# H01-waypoint_behavior_motion

Patch-driven moving correctness harness for
[`/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/waypoint_behavior_missions/waypoint_behavior_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/waypoint_behavior_missions/waypoint_behavior_motion).

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

- `single_point_arrival_pass`
  Baseline case. The vehicle drives to one point and completes.
- `endflag_pass`
  Baseline geometry with explicit grading on the waypoint behavior's end flag.
- `multi_point_sequence_pass`
  The route contains three points. The vehicle should sequence through them and
  finish.
- `wptflag_pass`
  Multi-point companion case requiring `WPT_HIT=true`.
- `repeat_once_cycle_pass`
  Two-point route with `repeat=1`. The case requires a cycle flag and final
  completion.
- `repeat_forever_cycle_pass`
  Two-point route with `repeat=forever`. The case evaluates by timer and
  requires a cycle flag without final completion.
- `dynamic_points_update_pass`
  The behavior starts with `points=empty`, then receives a runtime
  `points=...` update and completes.
- `newpt_update_pass`
  The behavior starts empty, receives a runtime `newpt=...` update, and
  completes.
- `xpoints_update_pass`
  The route starts with two points and receives a same-length `xpoints=...`
  update while preserving traversal state.
- `capture_radius_large_pass`
  A large capture radius allows immediate completion without needing to drive
  to the exact point.
- `capture_line_absolute_pass`
  The behavior uses `capture_line=absolute`, exercising line-crossing arrival
  with capture and slip radii disabled.
- `slip_radius_pass`
  The behavior disables capture-line arrival and uses a tiny capture radius
  with a larger `slip_radius`, exercising nonmonotonic closest-approach
  arrival.
- `order_reverse_pass`
  A three-point route is traversed in `order=reverse`, exercising reverse route
  ordering while still completing cleanly.
- `currix_start_pass`
  A three-point route starts at `currix=1`, exercising nonzero starting index
  handling.
- `lead_to_start_pass`
  The first leg uses `lead_to_start=true`, exercising trackline lead from the
  launch position into the route.
- `lead_condition_pass`
  Lead behavior is gated on `ALLOW_LEAD=true`, exercising `lead_condition`
  registration and runtime evaluation.
- `end_spd_slowdown_pass`
  The route uses `end_spd=slow_dist=25,stop_dist=1`, exercising final approach
  speed tapering without preventing completion.
- `speed_alt_update_pass`
  The behavior starts at low nominal speed with `speed_alt` configured, then a
  runtime update enables alternate speed and the vehicle completes.
- `reset_on_idle_pass`
  The behavior idles briefly during transit with `reset_on_idle=true`, then
  redeploys and completes from a reset traversal state.
- `empty_start_then_update_pass`
  The behavior starts with `points=empty`, receives `point=start`, and
  completes against the current ownship position.
- `wptflag_on_start_pass`
  The behavior uses `wptflag_on_start=true`; the case evaluates early and
  requires `WPT_HIT=true` before route completion.
- `custom_status_vars_pass`
  The behavior posts custom status, index, and distance variables. The case
  evaluates in transit and requires the custom numeric outputs to be bridged to
  shoreside.
- `polygon_points_pass`
  The route is configured through `polygon=pts={...}` rather than `points=...`.
- `slow_speed_no_arrival_pass`
  A low-speed configuration is evaluated before arrival and should still be in
  progress.
- `no_points_timeout_fail`
  The behavior starts empty and receives no useful update. The case is expected
  to grade `fail`.
- `malformed_update_fail`
  The behavior starts empty and receives a malformed update. The case is
  expected to grade `fail` because no valid waypoint is accepted.
- `bad_xpoints_size_fail`
  The behavior starts with a two-point route and receives a one-point
  `xpoints` update. The update should be rejected, so the short timer verdict
  is expected to grade `fail`.

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=repeat_once_cycle_pass 10
./zlaunch.sh --case=repeat_forever_cycle_pass --gui 1
./zlaunch.sh --just_make 10
./zlaunch.sh --jobs=4 10
./zlaunch.sh --jobs=4 --port_base=9700 10
```

Results are appended to `results.txt` with the mission-owned `grade` and the
waypoint flags used for grading. Wave mode runs the full matrix in isolated
temporary mission copies. Each case gets its own MOOSDB and pShare port block
derived from `--port_base`, with pShare ports offset above the MOOSDB range.
The default is serial `--jobs=1`; use `--keep_workdirs` when preserving the
temporary case folders is useful for debugging.

Latest validation:

- April 24, 2026
- full matrix: `27/27` expected outcomes matched
- wave matrix: `27/27` expected outcomes matched with `--jobs=4`
- warp: `10`
