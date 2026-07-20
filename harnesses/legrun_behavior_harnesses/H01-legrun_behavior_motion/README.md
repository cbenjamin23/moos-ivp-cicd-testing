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
  Sets `init_leg_mode=far_turn` to start on leg 2; passes when `LR_DONE=true`
  with `LR_LEG2_DONE=true` while `LR_LEG1_DONE` and `LR_TURN1_DONE` remain
  false.
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
  Configures finite `leg_spds=1.2,1.0,0.8,1.4`; passes when behavior-owned
  `LR_NOW_SPD` follows leg 2 `1.2`, leg 1 `1.0`, leg 2 `0.8`, leg 1 `1.4`,
  then returns to cruise speed `1.6` on leg 2.
- `leg_speed_onturn_pass`
  Configures repeating `leg_spds=1.1,0.9,1.3` with
  `leg_spds_onturn=true`; passes when each turn retains its preceding leg
  speed through the ordered `1.1`, `0.9`, and `1.3` entries, then the wrapped
  leg returns to `1.1`.
- `leg_speed_count_repeat_pass`
  Configures repeating count-expanded `leg_spds=2:1.0,1.4`; passes when two
  consecutive legs use `1.0`, the next uses `1.4`, the wrapped leg returns to
  `1.0`, and every intervening turn remains at cruise speed `1.6`.
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
  Sets `mid_leg_pct=70` and posts `$[PCT_NP]` with the mid-leg flag; passes
  when that flag reports leg progress between `0.68` and `0.78`, the mid-turn
  flag fires before completion, and no behavior error occurs.
- `patience_clip_pass`
  Supplies `patience=150` and extracts the behavior's reported setting with
  `pEchoVar`; passes when the extracted value is exactly `patience=99`, the
  helm is in `DRIVE`, and `LR_NOW_SPD` remains at cruise speed `1.6`.
- `bad_leg_config_fail`
  Sets nonnumeric `len=bad` in the leg specification; passes when the bridged
  helm state becomes exactly `MALCONFIG` before the 15-second deadline.
- `bad_speed_schedule_fail`
  Sets malformed `leg_spds=1.0,fast`; passes when the bridged helm state
  becomes exactly `MALCONFIG` before the 15-second deadline.
- `bad_init_mode_fail`
  Sets unsupported `init_leg_mode=sideways`; passes when the bridged helm state
  becomes exactly `MALCONFIG` before the 15-second deadline.
- `bad_mid_pct_fail`
  Sets out-of-range `mid_leg_pct=101`; passes when the bridged helm state
  becomes exactly `MALCONFIG` before the 15-second deadline.
- `bad_turn_radius_fail`
  Sets invalid shared `turn_rad=0`; passes when the bridged helm state becomes
  exactly `MALCONFIG` before the 15-second deadline.
- `bad_turn_ext_fail`
  Sets invalid shared `turn_ext=-1`; passes when the bridged helm state becomes
  exactly `MALCONFIG` before the 15-second deadline.
- `bad_turn_dir_fail`
  Sets unsupported shared `turn_dir=sideways`; passes when the bridged helm
  state becomes exactly `MALCONFIG` before the 15-second deadline.
- `bad_speed_high_fail`
  Sets `speed=9`, above the helm speed domain; passes when the bridged helm
  state becomes exactly `MALCONFIG` before the 15-second deadline.
- `bad_speed_count_fail`
  Sets invalid zero-count schedule entry `leg_spds=0:1.0`; passes when the
  bridged helm state becomes exactly `MALCONFIG` before the 15-second deadline.

Expected-negative cases use mission-owned criteria. They grade `pass` only
when the armed evaluator observes literal `IVPHELM_STATE=MALCONFIG` before its
deadline; the current telemetry does not identify which parameter was rejected.

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

- July 20, 2026
- generated-file matrix: `33/33` cases completed with `--just_make --jobs=4`
- minimal rolling matrix: `33/33` mission-owned verdicts passed
- full-logging rolling matrix: `33/33` mission-owned verdicts passed
- deliberate mutations of far-turn initialization, static speed scheduling,
  mid-leg percentage, patience clipping, and invalid-leg rejection all failed
  their focused evaluators
- warp: `10`

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for a fully logged diagnostic case.
