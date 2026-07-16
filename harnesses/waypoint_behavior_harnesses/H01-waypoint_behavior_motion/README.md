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
  The route contains three non-collinear points. The vehicle should visibly
  turn through the sequence and finish.
- `wptflag_pass`
  Multi-point companion case requiring `WPT_HIT=true`.
- `repeat_once_cycle_pass`
  Two-point route with `repeat=1`. The case requires a cycle flag and final
  completion.
- `repeat_forever_cycle_pass`
  Vertical two-point route with `repeat=forever`. The case evaluates by timer
  and requires a cycle flag without final completion.
- `dynamic_points_update_pass`
  The behavior starts with `points=empty`, then receives a delayed runtime
  two-leg `points=...` update and completes.
- `newpt_update_pass`
  The behavior starts empty, receives a runtime `newpt=...` update, and
  completes.
- `xpoints_update_pass`
  The route starts as a horizontal two-point line, then receives a same-length
  `xpoints=...` update that bends the second leg while preserving traversal
  state.
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
  The route is configured through `polygon=pts={...}` rather than a literal points list.
- `ipf_type_roc_pass`
  The behavior uses `ipf_type=roc` and should still complete the route with the
  rate-of-closure objective function.
- `ipf_type_zaic_spd_pass`
  The behavior uses `ipf_type=zaic_spd` and should still complete the route
  with the alternate speed ZAIC objective.
- `greedy_tour_pass`
  A multi-point route enables `greedy_tour=true`; the route should be reordered
  from ownship position and still complete cleanly.
- `cycle_index_output_pass`
  A repeat-once route must publish `CYCLE_INDEX>=1` in addition to completing
  and posting the cycle flag.
- `efficiency_measure_pass`
  A multi-leg route enables `efficiency_measure=all`; the case requires
  positive distance and time efficiency outputs after completion.
- `slow_speed_no_arrival_pass`
  A low-speed configuration is evaluated before arrival and should still be in
  progress.
- `no_points_timeout_fail`
  The behavior starts empty and receives no useful update, so the mission should
  pass when the timer verdict observes no waypoint completion.
- `malformed_update_fail`
  The behavior starts empty and receives a malformed update. The mission should
  pass when no valid waypoint is accepted and no waypoint completion occurs.
- `bad_xpoints_size_fail`
  The behavior starts with a two-point route and receives a one-point
  `xpoints` update. The update should be rejected, and the short timer verdict
  should pass when completion remains absent.
- `bad_speed_fail`
  Launch-time `speed=-1` should be rejected by `BHV_Waypoint`, putting the helm
  into malconfig and producing an expected pass verdict.
- `bad_capture_line_fail`
  Launch-time `capture_line=diagonal` should be rejected and put the helm into
  malconfig.
- `bad_capture_radius_fail`
  Launch-time `capture_radius=-1` should be rejected and put the helm into
  malconfig.
- `bad_slip_radius_fail`
  Launch-time `slip_radius=-1` should be rejected and put the helm into
  malconfig.
- `bad_order_fail`
  Launch-time `order=sideways` should be rejected and put the helm into
  malconfig.
- `bad_repeat_fail`
  Launch-time `repeat=-1` should be rejected and put the helm into malconfig.
- `bad_lead_damper_fail`
  Launch-time `lead_damper=0` should be rejected because the source requires a
  positive damper value.
- `bad_lead_condition_fail`
  Launch-time malformed `lead_condition` should be rejected by the logic
  condition parser.
- `bad_end_spd_fail`
  Launch-time `end_spd=slow_dist=10,stop_dist=20` should be rejected because
  the stop distance exceeds the slow-down distance.
- `bad_patience_fail`
  Launch-time `patience=0` should be rejected because the course/speed ZAIC
  ratio must stay in the accepted 1..99 range.
- `bad_efficiency_measure_fail`
  Launch-time `efficiency_measure=summary` should be rejected because only
  `all` and `off` are accepted.

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
