# H01-fixedturn_behavior_motion

Patch-driven moving correctness harness for
`missions/fixedturn_behavior_missions/fixedturn_behavior_motion`.

This harness focuses on `BHV_FixedTurn` as the behavior under test. The stem
mission first establishes a short moving leg, then activates the fixed-turn
behavior and grades mission-owned evidence:

- `FT_DONE`
- final `NAV_HEADING` for directional 90-degree cases
- `TURNING_TIME` and `TURNING_DIST`
- scheduled-turn flags such as `TURN_ONE` and `TURN_TWO`
- clean behavior state through `BHV_ERROR_SEEN=false`

## Current Matrix

- `starboard_90_pass`
  Baseline case. The vehicle runs the approach leg, activates `BHV_FixedTurn`,
  turns starboard by about 90 degrees, and must finish with `FT_DONE=true`,
  heading between `105` and `170`, positive turn time/distance, and no behavior
  error.
- `port_90_pass`
  Mirrored 90-degree case. The same approach leg activates a port turn, and the
  case must finish with `FT_DONE=true`, final heading above `285`, positive
  turn time/distance, and no behavior error.
- `starboard_360_pass`
  Long-turn starboard case. The behavior uses a stronger `mod_hdg` to complete
  a near-full starboard circle, and the case requires `FT_DONE=true`,
  `TURNING_TIME > 15`, `TURNING_DIST > 25`, and a populated `FT_REPORT` with
  radius data.
- `port_360_pass`
  Long-turn mirror case. The vehicle completes a near-full port circle with the
  same timing and distance thresholds as the starboard long-turn case, proving
  the port direction is covered separately.
- `speed_auto_pass`
  Automatic speed case. The fixed-turn behavior uses `speed=auto`, so
  `FT_REPORT` should show the inherited stem speed while the mission
  still completes the baseline starboard 90-degree success criteria.
- `fixed_speed_pass`
  Explicit speed case. The fixed-turn behavior uses `speed=1.3`, which is
  distinct from both the default and `speed=auto` cases, and must still complete
  the baseline starboard 90-degree success criteria.
- `turn_delay_pass`
  Delay case. The behavior holds its current heading briefly before turning,
  then must complete normally with `FT_DONE=true`, positive turn time/distance,
  and no behavior error.
- `timeout_complete_pass`
  Timeout completion case. The behavior is configured with a long turn and a
  short `timeout=4`; success requires timeout-driven completion with
  `FT_DONE=true`, `TURNING_TIME` between `3` and `8`, positive distance, and no
  behavior error.
- `turn_spec_sequence_pass`
  Scheduled-turn case. The behavior runs two `turn_spec` entries, and success
  requires both `TURN_ONE=true` and `TURN_TWO=true`, the final `FT_DONE=true`
  flag from the second scheduled turn, positive turn time/distance, and no
  behavior error.
- `turn_spec_fallback_pass`
  Scheduled-turn fallback case. A `turn_spec` entry leaves speed, fixed turn,
  and `mod_hdg` unset so the behavior must fall back to the static behavior
  parameters, post `TURN_ONE=true`, complete the baseline starboard turn, and
  avoid behavior errors.
- `turn_spec_key_update_pass`
  Runtime keyed-update case. The first scheduled turn is replaced by a keyed
  runtime update before activation, and success requires the updated port turn
  to complete with `FT_DONE=true`, `TURN_ONE=true`, `TURN_TWO=true`, final
  heading above `285`, positive turn time/distance, and no behavior error. The
  original `TURN_ONE` flag also remains true because keyed `turn_spec` updates
  append flags onto the keyed entry.
- `turn_spec_bad_update_recover_pass`
  Runtime recovery case. The vehicle receives one malformed keyed update and
  then a valid replacement before activation; success requires the valid
  replacement to run, both recovery flags to be posted, final heading between
  `70` and `130`, positive turn time/distance, and no behavior error.
- `turn_spec_clear_pass`
  Runtime clear case. A queued scheduled turn is cleared before activation so
  the behavior must ignore `TURN_ONE`, fall back to the static starboard turn,
  complete with `FT_DONE=true`, and keep behavior state clean.
