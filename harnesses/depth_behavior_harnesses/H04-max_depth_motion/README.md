# H04-max_depth_motion

Patch-driven moving correctness harness for `BHV_MaxDepth`.

The harness reuses
`missions/depth_behavior_missions/depth_behavior_motion`. Cases command unsafe
deeper targets while max-depth constraints should keep the UUV at or above the
allowed depth boundary.

## Current Matrix

- `max_depth_guard_pass` Commands 30 meters while setting `max_depth=12` and `tolerance=4`. Passes after `x>=45` when `NAV_DEPTH<=16`, `DEPTH_SLACK>=-4`, and no behavior error is observed.
- `max_depth_zero_guard_pass` Commands 20 meters while setting `max_depth=0` and `tolerance=2`. Passes after `x>=45` when `NAV_DEPTH<=2`, `DEPTH_SLACK>=-2`, and no behavior error is observed.
- `max_depth_negative_clip_pass` Commands 20 meters while configuring `max_depth=-5` to test clamping the guard to the surface. Passes after `x>=45` when `NAV_DEPTH<=2`, `DEPTH_SLACK>=-2`, and no behavior error is observed.
- `max_depth_tight_tolerance_pass` Commands 30 meters while setting `max_depth=10` and `tolerance=1` to exercise a narrow guard transition. Passes after `x>=45` when `NAV_DEPTH<=12`, `DEPTH_SLACK>=-2`, and no behavior error is observed.
- `max_depth_basewidth_alias_pass` Commands 30 meters while using `basewidth=4` as the alias for the 12-meter guard tolerance. Passes after `x>=45` when `NAV_DEPTH<=16`, `DEPTH_SLACK>=-4`, and no behavior error is observed.
- `max_depth_no_slack_var_pass` Commands 30 meters with a 12-meter guard but omits the optional `depth_slack_var`. Passes after `x>=45` when `NAV_DEPTH<=16` and no behavior error is observed.
- `max_depth_unconstrained_shallow_pass` Commands eight meters below a `max_depth=20` guard to exercise the inactive side of the constraint. Passes after `x>=45` when `6<=NAV_DEPTH<=10`, `DEPTH_SLACK>=8`, and no behavior error is observed.
- `max_depth_zero_tolerance_pass` Commands 30 meters while setting a 12-meter guard with `tolerance=0`. Passes after `x>=45` when `NAV_DEPTH<=14`, `DEPTH_SLACK>=-2`, and no behavior error is observed.
- `max_depth_bad_config_fail` Sets `max_depth=deep` to exercise rejection of a nonnumeric guard depth. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `max_depth_bad_tolerance_fail` Sets `tolerance=wide` to exercise rejection of a nonnumeric tolerance. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `max_depth_bad_basewidth_fail` Sets the `basewidth` alias to `wide` to exercise rejection of a nonnumeric tolerance alias. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `max_depth_bad_slack_var_fail` Sets `depth_slack_var=BAD VAR` to exercise rejection of a MOOS variable name containing whitespace. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `max_depth_bad_ascent_field_fail` Adds unsupported `ascent_speed=1.0` and `ascent_grade=fullspeed` fields to an otherwise valid max-depth guard. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `max_depth_missing_nav_depth_fail` Changes the simulator output prefix to `SIM`, leaving the helm without `NAV_X` or `NAV_DEPTH`. The harness passes at 45 seconds when both navigation variables remain absent, `DESIRED_ELEVATOR=0`, and no behavior error is observed.
- `max_depth_domain_missing_fail` Removes `depth` from the IvP helm domain while retaining the max-depth behavior. The harness passes when `ABE_BHV_ERROR` is observed.

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
unique MOOSDB ports, 30 unique pShare ports, unknown-case rejection, Homebrew
Bash re-execution, and explicit Bash 3.2
rejection. Bash syntax, ShellCheck, the harness checker, and all fifteen
generated-case evaluator checks pass. No tested MOOS process survived cleanup.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case.
