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
  `post_dist_to_polys=close` it should not produce a distance report or an
  obstacle alert.
- `given_general_alert_pass`
  A broader `general_alert` variable should post even when the main alert range
  is too tight for the obstacle. The mission requires the obstacle to be
  accepted and the complete named general-alert payload to match.
- `general_alert_default_name_pass`
  The same general-alert configuration should work without an explicit `name=`
  field, using the default `gen_alert_` prefix in the complete payload.
- `given_duration_resolve_pass`
  A short-lived given obstacle should age out and post `OBM_RESOLVED`.
- `given_max_duration_reject_absent_pass`
  A mailed-in obstacle whose duration exceeds `given_max_duration` should be
  rejected before it is ever accepted as a live obstacle.
- `given_max_duration_missing_absent_pass`
  A mailed-in obstacle without `duration=...` should also be rejected before it
  can become a live given obstacle.
- `invalid_given_nonconvex_absent_pass`
  A non-convex/degenerate `GIVEN_OBSTACLE` payload should be rejected before it
  posts `new_obs_flag`, `OBM_DIST_TO_OBJ`, or an obstacle alert.
- `post_dist_always_pass`
  With `post_dist_to_polys=true`, a far obstacle should still publish
  `OBM_DIST_TO_OBJ`, distinct from the default close-only behavior.
- `post_dist_false_absent_pass`
  With `post_dist_to_polys=false`, the mission should not publish the normal
  near-obstacle distance report for this fixed geometry.

### Point-cluster and hull-generation cases

- `points_cluster_dist_pass`
  A cluster of `TRACKED_FEATURE` points should generate a convex hull and a
  stable `OBM_DIST_TO_OBJ` distance report.
- `custom_point_var_pass`
  The same point cluster is posted under a custom `point_var` instead of the
  default `TRACKED_FEATURE`, and it should still become a valid obstacle.
- `invalid_point_missing_key_absent_pass`
  Point input without a key or label should be ignored before it can become an
  obstacle cluster. The mission grades on the absence of a generic cluster
  alert.
- `max_pts_per_cluster_trim_pass`
  The point set intentionally exceeds `max_pts_per_cluster`, so the hull should
  be built from the trimmed cluster rather than from the full history of points.
- `points_ignore_range_absent_pass`
  The same point cluster should be ignored entirely when `ignore_range` is
  tighter than the point distances.
- `points_age_resolve_pass`
  Point-based obstacles should resolve once all points age out past
  `max_age_per_point`.
- `lasso_cluster_pass`
  With `lasso=true`, the manager should build a pseudo-hull instead of the
  normal convex hull. The reported distance is checked in a numeric band because
  the case is about the lasso geometry, not a specific polygon string.
- `placeholder_hull_pass`
  Three colinear points do not form a valid convex hull, so `pObstacleMgr`
  should fall back to a placeholder radial polygon. The test checks the
  resulting minimum distance band rather than the polygon text itself.
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
  that source tag when a hull update is posted. `pMissionEval` requires the
  expected distance and the complete obstacle-alert payload.
- `given_after_points_same_key_reject_pass`
  If a point-cluster obstacle already owns a key, a later mailed-in
  `GIVEN_OBSTACLE` with the same label should be rejected. The point cluster
  must remain alertable, and the given-obstacle `new_obs_flag` must stay absent.

### Obstacle-modification cases

- `disable_obstacle_pass`
  `disable_var` should post `BHV_ABLE_FILTER=obstacle_id=...,action=disable`,
  confirming that obstacle-disable mail is generated for the named obstacle.
- `disable_vsource_pass`
  The modification parser also accepts `vsource=...` selectors, not only bare
  obstacle IDs. This case sends a vsource disable command, and `pMissionEval`
  requires the complete `vsource=radar,action=disable` filter message.
- `enable_obstacle_pass`
  `enable_var` should post `BHV_ABLE_FILTER=obstacle_id=...,action=enable`,
  confirming that obstacle-enable mail is generated for the named obstacle.
- `expunge_obstacle_pass`
  `expunge_var` should post `BHV_ABLE_FILTER=obstacle_id=...,action=expunge`
  and remove the obstacle from the manager's internal map.

## Typical Runs

```bash
./zlaunch.sh
./zlaunch.sh --log=full
./zlaunch.sh --jobs=2 --port_base=15000 10
./zlaunch.sh --case=given_baseline_pass 10
./zlaunch.sh --case=points_cluster_dist_pass 10
./zlaunch.sh --case=new_obs_flag_macros_pass --port_base=15000 10
./zlaunch.sh --just_make --keep_workdirs 10
./zlaunch.sh --max_time=90 10
```

The launcher requires Bash 5.1 or newer because `--jobs` is a rolling
scheduler. On macOS it will try a known Homebrew/Linuxbrew Bash before printing
a clear version error. `--gui` is limited to one explicit `--case`; full-matrix
runs are headless.

## Execution Contract

Every selected case, including a serial or single-case run, uses its own copy of
the stem beneath this harness's `.harness_runs/` directory. The source stem is
never patched or launched in place. `--keep_workdirs` preserves that run tree
for inspection; otherwise the canonical repository helper performs
root-scoped teardown and the tree is removed.

Each case index receives a distinct port block with stride `30`:

- shoreside MOOSDB: case base `+0`
- vehicle MOOSDB: case base `+1`
- shoreside pShare: case base `+15`
- vehicle pShare: case base `+16`

With `--jobs=N`, the launcher starts up to `N` copies and starts the next
pending case as soon as any active case finishes. It does not wait for a whole
wave. Selected-case order is preserved in the final `results.txt` even when
completion order differs.

