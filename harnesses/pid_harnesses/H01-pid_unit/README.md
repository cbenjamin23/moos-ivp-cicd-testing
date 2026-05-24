# H01-pid_unit

Patch-driven unit harness for `pMarinePIDV22`.

This harness reuses the `missions/pid_missions/pid_unit` one-community stem.
Each case patches the scripted input mail, PID configuration, or mission-owned
`pMissionEval` conditions while keeping the run headless and deterministic.
The cases are intentionally direct: they demonstrate the PID app's MOOS
input/output contract before introducing a vehicle simulator or helm behavior.

## Current Matrix

- `rudder_starboard_pass` - default stem; desired heading east from north must yield positive rudder and thrust near `40`.
- `rudder_port_pass` - desired heading west from north must yield negative rudder while maintaining thrust near `40`.
- `heading_wrap_pass` - desired `5` degrees from current `355` degrees must take the short positive wrap and produce rudder near `10`.
- `speed_factor_update_pass` - runtime `SPEED_FACTOR=10` must map desired speed `3` to thrust near `30` with near-zero rudder.
- `speed_pid_control_pass` - `speed_factor=0` must switch to speed PID control and reach the configured thrust cap.
- `max_thrust_limit_pass` - high speed-factor demand must be clipped at `maxthrust=60` while still producing a steering command.
- `depth_elevator_pass` - depth control must combine desired depth, current depth, and pitch into a non-saturated elevator command near `11.5`.
- `manual_override_zero_pass` - active manual override must release PID control and publish zero rudder/thrust commands.
- `stale_nav_pass` - stale nav mail must zero actuator output and report a stale-input reason.
- `nav_yaw_used_pass` - `NAV_YAW` must be converted to heading when `NAV_HEADING` is absent, yielding the expected rudder sign.
- `ignore_nav_yaw_pass` - `ignore_nav_yaw=true` must keep `NAV_HEADING` authoritative even when `NAV_YAW` is present.
- `heading_debug_saturation_pass` - heading debug output must be posted while rudder clips at `maxrudder=10`.
- `speed_zero_allstop_pass` - desired speed `0` must suppress thrust and rudder even with a heading error.
- `speed_negative_allstop_pass` - negative desired speed must be clipped to all-stop output.
- `output_suffix_alt_pass` - alternate `output_suffix=_ALT` must move actuator output to `DESIRED_*_ALT`.
- `manual_overide_alias_zero_pass` - legacy `MOOS_MANUAL_OVERIDE` must still zero output and drop PID control.
- `override_release_needs_fresh_mail_pass` - releasing override after old mail must keep output at zero until fresh desired/nav mail arrives.
- `override_recover_fresh_mail_pass` - fresh desired/nav mail after override release must restore rudder and thrust output.
- `stale_helm_pass` - stale desired heading/speed mail must zero actuator output and report `PID_STALE`.
- `stale_recover_pass` - fresh desired/nav mail after a stale interval must restore rudder and thrust output.
- `depth_negative_elevator_pass` - the opposite depth error sign must produce a negative elevator command.
- `depth_debug_pass` - `depth_debug=true` must publish depth debug text alongside nonzero elevator output.
- `max_elevator_limit_pass` - `maxelevator=5` must clip otherwise larger elevator output at the configured limit.
- `speed_debug_pass` - `speed_debug=true` must publish speed debug text alongside thrust output.
- `nav_heading_after_yaw_pass` - a later `NAV_HEADING` update must override an earlier `NAV_YAW`-derived heading.
- `simulation_mode_fail` - current-bug coverage for `simulation=true`, where actuator output stays zero despite a misleading control flag.
- `thrust_cap_runtime_fail` - current-bug coverage for runtime `PID_THRUST_CAP`, which computes a cap but still posts uncapped thrust.
- `heading_wrap_negative_pass` - desired `355` degrees from current `5` degrees must take the short negative wrap, with bounded negative rudder.
- `nav_heading_normalize_pass` - out-of-range `NAV_HEADING=-90` must be normalized before rudder calculation.
- `runtime_speed_factor_zero_pid_pass` - runtime `SPEED_FACTOR=0` must switch from factor mapping to PID speed control.
- `runtime_speed_factor_negative_pass` - runtime negative `SPEED_FACTOR` must be accepted and clip thrust to zero.
- `missing_desired_speed_zero_pass` - missing initial desired-speed mail must keep actuator output at the all-stop publication.
- `missing_nav_speed_zero_pass` - missing initial nav-speed mail must keep actuator output at the all-stop publication.
- `depth_missing_pitch_zero_pass` - depth control must wait for `NAV_PITCH` before publishing nonzero rudder, thrust, or elevator.

## Edge Audit

Covered in H01:

- Direct actuator sign, positive and negative wrap, heading normalization, clipping, suffix, debug, stale, override, yaw-source, and depth-control branches.
- Runtime mail updates that can be evaluated without a simulated vehicle, including speed-factor mode changes.
- Initial-mail gates for desired speed, nav speed, and depth-control pitch input.
- Current-bug coverage where the app's direct output contract appears wrong but the failure is still deterministic.

Deferred to H02:

- Closed-loop speed, turn, and depth response with `uSimMarineV22`.
- Interaction with `pHelmIvP`, behavior priority, and mission-level arrival or track quality.
- Long-duration integral behavior and oscillation/settling quality.
- Environmental timing/skew behavior that needs generated MOOS mail with nonlocal timestamps.

## Quality Criteria

Each H01 case should prove one app-level contract with a narrow `pMissionEval`
grade. Numeric pass bands are tight where output is deterministic, and wider
only when the exact timestamp of a repeated PID iteration can vary slightly.
Current-bug cases are still named with `_fail`, but the mission criteria own the
verdict. They grade `pass` only when the documented buggy evidence is observed:
`simulation_mode_fail` requires zero actuator output with `PID_HAS_CONTROL=true`,
and `thrust_cap_runtime_fail` requires uncapped thrust near `100`. If upstream
fixes the app, those cases should flip to `grade=fail` and prompt a case update.
The missing-mail cases intentionally grade zero actuator publications from the
app's all-stop path; their `PID_HAS_CONTROL` field reflects the app's current
override-release publication, not proof that a complete input set was accepted.

## Typical Runs

```bash
./zlaunch.sh --case=rudder_starboard_pass 10
./zlaunch.sh --case=depth_elevator_pass 10
./zlaunch.sh --jobs=4 --port_base=22000 10
```

The wrapper supports `--jobs` and `--port_base` so local grouped runs and
GitHub Actions dispatches can isolate each case's MOOSDB and pShare port block.
