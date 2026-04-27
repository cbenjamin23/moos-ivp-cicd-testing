# H01-obmgr_unit

Patch-driven harness for [`missions/obmgr_missions/obmgr_unit`](../../../missions/obmgr_missions/obmgr_unit).

This harness turns one small obstacle-manager stem mission into a broader
`pObstacleMgr` test matrix. Each case behaves like a short mission-level unit
test: the harness patches the obstacle inputs and, when needed, patches
`pMissionEval` so the case grades exactly the obstacle-manager behavior under
test. The stem mission uses the lower-band Forrest 19 / MIT_SP framing with
the ownship parked at `(0,-60)`.

For the patching mechanics, see [`NSPATCH.md`](./NSPATCH.md).

## Current Matrix

### Given-obstacle and alert-path cases

- `given_baseline_pass`
  Default runtime alert request plus a near given obstacle should produce both
  `OBM_NEW_GIVEN=true` and `OBM_DIST_TO_OBJ=OB_NEAR,18.0`.
- `config_given_obstacle_pass`
  A startup `given_obstacle` entry should seed the manager before runtime mail
  arrives. The case then sends only the alert request and grades on the
  resulting distance publication for that config-owned polygon.
- `given_obstable_alias_pass`
  The source still accepts the historical misspelled `given_obstable` config
  token. This case keeps that alias under regression coverage by requiring the
  alias-loaded polygon to alert exactly like the canonical spelling.
- `no_alert_request_absent_pass`
  The same obstacle is still accepted, but the runtime `OBM_ALERT_REQUEST` is
  removed. This matters because `pObstacleMgr` will not post `OBM_DIST_TO_OBJ`
  or obstacle alerts at all until an alert request exists.
- `bad_alert_request_absent_pass`
  A malformed runtime alert request should be ignored rather than partially
  installed. The obstacle is accepted, but the mission fails if the invalid
  request causes a distance report or obstacle alert to appear.
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
- `invalid_given_nonconvex_absent_pass`
  A non-convex/degenerate `GIVEN_OBSTACLE` payload should be rejected before it
  posts `new_obs_flag`, `OBM_DIST_TO_OBJ`, or an obstacle alert.
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
- `invalid_point_missing_key_absent_pass`
  `pObstacleMgr` requires each point input to include a key or label before it
  can become an obstacle cluster. This case sends otherwise valid points without
  keys and grades on the absence of a generic cluster alert.
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
- `post_view_polys_false_absent_pass`
  A point cluster should still produce obstacle distance output when
  `post_view_polys=false`, but it should not publish a `VIEW_POLYGON`
  visualization. The mission watches for that visualization and fails if it
  appears.
- `new_obs_flag_macros_pass`
  `new_obs_flag` supports `pObstacleMgr` macros such as `$[OBS_NOW]` and
  `$[OBS_EVER]`. This case confirms a first accepted given obstacle expands
  those counters to `now_1_ever_1`.
- `point_vsource_alert_pass`
  Point inputs may include `vsource=...`, and the alert payload should preserve
  that source tag when a hull update is posted. The mission grades on alert and
  distance, while the harness verifies the `vsource=radar` token in the result
  line.
- `given_after_points_same_key_reject_pass`
  If a point-cluster obstacle already owns a key, a later mailed-in
  `GIVEN_OBSTACLE` with the same label should be rejected. The point cluster
  must remain alertable, and the given-obstacle `new_obs_flag` must stay absent.

### Obstacle-modification cases

- `disable_obstacle_pass`
  `disable_var` should post `BHV_ABLE_FILTER=obstacle_id=...,action=disable`,
  and the mission grades on a confirmation flag posted when that filter-message
  path is exercised.
