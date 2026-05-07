# H05-min_altitude_motion

Patch-driven moving correctness harness for `BHV_MinAltitudeX`.

The harness reuses
`missions/depth_behavior_missions/depth_behavior_motion`. Cases vary simulated
bottom depth and altitude clearance requirements while a lower-priority
constant-depth behavior commands unsafe deeper targets.

## Current Matrix

- `min_altitude_guard_pass` Sets a shallow simulated bottom and requires the behavior to preserve bottom clearance.
- `min_altitude_shallow_bottom_pass` Sets clearance tighter than water depth and verifies the behavior clamps the allowed depth near the surface.
- `min_altitude_zero_min_pass` Sets zero minimum altitude and verifies the behavior allows the deeper target down to the bottom estimate.
- `min_altitude_low_threshold_pass` Sets a low positive clearance and verifies the behavior allows a near-bottom but nonzero altitude.
- `min_altitude_noncritical_available_pass` Sets `missing_altitude_critical=false` with valid altitude mail and verifies normal clearance guarding still works.
- `min_altitude_unconstrained_deep_bottom_pass` Uses ample simulated water depth and verifies the clearance guard does not interfere with a normal deep target.
- `min_altitude_clearance_boundary_pass` Commands past the bottom-clearance limit and verifies the vehicle settles near the computed clearance boundary.
- `min_altitude_zero_altitude_fail` Sets zero simulated altitude while commanding a dive and expects the normal clearance verdict to fail.
- `min_altitude_bad_config_fail` Supplies malformed minimum-altitude config and expects the normal good-case verdict to fail.
- `min_altitude_negative_min_fail` Supplies a negative minimum altitude and expects the normal good-case verdict to fail.
- `min_altitude_bad_critical_bool_fail` Supplies an invalid `missing_altitude_critical` value and expects the normal good-case verdict to fail.
- `min_altitude_missing_nav_fail` Removes usable ownship depth/altitude mail and expects the clearance verdict to fail.
- `min_altitude_noncritical_missing_altitude_fail` Sets `missing_altitude_critical=false` while depth/altitude mail is unavailable and verifies the current failure path remains caught.

## Running

```bash
./zlaunch.sh --case=min_altitude_guard_pass 10
./zlaunch.sh --jobs=3 --port_base=45000 10
./zlaunch.sh --just_make 10
```

The wrapper supports `--jobs` and `--port_base` for isolated grouped local and
CI runs.