- `turn_spec_timeout_pass`
  Scheduled timeout case. A `turn_spec` entry defines its own long turn and
  short timeout; success requires the scheduled turn to complete by timeout,
  post `TURN_ONE=true`, finish with `FT_DONE=true`, and report a short positive
  turn time/distance.
- `zero_fix_turn_pass`
  Zero-turn case. The behavior is configured with `fix_turn=0`; success
  requires immediate completion with `FT_DONE=true`, zero turn time/distance,
  and no behavior error.
- `runtime_static_update_pass`
  Generic runtime-update case. The stem behavior starts as a starboard turn,
  then receives `FTURN_UPDATE` messages changing `turn_dir` and `speed` before
  activation; success requires the updated port turn to complete with
  `FT_DONE=true`, final heading above `285`, positive turn time/distance, and
  no behavior error.
- `turn_spec_clear_add_pass`
  Runtime clear-and-add case. A queued scheduled turn is replaced by a single
  `turn_spec=clear,...` runtime update; success requires `TURN_ONE=false`,
  `TURN_TWO=true`, `FT_DONE=true`, positive turn time/distance, and no behavior
  error.
- `turn_spec_aliases_pass`
  Scheduled-turn alias case. The scheduled turn uses `fhdg` and `tdir` aliases
  instead of `fix` and `turn`; success requires the alias-defined starboard
  turn to post `TURN_ONE=true`, complete with `FT_DONE=true`, and avoid
  behavior errors.
- `bad_fix_turn_fail`
  Invalid fixed-turn case. The behavior patch sets an invalid negative
  `fix_turn`; the case passes when the mission observes the expected rejection
  evidence: `FT_DONE=false`, timeout reached, no vehicle motion, unchanged
  heading, and no behavior runtime error.
- `bad_stale_nav_thresh_fail`
  Invalid stale-navigation threshold case. The behavior patch sets an invalid
  `stale_nav_thresh`; the case passes when the behavior does not run to
  completion and the mission observes timeout, no motion, unchanged heading,
  and `FT_DONE=false`.
- `bad_turn_dir_fail`
  Invalid turn-direction case. The behavior patch sets an unsupported
  `turn_dir`; the case passes when the behavior does not complete and the
  mission observes timeout, no motion, unchanged heading, and `FT_DONE=false`.
- `bad_speed_fail`
  Invalid speed case. The behavior patch sets a negative fixed speed; the
  case passes when the behavior rejects the configuration, never posts
  `FT_DONE`, and the vehicle remains stationary until the evaluation timeout.
- `bad_turn_spec_fail`
  Invalid scheduled-turn case. The behavior patch provides a malformed
  `turn_spec`; the case passes when no valid turn completes before the
  evaluation timeout and the vehicle remains at the initial heading and speed.
- `bad_schedule_repeat_fail`
  Invalid schedule-repeat case. The behavior patch sets `schedule_repeat` to a
  non-boolean token; the case passes when the behavior rejects the
  configuration, never posts `FT_DONE`, and the vehicle remains stationary
  until the evaluation timeout.

The stem also posts viewer-only context markers: a white approach-leg segment
and an orange fixed-turn start point. These markers do not participate in
grading. Near-360-degree cases additionally produce the `BHV_FixedTurn`
radial `VIEW_SEGLIST` report after completion; 90-degree turns do not produce
that built-in radial report because the behavior has no opposite-heading
samples from a quarter turn.

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=port_90_pass 10
./zlaunch.sh --jobs=4 --port_base=29400 --port_stride=12 10
./zlaunch.sh --just_make --jobs=4 --port_base=29400 --port_stride=12 10
```

The default `--port_base` is `15000` to avoid the repository's older 9000-range
defaults during active development. Grouped runs use 30-port case blocks by
default so each live case gets isolated MOOSDB and pShare ports. Use
`--port_stride=12` when a validation window must keep the full 24-case matrix
inside a 300-port range; each case uses only the `+0`, `+1`, `+10`, and `+11`
offsets within its block.

Latest validation:

- May 20, 2026
- focused expected-negative cases: `6/6` mission `grade=pass`
- wave matrix: `24/24` mission `grade=pass` with `--jobs=4 --port_base=29400 --port_stride=12`
- port range used by grouped run: `29400-29687`
- warp: `10`
