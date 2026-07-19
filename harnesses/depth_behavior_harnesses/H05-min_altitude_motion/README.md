# H05-min_altitude_motion

Patch-driven moving correctness harness for `BHV_MinAltitudeX`.

The harness reuses
`missions/depth_behavior_missions/depth_behavior_motion`. Cases vary simulated
bottom depth and altitude clearance requirements while a lower-priority
constant-depth behavior commands unsafe deeper targets.

## Current Matrix

- `min_altitude_guard_pass` Opposes a 30-meter depth command over a 20-meter bottom with `min_altitude=8`, testing that the computed 12-meter maximum-depth objective stops the dive; passes at `x>=80` when `NAV_DEPTH<=16`, `NAV_ALTITUDE>=4`, and no behavior error is observed.
- `min_altitude_shallow_bottom_pass` Opposes a 30-meter command over a six-meter bottom with `min_altitude=10`, testing that a negative computed depth limit is clipped to the surface; passes at `x>=80` when `NAV_DEPTH<=2`, `NAV_ALTITUDE>=4`, and no behavior error is observed.
- `min_altitude_zero_min_pass` Commands 30 meters over a 20-meter bottom with `min_altitude=0`, testing that the boundary setting permits descent near the bottom rather than retaining the ordinary eight-meter guard; passes when the vehicle reaches `x>=45`, `NAV_DEPTH>=16`, and `NAV_ALTITUDE<=4`, with 75 seconds used only as a missing-state deadline.
- `min_altitude_unconstrained_deep_bottom_pass` Commands 22 meters over an 80-meter bottom with `min_altitude=8`, testing that the one-sided constraint does not disturb a target far above the computed 72-meter limit; passes when `x>=100`, `NAV_DEPTH>=20`, and `NAV_ALTITUDE>=40`, with 60 seconds used only as a missing-state deadline.
- `min_altitude_clearance_boundary_pass` Commands 30 meters over a 20-meter bottom with `min_altitude=8`, testing control near the computed 12-meter depth boundary; passes when `x>=45` and `NAV_DEPTH>=10`, with depth constrained to 10–14 meters, altitude to 6–10 meters, and 75 seconds used only as a missing-state deadline.
- `min_altitude_zero_altitude_fail` Configures a zero-meter water depth while commanding 30 meters with `min_altitude=8`, testing the behavior's no-objective path when reported altitude is zero; passes when `ABE_BHV_ERROR` is observed with `NAV_DEPTH<1`, with 45 seconds used only as a missing-error deadline.
- `min_altitude_bad_config_fail` Sets `min_altitude=deep`, testing rejection of a nonnumeric clearance; passes only when the helm reports the exact state `IVPHELM_STATE=MALCONFIG` before the 12-second missing-state deadline.
- `min_altitude_negative_min_fail` Sets `min_altitude=-1`, testing rejection of a negative clearance; passes only when the helm reports the exact state `IVPHELM_STATE=MALCONFIG` before the 12-second missing-state deadline.
- `min_altitude_bad_critical_bool_fail` Sets `missing_altitude_critical=maybe`, testing rejection of an invalid Boolean; passes only when the helm reports the exact state `IVPHELM_STATE=MALCONFIG` before the 12-second missing-state deadline.
- `min_altitude_missing_nav_fail` Changes the simulator prefix to `SIM`, withholding `NAV_X` and `NAV_DEPTH` from the helm; passes at the 45-second absence deadline when both variables and depth-mismatch mail remain absent, `DESIRED_ELEVATOR=0`, and no behavior error is observed.

## Running

```bash
./zlaunch.sh --case=min_altitude_guard_pass 10
./zlaunch.sh --jobs=3 --port_base=45000 --port_stride=12 10
./zlaunch.sh --just_make 10
```

The Bash 5.1 wrapper creates an isolated mission copy for every serial and
rolling case, refills rolling slots as cases finish, aggregates one strict
mission-owned result row per selected case, and performs root-scoped cleanup.

## Validation

The 10-case live matrix passes with ordinary motion cases evaluated on vehicle
state, malformed cases evaluated on the exact helm state, and timers retained
only as missing-state or missing-input deadlines. Direct
`BHVMinAltitudeXTest` coverage computes a known 20-meter bottom and proves that
`min_altitude=1` places the utility drop between depths 19 and 20; changing the
minimum to zero fails that assertion.

Replacing malformed `missing_altitude_critical=maybe` with valid `true` reaches
`DRIVE` and fails the exact-state evaluator. The former one-meter motion case
was removed because its checkpoint varied between roughly one and five meters
with controller phase and could not prove the utility boundary. Both
noncritical live cases were also removed: the flag is irrelevant while altitude
mail is present, and the current upstream implementation posts the same error
when altitude is absent for either Boolean value. That upstream discrepancy is
tracked in the repository review plan while the direct test records current
behavior.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case.
