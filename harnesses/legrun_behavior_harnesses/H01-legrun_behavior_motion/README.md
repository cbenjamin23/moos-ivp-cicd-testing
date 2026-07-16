# H01-legrun_behavior_motion

Patch-driven moving correctness harness for
[`/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/legrun_behavior_missions/legrun_behavior_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/legrun_behavior_missions/legrun_behavior_motion).

This harness focuses on `BHV_LegRun` as the behavior under test. The stem runs a
single simulated vehicle on a two-leg pattern and grades on behavior-owned mode,
leg, turn, mid-leg, mid-turn, completion, and warning/error signals.

## Current Matrix

- `baseline_cycle_pass` Exercises the stock leg-run pattern and requires the first leg, first turn, second leg, behavior completion, mid-leg, and mid-turn flags without behavior errors.
- `finite_complete_pass` Runs the same pattern with `perpetual=false` and requires the behavior completion end flag after one traversal.
- `colon_leg_format_pass` Supplies the leg geometry in `x:y:ang:len` form and verifies it produces the same clean cycle as the keyword form.
- `vertex_points_pass` Supplies explicit `p1` and `p2` vertices instead of a center/angle/length leg and verifies clean completion.
- `init_close_turn_pass` Starts from the closest-turn initialization mode and requires a clean full cycle.
- `init_far_turn_pass` Starts from the far-turn initialization mode and verifies the behavior selects the far leg path and completes cleanly.
- `turn_rad_alias_pass` Uses the shared `turn_rad`, `turn_bias`, and `turn_dir` aliases and verifies both turn generators remain usable.
- `turn_bias_ext_gap_pass` Exercises shared turn bias, extension, and turn-point-gap shaping while preserving clean progression.
- `individual_turn_params_pass` Uses separate turn-one and turn-two radius, bias, direction, and extension settings and verifies the asymmetric turn geometry remains usable.
- `turn_rad_min_clip_pass` Sets `turn_rad_min` before a too-small `turn_rad` and verifies the behavior clips into a valid maneuver.
- `capture_radius_alias_pass` Uses the `radius` alias for waypoint capture radius and verifies arrival and turn progression remain clean.
- `capture_line_absolute_pass` Enables absolute capture-line semantics and verifies the vehicle still progresses through the cycle.
- `no_capture_line_pass` Disables capture-line use with radius-based capture and verifies the behavior still completes.
- `leg_speed_schedule_pass` Adds a finite `leg_spds` schedule and verifies the behavior still completes leg and turn progression cleanly.
- `leg_speed_onturn_pass` Enables scheduled speeds during turns with repeat enabled and verifies clean progression.
- `leg_speed_count_repeat_pass` Uses count-expanded speed schedule syntax with repeat enabled and verifies clean progression.
- `runtime_length_angle_update_pass` Posts runtime leg length and angle updates through `LEG_UPDATE` and verifies the behavior keeps cycling cleanly.
- `runtime_turn_radius_update_pass` Posts runtime turn radius and extension updates through `LEG_UPDATE` on a compact leg and verifies turn progression remains clean.
- `runtime_speed_replace_reset_pass` Posts `replace`, `reset`, and `clear` speed-schedule updates through `LEG_UPDATE` on a compact leg and verifies the behavior continues cleanly.
- `runtime_shift_point_pass` Posts a runtime `shift_pt` update and verifies the leg-run center can move without breaking completion.
- `runtime_turn_bias_gap_update_pass` Posts runtime turn-bias, turn-extension, and turn-point-gap updates and verifies clean turn progression.
- `flag_aliases_pass` Uses the underscore flag aliases for leg, turn, mid-leg, and mid-turn events and requires those alias flags to reach shoreside.
- `mid_pct_late_pass` Moves mid-leg and mid-turn flag thresholds later in each segment and verifies those events still fire before completion.
- `patience_clip_pass` Supplies an above-range patience value and verifies the clipped objective weighting still completes the leg-run cycle cleanly.
- `bad_leg_config_fail` Supplies malformed leg geometry and passes when the expected no-progress timeout evidence is observed.
- `bad_speed_schedule_fail` Supplies a malformed speed schedule and passes when the expected no-progress timeout evidence is observed.
- `bad_init_mode_fail` Supplies an invalid `init_leg_mode` and passes when the expected no-progress timeout evidence is observed.
- `bad_mid_pct_fail` Supplies an out-of-range mid-leg percentage and passes when the expected no-progress timeout evidence is observed.
- `bad_turn_radius_fail` Supplies a zero turn radius and passes when the expected no-progress timeout evidence is observed.
- `bad_turn_ext_fail` Supplies a negative turn extension and passes when the expected no-progress timeout evidence is observed.
- `bad_turn_dir_fail` Supplies an invalid shared turn direction and passes when the expected no-progress timeout evidence is observed.
- `bad_speed_high_fail` Supplies a cruise speed above the helm domain maximum and passes when the expected no-progress timeout evidence is observed.
- `bad_speed_count_fail` Supplies a zero-count speed schedule entry and passes when the expected no-progress timeout evidence is observed.

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
