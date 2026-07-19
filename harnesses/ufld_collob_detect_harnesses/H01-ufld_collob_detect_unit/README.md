# H01-ufld_collob_detect_unit

Live `uFldCollObDetect` harness for vehicle/obstacle encounter reporting. Each
case runs in an isolated copy of the shared uField app stem, and `pMissionEval`
owns the substantive pass/fail verdict.

```sh
./zlaunch.sh --jobs=4 --port_base=4900 10
./zlaunch.sh --case=collision_flag_pass --port_base=4900 10
```

## Cases

- `collision_flag_pass` Defines square obstacle `obs` from `x=-5` to `5`, moves Alpha from 20 meters outside it to 0.5 meters from its edge and back out, and uses 1/5/15-meter collision, near-miss, and encounter thresholds; passes when the exact collision and encounter flags both report `alpha:obs:0.5:1`.
- `near_miss_flag_pass` Moves Alpha to 3 meters from the same obstacle edge and back outside the encounter range; passes when the exact near-miss and encounter flags both report `alpha:obs:3:1`.
- `encounter_only_flag_pass` Moves Alpha to 9 meters from the obstacle edge and back out; passes when `COD_ENCOUNTER=alpha:obs:9:1` and neither a near-miss nor collision flag is published.
- `global_min_distance_pass` Places Alpha at `x=12`, 7 meters from the right edge of the square obstacle; passes when `OB_GLOBAL_MIN=7` without requiring a completed encounter.
- `invalid_obstacle_absent_pass` Posts a non-convex polygon labeled `bad` and places Alpha inside its footprint; passes when no encounter, near-miss, collision, or global-minimum output is published.
- `obstacle_clear_blocks_later_pass` Posts `KNOWN_OBSTACLE_CLEAR=all` 4.6 seconds after creating `obs`, then places Alpha inside the former obstacle and moves it away; passes when no encounter flags or global-minimum distance are published, showing the obstacle older than the four-second clear threshold was removed.
- `upper_macro_flag_pass` Configures `collision_flag=COD_COLLISION_$UP_VNAME=$ID:$COLL_CNT` and completes Alpha's 0.5-meter collision with `obs`; passes when the expanded variable and value are exactly `COD_COLLISION_ALPHA=obs:1`.
- `fresh_clear_keeps_obstacle_pass` Posts `KNOWN_OBSTACLE_CLEAR=all` only 1.6 seconds after creating `obs`, then completes a 0.5-meter collision; passes when the exact collision and encounter flags show the obstacle survived the four-second clear-age guard.
- `encounter_double_flag_pass` Configures `encounter_flag=COD_ENCOUNTER_DIST=$DIST` and completes a 9-meter clear encounter; passes when `COD_ENCOUNTER_DIST` is posted as numeric value `9`.
- `near_miss_upper_macro_flag_pass` Configures `near_miss_flag=COD_NEAR_$UP_VNAME=$ID:$MISS_CNT` and completes Alpha's 3-meter near miss with `obs`; passes when the expanded variable and value are exactly `COD_NEAR_ALPHA=obs:1`.
- `collision_boundary_counts_near_pass` Completes an encounter whose minimum distance is exactly the 1-meter `collision_dist`; passes when near-miss and encounter flags report `alpha:obs:1:1` and no collision flag is published, demonstrating the collision comparison is strict `<`.
- `near_boundary_counts_encounter_pass` Completes an encounter whose minimum distance is exactly the 5-meter `near_miss_dist`; passes when the encounter flag reports `alpha:obs:5:1` and neither near-miss nor collision flags are published, demonstrating the near-miss comparison is strict `<`.
- `named_clear_only_removes_target_pass` Creates `obs1` near `x=0` and `obs2` near `x=40`, clears only `obs1` after both are old enough for deletion, then completes a 0.5-meter collision with `obs2`; passes when the exact collision and encounter flags still identify `obs2`.
- `range_normalization_collision_pass` Configures collision, near-miss, and encounter distances as `6`, `3`, and `4`, then moves Alpha to 5.5 meters from the obstacle and back out; passes when exact collision and encounter flags report `alpha:obs:5.5:1`, demonstrating the outer encounter distance was expanded enough to admit the collision-range event.

Logging is minimal by default and runs without `pLogger`. Use `--log=full` for
the complete matrix, or combine it with `--case=NAME` for one fully logged
diagnostic case.
