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
- did it post behavior-specific range or CPA flags when configured
- did it run to completion and let the transit finish cleanly

The stem keeps the same lower-band MIT_SP framing as the other motion suites:
the ownship starts at `(0,-60)` and drives east along a short corridor to
`(70,-60)`.

## Current Matrix

- `default_auto_request_pass` Posts the lane-blocking polygon `ob_lane` through `KNOWN_OBSTACLE`, testing the spawned behavior's automatic alert request and one-obstacle lifecycle; passes at 68 seconds when alert-request and obstacle-alert mail have appeared, `OBAVOIDING=end`, `ARRIVED=true`, and collisions remain zero.
- `rng_flag_pass` Adds `rng_flag=<20 AVOID_RANGE_SEEN=true`, testing the thresholded range flag during the stock one-obstacle avoidance; passes when alert request, obstacle alert, and range-flag mail have appeared, the behavior reaches `OBAVOIDING=end`, the vehicle arrives, and collisions remain zero.
- `cpa_flag_pass` Adds `cpa_flag=AVOID_CPA_SEEN=true`, testing publication after the behavior records a closest-point-of-approach event; passes when alert request, obstacle alert, and CPA-flag mail have appeared and collisions remain zero, without requiring final arrival or `OBAVOIDING=end`.
- `offlane_no_engagement_pass` Moves the obstacle to the box `(32,-8)`–`(44,4)`, well north of the eastbound lane, testing that automatic alert requests do not force engagement with an irrelevant obstacle; passes when a request was seen, no obstacle alert was ever seen, the vehicle arrives collision-free, and the final `OBAVOIDING` value is not `active`, `running`, or `end`.
- `no_refinery_pass` Sets `use_refinery=false` for the same lane obstacle, exercising direct objective construction without the IvP refinery; passes when request and alert mail appear, `OBAVOIDING=end`, the vehicle arrives, and collisions remain zero.
- `two_obstacles_clean_pass` Posts `ob_one` near `(19,-66)` at one second and `ob_two` near `(55,-57)` at two seconds, lowers transit speed to 1.6, and adds `spawnxflag=OBSTACLE_SPAWNED=$[OID]`, testing two independent spawned avoiders; passes with an alert request, at least two `OBSTACLE_ALERT` deliveries, final spawn ID `ob_two`, arrival, and zero collisions.
- `static_polygon_pass` Configures the lane box directly with the behavior's `polygon` parameter instead of using `OBSTACLE_ALERT` templating, testing a launch-time obstacle; passes when the static behavior reaches `OBAVOIDING=end`, the vehicle arrives, and collisions remain zero.
- `poly_alias_pass` Configures the same launch-time lane box through the `poly` alias, testing equivalence with `polygon`; passes when the behavior reaches `OBAVOIDING=end`, the vehicle arrives, and collisions remain zero.
- `rng_flag_no_threshold_pass` Sets `rng_flag=AVOID_RANGE_SEEN=true` without a range predicate, testing the unconditional range-flag form; passes when request, obstacle-alert, and range-flag mail appear, `OBAVOIDING=end`, the vehicle arrives, and collisions remain zero.
- `avoid_disabled_fail` Changes the spawned behavior condition to `AVOID=false` while `AVOID` remains initialized true, testing the no-avoidance outcome after normal request and alert delivery; the harness passes when the vehicle still arrives with exactly one encounter and one collision and `OBAVOIDING=end`.
- `bad_polygon_fail` Supplies the crossed polygon `{29,-53:43,-67:29,-67:43,-53}`, exercising invalid launch-time geometry; the harness passes when any `IVPHELM_STATE` mail latches `HELM_MALCONFIG=true`, while the raw state is only reported.
- `bad_pwt_inner_dist_fail` Sets `pwt_inner_dist=40` above `pwt_outer_dist=28` while raising `completed_dist` to 42, isolating the invalid relevance-distance ordering; the harness passes when any `IVPHELM_STATE` mail latches `HELM_MALCONFIG=true`.
- `bad_pwt_outer_dist_fail` Sets `pwt_outer_dist=8` below `pwt_inner_dist=14`, exercising the opposite invalid relevance-distance ordering; the harness passes when any `IVPHELM_STATE` mail latches `HELM_MALCONFIG=true`.
- `bad_completed_dist_fail` Sets `completed_dist=20` below `pwt_outer_dist=28`, exercising invalid completion/relevance ordering; the harness passes when any `IVPHELM_STATE` mail latches `HELM_MALCONFIG=true`.
- `bad_min_util_cpa_dist_fail` Sets `min_util_cpa_dist=20` above `max_util_cpa_dist=16`, exercising invalid CPA utility bounds; the harness passes when any `IVPHELM_STATE` mail latches `HELM_MALCONFIG=true`.
- `bad_max_util_cpa_dist_fail` Sets `max_util_cpa_dist=6` below `min_util_cpa_dist=8`, exercising the opposite invalid CPA utility ordering; the harness passes when any `IVPHELM_STATE` mail latches `HELM_MALCONFIG=true`.
- `bad_allowable_ttc_fail` Sets `allowable_ttc=-1`, exercising rejection of a negative time-to-collision limit; the harness passes when any `IVPHELM_STATE` mail latches `HELM_MALCONFIG=true`.
- `bad_rng_flag_fail` Sets `rng_flag=<bogus AVOID_RANGE_SEEN=true`, exercising rejection of a nonnumeric range threshold; the harness passes when any `IVPHELM_STATE` mail latches `HELM_MALCONFIG=true`.
- `bad_cpa_flag_fail` Sets `cpa_flag=AVOID_CPA_SEEN` without an assignment value, exercising malformed flag syntax; the harness passes when any `IVPHELM_STATE` mail latches `HELM_MALCONFIG=true`.
- `bad_use_refinery_fail` Sets `use_refinery=maybe`, exercising rejection of a non-boolean refinery selector; the harness passes when any `IVPHELM_STATE` mail latches `HELM_MALCONFIG=true`.
- `bad_pwt_grade_fail` Sets the unsupported parameter `pwt_grade=quadratic`, exercising the current `setParam()` rejection surface; the harness passes when any `IVPHELM_STATE` mail latches `HELM_MALCONFIG=true`.

