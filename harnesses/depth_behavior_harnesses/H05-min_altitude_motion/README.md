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
- `min_altitude_zero_altitude_fail` Sets zero simulated altitude while commanding a dive, and passes when the mission observes zero-depth rejection evidence and a behavior error.
- `min_altitude_bad_config_fail` Supplies malformed minimum-altitude config, and passes when the helm reports `MALCONFIG`.
- `min_altitude_negative_min_fail` Supplies a negative minimum altitude, and passes when the helm reports `MALCONFIG`.
- `min_altitude_bad_critical_bool_fail` Supplies an invalid `missing_altitude_critical` value, and passes when the helm reports `MALCONFIG`.
- `min_altitude_missing_nav_fail` Removes usable ownship depth/altitude mail, and passes when depth/nav data stays absent and the vehicle does not command elevator authority.
- `min_altitude_noncritical_missing_altitude_fail` Sets `missing_altitude_critical=false` while depth/altitude mail is unavailable, and passes when the current no-depth-data failure path remains caught.

## Running

```bash
./zlaunch.sh --case=min_altitude_guard_pass 10
./zlaunch.sh --jobs=3 --port_base=45000 --port_stride=12 10
./zlaunch.sh --just_make 10
```

The Bash 5.1 wrapper creates an isolated mission copy for every serial and
rolling case, refills rolling slots as cases finish, aggregates one strict
mission-owned result row per selected case, and performs root-scoped cleanup.

### Migration evidence

Two clean untouched legacy `--jobs=4` matrices passed 26/26 rows in 59.42 and
52.89 seconds, for a 56.16-second mean. A third untouched matrix finished in
51.83 seconds but sampled `min_altitude_low_threshold_pass` before its required
near-bottom state; that unchanged case passed 9/10 focused legacy repetitions.
The untouched serial matrix passed 13/13 in 118.51 seconds.

Three timing-sensitive positive cases now use existing navigation state as
their pMissionEval trigger, with the existing timer moved to 75 mission seconds
as a failure deadline. `zero_min_pass` waits for depth at least 16 and x at
least 45; `low_threshold_pass` waits for altitude at most 4 and x at least 45;
and `clearance_boundary_pass` waits for depth at least 10 and x at least 45.
Their original pass windows, behavior parameters, stimuli, and error checks are
unchanged. Each final form passed 10/10 focused repetitions.

The simple intermediate idea of moving the low-threshold snapshot from 45 to
60 seconds was rejected after it passed only 8/10. A depth-only boundary
trigger was also rejected because it could fire before the original x
requirement. These failed experiments are not present in the final design.

The unconstrained-deep-bottom case sometimes reports its existing sticky
`bhv_error=true` field while still passing because that field has never been a
pass condition for this case. It did so in every legacy baseline matrix but
not every migrated run. The migration leaves that pre-existing coverage choice
unchanged for later case-quality review.

After the three repairs, three rolling matrices passed 39/39 rows in 43.86,
44.24, and 42.76 seconds, for a 43.62-second mean. The final consistency edit
added the already-required x condition to the low-threshold trigger; an exact
final-source rolling confirmation passed 13/13 in 43.55 seconds. The exact
final-source serial matrix passed 13/13 in 132.98 seconds, 14.47 seconds or
about 12.2 percent slower than legacy, roughly 1.11 seconds per case for
isolated copying, xlaunch lifecycle, and verified scoped cleanup.

Validation covered final-source generation and live execution, focused sweeps,
full rolling and serial matrices, exact README/case/patch-map reconciliation,
rolling refill, 26 unique MOOSDB ports, 26 unique pShare ports, unknown-case
rejection, active-lock behavior, Homebrew Bash re-execution, and explicit Bash
3.2 rejection. Bash syntax, ShellCheck, the harness checker, and all thirteen
generated-case evaluator checks pass. No tested MOOS process survived cleanup,
and the shared depth stem was not changed.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case.
