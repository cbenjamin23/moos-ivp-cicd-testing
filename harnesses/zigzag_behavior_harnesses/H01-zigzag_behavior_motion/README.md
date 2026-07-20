# H01-zigzag_behavior_motion

Harness for `BHV_ZigZag` using the stem mission at
`missions/zigzag_behavior_missions/zigzag_behavior_motion`.

The vehicle first completes a short approach leg, then `ZIGZAG=true` activates
the behavior under test. The mission grades completion, zig/zag counts,
side-entry and side-exit flags, stem-line telemetry, runtime-update effects,
and expected malformed-configuration failures.

## Current Matrix

- `baseline_port_first_pass`
  Runs a 1.4 m/s, port-first zigzag about a 45-degree stem with a 25-degree
  angle and three-leg limit; passes when the approach and behavior complete,
  at least four zig and two zag flags fire, both sides are entered and exited,
  the last side is starboard, and no behavior error occurs.
- `star_first_pass`
  Changes `zig_first` from `port` to `star`, exercising the starboard-first
  sequence; passes on the baseline completion and side-transition evidence
  with the last side reported as port and no behavior error.
- `zig_first_right_alias_pass`
  Uses `zig_first=right` as an alias for starboard-first selection; passes on
  the same completion, side-transition, and final-port-side evidence as the
  canonical `star` case, with no behavior error.
- `zig_first_starboard_alias_pass`
  Uses `zig_first=starboard` as an alias for starboard-first selection; passes
  on the same completion, side-transition, and final-port-side evidence as the
  canonical `star` case, with no behavior error.
- `stem_hdg_wrap_pass`
  Sets `stem_hdg=405` to exercise positive heading normalization; passes when
  the behavior completes, `ZZ_STEM_HDG` is between 44 and 46 degrees, and no
  behavior error occurs.
- `stem_hdg_negative_wrap_pass`
  Sets `stem_hdg=-315` to exercise negative heading normalization; passes when
  the behavior completes, `ZZ_STEM_HDG` is between 44 and 46 degrees, and no
  behavior error occurs.
- `hdg_thresh_loose_pass`
  Raises `hdg_thresh` from 8 to 40 degrees so each leg is accepted well before
  the requested heading; passes when the approach and behavior complete with
  at least four zigs, two zags, a final starboard side, less than 8 meters of
  stem odometry, and no behavior error.
- `small_angle_pass`
  Sets the accepted lower boundary `zig_angle=1` with `hdg_thresh=1`; passes
  when four zigs and two zags complete, the final heading is 43 to 47 degrees,
  final stem distance is below 2 meters, and no behavior error occurs.
- `wide_angle_pass`
  Sets the accepted upper boundary `zig_angle=75` with a two-leg limit;
  passes when both sides are entered, at least three zig and one zag flags fire,
  final stem distance exceeds 8 meters, and the behavior completes without an
  error.
- `max_zig_zags_pass`
  Uses `max_zig_zags=2` instead of `max_zig_legs`, exercising conversion from
  complete zigzags to a leg limit; passes when the behavior completes with at
  least five zigs, two zags, a final port side, and no behavior error.
- `single_leg_complete_pass`
  Sets `max_zig_legs=1`, testing that the limit is exceeded only after the
  second completed leg; passes with exactly two zigs, one zag, both side-entry
  and side-exit flags, `last_side=star`, no timeout, and no behavior error.
- `zero_leg_limit_pass`
  Sets the accepted edge `max_zig_legs=0`, testing completion immediately
  after the first port leg; passes with exactly one zig, zero zags, port entry
  and exit, no starboard entry or exit, `last_side=port`, no timeout, and no
  behavior error.
- `stem_odo_complete_pass`
  Sets `max_zig_legs=99` and `max_stem_odo=22` so stem odometry, rather than
  leg count, ends the behavior; passes when final stem odometry is between 18
  and 35 meters and the approach and behavior complete without an error.
- `short_stem_odo_complete_pass`
  Sets `max_zig_legs=99` and `max_stem_odo=2` to exercise the short odometry
  cutoff; passes when final stem odometry is between 1 and 8 meters, no more
  than one zig flag fires, and the approach and behavior complete without an
  error.
- `stem_on_active_pass`
  Configures `stem_hdg=120` with `stem_on_active=true`, requiring activation to
  replace the configured heading with the vehicle's live approach heading;
  passes when `ZZ_STEM_HDG` is between 30 and 60 degrees and the behavior
  completes without an error.
- `speed_on_active_pass`
  Configures `speed=0.4` with `speed_on_active=true`, requiring activation to
  capture the faster live approach speed; passes when `ZZ_STEM_SPEED` exceeds
  0.8 m/s and the behavior completes without an error.
- `mod_speed_increase_pass`
  Posts `ZIG_UPDATE="mod_speed=0.8"` three seconds after deployment;
  passes when the behavior completes with final `NAV_SPEED>1.45` and no
  behavior error.