## Results Lines

Typical pass line:

```text
case=default_auto_request_pass  grade=pass  form=obstacle_behavior_motion_tests  mmod=default_auto_request_pass  eval=true  obavoiding=end  alert_req_seen=true  obstacle_alert_seen=true  resolved_seen=$[RESOLVED_SEEN]  range_flag_seen=$[RANGE_FLAG_SEEN]  cpa_flag_seen=$[CPA_FLAG_SEEN]  resolved=$[OBM_RESOLVED]  arrived=true  collisions=0  near_misses=0  encounters=0  mhash=[...]
```

Typical expected-negative line:

```text
case=avoid_disabled_fail  grade=pass  form=obstacle_behavior_motion_tests  mmod=avoid_disabled_fail  eval=true  obavoiding=end  alert_req_seen=true  obstacle_alert_seen=true  arrived=true  collisions=1  near_misses=0  encounters=1  mhash=[...]
```

Typical bad-config expected-negative line:

```text
case=bad_polygon_fail  grade=pass  form=obstacle_behavior_motion_tests  mmod=bad_polygon_fail  eval=true  helm_malconfig=true  helm_state=MALCONFIG  mhash=[...]
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
- `range_flag_seen` / `cpa_flag_seen`: helper booleans for the optional
  behavior flags
- `resolved`: raw `OBM_RESOLVED` value at evaluation time
- `arrived`, `collisions`, `near_misses`, `encounters`: mission outcome data
- `helm_malconfig` / `helm_state`: malformed behavior configuration evidence
  for bad-config expected-negative cases

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=cpa_flag_pass 10
./zlaunch.sh --jobs=4 --port_base=36600 --port_stride=20 10
./zlaunch.sh --just_make 10
```

Latest validation:

- July 16, 2026
- final rolling matrices: `105/105` mission-owned grades passed across five
  runs in `62`, `62`, `62`, `61`, and `63` seconds (mean `62.0`)
- final serial matrix: `21/21` passed in `222` seconds
- focused `two_obstacles_clean_pass`: `10/10` passed at speed `1.6`
- mutation probe: removing the `ob_two` vehicle stimulus produced
  `obstacle_alert_count=1 obstacle_spawned=ob_one grade=fail`
- warp: `10`
- rolling command: `./zlaunch.sh --jobs=4 --port_base=36600 --port_stride=30 10`
