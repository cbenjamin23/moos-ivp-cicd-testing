# H01-ufld_collob_detect_unit

Live `uFldCollObDetect` harness for vehicle/obstacle encounter reporting. Each
case runs in an isolated copy of the shared uField app stem, and `pMissionEval`
owns the substantive pass/fail verdict.

```sh
./zlaunch.sh --jobs=4 --port_base=4900 10
./zlaunch.sh --case=collision_flag_pass --port_base=4900 10
```

## Cases

- `collision_flag_pass` Verifies a vehicle crossing through a known obstacle posts collision and encounter flags.
- `near_miss_flag_pass` Verifies an obstacle pass inside the near-miss band posts near-miss and encounter flags.
- `encounter_only_flag_pass` Verifies a wider obstacle pass posts an encounter flag without near-miss or collision flags.
- `global_min_distance_pass` Verifies `OB_GLOBAL_MIN` tracks the closest vehicle-to-obstacle distance.
- `invalid_obstacle_absent_pass` Verifies a non-convex obstacle spec is rejected without distance or flag output.
- `obstacle_clear_blocks_later_pass` Verifies an aged `KNOWN_OBSTACLE_CLEAR=all` removes the obstacle before later node reports.
- `upper_macro_flag_pass` Verifies `$UP_VNAME`, `$ID`, and count macros expand in collision flag output.
- `fresh_clear_keeps_obstacle_pass` Verifies a too-fresh obstacle clear is ignored and the obstacle remains active.
- `encounter_double_flag_pass` Verifies `$DIST` can be posted as a numeric encounter flag value.
- `near_miss_upper_macro_flag_pass` Verifies near-miss flags expand uppercase vehicle-name and count macros.
- `collision_boundary_counts_near_pass` Verifies a distance exactly on `collision_dist` is treated as near-miss, not collision.
- `near_boundary_counts_encounter_pass` Verifies a distance exactly on `near_miss_dist` is treated as encounter-only.
- `named_clear_only_removes_target_pass` Verifies a named obstacle clear removes only the requested obstacle and leaves another obstacle active.
- `range_normalization_collision_pass` Verifies invalid obstacle range ordering is normalized so collision-distance encounters remain reportable.
