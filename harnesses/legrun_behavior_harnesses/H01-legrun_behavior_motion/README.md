# H01-legrun_behavior_motion

Patch-driven moving correctness harness for
[`missions/legrun_behavior_missions/legrun_behavior_motion`](../../../missions/legrun_behavior_missions/legrun_behavior_motion).

This harness focuses on `BHV_LegRun` as the behavior under test. The stem runs a
single simulated vehicle on a two-leg pattern and grades on behavior-owned mode,
leg, turn, mid-leg, mid-turn, completion, and warning/error signals.

## Current Matrix

- `baseline_cycle_pass`
  Runs the stock 60-meter leg centered at `(7,-57)` on heading 64.64 degrees,
  with 8-meter alternating turns at 1.6 m/s; passes when leg 1, turn 1, leg 2,
  mid-leg, mid-turn, and completion flags fire without a behavior error.
- `finite_complete_pass`
  Sets `perpetual=false` on the stock pattern to exercise finite completion;
  passes when leg 1, turn 1, leg 2, and `LR_DONE` fire without a behavior error.
- `colon_leg_format_pass`
  Supplies the stock geometry as `leg=7:-57:64.64:60` instead of named
  fields, exercising the colon parser; passes on the full baseline flag set and
  no behavior error.
- `vertex_points_pass`
  defines the leg by `p1=-20.12,-69.82` and `p2=34.12,-44.18` instead of
  center, heading, and length; passes on the full baseline flag set and no
  behavior error.
- `init_close_turn_pass`
  Sets `init_leg_mode=close_turn` to choose the initial leg from the nearer
  turn; passes on the full baseline leg, turn, midpoint, completion, and
  no-error evidence.
- `init_far_turn_pass`
  Sets `init_leg_mode=far_turn` to begin on the opposite leg; passes when leg 2
  and `LR_DONE` fire without a behavior error.
- `turn_rad_alias_pass`
  Configures both turns through shared aliases `turn_bias=100`, `turn_dir=port`,
  `turn_rad=8`, and `turn_ext=4`; passes on the baseline progression flags
  through turn 1 and leg 2, with no behavior error.
- `turn_bias_ext_gap_pass`
  Sets shared `turn_bias=85`, `turn_rad=9`, `turn_ext=8`, and
  `turn_pt_gap=18` to exercise shaped turn generation; passes on the baseline
  progression flags through turn 1 and leg 2, with no behavior error.
- `individual_turn_params_pass`
  Configures asymmetric turns with radii 7 and 10, biases 80 and 95,
  extensions 3 and 6, and opposite directions; passes on the baseline
  progression flags through turn 1 and leg 2, with no behavior error.
- `turn_rad_min_clip_pass`
  Sets `turn_rad_min=9` before shared `turn_rad=2`, exercising minimum-radius
  clipping; passes on the baseline progression flags through turn 1 and leg 2,
  with no behavior error.
- `capture_radius_alias_pass`
  Replaces `capture_radius=5` with the equivalent `radius=5` alias; passes on
  the full baseline progression, midpoint, completion, and no-error evidence.
- `capture_line_absolute_pass`
  Sets `capture_line=absolute` instead of `true`; passes on the full baseline
  progression, midpoint, completion, and no-error evidence.
- `no_capture_line_pass`
  Disables capture lines and uses `capture_radius=6` and `slip_radius=16` for
  radius-only endpoint capture; passes on the full baseline progression,
  midpoint, completion, and no-error evidence.
- `leg_speed_schedule_pass`
  Configures the finite schedule `leg_spds=1.2,1.0,0.8,1.4`; passes on the
  ordinary progression, midpoint, completion, and no-error evidence, without
  grading the scheduled speeds.
- `leg_speed_onturn_pass`
  Configures repeating speeds `1.1,0.9,1.3` with `leg_spds_onturn=true` so
  schedule entries also apply during turns; passes on ordinary progression,
  midpoint, completion, and no-error evidence.
- `leg_speed_count_repeat_pass`
  Configures repeating count-expanded schedule `leg_spds=2:1.0,1.4`;
  passes on ordinary progression, midpoint, completion, and no-error evidence,
  without grading the expanded speed sequence.
- `runtime_length_angle_update_pass`
  Posts `leg_len_mod=8` at six seconds and `leg_ang_mod=-5` at eight seconds;
  passes on ordinary progression, midpoint, completion, and no-error evidence,
  without measuring the updated leg geometry.
