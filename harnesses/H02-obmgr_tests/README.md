# H02-obmgr_tests

Patch-driven harness for [`missions/obmgr_tests`](../../missions/obmgr_tests).

This harness turns one small obstacle-manager stem mission into a broader
`pObstacleMgr` test matrix. Each case behaves like a short mission-level unit
test: the harness patches the obstacle inputs and, when needed, patches
`pMissionEval` so the case grades exactly the obstacle-manager behavior under
test.

For the patching mechanics, see [`NSPATCH.md`](./NSPATCH.md).

## Current Matrix

### Given-obstacle and alert-path cases

- `given_baseline_pass`
  Default runtime alert request plus a near given obstacle should produce both
  `OBM_NEW_GIVEN=true` and `OBM_DIST_TO_OBJ=OB_NEAR,18.0`.
- `no_alert_request_absent_pass`
  The same obstacle is still accepted, but the runtime `OBM_ALERT_REQUEST` is
  removed. This matters because `pObstacleMgr` will not post `OBM_DIST_TO_OBJ`
  or obstacle alerts at all until an alert request exists.
- `given_far_absent_pass`
  A farther given obstacle should still be accepted, but with default
  `post_dist_to_polys=close` it should stay out of both the distance posting
  path and the obstacle-alert path.
- `given_general_alert_pass`
  This is slightly more niche than the basic near/far cases. It checks the
  separate `general_alert` configuration branch, where a broader alert variable
  can be posted even when the main alert range is too tight for the obstacle.
  The mission grades only when the accepted obstacle and a general-alert
  publication are both present at the checkpoint. The harness still verifies the
  exact payload string for this case because `pMissionEval` cannot directly
  express values that themselves contain `=` tokens.
- `general_alert_default_name_pass`
  This is the same branch, but it leaves the `name=` field out of the config
  string so the default `gen_alert_` prefixing behavior is exercised.
- `given_duration_resolve_pass`
  A short-lived given obstacle should age out and post `OBM_RESOLVED`.
- `given_max_duration_reject_absent_pass`
  `pObstacleMgr` can reject mailed-in obstacles whose declared duration exceeds
  `given_max_duration`. This case proves the obstacle is rejected before it is
  ever accepted as a live obstacle.
- `given_max_duration_missing_absent_pass`
  This is the missing-duration version of the same branch. The obstacle is
  mailed in without `duration=...`, and the correct outcome is rejection before
  it can ever become a live given obstacle.
- `post_dist_always_pass`
  With `post_dist_to_polys=true`, a far obstacle should still publish
  `OBM_DIST_TO_OBJ`. This is the always-post branch, distinct from the default
  close-only behavior.
- `post_dist_false_absent_pass`
  The opposite branch: with `post_dist_to_polys=false`, the mission should not
  publish the normal near-obstacle distance report for this fixed geometry. This
  is still an absence-style case, so it is best read as a regression guard on
  the known baseline publication rather than as a universal proof that no
  alternate distance string could ever appear.

### Point-cluster and hull-generation cases

- `points_cluster_dist_pass`
  A cluster of `TRACKED_FEATURE` points should generate a convex hull and a
  stable `OBM_DIST_TO_OBJ` distance report.
- `custom_point_var_pass`
  This exercises the custom `point_var` input path. The same point cluster is
  posted under an alternate variable name instead of the default
  `TRACKED_FEATURE`.
- `max_pts_per_cluster_trim_pass`
  This is the cluster-trimming branch. The point set intentionally exceeds
  `max_pts_per_cluster`, so the hull is built from the trimmed cluster rather
  than from the full history of points.
- `points_ignore_range_absent_pass`
  The same point cluster should be ignored entirely when `ignore_range` is
  tighter than the point distances.
- `points_age_resolve_pass`
  Point-based obstacles should resolve once all points age out past
  `max_age_per_point`.
- `lasso_cluster_pass`
  This exercises the `lasso=true` pseudo-hull branch rather than the normal
  convex-hull generator. The reported distance is checked in a numeric band
  instead of by exact string because the case is about the lasso geometry, not
  about a specific polygon serialization.
- `placeholder_hull_pass`
  This is another niche branch: three colinear points do not form a valid
  convex hull, so `pObstacleMgr` falls back to a placeholder radial polygon.
  The test checks the resulting minimum distance band rather than the polygon
  text itself.

### Obstacle-modification cases

- `disable_obstacle_pass`
  `disable_var` should post `BHV_ABLE_FILTER=obstacle_id=...,action=disable`,
  and the mission grades on a confirmation flag posted when that filter-message
  path is exercised.
- `enable_obstacle_pass`
  `enable_var` should post `BHV_ABLE_FILTER=obstacle_id=...,action=enable`,
  and the mission grades on a confirmation flag posted when that filter-message
  path is exercised.
- `expunge_obstacle_pass`
  `expunge_var` should post `BHV_ABLE_FILTER=obstacle_id=...,action=expunge`
  and remove the obstacle from the manager's internal map. The mission grades on
  a confirmation flag posted when that filter-message path is exercised.

