# H02-obmgr_motion

Patch-driven harness for the moving obstacle-avoidance stem in
[`missions/obmgr_missions/obmgr_motion`](../../../missions/obmgr_missions/obmgr_motion).

This harness is intentionally smaller than the app-level suites. Each case is a
short integration check over the same moving mission:
- ownship launches and auto-deploys
- ownship drives a short transit corridor
- `pObstacleMgr` publishes `OBSTACLE_ALERT`
- `BHV_AvoidObstacleV24` reacts in the helm
- shoreside grades on arrival plus collision outcome, and only uses encounter
  count where that count is stable enough to be meaningful
- expected collision stress cases now grade `pass` when the collision evidence
  is observed, so the harness reports the mission row directly

The stem uses the lower-band Forrest 19 / MIT_SP framing, with the ownship
starting at `(0,-60)` and heading east along a `70m` corridor.

Run the full matrix:

```bash
cd /Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/obmgr_harnesses/H02-obmgr_motion
./zlaunch.sh 10
./zlaunch.sh --log=full 10
./zlaunch.sh --jobs=2 --port_base=24000 10
```

Run one case:

```bash
./zlaunch.sh --case=baseline_center_pass 10
```

Execution notes:

- logging defaults to `minimal`; `--log=full` restores the previous stem
  logger configuration before applying each case patch
- every selected case, including single-case and `--jobs=1` runs, uses an
  isolated mission copy and a unique port block
- `--jobs=<N>` uses rolling scheduling: the next pending case starts whenever
  any active case finishes
- final result rows are aggregated in the case-list order, not completion
  order
- `--keep_workdirs` preserves the current invocation's unique directory under
  `.harness_runs/`
- `--gui` requires one explicit `--case`; full matrices remain headless
- cleanup is scoped to the invocation root; the harness does not use global
  `ktm`, `pkill`, or `killall`

The harness owns case selection, isolated copies, overlays, ports, scheduling,
and ordered aggregation. Each copied stem owns launch and grading. A normal
case row retains the single `pMissionEval` `grade=` field and evidence exactly
as written by the mission, with only `case=<name>` prepended. Runner failures
instead use `grade=fail reason=<runner_reason>`.

## Cases
- `baseline_center_pass`: One obstacle blocks the lane and the vehicle is expected to arrive cleanly with zero collisions.
- `offset_clear_pass`: One obstacle sits well above the transit lane, the vehicle should arrive with zero encounters.
- `tight_alert_pass`: The alert range is reduced but still large enough for a clean one-obstacle avoidance transit.
- `runtime_given_late_pass`: The obstacle is mailed to `pObstacleMgr` after launch with `GIVEN_OBSTACLE` instead of being present in startup config; the vehicle should still receive a usable alert and complete cleanly.
- `point_cluster_pass`: The obstacle is built from `TRACKED_FEATURE` points and a generated convex hull instead of a configured polygon; the resulting alert should still support a clean avoidance transit.
- `lasso_cluster_pass`: The obstacle is built from point inputs with `lasso=true`, exercising the pseudo-hull branch while still requiring clean transit.
- `two_sequential_fail`: Two obstacles are placed in sequence as a stress case; the case passes when the mission observes arrival, two encounters, and one collision.
- `wide_center_fail`: A wider centered obstacle exceeds what this short corridor stem can safely clear; the case passes when the mission observes arrival, one encounter, and one collision.
- `avoid_disabled_fail`: Obstacle alerts are redirected onto an unused variable instead of `OBSTACLE_ALERT`; the case passes when the mission observes arrival, one encounter, and one collision.
- `no_alert_request_fail`: The obstacle exists in `pObstacleMgr`, but no `OBM_ALERT_REQUEST` is sent; the case passes when the mission observes arrival, one encounter, and one collision.

Typical results line:

```text
case=baseline_center_pass  grade=pass  form=obavoid_tests_harness  mmod=baseline_center_pass  eval=true  arrived=true  collisions=0  near_misses=0  encounters=1  mhash=[NEAR-TODD]
```

Key fields:
- `case` is the harness-owned case identifier.
- `grade` is the mission-owned case verdict; `pass` means the case passed.
- `expected=collision` appears on expected collision stress cases.
- `form`, `mmod`, and the trailing metrics come from the mission-level `pMissionEval` report.

Latest migration validation:

- July 13, 2026
- isolated serial matrix: `10/10` pass in 110.13 seconds
- three rolling `--jobs=2 --port_base=24000 10` matrices: `30/30`
  published rows pass, with wall times 58.18, 55.16, and 56.77 seconds
- rolling mean: 56.70 seconds, about 1.8 percent faster than the 57.74-second
  three-run legacy mean on the same host and settings
- all four expected-collision cases retained mission-owned `grade=pass`,
  `expected=collision`, and the required collision/encounter evidence
- an intentional `--max_time=1` matrix published one ordered
  `reason=missing_result` row for each of all ten cases and exited nonzero
- the final full matrix passed `10/10` in 56.15 seconds and left no scoped
  process or listener behind

Skill 1.4.2 alignment, validated on July 14, 2026:

- the stem's Apple Bash 3.2 paths safely handle empty GUI/forwarding arrays and
  report zero or duplicate mission rows without an unbound-variable error
- runner-owned launch failures retain a valid mission row as renamed
  `mission_*` provenance instead of discarding it
- `baseline_center_pass` passed in `11.69` seconds and the expected-collision
  `wide_center_fail` case passed in `10.83` seconds
- the isolated serial matrix passed `10/10` in `112.37` seconds
- three canonical rolling matrices passed `30/30` in `58`, `58`, and `57`
  seconds, for a `57.67`-second mean
- the `0.97`-second difference from the earlier `56.70`-second mean is within
  the earlier `55.16`-to-`58.18` observed range
- a retained `58.25`-second rolling run confirmed immediate refill, all 19
  intended sidecars, 40 unique listener ports, four expected-collision pass
  rows, and no log warnings or cleanup residue

No case, expected-collision condition, mission variable, geometry, port layout,
or scheduler policy changed during this alignment.
