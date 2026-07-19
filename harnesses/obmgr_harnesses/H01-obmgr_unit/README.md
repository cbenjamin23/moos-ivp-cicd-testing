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

- `given_baseline_pass`: Posts a 4-by-4-meter `GIVEN_OBSTACLE` 18 meters east
  of the stationary vehicle after a 25-meter `OBM_ALERT_REQUEST`, testing
  runtime obstacle ingestion and distance calculation; passes when
  `OBM_NEW_GIVEN=true` and `OBM_DIST_TO_OBJ=OB_NEAR,18.0`.
- `config_given_obstacle_pass`: Loads the same near polygon through the startup
  `given_obstacle` parameter and sends only the runtime alert request, testing
  the configuration-owned obstacle path; passes when
  `OBM_DIST_TO_OBJ=CONFIG_NEAR,18.0`.
- `given_obstable_alias_pass`: Loads the near polygon through the historical
  misspelled `given_obstable` parameter, testing that the alias still reaches
  the normal alert-distance path; passes when
  `OBM_DIST_TO_OBJ=ALIAS_NEAR,18.0`.
- `no_alert_request_absent_pass`: Posts the near `GIVEN_OBSTACLE` without an
  `OBM_ALERT_REQUEST`, testing that an accepted obstacle does not produce
  request-scoped output; passes when `OBM_NEW_GIVEN=true`,
  `OBM_DIST_TO_OBJ=OB_NEAR,18.0` is absent, and `OBSTACLE_ALERT` remains empty.
- `bad_alert_request_absent_pass`: Sends an `OBM_ALERT_REQUEST` with
  `alert_range=-25` before posting the near obstacle, testing rejection of an
  invalid request; passes when `OBM_NEW_GIVEN=true`, the expected
  `OBM_DIST_TO_OBJ` report is absent, and `OBSTACLE_ALERT` remains empty.
- `given_far_absent_pass`: Posts a `GIVEN_OBSTACLE` 35 meters east while the
  alert range is 25 meters and `post_dist_to_polys=close`, testing close-range
  output gating; passes when `OBM_NEW_GIVEN=true`,
  `OBM_DIST_TO_OBJ=OB_FAR,35.0` is absent, and `OBSTACLE_ALERT` remains empty.
- `given_general_alert_pass`: Configures a named `general_alert` with a
  40-meter range, then posts an obstacle 35 meters away and outside the normal
  25-meter alert range, testing the independent general-alert path; passes when
  `OBM_NEW_GIVEN=true`, `GEN_ALERT_SEEN=true`, and `GEN_OBSTACLE_ALERT` exactly
  matches the `gen_ob_far` polygon payload.
- `general_alert_default_name_pass`: Omits `name=` from the same 40-meter
  `general_alert`, testing the default `gen_alert_` prefix; passes when
  `OBM_NEW_GIVEN=true`, `GEN_ALERT_SEEN=true`, and `GEN_OBSTACLE_ALERT` exactly
  matches the `gen_alert_ob_far` polygon payload.
- `given_duration_resolve_pass`: Posts the near obstacle with `duration=3`,
  testing expiration of a runtime given obstacle; passes at 10 seconds when
  `OBM_RESOLVED=ob_near`.
- `given_max_duration_reject_absent_pass`: Sets `given_max_duration=5` and
  posts an obstacle with `duration=10`, testing rejection above the configured
  duration limit; passes when `OBM_NEW_GIVEN` is never set and
  `OBSTACLE_ALERT` remains empty.
- `given_max_duration_missing_absent_pass`: Sets `given_max_duration=30` and
  posts a `GIVEN_OBSTACLE` without `duration=`, testing the required-duration
  check; passes when `OBM_NEW_GIVEN` is never set.
- `invalid_given_nonconvex_absent_pass`: Posts a `GIVEN_OBSTACLE` whose three
  vertices are collinear, testing invalid-polygon rejection; passes when
  `OBM_NEW_GIVEN` is never set, `OBM_DIST_TO_OBJ=BAD_GIVEN,20.0` is absent, and
  `OBSTACLE_ALERT` remains empty.
- `post_dist_always_pass`: Sets `post_dist_to_polys=true` and posts an obstacle
  35 meters away, outside the 25-meter alert range, testing unconditional
  distance publication; passes when `OBM_NEW_GIVEN=true` and
  `OBM_DIST_TO_OBJ=OB_FAR,35.0`.
- `post_dist_false_absent_pass`: Sets `post_dist_to_polys=false` before posting
  the near obstacle, testing distance-output suppression without rejecting the
  obstacle; passes when `OBM_NEW_GIVEN=true` and
  `OBM_DIST_TO_OBJ=OB_NEAR,18.0` is absent.

### Point-cluster and hull-generation cases

- `points_cluster_dist_pass`: Posts four `TRACKED_FEATURE` points under
  `key=rock_a`, testing conversion of a point cluster into an obstacle hull and
  its distance calculation; passes when `OBM_DIST_TO_OBJ=ROCK_A,18.0` and
  `17 < OBM_MIN_DIST_EVER < 19`.