- `runtime_turn_radius_update_pass`
  Uses a 44-meter leg and posts `turn_rad_mod=2` and `turn_ext_mod=1` before
  the first turn; passes on ordinary progression, midpoint, completion, and
  no-error evidence, without measuring the updated turn.
- `runtime_speed_replace_reset_pass`
  Applies `leg_spds` replace, reset, clear, and final `replace,1.2` updates at
  two-second intervals on a 44-meter leg; passes on ordinary progression,
  midpoint, completion, and no-error evidence, without grading update history
  or final speed.
- `runtime_shift_point_pass`
  Posts `shift_pt=9,-55` four seconds after deployment to move the leg center;
  passes on ordinary progression, midpoint, completion, and no-error evidence,
  without measuring the shifted endpoints.
- `runtime_turn_bias_gap_update_pass`
  Posts `turn_bias_mod=-10`, `turn_ext_mod=2`, and `turn_pt_gap=15` during the
  run; passes on ordinary progression, midpoint, completion, and no-error
  evidence, without measuring the resulting turn geometry.
- `flag_aliases_pass`
  Adds `leg_end_flag`, `turn_end_flag`, `mid_leg_flag`, and `mid_turn_flag`
  alongside the canonical flag names; passes when every canonical and alias
  flag reaches shoreside, the behavior completes, and no behavior error occurs.
- `mid_pct_late_pass`
  Moves `mid_leg_pct` and `mid_turn_pct` from 30 to 70; passes when both
  midpoint flags eventually fire with the ordinary progression and completion
  flags and no behavior error, without grading when they fired.
- `patience_clip_pass`
  Supplies `patience=150`, which the behavior reports as the clipped value 99;
  passes on ordinary progression, midpoint, completion, and no-error evidence,
  while the reported clipped setting is not itself graded.
- `bad_leg_config_fail`
  Sets nonnumeric `len=bad` in the leg specification; the harness passes when
  the mission times out without reaching turn 2 and without a `BHV_ERROR` post.
- `bad_speed_schedule_fail`
  Sets malformed `leg_spds=1.0,fast`; the harness passes when the mission times
  out without reaching turn 2 and without a `BHV_ERROR` post.
- `bad_init_mode_fail`
  Sets unsupported `init_leg_mode=sideways`; the harness passes when the mission
  times out without reaching turn 2 and without a `BHV_ERROR` post.
- `bad_mid_pct_fail`
  Sets out-of-range `mid_leg_pct=101`; the harness passes when the mission times
  out without reaching turn 2 and without a `BHV_ERROR` post.
- `bad_turn_radius_fail`
  Sets invalid shared `turn_rad=0`; the harness passes when the mission times
  out without reaching turn 2 and without a `BHV_ERROR` post.
- `bad_turn_ext_fail`
  Sets invalid shared `turn_ext=-1`; the harness passes when the mission times
  out without reaching turn 2 and without a `BHV_ERROR` post.
- `bad_turn_dir_fail`
  Sets unsupported shared `turn_dir=sideways`; the harness passes when the
  mission times out without reaching turn 2 and without a `BHV_ERROR` post.
- `bad_speed_high_fail`
  Sets `speed=9`, above the helm speed domain; the harness passes when the
  mission times out without reaching turn 2 and without a `BHV_ERROR` post.
- `bad_speed_count_fail`
  Sets invalid zero-count schedule entry `leg_spds=0:1.0`; the harness passes
  when the mission times out without reaching turn 2 and without a `BHV_ERROR`
  post.

Expected-negative cases use mission-owned criteria. The result row grades
`pass` only when `timeout=true`, `turn2=false`, and `bhv_error=false`, with
`expected=malconfig_timeout` included as the evidence label.

## Running

```sh
./zlaunch.sh --case=baseline_cycle_pass
./zlaunch.sh --jobs=4 --port_base=15400
```

Headless `--nogui` mode is the default. Use `--gui` with a single case for
visual inspection.

Serial and rolling modes both use isolated mission copies and deterministic
per-case port blocks. Rolling mode starts the next pending case whenever an
active slot finishes. Use `--keep_workdirs` to preserve the harness-owned run
tree for inspection.

Latest validation:

- July 16, 2026
- generated-file matrix: `33/33` cases completed with `--just_make --jobs=4`
- three full rolling matrices: `99/99` mission-owned verdicts passed
- full serial matrix: `33/33` mission-owned verdicts passed
- warp: `10`

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for a fully logged diagnostic case.
