# H01-loiter_behavior_motion

Logging is minimal by default in both communities. Use `--log=full` for the
whole matrix or with `--case=NAME` for one diagnostic case.

Patch-driven moving correctness harness for
[`missions/loiter_behavior_missions/loiter_behavior_motion`](../../../missions/loiter_behavior_missions/loiter_behavior_motion).

This harness focuses on `BHV_Loiter` as the behavior under test. The stem
mission keeps one simulated vehicle near a compact loiter polygon and grades on
behavior-owned outputs:

- `LOITER_INDEX`
- `LOITER_MODE`
- `LOITER_ACQUIRE`
- `LOITER_DIST_TO_POLY`
- `LOITER_ETA_TO_POLY`
- `LOITER_BNG_TOTAL`
- `LOITER_REPORT`
- `BHV_ERROR_SEEN`

## Current Matrix

- `radial_clockwise_pass` Configures an eight-point radial polygon centered at `(12,-121)` with radius 18 and `clockwise=true`, exercising the stock clockwise loiter at the late 45-second gate; passes with a nonnegative index, `LOITER_MODE=stable`, `LOITER_ACQUIRE=0`, distance to polygon at most 10, and no `BHV_ERROR` mail.
- `radial_counterclockwise_pass` Uses the same radial geometry with `clockwise=false`, exercising counterclockwise configuration; passes at 25 seconds with a nonnegative index, stable mode, acquisition off, distance to polygon at most 10, and no behavior error.
- `clockwise_best_pass` Sets `clockwise=best`, exercising automatic direction selection from the vehicle's approach; passes at 25 seconds on the same nonnegative-index, stable-mode, acquisition-off, distance-at-most-10, and no-error conditions.
- `polygon_box_pass` Replaces the radial polygon with the rectangle `{-6,-139:-6,-103:30,-103:30,-139}`, exercising explicit four-vertex geometry; passes at 25 seconds when the loiter reports a nonnegative index, stable mode, acquisition off, distance to that reported polygon at most 10, and no behavior error.
- `triangle_polygon_pass` Uses the triangle `{0,-139:34,-121:0,-103}`, exercising explicit three-vertex geometry; passes at 25 seconds when the loiter reports a nonnegative index, stable mode, acquisition off, distance to polygon at most 10, and no behavior error.
- `start_inside_loiter_pass` Centers the radius-18 polygon on the vehicle's initial position `(-45,-121)`, exercising activation from inside the loiter area; passes at 25 seconds with a nonnegative index, stable mode, acquisition off, distance to polygon at most 10, and no behavior error.
- `acquire_from_far_pass` Places a radius-16 polygon at `(6,-121)`, 51 meters east of the starting position, exercising an external approach; passes at 25 seconds after the final outputs show a nonnegative index, stable mode, acquisition off, distance to polygon at most 10, and no behavior error.
- `early_acquire_mode_pass` Uses the far polygon but evaluates after six seconds, testing the external-acquisition flag before arrival; passes with a nonnegative loiter index, `LOITER_ACQUIRE=1`, distance to polygon greater than 10, and no behavior error.
- `center_activate_pass` Sets `center_activate=true` on the stock polygon, exercising recentering when the behavior activates; passes at 25 seconds on the standard stable-loiter conditions: nonnegative index, acquisition off, distance at most 10, and no behavior error.
- `capture_radius_large_pass` Raises `capture_radius` from 4 to 12 and `slip_radius` from 12 to 18, exercising the larger capture/slip thresholds; passes at 25 seconds on the standard stable-loiter conditions.
- `slip_radius_pass` Lowers `capture_radius` to 0.5 while leaving `slip_radius=12`, exercising the path in which a point can be accepted through the wider slip/nonmonotonic rule; passes at 25 seconds on the standard stable-loiter conditions.
- `speed_alt_update_pass` Configures `speed=0.9` and `speed_alt=2.2`, then posts `LOITER_UPDATES="use_alt_speed=true"` at three seconds, exercising runtime alternate-speed selection; passes at 25 seconds on the standard stable-loiter conditions.
- `use_alt_speed_static_pass` Configures `speed=0.8`, `speed_alt=2.8`, and `use_alt_speed=true`, testing static alternate-speed selection against a 25-second arrival gate; passes on the standard stable-loiter conditions.
- `center_assign_xy_pass` Posts `LOITER_UPDATES="center_assign=x=18,y=-121"` at four seconds, exercising keyed pair syntax for a runtime center assignment; passes at 25 seconds on the standard stable-loiter conditions.
- `center_assign_pair_pass` Starts with the radius-18 center at `(2,-121)`, posts `center_assign=32,-121` at 22 seconds, and evaluates at 45 seconds, exercising unkeyed pair syntax and a 30-meter eastward shift; passes on the standard stable-loiter conditions.
- `xcenter_ycenter_update_pass` Posts `xcenter_assign=18` and then `ycenter_assign=-121` at four and five seconds, exercising separate runtime center-coordinate updates; passes at 25 seconds on the standard stable-loiter conditions.
- `mod_poly_rad_expand_pass` Posts `mod_poly_rad=6` at 20 seconds to expand the stock radius from 18 to 24, exercising positive runtime radius modification; passes at 45 seconds on the standard stable-loiter conditions.
- `mod_poly_rad_shrink_pass` Posts `mod_poly_rad=-5` at 14 seconds to shrink the stock radius from 18 to 13, exercising negative runtime radius modification; passes at 25 seconds on the standard stable-loiter conditions.
- `slingshot_bearing_pass` Sets `slingshot=5`, testing bearing-based completion after the vehicle accumulates the configured angular travel; passes at 35 seconds when `LOITER_DONE=true`, `LOITER_BNG_TOTAL>=5`, mode is stable, acquisition is off, and no behavior error occurred.
- `post_suffix_outputs_pass` Sets `post_suffix=custom`, testing suffixed status publication; passes when `LOITER_INDEX_CUSTOM>=0`, `LOITER_MODE_CUSTOM=stable`, `LOITER_ACQUIRE_CUSTOM=0`, `LOITER_DIST_TO_POLY_CUSTOM<=10`, and no behavior error occurred.
- `eta_output_pass` Uses the stock loiter to test ETA publication; passes at 25 seconds when the stable-loiter conditions hold and `LOITER_ETA_TO_POLY>=0`, while `LOITER_REPORT` is recorded but not graded.
- `ipf_zaic_spd_pass` Sets `ipf_type=zaic_spd`, exercising the ZAIC speed objective instead of the default loiter IvP-function type; passes at 25 seconds on the standard stable-loiter conditions.
- `slow_speed_acquire_pass` Sets `speed=0.25` and evaluates after ten seconds, testing acquisition status during a deliberately slow approach; passes with a nonnegative index, `LOITER_ACQUIRE=1`, distance to polygon greater than 10, and no behavior error.
- `empty_polygon_fail` Omits the required `polygon` parameter, exercising invalid behavior initialization; the harness passes as soon as any `IVPHELM_STATE` publication latches `HELM_MALCONFIG=true`.
- `bad_polygon_fail` Supplies the self-intersecting point sequence `{0,-121:20,-101:0,-101:20,-121}`, exercising rejection of invalid polygon geometry; the harness passes as soon as any `IVPHELM_STATE` publication latches `HELM_MALCONFIG=true`.
- `bad_update_fail` Posts the same self-intersecting polygon through `LOITER_UPDATES` at two seconds, exercising malformed runtime-update rejection while the original loiter continues; the harness passes at 25 seconds when any `BHV_WARNING` has appeared and the stock loiter satisfies the standard stable-loiter conditions without a `BHV_ERROR`.
- `bad_clockwise_fail` Sets `clockwise=sideways`, exercising rejection of a value outside the boolean/`best` forms; the harness passes as soon as any `IVPHELM_STATE` publication latches `HELM_MALCONFIG=true`.
- `bad_use_alt_speed_fail` Sets `use_alt_speed=maybe`, exercising rejection of a non-boolean alternate-speed selector; the harness passes as soon as any `IVPHELM_STATE` publication latches `HELM_MALCONFIG=true`.
- `bad_patience_fail` Sets `patience=100`, exercising rejection above the accepted 1–99 range; the harness passes as soon as any `IVPHELM_STATE` publication latches `HELM_MALCONFIG=true`.
- `bad_capture_radius_fail` Sets `capture_radius=-1`, exercising rejection of a negative capture radius; the harness passes as soon as any `IVPHELM_STATE` publication latches `HELM_MALCONFIG=true`.
- `center_bad_update_recover_pass` Posts `center_assign=bad_center` at three seconds and the valid `center_assign=x=18,y=-121` at five seconds, exercising recovery after a malformed center update; passes at 25 seconds on the standard stable-loiter conditions, without requiring a warning for the bad update.
- `spiral_factor_pass` Sets `spiral_factor=0.4`, exercising a nondefault acquisition spiral; passes at 25 seconds on the standard stable-loiter conditions.
- `patience_low_pass` Sets `patience=15`, exercising the low course/speed ZAIC ratio used during point switching; passes at 25 seconds on the standard stable-loiter conditions.
- `patience_high_pass` Sets `patience=90`, exercising the high course/speed ZAIC ratio used during point switching; passes at 25 seconds on the standard stable-loiter conditions.

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=radial_clockwise_pass 10
./zlaunch.sh --case=center_activate_pass --gui 1
./zlaunch.sh --case=eta_output_pass --gui 1
./zlaunch.sh --just_make 10
./zlaunch.sh --jobs=4 --port_base=27000 10
```

Results are written to `results.txt` as `case=<name>` followed by the
mission-owned row. Expected-negative cases still produce `grade=pass` when their
specific evidence is observed, using `expected=helm_malconfig` or
`expected=bad_update_rejected` plus the supporting fields. Harness-synthesized
`grade=fail reason=...` rows are reserved for runner failures such as launch or
missing-result errors. Wave mode runs the full matrix in isolated temporary
mission copies. Each case gets its own MOOSDB and pShare port block using
`case_base = port_base + case_idx*PORT_STRIDE`, with pShare ports starting at
`case_base + 10`. The default is serial `--jobs=1`; use `--keep_workdirs` when
preserving temporary case folders is useful for debugging.

Latest validation:

- May 20, 2026
- focused expected-negative cases:
  `empty_polygon_fail`, `bad_polygon_fail`, `bad_update_fail`,
  `bad_clockwise_fail`, `bad_use_alt_speed_fail`, `bad_patience_fail`,
  `bad_capture_radius_fail`
- full reserved-port matrix: `34/34` cases passed via serial `--case` loop with
  `--port_base=29100`
- warp: `10`
