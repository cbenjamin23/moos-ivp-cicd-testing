# H02-pid_motion

Patch-driven closed-loop motion harness for `pMarinePIDV22`.

This harness reuses the `missions/pid_missions/pid_motion` two-community stem.
Each case runs `pMarinePIDV22` with `pHelmIvP` and `uSimMarineV22`, then lets
shoreside `pMissionEval` grade bridged motion and actuator evidence. H02 is the
integration companion to H01: H01 proves direct PID publications; H02 proves
that those publications can drive simulated motion.

## Current Matrix

- `baseline_transit_pass` - default straight-line transit must arrive inside the target corridor with eastbound heading and bounded speed.
- `turn_recover_pass` - starting north of the eastbound leg must still recover to arrival inside a wider cross-track corridor.
- `hard_turn_recover_pass` - starting nearly opposite the desired leg must unwind and arrive with bounded speed and acceptable final heading.
- `rudder_bias_recover_pass` - simulated rudder bias must be corrected strongly enough to arrive despite a wider heading tolerance.
- `speed_pid_transit_pass` - `speed_factor=0` must still complete the transit through speed PID control and bounded final speed.
- `runtime_speed_factor_update_pass` - an underway `SPEED_FACTOR` update must restore actuator authority and still complete the transit.
- `depth_step_pass` - depth control must descend during transit and finish in the commanded depth band while holding eastbound heading.
- `override_release_pass` - startup manual override must release cleanly, reacquire helm/nav mail, and still satisfy the arrival gate.
- `low_rudder_authority_fail` - intentionally low `maxrudder` should pass when the vehicle remains far from the arrival corridor with low rudder authority.
- `low_thrust_authority_fail` - intentionally low `maxthrust` should pass when the vehicle remains short of the arrival corridor with low speed and low thrust.
- `low_elevator_authority_fail` - intentionally low `maxelevator` should pass when the vehicle moves east but remains below the aggressive depth-response band.
- `depth_control_disabled_fail` - a depth behavior with `depth_control=false` should pass when the vehicle moves east but depth remains near zero.
- `manual_override_fail` - holding manual override should pass when the vehicle remains stopped with zero thrust.

## Edge Audit

Covered in H02:

- Closed-loop heading correction through `uSimMarineV22`.
- Large heading-error recovery and rudder-bias disturbance recovery.
- Closed-loop speed-factor and speed-PID modes.
- Runtime `SPEED_FACTOR` mail updates while underway.
- Depth-control actuation through `BHV_ConstantDepth`, `pMarinePIDV22`, and simulated depth response.
- PID control reacquisition after manual override release.
- Expected-negative sentinels for insufficient rudder, thrust, and elevator authority, disabled depth control, and held manual override.

Deferred beyond this first H02 layer:

- Long-duration oscillation scoring and formal settling-time metrics.
- Disturbance or current modeling.
- Multi-leg waypoint courses and repeated depth setpoint sequences.

These deferred items are not missing basic `pMarinePIDV22` coverage. H01 owns
direct app-contract and source-branch checks, while H02 owns closed-loop
mission correctness with the helm and simulator in the loop. A future H03
should therefore be framed as a dynamics or quality harness, not another
correctness expansion: smaller case count, longer runs, and explicit evaluation
of settling, oscillation, repeated setpoint changes, multi-leg recovery, or
disturbance rejection. With the current `pMissionEval`-only structure, those
criteria should stay window/corridor based unless a separate metric source is
added.

Candidate H03 quality metrics include time-to-arrival or time-to-depth-band,
maximum cross-track error, heading overshoot and final settling band, depth
overshoot and steady-state error, actuator saturation duration, and oscillation
count or RMS error over a final evaluation window. Those metrics are valuable
for PID tuning regression, but they are intentionally separate from H02's
closed-loop correctness contract.

## Quality Criteria

Each case grades motion outcome through `pMissionEval`, not by parsing alogs in
the harness. Arrival-oriented cases evaluate on arrival or deadline and use
endpoint, corridor, heading, and bounded-output checks. Depth-oriented cases use
underway evaluation windows with tighter depth and heading bands.
Expected-negative cases are named with `_fail`, but now emit `grade=pass` when
their degraded-motion evidence is observed, using `expected=<reason>` plus the
supporting motion and actuator fields.

## Typical Runs

```bash
./zlaunch.sh --case=baseline_transit_pass 10
./zlaunch.sh --case=depth_step_pass 10
./zlaunch.sh --jobs=3 --port_base=26000 10
```

The wrapper supports `--jobs` and `--port_base` so local grouped runs and
GitHub Actions dispatches can isolate each case's MOOSDB and pShare port block.

Latest validation:

- May 20, 2026
- focused expected-negative cases:
  `low_rudder_authority_fail`, `low_thrust_authority_fail`,
  `low_elevator_authority_fail`, `depth_control_disabled_fail`,
  `manual_override_fail`
- grouped matrix: `13/13` cases passed with `--jobs=3 --port_base=23800`
- serial matrix: `13/13` cases passed with `--port_base=24300`
- warp: `10`
