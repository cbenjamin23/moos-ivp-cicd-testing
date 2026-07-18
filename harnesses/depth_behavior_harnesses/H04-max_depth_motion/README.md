# H04-max_depth_motion

Patch-driven moving correctness harness for `BHV_MaxDepth`.

The harness reuses
`missions/depth_behavior_missions/depth_behavior_motion`. Cases command unsafe
deeper targets while max-depth constraints should keep the UUV at or above the
allowed depth boundary.

## Current Matrix

- `max_depth_guard_pass` Commands an unsafe deeper target while `BHV_MaxDepth` constrains the vehicle near the guard depth.
- `max_depth_zero_guard_pass` Commands a dive while the guard clamps the allowed depth to the surface boundary.
- `max_depth_negative_clip_pass` Supplies a negative guard depth and verifies clipping to the surface boundary.
- `max_depth_tight_tolerance_pass` Uses a tight tolerance and verifies the vehicle remains near the constrained depth.
- `max_depth_basewidth_alias_pass` Uses the `basewidth` parameter alias and verifies it constrains the over-depth command like `tolerance`.
- `max_depth_no_slack_var_pass` Omits the optional slack telemetry variable and verifies the guard still constrains maximum depth.
- `max_depth_unconstrained_shallow_pass` Commands a target shallower than the max-depth limit and verifies the guard does not interfere.
- `max_depth_zero_tolerance_pass` Uses exact zero tolerance and verifies the max-depth guard remains stable at the boundary.
- `max_depth_bad_config_fail` Supplies malformed max-depth config and expects the normal good-case verdict to fail.
- `max_depth_bad_tolerance_fail` Supplies malformed tolerance config and expects the normal good-case verdict to fail.
- `max_depth_bad_basewidth_fail` Supplies malformed basewidth config and expects the normal good-case verdict to fail.
- `max_depth_bad_slack_var_fail` Supplies a whitespace-bearing depth-slack variable name and expects the normal good-case verdict to fail.
- `max_depth_bad_ascent_field_fail` Supplies unsupported ascent fields from legacy mission style and expects helm configuration failure.
- `max_depth_missing_nav_depth_fail` Removes usable ownship depth mail and expects the max-depth verdict to fail.
- `max_depth_domain_missing_fail` Removes the helm depth domain and expects the max-depth verdict to fail.

## Running

```bash
./zlaunch.sh --case=max_depth_guard_pass 10
./zlaunch.sh --jobs=3 --port_base=44000 10
./zlaunch.sh --just_make 10
```

The Bash 5.1 wrapper creates an isolated mission copy for every serial and
rolling case, refills rolling slots as cases finish, aggregates one strict
mission-owned result row per selected case, and performs root-scoped cleanup.

### Migration evidence

Three untouched legacy `--jobs=4` matrices passed 45/45 rows in 49.27, 50.73,
and 54.56 seconds, for a 51.52-second mean. The untouched serial matrix passed
15/15 in 131.17 seconds. No baseline outlier occurred.

The migration changed only the harness launcher. All fifteen case overlays,
stimuli, thresholds, and pMissionEval conditions are unchanged, and the shared
depth mission stem was not modified. Existing mission-owned verdicts cover the
nominal boundary guards, aliases, optional telemetry, unconstrained behavior,
expected helm malconfiguration, missing navigation input, and missing-domain
behavior error.

Three migrated rolling matrices passed 45/45 rows in 54.58, 45.62, and 49.26
seconds. Their 49.82-second mean is 1.70 seconds, about 3.3 percent, faster than
the legacy mean and within normal run-to-run variation. The isolated serial
matrix passed 15/15 in 158.67 seconds, 27.50 seconds or about 21.0 percent
slower than legacy, roughly 1.83 seconds per case for isolated copying,
xlaunch lifecycle, and verified scoped cleanup.

Validation covered all-case generation, three full rolling matrices, one full
serial matrix, exact README/case/patch-map reconciliation, rolling refill, 30
unique MOOSDB ports, 30 unique pShare ports, unknown-case rejection,
active-lock behavior, Homebrew Bash re-execution, and explicit Bash 3.2
rejection. Bash syntax, ShellCheck, the harness checker, and all fifteen
generated-case evaluator checks pass. No tested MOOS process survived cleanup.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case.
