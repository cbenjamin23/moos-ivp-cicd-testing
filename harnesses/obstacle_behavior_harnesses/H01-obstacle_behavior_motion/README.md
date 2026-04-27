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
  The behavior is patched with `rng_flag=<20 AVOID_RANGE_SEEN=true`. The
  mission passes only if that behavior-owned range flag is actually seen.
- `cpa_flag_pass`
  The behavior is patched with `cpa_flag=AVOID_CPA_SEEN=true`. This is a more
  niche path because the flag only posts after a real CPA event is observed.
  This case intentionally grades only the CPA helper path plus basic safety,
  not late transit completion, so the verdict stays behavior-owned.
- `offlane_no_engagement_pass`
  The obstacle is moved well above the lane. The behavior should still request
  its alert path, but it should never receive an obstacle alert or run.
- `no_refinery_pass`
  The behavior is patched with `use_refinery=false`. This keeps the same
  geometry, but exercises a different internal objective-building path.
- `two_obstacles_clean_pass`
  Two obstacles are placed in the corridor with one lower and one upper bias.
  This is the suite's multi-obstacle clean-completion case: the vehicle should
  still alert, avoid, and finish with zero collisions. This case deliberately
  grades on clean completion rather than a specific final `OBAVOIDING` state or
  exact encounter count, because those were less stable than the mission
  outcome itself.
- `static_polygon_pass`
  The behavior is configured with a launch-time `polygon` instead of the
  templated `OBSTACLE_ALERT` path. It should still complete avoidance and
  arrive without collision.
- `poly_alias_pass`
  The same launch-time obstacle is configured through the `poly` alias. It
  should behave like `polygon` and complete cleanly.
- `rng_flag_no_threshold_pass`
  The behavior uses `rng_flag=AVOID_RANGE_SEEN=true` with no threshold. The
  mission passes only if the no-threshold range flag is posted during a clean
  avoidance run.
- `avoid_disabled_fail`
  The behavior is patched so its `AVOID=true` condition can never hold. The
  alert request still happens, but the avoid-obstacle lifecycle should fail to
  complete and the mission is expected to grade `fail`.
- `bad_polygon_fail`
  The launch-time `polygon` is non-convex. The behavior configuration should be
  rejected and the mission should grade fail.
- `bad_pwt_inner_dist_fail`
  `pwt_inner_dist` is set above `pwt_outer_dist`. The mission should grade fail
  rather than accepting an invalid relevance band.
- `bad_pwt_outer_dist_fail`
  `pwt_outer_dist` is set below `pwt_inner_dist`. The mission should grade
  fail.
- `bad_completed_dist_fail`
  `completed_dist` is set below the configured outer relevance distance. The
  mission should grade fail.
- `bad_min_util_cpa_dist_fail`
  `min_util_cpa_dist` is set above `max_util_cpa_dist`. The mission should
  grade fail.
- `bad_max_util_cpa_dist_fail`
  `max_util_cpa_dist` is set below `min_util_cpa_dist`. The mission should
  grade fail.
- `bad_allowable_ttc_fail`
  `allowable_ttc` is set negative. The behavior configuration should be
  rejected and the mission should grade fail.
- `bad_rng_flag_fail`
  `rng_flag` is given a malformed threshold. The behavior configuration should
  be rejected and the mission should grade fail.
- `bad_cpa_flag_fail`
  `cpa_flag` is missing an assignment payload. The behavior configuration
  should be rejected and the mission should grade fail.
- `bad_use_refinery_fail`
  `use_refinery` is set to a non-boolean token. The mission should grade fail.
- `bad_pwt_grade_fail`
  `pwt_grade` is present in the source state but is not accepted by the current
  `setParam()` surface. The mission should grade fail if someone configures it.

## Results Lines

Typical pass line:

```text
case=default_auto_request_pass  case_result=success  expected=pass  actual=pass  grade=pass  form=obstacle_behavior_motion_tests  mmod=default_auto_request_pass  eval=true  obavoiding=end  alert_req_seen=true  obstacle_alert_seen=true  resolved_seen=true  range_flag_seen=false  cpa_flag_seen=false  resolved=ob_lane  arrived=true  collisions=0  near_misses=0  encounters=1  mhash=[...]
```

Typical fail line:

```text
case=avoid_disabled_fail  case_result=success  expected=fail  actual=fail  grade=fail  form=obstacle_behavior_motion_tests  mmod=avoid_disabled_fail  eval=true  obavoiding=idle  alert_req_seen=true  obstacle_alert_seen=true  resolved_seen=false  range_flag_seen=false  cpa_flag_seen=false  resolved=  arrived=false  collisions=0  near_misses=0  encounters=1  mhash=[...]
```

Field anatomy:

- `obavoiding`: behavior-owned lifecycle flag from `BHV_AvoidObstacleV24`
- `alert_req_seen`: whether the behavior posted `OBM_ALERT_REQUEST`
- `obstacle_alert_seen`: whether the behavior actually received `OBSTACLE_ALERT`
- `resolved_seen`: whether `OBM_RESOLVED` was seen before grading when that
  path is active for the case. When a helper flag is never posted, the raw
  report may still show the unevaluated `$[...]` placeholder.
- `range_flag_seen` / `cpa_flag_seen`: helper booleans for the optional
  behavior flags
- `resolved`: raw `OBM_RESOLVED` value at evaluation time
- `arrived`, `collisions`, `near_misses`, `encounters`: mission outcome data

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=cpa_flag_pass 10
./zlaunch.sh --jobs=4 --port_base=18000 10
./zlaunch.sh --just_make 10
```

Latest validation:

- April 26, 2026
- full matrix: `21/21` expected outcomes matched
- warp: `10`
- command: `./zlaunch.sh --jobs=4 --port_base=18000 10`
