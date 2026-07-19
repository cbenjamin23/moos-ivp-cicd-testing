# H01-pid_unit

Patch-driven unit harness for `pMarinePIDV22`.

This harness reuses the `missions/pid_missions/pid_unit` one-community stem.
Each case patches the scripted input mail, PID configuration, or mission-owned
`pMissionEval` conditions while keeping the run headless and deterministic.
The cases are intentionally direct: they demonstrate the PID app's MOOS
input/output contract before introducing a vehicle simulator or helm behavior.

## Current Matrix

- `rudder_starboard_pass`: Requests heading `90` from `NAV_HEADING=0` with desired speed `2`, testing the positive-rudder course correction; passes when `DESIRED_RUDDER_PID>1` and `39<=DESIRED_THRUST_PID<=41`.
- `rudder_port_pass`: Requests heading `270` from `NAV_HEADING=0` with desired speed `2`, testing the negative-rudder course correction; passes when `DESIRED_RUDDER_PID<-1` and `39<=DESIRED_THRUST_PID<=41`.
- `heading_wrap_pass`: Requests heading `5` from `NAV_HEADING=355`, testing the positive 10-degree path across north instead of a 350-degree turn; passes when `9.5<=DESIRED_RUDDER_PID<=10.5` and `39<=DESIRED_THRUST_PID<=41`.
- `speed_factor_update_pass`: Posts `SPEED_FACTOR=10` before requesting speed `3` with no heading error, testing runtime factor-based thrust scaling; passes when rudder remains within `[-0.1,0.1]` and thrust is in `[29,31]`.
- `speed_pid_control_pass`: Configures `speed_factor=0` with desired speed `2` and `NAV_SPEED=0`, selecting the speed PID instead of direct factor scaling; passes when positive rudder is produced and thrust reaches the configured `[99,100]` cap band.
- `max_thrust_limit_pass`: Configures `speed_factor=50` and `maxthrust=60` for a speed demand that would otherwise produce thrust `100`, testing the configured thrust limit; passes when rudder is positive and thrust is in `[59,60]`.
- `depth_elevator_pass`: Enables depth control with desired depth `6`, `NAV_DEPTH=5`, and `NAV_PITCH=0`, testing the depth-to-pitch and pitch-to-elevator cascade; passes when rudder is positive, thrust is in `[39,41]`, and elevator is in `[11.3,11.7]`.
- `manual_override_zero_pass`: Establishes nonzero PID inputs, then posts `MOOS_MANUAL_OVERRIDE=true`, testing the normal override input; passes when rudder and thrust are each within `[-0.1,0.1]` and `PID_HAS_CONTROL=false`.
- `stale_nav_pass`: Sets the nav tardy threshold to one second, keeps desired heading, desired speed, and nav heading fresh, and lets only `NAV_SPEED` expire; passes after first proving `DESIRED_RUDDER_PID>1` and thrust in `[39,41]`, then requiring both outputs within `[-0.1,0.1]` and a `PID_STALE` publication.
- `nav_yaw_used_pass`: Omits `NAV_HEADING` and posts `NAV_YAW=-1.5708` with desired heading `0`, testing conversion from yaw radians to heading; passes when rudder is negative and thrust is in `[39,41]`.
- `ignore_nav_yaw_pass`: Posts conflicting `NAV_YAW=-1.5708` and `NAV_HEADING=0` with `ignore_nav_yaw=true`, testing that heading mail remains authoritative; passes when rudder is positive and thrust is in `[39,41]`.
- `heading_debug_saturation_pass`: Enables `heading_debug`, requests heading `180` from `0`, and sets `maxrudder=10`; passes when `PID_HDG_DEBUG` publishes while rudder is saturated in `[-10,-9]` and thrust is in `[39,41]`, with the diagnostic's desired, current, error, and rudder fields retained in the result.
- `speed_zero_allstop_pass`: Requests `DESIRED_SPEED=0` while retaining a 90-degree heading error, testing the zero-speed all-stop branch; passes when rudder and thrust are each within `[-0.1,0.1]` and `PID_HAS_CONTROL=true`.
- `speed_negative_allstop_pass`: Requests `DESIRED_SPEED=-1` while retaining a 90-degree heading error, testing clipping of a negative speed request to all-stop; passes when rudder and thrust are each within `[-0.1,0.1]` and `PID_HAS_CONTROL=true`.
- `output_suffix_alt_pass`: Configures `output_suffix=_ALT`, requiring `DESIRED_RUDDER_ALT>1` and `39<=DESIRED_THRUST_ALT<=41` while failing on any unsuffixed `DESIRED_RUDDER` or `DESIRED_THRUST` publication.
- `manual_overide_alias_zero_pass`: Establishes nonzero PID inputs, then posts the legacy misspelled `MOOS_MANUAL_OVERIDE=true`, testing compatibility with that alias; passes when rudder and thrust are each within `[-0.1,0.1]` and `PID_HAS_CONTROL=false`.
- `override_release_needs_fresh_mail_pass`: First proves starboard control with rudder above `1` and thrust in `[39,41]`, then posts a port command while override is active and requires zero outputs with `PID_HAS_CONTROL=false`; after release without fresh mail, it passes only while outputs remain zero even though `PID_HAS_CONTROL=true`.
- `override_recover_fresh_mail_pass`: First proves port control with rudder below `-1` and thrust in `[19,21]`, then requires zero outputs with `PID_HAS_CONTROL=false` during override; after release and a fresh starboard command, it passes on rudder above `1`, thrust in `[39,41]`, and `PID_HAS_CONTROL=true`.
- `stale_helm_pass`: Sets the helm tardy threshold to one second, leaves desired heading and speed at their one-second values, and refreshes nav mail at five seconds, isolating stale helm input; passes when rudder and thrust are within `[-0.1,0.1]` and `PID_STALE` is present.
- `stale_recover_pass`: First proves port control with rudder below `-1` and thrust in `[19,21]`, then requires a `PID_STALE` publication and zero outputs after every helm/nav input expires; after all four inputs are refreshed with a starboard command, it passes on rudder above `1`, thrust in `[39,41]`, and `PID_HAS_CONTROL=true`.
- `depth_negative_elevator_pass`: Enables depth control with desired depth `5`, `NAV_DEPTH=10`, and zero pitch, testing the opposite depth-error sign; passes when rudder is positive, thrust is in `[39,41]`, and `DESIRED_ELEVATOR_PID<-1`.
- `depth_debug_pass`: Enables `depth_debug` with desired depth `10`, `NAV_DEPTH=5`, and zero pitch, testing depth diagnostic publication during elevator control; passes when `DESIRED_ELEVATOR_PID>1` and `PID_DEP_DEBUG` is present.
- `max_elevator_limit_pass`: Enables depth control with a five-meter depth error and `maxelevator=5`, testing elevator saturation; passes when `DESIRED_ELEVATOR_PID` is in `[4.9,5.0]`.
- `speed_debug_pass`: Enables `speed_debug` with desired speed `2` and `NAV_SPEED=0`, testing speed diagnostic publication during factor-based thrust control; passes when thrust is in `[39,41]` and `PID_SPD_DEBUG` is present.
- `nav_heading_after_yaw_pass`: Posts `NAV_YAW=-1.5708`, then posts `NAV_HEADING=0` half a second later for desired heading `90`, testing that later heading mail replaces the yaw-derived value; passes when rudder is positive and thrust is in `[39,41]`.
- `simulation_mode_fail`: Configures `simulation=true` with otherwise valid steering and speed inputs, preserving a known bug in which the app claims control without actuator output; the harness passes only while rudder and thrust remain within `[-0.1,0.1]` and `PID_HAS_CONTROL=true`.
- `thrust_cap_runtime_fail`: Posts `PID_THRUST_CAP=50` before a factor-based demand of `100`, preserving a known bug in which the runtime cap is ignored; the harness passes only while rudder is positive and thrust remains in `[99,100]`.
- `heading_wrap_negative_pass`: Requests heading `355` from `NAV_HEADING=5`, testing the negative 10-degree path across north; passes when rudder is between `-20` and `-1` and thrust is in `[39,41]`.
- `nav_heading_normalize_pass`: Posts `NAV_HEADING=-90` with desired heading `0`, testing normalization of an out-of-range heading before error calculation; passes when rudder is between `80` and `100` and thrust is in `[39,41]`.
- `runtime_speed_factor_zero_pid_pass`: Posts `SPEED_FACTOR=0` before requesting speed `2` from `NAV_SPEED=0`, testing the runtime switch from factor mapping to speed PID control; passes when rudder is positive and thrust is in `[99,100]`.
- `runtime_speed_factor_negative_pass`: Posts `SPEED_FACTOR=-10` before requesting speed `2`, exercising the accepted negative-factor path and its zero-clipped output; passes when rudder and thrust are each within `[-0.1,0.1]` and `PID_HAS_CONTROL=true`.
- `missing_desired_speed_zero_pass`: Supplies desired heading and both nav inputs but never posts `DESIRED_SPEED`, testing the initial desired-speed gate; passes when rudder and thrust remain within `[-0.1,0.1]` and `PID_HAS_CONTROL=true`.
- `missing_nav_speed_zero_pass`: Supplies desired heading, desired speed, and nav heading but never posts `NAV_SPEED`, testing the initial nav-speed gate; passes when rudder and thrust remain within `[-0.1,0.1]` and `PID_HAS_CONTROL=true`.
- `depth_missing_pitch_zero_pass`: Enables depth control and supplies every normal input except `NAV_PITCH`, testing the depth-mode pitch gate; passes when rudder, thrust, and elevator remain within `[-0.1,0.1]` and `PID_HAS_CONTROL=true`.

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
scheduler stops launching new cases and retains the run root for diagnosis.

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