- `disable_vsource_pass`
  The modification parser also accepts `vsource=...` selectors, not only bare
  obstacle IDs. This case sends a vsource disable command and checks that the
  resulting filter message preserves `vsource=radar,action=disable`.
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
./zlaunch.sh --jobs=4 --port_base=15000 10
./zlaunch.sh --case=given_baseline_pass 10
./zlaunch.sh --case=points_cluster_dist_pass 10
./zlaunch.sh --case=new_obs_flag_macros_pass --port_base=15000 10
./zlaunch.sh --just_make 10
./zlaunch.sh --max_time=90 10
```

Wave mode notes:

- `--jobs=<N>` runs the matrix in waves of up to `N` isolated case copies
- each live case in a wave gets its own temp mission directory and unique port
  block
- the harness uses scoped mission teardown between waves rather than global
  `ktm`
- this mode is intended for CI wall-clock reduction, not for interactive use
  alongside other MOOS missions

## What `./zlaunch.sh` Does Here

When run from this harness directory, `./zlaunch.sh` does not call the stem
mission's own `zlaunch.sh`. Instead it owns the full run:

1. Choose one case or the full default case list.
2. Run the stem mission `clean.sh` and scoped harness teardown so the next case
   starts clean.
3. Apply the selected `nspatch` overlays to `missions/obmgr_missions/obmgr_unit`.
4. Call shared `xlaunch.sh`, which in turn calls the stem mission `launch.sh`.
5. Read the mission-local `results.txt`.
6. Compare `actual` against `expected`.
7. Append one summary line to the harness `results.txt`.
8. On harness exit, run one final `clean.sh` and scoped harness teardown.

In wave mode, those same steps happen inside per-case temporary mission copies.
The shared stem directory remains the source for patches and the default serial
execution path.

## Results Lines

Typical baseline line:

```text
case=given_baseline_pass  case_result=success  expected=pass  actual=pass  form=obmgr_tests  mmod=given_baseline_pass  grade=pass  eval=true  new_given=true  dist=OB_NEAR,18.0  mindist=18  alert=name=avd_ob_near#poly=pts={18,-62:22,-62:22,-58:18,-58},label=ob_near#id=ob_near  mhash=[THEN-PONY]
```

Typical resolution line:

```text
case=points_age_resolve_pass  case_result=success  expected=pass  actual=pass  form=obmgr_tests  mmod=points_age_resolve_pass  grade=pass  eval=true  resolved=rock_a  dist=$[OBM_DIST_TO_OBJ]  mhash=[....]
```

Typical modification line:

```text
case=disable_obstacle_pass  case_result=success  expected=pass  actual=pass  form=obmgr_tests  mmod=disable_obstacle_pass  grade=pass  eval=true  filter=obstacle_id=ob_near,action=disable  mhash=[....]
```

Typical default-name alert line:

```text
case=general_alert_default_name_pass  case_result=success  expected=pass  actual=pass  form=obmgr_tests  mmod=general_alert_default_name_pass  grade=pass  eval=true  new_given=true  gen_alert=name=gen_alert_ob_far#poly=pts={35,-62:39,-62:39,-58:35,-58},label=ob_far#id=ob_far  mhash=[MERE-NAVY]
```

Typical reject-before-accept line:

```text
case=given_max_duration_missing_absent_pass  case_result=success  expected=pass  actual=pass  form=obmgr_tests  mmod=given_max_duration_missing_absent_pass  grade=pass  eval=true  new_given=$[OBM_NEW_GIVEN]  alert=$[OBSTACLE_ALERT]  mhash=[SOFT-HELM]
```

Field anatomy:

- `case`
  The harness case name that was run.
- `expected`
  What the harness expected the mission to grade.
- `actual`
  What the mission actually graded after parsing the mission result line.
- `case_result`
  `success` if expected equals actual, otherwise `mismatch`.
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
- `view_poly_seen`
  Mission-side confirmation that a `VIEW_POLYGON` was observed, used by the
  `post_view_polys=false` absence case.
- `new_stats`
  Macro-expanded `new_obs_flag` output for the accepted-obstacle counter case.
- `alert_seen`
  Mission-side confirmation that an obstacle alert was observed before the
  harness applies an additional exact-token payload check.
- `mhash`
  Short mission hash.

As with the contact-manager harness, every current obstacle case is expected to
end with mission `grade=pass`. Even the absence cases use `grade=pass` when the
correct outcome is “accepted but not alerted” or “rejected before acceptance”.

Most current obstacle cases are graded fully inside the mission. The remaining
residual harness-side exact-token checks are limited to payload-format cases:
the two general-alert name cases plus the point-vsource and vsource-disable
selectors, where the literal alert or filter payload is the behavior under
test. For every other case, the harness just compares the expected and actual
mission grade and preserves the mission's own report columns in the summary
line.

Current matrix size: `30` cases.

## Wall-Clock Efficiency

The stem itself is intentionally short and stationary. Most wall-clock time
comes from launch and teardown rather than the obstacle logic. Revalidation
timings:

- full 30-case matrix passed at warp `10`
- `--jobs=4 --port_base=15000` passed in grouped validation
- launch and teardown still dominate the wall-clock cost

As with the contact-manager harness, launch and teardown overhead is a larger
wall-clock cost than the obstacle logic itself.
