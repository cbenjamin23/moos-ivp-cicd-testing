# H01-zigzag_behavior_motion

Harness for `BHV_ZigZag` using the stem mission at
`missions/zigzag_behavior_missions/zigzag_behavior_motion`.

The vehicle first completes a short approach leg, then `ZIGZAG=true` activates
the behavior under test. The mission grades completion, zig/zag counts,
side-entry and side-exit flags, stem-line telemetry, runtime-update effects,
and expected malformed-configuration failures.

## Current Matrix

- `baseline_port_first_pass`
  Baseline port-first zigzag. Requires approach completion, behavior
  completion, at least four zig legs, at least two zags, both side-entry flags,
  both side-exit flags, and no behavior error.
- `star_first_pass`
  Mirrors the baseline with `zig_first=star`.
- `zig_first_right_alias_pass`
  Verifies the `right` alias for starboard-first selection.
- `zig_first_starboard_alias_pass`
  Verifies the full `starboard` alias for starboard-first selection.
- `stem_hdg_wrap_pass`
  Sets `stem_hdg=405` and requires the behavior macro to report the wrapped
  heading near `45`.
- `stem_hdg_negative_wrap_pass`
  Sets `stem_hdg=-315` and requires the same wrapped heading near `45`.
- `hdg_thresh_loose_pass`
  Uses a loose `hdg_thresh=40` while still requiring normal completion and
  side-transition evidence.
- `small_angle_pass`
  Exercises the lower accepted `zig_angle=1` boundary with a tight heading
  threshold.
- `wide_angle_pass`
  Exercises the upper accepted `zig_angle=75` boundary with a shorter
  completion gate so the case tests angle acceptance rather than duration.
- `max_zig_zags_pass`
  Uses `max_zig_zags=2` and checks the derived leg-limit path.
- `single_leg_complete_pass`
  Uses a one-leg limit and grades early behavior completion with side flags.
- `zero_leg_limit_pass`
  Uses `max_zig_legs=0`, verifying the valid zero-limit edge completes after
  the first leg.
- `stem_odo_complete_pass`
  Uses a high leg limit and `max_stem_odo=22`, proving odometry-based
  completion can end the behavior before leg exhaustion.
- `short_stem_odo_complete_pass`
  Uses `max_stem_odo=2`, proving near-immediate odometry completion occurs
  before the first full zig leg is required.
- `stem_on_active_pass`
  Starts with a mismatched configured stem heading and requires
  `stem_on_active=true` to capture the live vehicle heading.
- `speed_on_active_pass`
  Starts with a low configured behavior speed and requires
  `speed_on_active=true` to capture the live vehicle speed.
- `mod_speed_increase_pass`
  Posts a runtime `ZIG_UPDATE` with `mod_speed=0.8` and requires the vehicle to
  finish with higher speed evidence.
- `mod_speed_reset_pass`
  Posts `mod_speed=0.8` followed by `mod_speed=reset`, requiring the final
  speed evidence to return to the baseline branch.
- `mod_speed_clip_high_pass`
  Posts a very large positive speed modifier and requires high-speed evidence,
  exercising the domain-clipping path.
- `mod_speed_clip_low_pass`
  Posts a very large negative speed modifier and requires near-zero speed
  evidence, exercising the low-clipping path.
- `bad_runtime_update_recover_pass`
  Posts an invalid runtime `zig_angle=0` update, then a valid speed modifier,
  proving a rejected update does not poison later valid updates.
- `fierce_zigging_pass`
  Enables fierce zigging with a separate fierce heading delta.
- `delta_heading_alias_pass`
  Uses the `delta_heading` alias for the fierce heading delta.
- `visual_hints_off_pass`
  Verifies valid visual-hint disabling does not break behavior execution.
- `visual_hints_custom_pass`
  Verifies valid custom `set_hdg_color` and `req_hdg_color` visual hints are
  accepted while the behavior still completes normally.
- `bad_zig_angle_low_fail`
  Expected-negative case for `zig_angle=0`; requires helm malconfiguration
  evidence.
- `bad_zig_angle_high_fail`
  Expected-negative case for `zig_angle=76`; requires helm malconfiguration
  evidence.
- `bad_zig_first_fail`
  Expected-negative case for an unsupported first-side value; requires helm
  malconfiguration evidence.
- `bad_max_zig_zags_fail`
  Expected-negative case for `max_zig_zags=0`; requires helm malconfiguration
  evidence.
- `bad_max_stem_odo_fail`
  Expected-negative case for `max_stem_odo=0`; requires helm malconfiguration
  evidence.
- `bad_speed_fail`
  Expected-negative case for negative speed; requires helm malconfiguration
  evidence.
- `bad_delta_heading_fail`
  Expected-negative case for `delta_heading=0`; requires helm malconfiguration
  evidence.
- `bad_stale_nav_thresh_fail`
  Expected-negative case documenting that `stale_nav_thresh` is not accepted as
  a `BHV_ZigZag` configuration parameter even though the behavior has an
  internal stale-nav threshold; requires helm malconfiguration evidence.
- `bad_hdg_thresh_fail`
  Expected-negative case for negative `hdg_thresh`; requires helm
  malconfiguration evidence.
- `bad_visual_hint_fail`
  Expected-negative case for malformed visual-hint boolean input; requires helm
  malconfiguration evidence.

## Manual Commands

```bash
./zlaunch.sh --case=baseline_port_first_pass --port_base=15000 10
./zlaunch.sh --case=wide_angle_pass --gui --port_base=15000 10
./zlaunch.sh --jobs=4 --port_base=15000 10
```

## Notes

`ZZ_STEM_SPEED` currently reports `0` in static-speed cases because
`BHV_ZigZag` stores its original speed only on some configuration paths. The
`speed_on_active_pass` case is therefore the one that grades that macro
directly.
