# H01-loiter_behavior_motion

Patch-driven moving correctness harness for
[`/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/loiter_behavior_missions/loiter_behavior_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/loiter_behavior_missions/loiter_behavior_motion).

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

- `radial_clockwise_pass`
  Baseline radial polygon, clockwise loiter, stable acquisition.
- `radial_counterclockwise_pass`
  Same geometry with `clockwise=false`.
- `clockwise_best_pass`
  Dynamic clockwise selection through `clockwise=best`.
- `polygon_box_pass`
  Explicit rectangular polygon input.
- `triangle_polygon_pass`
  Explicit triangular polygon input.
- `start_inside_loiter_pass`
  Starts inside the loiter area and stabilizes without a long approach.
- `acquire_from_far_pass`
  Starts well outside the polygon, then enters stable loiter.
- `early_acquire_mode_pass`
  Evaluates early and requires `LOITER_ACQUIRE=1` while outside the polygon.
- `center_activate_pass`
  Exercises `center_activate=true`, centering the loiter on activation.
- `capture_radius_large_pass`
  Uses a large capture radius while still stabilizing cleanly.
- `slip_radius_pass`
  Exercises nonmonotonic/slip-radius capture behavior.
- `speed_alt_update_pass`
  Starts slow, then enables alternate speed through `LOITER_UPDATES`.
- `use_alt_speed_static_pass`
  Uses static `use_alt_speed=true`; without the alternate speed the case would
  not reach stable loiter by the evaluation gate.
- `center_assign_xy_pass`
  Updates the center through `xcenter_assign` and `ycenter_assign`.
- `center_assign_pair_pass`
  Updates the center through combined `center_assign`.
- `xcenter_ycenter_update_pass`
  Updates the polygon center through runtime x/y center mail.
- `mod_poly_rad_expand_pass`
  Runtime polygon-radius expansion.
- `mod_poly_rad_shrink_pass`
  Runtime polygon-radius shrink.
- `slingshot_bearing_pass`
  Exercises slingshot completion path and grades on accumulated bearing.
- `post_suffix_outputs_pass`
  Uses `post_suffix` and verifies suffixed status outputs.
- `eta_output_pass`
  Grades on `LOITER_ETA_TO_POLY` and records `LOITER_REPORT`.
- `ipf_zaic_spd_pass`
  Exercises the ZAIC speed IvP function path.
- `slow_speed_acquire_pass`
  Evaluates while a deliberately slow vehicle is still acquiring.
- `empty_polygon_fail`
  Empty polygon configuration is expected to fail.
- `bad_polygon_fail`
  Malformed polygon configuration is expected to fail.
- `bad_update_fail`
  Malformed runtime update is expected to fail.
- `center_bad_update_recover_pass`
  Bad center update followed by valid recovery update.
- `spiral_factor_pass`
  Nondefault `spiral_factor`.
- `patience_low_pass`
  Low `patience`.
- `patience_high_pass`
  High `patience`.

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=radial_clockwise_pass 10
./zlaunch.sh --case=center_activate_pass --gui 1
./zlaunch.sh --case=eta_output_pass --gui 1
./zlaunch.sh --just_make 10
./zlaunch.sh --jobs=4 10
./zlaunch.sh --jobs=4 --port_base=9700 10
```

Results are written to `results.txt` with the mission-owned `grade` and the
loiter status columns used for grading. Wave mode runs the full matrix in
isolated temporary mission copies. Each case gets its own MOOSDB and pShare
port block derived from `--port_base`, with pShare ports offset above the
MOOSDB range. The default is serial `--jobs=1`; use `--keep_workdirs` when
preserving temporary case folders is useful for debugging.

Latest validation:

- April 24, 2026
- full wave matrix: `30/30` expected outcomes matched with `--jobs=4`
- added cases also passed as individual live runs
- warp: `10`
