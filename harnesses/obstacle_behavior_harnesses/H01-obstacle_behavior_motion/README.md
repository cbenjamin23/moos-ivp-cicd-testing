# H01-obstacle_behavior_motion

Logging is minimal by default in both communities. Use `--log=full` for the
whole matrix or with `--case=NAME` for one diagnostic case.
The two-obstacle ANTLER replacement has separate minimal and full patches.

Patch-driven moving correctness harness for
[`missions/obstacle_behavior_missions/obstacle_behavior_motion`](../../../missions/obstacle_behavior_missions/obstacle_behavior_motion).

This harness focuses on `BHV_AvoidObstacleV24` itself rather than re-testing
`pObstacleMgr`. The stem mission keeps obstacle-manager input deterministic,
but the grading logic is behavior-owned:

- did `BHV_AvoidObstacleV24` auto-request `OBM_ALERT_REQUEST`
- did it actually receive `OBSTACLE_ALERT`
- did it post the exact configured range or CPA payload at the source-defined event
- what `$[RNG]` or `$[CPA]` value caused that event
- did it run to completion and let the transit finish cleanly

The stem keeps the same lower-band MIT_SP framing as the other motion suites:
the ownship starts at `(0,-60)` and drives east along a short corridor to
`(70,-60)`.

## Current Matrix

- `default_auto_request_pass` Posts the lane-blocking polygon `ob_lane` through `KNOWN_OBSTACLE`, testing the spawned behavior's automatic alert request and one-obstacle lifecycle; passes at 68 seconds when alert-request and obstacle-alert mail have appeared, `OBAVOIDING=end`, `ARRIVED=true`, and collisions remain zero.
- `rng_flag_pass` Adds paired `<20` range flags that post `AVOID_RANGE_SEEN=true` and the live `$[RNG]`; the first publication must carry `true` with range below 20 meters, and the vehicle must subsequently arrive collision-free.
- `cpa_flag_pass` Adds paired CPA flags that post `AVOID_CPA_SEEN=true` and the event's `$[CPA]`; passes when the source-defined close-then-open CPA event reports `true`, a minimum range below the 28-meter relevance boundary, prior request and alert delivery, and zero collisions.
- `offlane_no_engagement_pass` Moves the obstacle to the box `(32,-8)`–`(44,4)`, well north of the eastbound lane, testing that automatic alert requests do not force engagement with an irrelevant obstacle; passes when a request was seen, no obstacle alert was ever seen, the vehicle arrives collision-free, and the final `OBAVOIDING` value is not `active`, `running`, or `end`.
- `no_refinery_pass` Sets `use_refinery=false`, selecting the direct `OF_Reflector` path without refinery plateau/basin regions; this retained compatibility smoke test proves the setting is accepted and produces a collision-free arrival, but no published telemetry distinguishes the internal construction branch.
- `two_obstacles_clean_pass` Posts `ob_one` near `(19,-66)` at one second and `ob_two` near `(55,-57)` at two seconds, lowers transit speed to 1.6, and adds `spawnxflag=OBSTACLE_SPAWNED=$[OID]`, testing two independent spawned avoiders; passes with an alert request, at least two `OBSTACLE_ALERT` deliveries, final spawn ID `ob_two`, arrival, and zero collisions.
- `static_polygon_pass` Configures the lane box directly with the behavior's `polygon` parameter instead of using `OBSTACLE_ALERT` templating, testing a launch-time obstacle; passes when the static behavior reaches `OBAVOIDING=end`, the vehicle arrives, and collisions remain zero.
- `poly_alias_pass` Configures the same launch-time lane box through the `poly` alias, testing equivalence with `polygon`; passes when the behavior reaches `OBAVOIDING=end`, the vehicle arrives, and collisions remain zero.
- `rng_flag_no_threshold_pass` Adds the same payload and `$[RNG]` flags without a threshold; the first publication must occur above 20 meters with payload `true`, distinguishing this unconditional form from the `<20` case before collision-free arrival.
- `avoid_disabled_fail` Changes the spawned behavior condition to `AVOID=false` while `AVOID` remains initialized true, testing the no-avoidance outcome after normal request and alert delivery; the harness passes when the vehicle still arrives with exactly one encounter and one collision and `OBAVOIDING=end`.
- `bad_polygon_fail` Supplies the crossed polygon `{29,-53:43,-67:29,-67:43,-53}`; the expected-negative case passes only when the bridged helm state equals `MALCONFIG` before its deadline.
- `bad_pwt_inner_dist_fail` Sets `pwt_inner_dist=40` above `pwt_outer_dist=28` while raising `completed_dist` to 42; the expected-negative case requires exact helm `MALCONFIG`.
- `bad_pwt_outer_dist_fail` Sets `pwt_outer_dist=8` below `pwt_inner_dist=14`; the expected-negative case requires exact helm `MALCONFIG`.
- `bad_completed_dist_fail` Sets `completed_dist=20` below `pwt_outer_dist=28`; the expected-negative case requires exact helm `MALCONFIG`.
- `bad_min_util_cpa_dist_fail` Sets `min_util_cpa_dist=20` above `max_util_cpa_dist=16`; the expected-negative case requires exact helm `MALCONFIG`.
- `bad_max_util_cpa_dist_fail` Sets `max_util_cpa_dist=6` below `min_util_cpa_dist=8`; the expected-negative case requires exact helm `MALCONFIG`.
- `bad_allowable_ttc_fail` Sets `allowable_ttc=-1`; the expected-negative case requires exact helm `MALCONFIG`.
- `bad_rng_flag_fail` Sets `rng_flag=<bogus AVOID_RANGE_SEEN=true`, supplying a nonnumeric threshold; the expected-negative case requires exact helm `MALCONFIG`.
- `bad_cpa_flag_fail` Sets `cpa_flag=AVOID_CPA_SEEN` without an assignment value; the expected-negative case requires exact helm `MALCONFIG`.
- `bad_use_refinery_fail` Sets `use_refinery=maybe`; the expected-negative case requires exact helm `MALCONFIG`.
- `bad_pwt_grade_fail` supplies the unsupported `pwt_grade=quadratic`; the expected-negative case requires exact helm `MALCONFIG`.