The stem mission owns ordinary grading. Each live case must produce exactly one
nonempty `pMissionEval` row containing exactly one valid `grade=pass|fail`
field. The harness adds `case=<token>` and otherwise preserves the mission row.
An ordinary mission failure or case preparation/launch failure is recorded, but
does not prevent the remaining selected cases from running.

Only runner failures are normalized by the harness, for example
`reason=prepare_error`, `reason=launch_error`, `reason=missing_result_file`,
`reason=duplicate_results`, or `reason=teardown_error`. When a launch fails
after the mission wrote evidence, the row keeps that evidence under
`mission_*` provenance fields. A teardown failure is treated as an
infrastructure error: scheduler refill stops, pending cases receive explicit
aborted rows, and the run root and safety lock are kept for diagnosis.

## Mission-Owned Payload Comparisons

Four cases compare structured strings that contain their own `=` characters:

- `given_general_alert_pass`
- `general_alert_default_name_pass`
- `point_vsource_alert_pass`
- `disable_vsource_pass`

Each case uses the existing shoreside `uTimerScript` to publish its expected
literal under the one reused name `OBM_EXPECTED_PAYLOAD`. `pMissionEval` then
compares the observed alert or filter variable against that runtime
right-hand-side value. This avoids embedding a multi-assignment string directly
in the condition parser. It adds no application and no harness-side verdict.

For the two general-alert cases, the full-string comparisons are equivalent to
the legacy full-token checks. For `point_vsource_alert_pass` and
`disable_vsource_pass`, whole-field equality is deliberately stricter than the
legacy substring checks. That is a migration to a mission-owned expression of
the existing payload contract, not a broader redesign of the test matrix.

Seven expected-absence cases use the direct mission-side failure condition
`OBSTACLE_ALERT != ""` so any nonempty alert fails the case:

- `no_alert_request_absent_pass`
- `bad_alert_request_absent_pass`
- `given_far_absent_pass`
- `given_max_duration_reject_absent_pass`
- `invalid_given_nonconvex_absent_pass`
- `invalid_point_missing_key_absent_pass`
- `points_ignore_range_absent_pass`

## What `./zlaunch.sh` Does Here

For each selected case, the harness:

1. Copies and cleans the stem into an isolated case directory.
2. Applies optional full-logging restoration, then the case's explicit
   shoreside and vehicle `nspatch` overlays there.
3. Calls the copied stem's `zlaunch.sh`, forwarding the case token as `--mmod`,
   all four ports, `--max_time`, display mode, and time warp.
4. Validates and records the one mission-owned result row, or writes a
   normalized runner-failure row.
5. Stops MOOS processes scoped to that case root.
6. Aggregates all rows in the original selected-case order.

## Results Lines

Typical baseline line:

```text
case=given_baseline_pass form=obmgr_tests_harness mmod=given_baseline_pass grade=pass eval=true new_given=true dist=OB_NEAR,18.0 mindist=18 alert=name=avd_ob_near#poly=pts={18,-62:22,-62:22,-58:18,-58},label=ob_near#id=ob_near mhash=[THEN-PONY]
```

Typical resolution line:

```text
case=points_age_resolve_pass form=obmgr_tests_harness mmod=points_age_resolve_pass grade=pass eval=true resolved=rock_a dist=$[OBM_DIST_TO_OBJ] mhash=[....]
```

Typical modification line:

```text
case=disable_obstacle_pass form=obmgr_tests_harness mmod=disable_obstacle_pass grade=pass eval=true filter=obstacle_id=ob_near,action=disable mhash=[....]
```

Typical default-name alert line:

```text
case=general_alert_default_name_pass form=obmgr_tests_harness mmod=general_alert_default_name_pass grade=pass eval=true new_given=true gen_alert=name=gen_alert_ob_far#poly=pts={35,-62:39,-62:39,-58:35,-58},label=ob_far#id=ob_far mhash=[MERE-NAVY]
```

Typical reject-before-accept line:

```text
case=given_max_duration_missing_absent_pass form=obmgr_tests_harness mmod=given_max_duration_missing_absent_pass grade=pass eval=true new_given=$[OBM_NEW_GIVEN] alert=$[OBSTACLE_ALERT] mhash=[SOFT-HELM]
```

Field anatomy:

- `case`
  The harness case name that was run.
- `grade`
  Pass/fail result written by the mission itself.
- `form`
  Harness or mission family name.
- `mmod`
  The mission mode written by `pMissionEval` for the selected case.
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
  mission applies its exact whole-field comparison.
- `mhash`
  Short mission hash.

As with the contact-manager harness, every current obstacle case is expected to
end with mission `grade=pass`. Even the absence cases use `grade=pass` when the
correct outcome is “accepted but not alerted” or “rejected before acceptance”.

All current obstacle cases are graded fully inside the mission. The harness
does not compare expected and actual case verdicts or inspect payload tokens.

Current matrix size: `30` cases.

## Validation And Wall-Clock Measurements

All listed full-matrix measurements used warp `10`, completed with `30/30`
passing rows, and report wall-clock seconds on the same development machine.

- Legacy batch-wave `--jobs=2`: `123.24`, `121.97`, and `121.55` seconds; mean
  `122.25`.
- Legacy shared-stem serial: `182.36` seconds.
- Migrated isolated serial: `218.18` seconds.
- Migrated rolling `--jobs=2` with canonical teardown defaults: `115.57`,
  `108.39`, `142.95`, `123.22`, and `116.98` seconds; mean `121.42`, median
  `116.98`.
- One migrated rolling run with the helper's supported one-second grace
  overrides: `113.88` seconds.
- A representative default-versus-one-second case comparison measured `7.35`
  versus `6.96` seconds.

The measurements show normal run-to-run variation and make the serial isolation
cost visible; they are operational data, not grading evidence. The canonical
teardown defaults remain unchanged.