- `custom_point_var_pass`: Sets `point_var=OBM_POINTS` and posts the same four
  points on `OBM_POINTS`, testing the configurable point-input subscription;
  passes when `OBM_DIST_TO_OBJ=ROCK_A,18.0` and
  `17 < OBM_MIN_DIST_EVER < 19`.
- `invalid_point_missing_key_absent_pass`: Posts four `TRACKED_FEATURE` points
  without `key=` or `label=`, testing rejection before cluster creation; passes
  when `OBM_DIST_TO_OBJ=GENERIC,18.0` is absent and `OBSTACLE_ALERT` remains
  empty.
- `max_pts_per_cluster_trim_pass`: Sets `max_pts_per_cluster=3` and posts five
  points under `key=rock_a`, exercising the capped-cluster path before hull
  construction; the current evaluator passes when
  `17 < OBM_MIN_DIST_EVER < 19` but does not distinguish the retained points.
- `points_ignore_range_absent_pass`: Sets `ignore_range=10` and posts a point
  cluster whose nearest point is 18 meters away, testing point rejection
  outside the input range; passes when `OBM_DIST_TO_OBJ=ROCK_A,18.0` is absent
  and `OBSTACLE_ALERT` remains empty.
- `points_age_resolve_pass`: Sets `max_age_per_point=3` and posts four points
  under `key=rock_a`, testing point-cluster expiration; passes at 10 seconds
  when `OBM_RESOLVED=rock_a`.
- `lasso_cluster_pass`: Sets `lasso=true` and `lasso_radius=5` for three
  collinear points centered 20 meters east of the vehicle, testing pseudo-hull
  construction instead of the normal convex hull; passes when
  `14 < OBM_MIN_DIST_EVER < 16`.
- `placeholder_hull_pass`: Posts three collinear points with lasso disabled,
  testing the placeholder-polygon fallback when a convex hull cannot be
  formed; passes when `19 < OBM_MIN_DIST_EVER < 21`.
- `post_view_polys_false_absent_pass`: Sets `post_view_polys=false` and posts a
  valid four-point cluster, testing that visualization can be suppressed
  without suppressing obstacle processing; passes when
  `OBM_DIST_TO_OBJ=ROCK_A,18.0` and no `VIEW_POLYGON` mail is observed.
- `new_obs_flag_macros_pass`: Configures
  `new_obs_flag=OBM_NEW_STATS=now_$[OBS_NOW]_ever_$[OBS_EVER]` and posts the
  first near obstacle, testing macro expansion of current and lifetime obstacle
  counts; passes when `OBM_NEW_STATS=now_1_ever_1` and
  `OBM_DIST_TO_OBJ=OB_NEAR,18.0`.
- `point_vsource_alert_pass`: Adds `vsource=radar` to every point in the
  `rock_a` cluster, testing preservation of the source tag in the generated
  alert; passes when `ALERT_SEEN=true`, `OBM_DIST_TO_OBJ=ROCK_A,18.0`, and the
  complete `OBSTACLE_ALERT` exactly matches the polygon payload containing
  `vsource=radar`.
- `given_after_points_same_key_reject_pass`: Builds the `mix_key` point cluster
  and then posts a `GIVEN_OBSTACLE` with the same label, testing key-ownership
  conflict handling; passes when `OBM_DIST_TO_OBJ=MIX_KEY,18.0` and the
  given-obstacle `OBM_NEW_GIVEN` flag is never set.

### Obstacle-modification cases

- `disable_obstacle_pass`: Sets `disable_var=OBM_DISABLE`, accepts `ob_near`,
  and posts `OBM_DISABLE=ob_near`, testing generation of
  `BHV_ABLE_FILTER=obstacle_id=ob_near,action=disable`; the current evaluator
  passes when any `BHV_ABLE_FILTER` mail sets `OB_FILTER_SEEN=true` and records
  the payload without comparing it.
- `disable_vsource_pass`: Posts `OBM_DISABLE=vsource=radar`, testing the
  source-selector branch of the modification parser; passes when
  `OB_FILTER_SEEN=true` and
  `BHV_ABLE_FILTER=vsource=radar,action=disable` exactly.
- `enable_obstacle_pass`: Sets `enable_var=OBM_ENABLE`, accepts `ob_near`, and
  posts `OBM_ENABLE=ob_near`, testing generation of
  `BHV_ABLE_FILTER=obstacle_id=ob_near,action=enable`; the current evaluator
  passes when any `BHV_ABLE_FILTER` mail sets `OB_FILTER_SEEN=true` and records
  the payload without comparing it.
- `expunge_obstacle_pass`: Sets `expunge_var=OBM_EXPUNGE`, accepts `ob_near`,
  and posts `OBM_EXPUNGE=ob_near`, testing generation of
  `BHV_ABLE_FILTER=obstacle_id=ob_near,action=expunge`; the current evaluator
  passes when any `BHV_ABLE_FILTER` mail sets `OB_FILTER_SEEN=true` and does not
  verify removal from the manager's internal map.

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