- `mod_speed_reset_pass`
  Captures the live activation speed, posts `mod_speed=0.8` while ZigZag is
  active, then posts `mod_speed=reset`; an ordered evaluator first requires
  `NAV_SPEED>2.05` before its missing-state deadline, then requires completion
  with `1.45<NAV_SPEED<1.95`, no timeout, and no behavior error.
- `mod_speed_clip_high_pass`
  Posts `ZIG_UPDATE="mod_speed=9.0"` to exercise upper speed-domain clipping;
  passes when the behavior completes with final `NAV_SPEED>2.35` and no
  behavior error.
- `mod_speed_clip_low_pass`
  Posts `ZIG_UPDATE="mod_speed=-9.0"` to exercise lower speed-domain clipping;
  passes at the mission timeout when the approach had completed, `NAV_SPEED`
  is below 0.15 m/s, and no behavior error occurred.
- `bad_runtime_update_recover_pass`
  Posts invalid `zig_angle=0` and then valid `mod_speed=0.4` after ZigZag
  activates; the first phase requires the next-leg requested heading between
  60 and 75 degrees, proving the original 25-degree angle remained in force,
  and completion requires four zigs, two zags, `1.45<NAV_SPEED<2.0`, no
  timeout, and no behavior error.
- `fierce_zigging_pass`
  Sets `zig_angle=20`, `zig_angle_fierce=45`, and `fierce_zigging=true`,
  testing that a port-first vehicle near heading 45 initially requests a
  heading within 10 degrees of north rather than the ordinary 25-degree set
  heading; completion also requires exactly four zigs, two zags, both side
  exits, `last_side=star`, and no error or timeout.
- `delta_heading_alias_pass`
  Replaces only `zig_angle_fierce=45` with `delta_heading=45` in the fierce
  fixture, testing identical alias behavior; it must produce the same initial
  north-centered requested-heading band and exact four-zig, two-zag,
  final-starboard completion evidence without an error or timeout.
- `visual_hints_off_pass`
  Sets `draw_set_hdg=false` and `draw_req_hdg=false`, exercising visual-hint
  parser acceptance; passes only on the ordinary port-first completion,
  side-transition, final-side, and no-error evidence.
- `visual_hints_custom_pass`
  Sets `set_hdg_color=green` and `req_hdg_color=orange`, exercising custom
  visual-hint parser acceptance; passes only on the ordinary port-first
  completion, side-transition, final-side, and no-error evidence.
- `bad_zig_angle_low_fail`
  Sets invalid lower value `zig_angle=0`; the harness passes only when the
  armed evaluator observes exact `ABE_IVPHELM_STATE=MALCONFIG` before its
  missing-state deadline.
- `bad_zig_angle_high_fail`
  Sets invalid upper value `zig_angle=76`; the harness passes only on exact
  pre-deadline `ABE_IVPHELM_STATE=MALCONFIG` after the evaluator is armed.
- `bad_zig_first_fail`
  Sets unsupported `zig_first=upward`; the harness passes only on exact
  pre-deadline `ABE_IVPHELM_STATE=MALCONFIG` after the evaluator is armed.
- `bad_max_zig_zags_fail`
  Sets invalid `max_zig_zags=0`; the harness passes only on exact pre-deadline
  `ABE_IVPHELM_STATE=MALCONFIG` after the evaluator is armed.
- `bad_max_stem_odo_fail`
  Sets invalid `max_stem_odo=0`; the harness passes only on exact pre-deadline
  `ABE_IVPHELM_STATE=MALCONFIG` after the evaluator is armed.
- `bad_speed_fail`
  Sets invalid `speed=-1`; the harness passes only on exact pre-deadline
  `ABE_IVPHELM_STATE=MALCONFIG` after the evaluator is armed.
- `bad_delta_heading_fail`
  Enables fierce zigging with invalid `delta_heading=0`; the harness passes
  only on exact pre-deadline `ABE_IVPHELM_STATE=MALCONFIG` after the evaluator
  is armed.
- `bad_stale_nav_thresh_fail`
  Adds unsupported configuration parameter `stale_nav_thresh=5`; the harness
  passes only on exact pre-deadline `ABE_IVPHELM_STATE=MALCONFIG` after the
  evaluator is armed.
- `bad_hdg_thresh_fail`
  Sets invalid `hdg_thresh=-1`; the harness passes only on exact pre-deadline
  `ABE_IVPHELM_STATE=MALCONFIG` after the evaluator is armed.
- `bad_visual_hint_fail`
  Sets invalid visual hint `draw_set_hdg=maybe`; the harness passes only on
  exact pre-deadline `ABE_IVPHELM_STATE=MALCONFIG` after the evaluator is
  armed.

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

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for a fully logged diagnostic case.
