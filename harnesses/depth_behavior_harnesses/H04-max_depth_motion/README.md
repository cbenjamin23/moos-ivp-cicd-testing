# H04-max_depth_motion

Patch-driven moving correctness harness for `BHV_MaxDepth`.

The harness reuses
`missions/depth_behavior_missions/depth_behavior_motion`. Cases command unsafe
deeper targets while max-depth constraints should keep the UUV at or above the
allowed depth boundary.

## Current Matrix

- `max_depth_guard_pass` Opposes a 30-meter constant-depth command with `max_depth=12,tolerance=4`, testing that the higher-priority max-depth objective stops the dive; passes at `x>=80` when `NAV_DEPTH<=16`, `DEPTH_SLACK>=-4`, and no behavior error is observed.
- `max_depth_zero_guard_pass` Opposes a 20-meter command with `max_depth=0,tolerance=2`, testing that a guard at the domain minimum can hold the vehicle at the surface; passes at `x>=80` when `NAV_DEPTH<=2`, `DEPTH_SLACK>=-2`, and no behavior error is observed.
- `max_depth_negative_clip_pass` Configures `max_depth=-5` against a 20-meter command, testing that the negative boundary is accepted and clipped to zero; passes at `x>=80` when `NAV_DEPTH<=2`, `DEPTH_SLACK>=-2`, and no behavior error is observed.
- `max_depth_basewidth_alias_pass` Uses nondefault `basewidth=1` against a 30-meter command, testing that the alias creates a narrow max-depth guard rather than retaining the 100-meter default; passes at `x>=80` when `NAV_DEPTH<=13.25`, `DEPTH_SLACK>=-1.25`, and no behavior error is observed.
- `max_depth_no_slack_var_pass` Omits the optional `depth_slack_var` from the ordinary 12-meter guard, testing that telemetry configuration is not required to build and apply the constraint; passes at `x>=80` when `NAV_DEPTH<=16` and no behavior error is observed.
- `max_depth_unconstrained_shallow_pass` Commands eight meters while setting `max_depth=20`, testing that the one-sided constraint does not pull an already-shallow target toward its boundary; passes at `x>=80` when `6<=NAV_DEPTH<=10`, `DEPTH_SLACK>=8`, and no behavior error is observed.
- `max_depth_bad_config_fail` Sets `max_depth=deep`, testing rejection of a nonnumeric guard depth; passes only when the helm reports the exact state `IVPHELM_STATE=MALCONFIG` before the 12-second missing-state deadline.
- `max_depth_bad_tolerance_fail` Sets `tolerance=wide`, testing rejection of a nonnumeric tolerance; passes only when the helm reports the exact state `IVPHELM_STATE=MALCONFIG` before the 12-second missing-state deadline.
- `max_depth_bad_basewidth_fail` Sets `basewidth=wide`, testing rejection of a nonnumeric alias value; passes only when the helm reports the exact state `IVPHELM_STATE=MALCONFIG` before the 12-second missing-state deadline.
- `max_depth_bad_slack_var_fail` Sets `depth_slack_var=BAD VAR`, testing rejection of a MOOS variable name containing whitespace; passes only when the helm reports the exact state `IVPHELM_STATE=MALCONFIG` before the 12-second missing-state deadline.
- `max_depth_bad_ascent_field_fail` Adds unsupported `ascent_speed` and `ascent_grade` fields to an otherwise valid guard, testing that unimplemented configuration is rejected rather than ignored; passes only when the helm reports the exact state `IVPHELM_STATE=MALCONFIG` before the 20-second missing-state deadline.
- `max_depth_missing_nav_depth_fail` Changes the simulator prefix to `SIM`, withholding `NAV_X` and `NAV_DEPTH` from the helm; passes at the 45-second absence deadline when navigation and depth-mismatch mail remain absent, `DESIRED_ELEVATOR=0`, and no behavior error is observed.
- `max_depth_domain_missing_fail` Removes `depth` from the helm domain while retaining `BHV_MaxDepth`, testing its explicit missing-domain error path; passes when `ABE_BHV_ERROR` is observed.

## Running

```bash
./zlaunch.sh --case=max_depth_guard_pass 10
./zlaunch.sh --jobs=3 --port_base=44000 10
./zlaunch.sh --just_make 10
```

The Bash 5.1 wrapper creates an isolated mission copy for every serial and
rolling case, refills rolling slots as cases finish, aggregates one strict
mission-owned result row per selected case, and performs root-scoped cleanup.

## Validation

The 13-case live matrix passes with motion cases evaluated on the `x>=80`
transit state, malformed cases evaluated on the exact helm state, and timers
retained only as missing-state or missing-input deadlines. Direct
`BHVMaxDepthTest` coverage uses a 0.5-meter domain to prove the one-meter
tolerance slope, the zero-tolerance step, and point-for-point equality of
`basewidth` and `tolerance`; those objective-shape checks are intentionally not
inferred from vehicle motion.

Deliberate mutations confirm that replacing `basewidth=1` with the 100-meter
default fails the alias motion case and replacing malformed `basewidth=wide`
with a valid value reaches `DRIVE` and fails the exact-state evaluator. The
former live tight- and zero-tolerance cases were removed because the one-meter
helm domain made the zero and one-meter curves identical and a wider-tolerance
mutation still passed the motion checkpoint.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case.