## Results Lines

Thresholded range-flag pass line:

```text
case=rng_flag_pass  grade=pass  form=obstacle_behavior_motion_tests  mmod=rng_flag_pass  deadline=false  obavoiding=inactive  alert_req_seen=true  obstacle_alert_seen=true  range_flag=true  range=19.661  arrived=true  collisions=0  mhash=[...]
```

Typical expected-negative line:

```text
case=avoid_disabled_fail  grade=pass  form=obstacle_behavior_motion_tests  mmod=avoid_disabled_fail  eval=true  obavoiding=end  alert_req_seen=true  obstacle_alert_seen=true  arrived=true  collisions=1  near_misses=0  encounters=1  mhash=[...]
```

Typical bad-config expected-negative line:

```text
case=bad_polygon_fail  grade=pass  form=obstacle_behavior_motion_tests  mmod=bad_config_fail  eval=MALCONFIG  expected=helm_malconfig  armed=true  deadline=false  helm_state=MALCONFIG  mhash=[...]
```

Field anatomy:

- `obavoiding`: behavior-owned lifecycle flag from `BHV_AvoidObstacleV24`
- `alert_req_seen`: whether the behavior posted `OBM_ALERT_REQUEST`
- `obstacle_alert_seen`: whether the behavior actually received `OBSTACLE_ALERT`
- `obstacle_alert_count`: the number of `OBSTACLE_ALERT` deliveries observed;
  the two-obstacle case requires at least two
- `obstacle_spawned`: the obstacle label reported by the behavior's
  `spawnxflag`; the two-obstacle case requires the later `ob_two` spawn
- `resolved_seen`: whether `OBM_RESOLVED` was seen before grading when that
  path is active for the case. When a helper flag is never posted, the raw
  report may still show the unevaluated `$[...]` placeholder.
- `range_flag` / `cpa_flag`: exact payload posted by the configured optional flag
- `range` / `cpa`: behavior-expanded `$[RNG]` or `$[CPA]` at the graded event
- `range_flag_seen` / `cpa_flag_seen`: legacy any-mail helper fields retained in
  cases that do not grade the optional flag contract
- `resolved`: raw `OBM_RESOLVED` value at evaluation time
- `arrived`, `collisions`, `near_misses`, `encounters`: mission outcome data
- `helm_state`: exact malformed-configuration state; `armed` prevents startup
  evaluation from racing the result writer, while `deadline` reports a missing state

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=cpa_flag_pass 10
./zlaunch.sh --jobs=4 --port_base=36600 --port_stride=20 10
./zlaunch.sh --just_make 10
```

Latest validation:

- July 20, 2026
- full minimal-logging matrix: `21/21` passed with `--jobs=4`,
  `--port_base=39000`, and `--port_stride=30` in 55 seconds
- full-logging checks: `rng_flag_pass`, `cpa_flag_pass`, and `bad_polygon_fail`
- threshold mutations produced first-event ranges `27.816` and `19.776` and
  failed the thresholded and unconditional cases, respectively
- changing the CPA payload to `false` failed while preserving `cpa=12.207`;
  repairing the crossed polygon reached `DRIVE` and failed the exact-state oracle
- changing `use_refinery=false` to `true` still passed, confirming that case's
  explicitly deferred compatibility-smoke disposition
- warp: `10`
