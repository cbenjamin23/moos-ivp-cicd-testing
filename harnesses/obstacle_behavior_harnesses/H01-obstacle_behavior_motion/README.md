# H01-obstacle_behavior_motion

Patch-driven moving correctness harness for
[`/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/obstacle_behavior_missions/obstacle_behavior_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/obstacle_behavior_missions/obstacle_behavior_motion).

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

- `default_auto_request_pass`
  Baseline one-obstacle case. The behavior should request alerts, engage,
  complete, and still arrive with zero collisions.
- `rng_flag_pass`
  This case adds `rng_flag=<20 AVOID_RANGE_SEEN=true`. The
  mission passes only if that behavior-owned range flag is actually seen.
- `cpa_flag_pass`
  This case adds `cpa_flag=AVOID_CPA_SEEN=true`. This is a more
  focused case because the flag only posts after a real CPA event is observed.
  It grades the CPA flag plus basic safety, not late transit completion, so the
  verdict stays behavior-owned.
- `offlane_no_engagement_pass`
  The obstacle is moved well above the lane. The behavior should still request
  alerts, but it should never receive an obstacle alert or run.
- `no_refinery_pass`
  Disables the IvP refinery while keeping the obstacle geometry unchanged.
  The case should still complete avoidance, proving the non-refinery objective
  path remains valid.
- `two_obstacles_clean_pass`
  Two obstacles are injected after deploy so both obstacle-alert spawn paths
  are exercised. The evaluator requires two `OBSTACLE_ALERT` deliveries and
  the behavior's spawn flag must identify the second obstacle as `ob_two`, so
  a single spawned avoidance behavior is no longer enough to pass. The vehicle
  uses a case-specific speed of `1.6` to retain avoidance control margin under
  rolling load, while still having to route through the corridor, arrive, and
  finish with zero collisions.
- `static_polygon_pass`
  The behavior is configured with a launch-time `polygon` instead of the
  templated `OBSTACLE_ALERT` input. It should still complete avoidance and
  arrive without collision.
- `poly_alias_pass`
  The same launch-time obstacle is configured through the `poly` alias. It
  should behave like `polygon` and complete cleanly.
- `rng_flag_no_threshold_pass`
  The behavior uses `rng_flag=AVOID_RANGE_SEEN=true` with no threshold. The
  mission passes only if the no-threshold range flag is posted during a clean
  avoidance run.
- `avoid_disabled_fail`
  This case makes the `AVOID=true` condition impossible to satisfy. The
  alert request still happens, but avoidance is disabled and the mission should
  grade pass only when the expected collision evidence is observed.
- `bad_polygon_fail`
  The launch-time `polygon` is non-convex. The behavior configuration should be
  rejected and the mission should grade pass on helm malconfig evidence.
- `bad_pwt_inner_dist_fail`
  `pwt_inner_dist` is set above `pwt_outer_dist`. The mission should grade pass
  on helm malconfig evidence.
- `bad_pwt_outer_dist_fail`
  `pwt_outer_dist` is set below `pwt_inner_dist`. The mission should grade
  pass on helm malconfig evidence.
- `bad_completed_dist_fail`
  `completed_dist` is set below the configured outer relevance distance. The
  mission should grade pass on helm malconfig evidence.
- `bad_min_util_cpa_dist_fail`
  `min_util_cpa_dist` is set above `max_util_cpa_dist`. The mission should
  grade pass on helm malconfig evidence.
- `bad_max_util_cpa_dist_fail`
  `max_util_cpa_dist` is set below `min_util_cpa_dist`. The mission should
  grade pass on helm malconfig evidence.
- `bad_allowable_ttc_fail`
  `allowable_ttc` is set negative. The behavior configuration should be
  rejected and the mission should grade pass on helm malconfig evidence.
- `bad_rng_flag_fail`
  `rng_flag` is given a malformed threshold. The behavior configuration should
  be rejected and the mission should grade pass on helm malconfig evidence.
- `bad_cpa_flag_fail`
  `cpa_flag` is missing an assignment payload. The behavior configuration
  should be rejected and the mission should grade pass on helm malconfig
  evidence.
- `bad_use_refinery_fail`
  `use_refinery` is set to a non-boolean token. The mission should grade pass
  on helm malconfig evidence.
- `bad_pwt_grade_fail`
  `pwt_grade` is present in the source state but is not accepted by the current
  `setParam()` surface. The mission should grade pass on helm malconfig
  evidence if someone configures it.

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
