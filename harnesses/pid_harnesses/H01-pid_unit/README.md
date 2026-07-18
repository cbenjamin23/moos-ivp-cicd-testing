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
./zlaunch.sh --log=full --case=rudder_starboard_pass 10
./zlaunch.sh --case=depth_elevator_pass 10
./zlaunch.sh --jobs=2 --port_base=22000 10
./zlaunch.sh --just_make --keep_workdirs 10
```

## Launcher Contract

Every selected case runs from its own copy of the `pid_unit` stem, including
single-case and serial runs. The harness applies at most one shoreside
`nspatch` overlay inside that copy and calls the copy's `zlaunch.sh`; it never
patches the source stem in place.

Logging defaults to `minimal`; `--log=full` restores the previous one-community
logger configuration before the case overlay.

The launcher requires Bash 5.1 or newer for rolling `--jobs` scheduling. On
macOS it safely re-executes a suitable Homebrew or Linuxbrew Bash when one is
available. It uses stride-30 port blocks with the one-community MOOSDB at `+0`
and a reserved shoreside pShare slot at `+15`. This mission does not launch a
pShare process; the reserved slot is forwarded only to keep launcher and port
allocation contracts consistent across harnesses.

`pMissionEval` is the sole owner of the live verdict. The shell requires
exactly one mission row with exactly one `grade=pass|fail` field, adds the
selected `case=`, and writes exactly one aggregate row per selected case in
the declared order. Ordinary case failures do not stop later cases. Scoped
cleanup is limited to each case root; if cleanup itself cannot be proved, the
scheduler stops launching new cases and retains the run root and safety lock
for diagnosis.

Use `--keep_workdirs` to inspect generated targets, applied sidecars, case
logs, and intermediate result rows under `.harness_runs`. See
[`NSPATCH.md`](NSPATCH.md) for the exact patch contract.

## Harness-skill Migration Validation

Validation on July 14, 2026 preserved all thirty-four case tokens in their
declared order, the unpatched baseline, all thirty-three shoreside overlay
mappings, every hard-coded `mission_mod`, every evaluator condition, and every
report column. No evaluator block, application, grading variable, PID setting,
scripted input, or case-quality assertion changed.

Untouched legacy measurements at warp 10 and `--max_time=35` were:

- three two-worker batch-wave matrices: 102/102 rows passing in 110.77, 108.98,
  and 108.92 seconds, for a 109.56-second mean
- one shared-stem serial matrix: 34/34 passing in 170.52 seconds

Migrated validation included both skill static checkers, Bash syntax,
ShellCheck, Bash 3.2 rejection and Homebrew re-execution, invalid argument and
port probes, a retained full generation matrix, a direct stem pass, isolated
nominal and both expected-negative cases, a complete serial matrix, three
complete rolling matrices, and missing-file, missing-row, duplicate-row,
invalid-grade, duplicate-grade, nonzero-launch, preparation, timeout, and
teardown failure paths.

Complete migrated measurements were:

- isolated serial: 34/34 passing in 194.65 seconds
- rolling `--jobs=2`: 102/102 rows passing in 101.22, 96.81, and 101.26
  seconds, for a 99.76-second mean
- final checkout-state rolling matrix: 34/34 passing in 102.86 seconds from a
  port range verified clear on both TCP and UDP before and after the run

The rolling mean is 9.79 seconds, about 8.9 percent, faster than the matched
legacy mean. Serial is 24.13 seconds, about 14.2 percent, slower than the one
legacy sample because it now pays the same copy and scoped-cleanup cost for
every case. The retained rolling matrix had thirty-four targets, thirty-three
intended sidecars, thirty-four original `.alog` files, exact selected-order
results, no runtime error flags, and no pShare process blocks. Every assigned
TCP and UDP slot was clear afterward.

The migration deliberately leaves existing coverage choices for later work:
the heading-debug case reports but does not grade its debug string; the stale-
nav case reports its stale message while grading zero outputs; some temporal
or absence cases grade the final value rather than full publication history;
and the alternate-suffix case does not separately assert that unsuffixed
outputs are absent.
