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

- `radial_clockwise_pass` Configures the stock eight-point radial polygon centered at `(12,-121)` with `clockwise=true`; passes when the vehicle reaches stable loiter with a nonnegative index, acquisition off, distance to the polygon at most 10 meters, and no `BHV_ERROR`.
- `radial_counterclockwise_pass` Uses the same radial polygon with `clockwise=false`, which selects vertex 3 and then advances to vertex 4 from this fixed western approach; passes only after that ordered index sequence occurs before the missing-index deadline.
- `polygon_box_pass` Replaces the stock octagon with the rectangle `{-6,-139:-6,-103:30,-103:30,-139}`; live behavior macros must report four points, center `(12,-121)`, and radius `25.45–25.46`, so falling back to the stock polygon fails.
- `triangle_polygon_pass` Replaces the stock octagon with the triangle `{0,-139:34,-121:0,-103}`; live behavior macros must report three points, center `(17,-121)`, and radius `24.75–24.76`, so falling back to the stock polygon fails.
- `start_inside_loiter_pass` Centers the radius-18 polygon on the vehicle's initial position `(-45,-121)`; passes only after the ordered mode transition from `acquiring_internal` with distance below `-10` to `stable` within 10 meters of the polygon edge.
- `acquire_from_far_pass` Places a radius-16 polygon at `(6,-121)`, 51 meters east of the starting position; passes only after `LOITER_MODE` changes from `acquiring_external` with `LOITER_ACQUIRE=1` and distance above 10 to `stable` with acquisition off and distance at most 10.
- `early_acquire_mode_pass` Uses the far polygon and samples the approach after six seconds; passes when the behavior reports `LOITER_MODE=acquiring_external`, `LOITER_ACQUIRE=1`, a nonnegative index, distance above 10, and no behavior error.
- `center_activate_pass` Holds the behavior idle for five seconds with `center_activate=true`, then deploys from `(-45,-121)`; live center macros must change from the configured `(12,-121)` to the activation position `(-45,-121)` before the deadline.
- `slip_radius_pass` Sets `capture_radius=0.5` and `slip_radius=12`, forcing the vehicle to pass a vertex outside the capture radius before the nonmonotonic rule accepts it; passes only when `LOITER_REPORT` reaches `nonmono_hits=1`.
- `speed_alt_update_pass` Starts with desired speed `0.9`, configures `speed_alt=2.2`, and posts `use_alt_speed=true`; passes only after `DESIRED_SPEED` is observed first in `0.85–0.95` and then in `2.15–2.25`.
- `use_alt_speed_static_pass` Configures `speed=0.8`, `speed_alt=2.8`, and `use_alt_speed=true`; passes when the behavior publishes `DESIRED_SPEED` in `2.75–2.85` before the missing-speed deadline.
- `center_assign_xy_pass` Posts `center_assign=x=18,y=-121`, exercising keyed runtime-center syntax; passes when live center macros report exactly `(18,-121)`.
- `center_assign_pair_pass` Starts at center `(2,-121)` and posts `center_assign=32,-121`, exercising unkeyed pair syntax and a 30-meter shift; passes when live center macros report exactly `(32,-121)`.
- `xcenter_ycenter_update_pass` Posts `xcenter_assign=18` and `ycenter_assign=-121` separately; passes when the combined live center becomes exactly `(18,-121)`.
- `mod_poly_rad_expand_pass` Posts `mod_poly_rad=6` against the snapped stock radius `18.385`; passes when the live radius becomes `24.38–24.39`.
- `mod_poly_rad_shrink_pass` Posts `mod_poly_rad=-5` against the snapped stock radius `18.385`; passes when the live radius becomes `13.38–13.39`.
- `slingshot_bearing_pass` Sets `slingshot=5`, testing bearing-based completion after the vehicle accumulates the configured angular travel; passes at 35 seconds when `LOITER_DONE=true`, `LOITER_BNG_TOTAL>=5`, mode is stable, acquisition is off, and no behavior error occurred.
- `post_suffix_outputs_pass` Sets `post_suffix=custom`; passes when the suffixed report, index, mode, acquire, distance, and ETA variables drive stable-loiter evaluation and no corresponding unsuffixed publication is observed.
- `eta_output_pass` Uses the stock loiter to test ETA publication; passes at 25 seconds when the stable-loiter conditions hold and `LOITER_ETA_TO_POLY>=0`, while `LOITER_REPORT` is recorded but not graded.
- `slow_speed_acquire_pass` Sets `speed=0.25` and samples the approach after ten seconds; passes when the behavior reports `LOITER_MODE=acquiring_external`, `LOITER_ACQUIRE=1`, a nonnegative index, distance above 10, and no behavior error.
- `empty_polygon_fail` Omits the polygon while marking each behavior run; after a ten-second absence window, the expected-negative case passes when the helm remains in `DRIVE`, the run marker was seen, `LOITER_INDEX` remains `-1`, and no desired-speed or behavior-error mail was observed.
- `bad_polygon_fail` Supplies the self-intersecting sequence `{0,-121:20,-101:0,-101:20,-121}`; the expected-negative case passes only when the bridged helm state equals `MALCONFIG` before its deadline.
- `bad_update_fail` Posts that self-intersecting polygon through `LOITER_UPDATES`; passes when `IVPHELM_UPDATE_RESULT` identifies `bhv=loiter`, `var=LOITER_UPDATES`, and `result=param_error`, a warning is emitted, and the live polygon remains the stock eight-point center/radius geometry.
- `bad_clockwise_fail` Sets `clockwise=sideways`; the expected-negative case passes only when the bridged helm state equals `MALCONFIG` before its deadline.
- `bad_use_alt_speed_fail` Sets `use_alt_speed=maybe`; the expected-negative case passes only when the bridged helm state equals `MALCONFIG` before its deadline.
- `bad_patience_fail` Sets `patience=100`, one above the accepted 1–99 range; the expected-negative case passes only when the bridged helm state equals `MALCONFIG` before its deadline.
- `bad_capture_radius_fail` Sets `capture_radius=-1`; the expected-negative case passes only when the bridged helm state equals `MALCONFIG` before its deadline.
- `patience_low_pass` Sets the valid low-range value `patience=15`; passes when the behavior accepts the configuration and reaches stable loiter with a nonnegative index, acquisition off, distance at most 10, and no behavior error.
- `patience_high_pass` Sets the valid high-range value `patience=90`; passes when the behavior accepts the configuration and reaches stable loiter with a nonnegative index, acquisition off, distance at most 10, and no behavior error.

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

- July 20, 2026
- full minimal-logging matrix: `29/29` cases passed with `--jobs=4`,
  `--port_base=32000`, and `--port_stride=30`
- full-logging checks: `slip_radius_pass`, `bad_update_fail`, and
  `empty_polygon_fail`
- deliberate mutations confirmed clean failures for acquisition direction and
  phase, internal acquisition, live polygon shape/center/radius, alternate
  speed selection, nonmonotonic capture, suffixed-output absence, runtime
  update rejection, empty-spec objective suppression, and exact helm
  malconfiguration
- warp: `10`