## Typical Runs

```bash
./zlaunch.sh
./zlaunch.sh --case=given_baseline_pass 10
./zlaunch.sh --case=points_cluster_dist_pass 10
./zlaunch.sh --just_make 10
./zlaunch.sh --max_time=90 10
```

## What `./zlaunch.sh` Does Here

When run from this harness directory, `./zlaunch.sh` does not call the stem
mission's own `zlaunch.sh`. Instead it owns the full run:

1. Choose one case or the full default case list.
2. Run the stem mission `clean.sh` and `ktm` so the next case starts clean.
3. Apply the selected `nspatch` overlays to `missions/obmgr_tests`.
4. Call shared `xlaunch.sh`, which in turn calls the stem mission `launch.sh`.
5. Read the mission-local `results.txt`.
6. Compare `actual` against `expected`.
7. Append one summary line to the harness `results.txt`.
8. On harness exit, run one final `clean.sh` and `ktm`.

## Results Lines

Typical baseline line:

```text
case=given_baseline_pass  expected=pass  actual=pass  status=ok  form=obmgr_tests_harness  mmod=given_baseline_pass  grade=pass  eval=true  new_given=true  dist=OB_NEAR,18.0  mindist=18  alert=name=avd_ob_near#poly=pts={18,-2:22,-2:22,2:18,2},label=ob_near#id=ob_near  mhash=[THEN-PONY]
```

Typical resolution line:

```text
case=points_age_resolve_pass  expected=pass  actual=pass  status=ok  form=obmgr_tests_harness  mmod=points_age_resolve_pass  grade=pass  eval=true  resolved=rock_a  dist=$[OBM_DIST_TO_OBJ]  mhash=[....]
```

Typical modification line:

```text
case=disable_obstacle_pass  expected=pass  actual=pass  status=ok  form=obmgr_tests_harness  mmod=disable_obstacle_pass  grade=pass  eval=true  filter=obstacle_id=ob_near,action=disable  mhash=[....]
```

Typical default-name alert line:

```text
case=general_alert_default_name_pass  expected=pass  actual=pass  status=ok  form=obmgr_tests_harness  mmod=general_alert_default_name_pass  grade=pass  eval=true  new_given=true  gen_alert=name=gen_alert_ob_far#poly=pts={35,-2:39,-2:39,2:35,2},label=ob_far#id=ob_far  mhash=[MERE-NAVY]
```

Typical reject-before-accept line:

```text
case=given_max_duration_missing_absent_pass  expected=pass  actual=pass  status=ok  form=obmgr_tests_harness  mmod=given_max_duration_missing_absent_pass  grade=pass  eval=true  new_given=$[OBM_NEW_GIVEN]  alert=$[OBSTACLE_ALERT]  mhash=[SOFT-HELM]
```

Field anatomy:

- `case`
  The harness case name that was run.
- `expected`
  What the harness expected the mission to grade.
- `actual`
  What the mission actually graded after parsing the mission result line.
- `status`
  `ok` if expected equals actual, otherwise `mismatch`.
- `form`
  Harness or mission family name.
- `mmod`
  The case-specific mission mode written by `pMissionEval`.
- `grade`
  Pass/fail result written by the mission itself.
- `eval`
  Whether the case's evaluation checkpoint was reached.
- `new_given`
  Whether a given obstacle was accepted and triggered `new_obs_flag`.
- `dist`
  The last `OBM_DIST_TO_OBJ` value, when included by the case.
- `mindist`
  `OBM_MIN_DIST_EVER`, when included by the case.
- `resolved`
  The most recent `OBM_RESOLVED` value, when included by the case.
- `alert`
  The main obstacle alert string, when included by the case.
- `gen_alert`
  The general-alert publication, when included by the case.
- `gen_seen`
  Mission-side confirmation that a general-alert publication was observed.
- `filter`
  The `BHV_ABLE_FILTER` message for enable/disable/expunge cases.
- `filter_seen`
  Mission-side confirmation that an obstacle-filter message was observed.
- `mhash`
  Short mission hash.

As with the contact-manager harness, every current obstacle case is expected to
end with mission `grade=pass`. Even the absence cases use `grade=pass` when the
correct outcome is “accepted but not alerted” or “rejected before acceptance”.

Most current obstacle cases are graded fully inside the mission. The only
remaining residual harness-side exact-token checks are the two general-alert
name cases, because `pMissionEval` cannot directly compare strings that contain
their own `=` assignments. For every other case, the harness just compares the
expected and actual mission grade and preserves the mission's own report
columns in the summary line.

Current matrix size: `20` cases.

## Wall-Clock Efficiency

The stem itself is intentionally short and stationary. Most wall-clock time
comes from launch and teardown rather than the obstacle logic. Revalidation
timings:

- full 20-case matrix passed at warp `10`
- launch and teardown still dominate the wall-clock cost

As with the contact-manager harness, launch and teardown overhead is a larger
wall-clock cost than the obstacle logic itself.
