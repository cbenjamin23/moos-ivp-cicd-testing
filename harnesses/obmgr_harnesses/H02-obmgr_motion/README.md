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
cd /path/to/moos-ivp-cicd-testing/harnesses/obmgr_harnesses/H02-obmgr_motion
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

- `baseline_center_pass`: Starts `pObstacleMgr` with a 14-by-14-meter
  `given_obstacle` centered on the vehicle's route and requests alerts within
  22 meters, testing that the obstacle is reported early enough for the
  avoidance behavior to steer around it; passes when `ARRIVED=true` and
  `OB_TOTAL_COLLISIONS=0`.
- `offset_clear_pass`: Moves the same 14-by-14-meter `given_obstacle` so its
  nearest edge is 14 meters north of the route, testing that an off-lane
  obstacle does not create a collision-detector encounter; passes when
  `ARRIVED=true`, `OB_TOTAL_COLLISIONS=0`, and `OB_TOTAL_ENCOUNTERS=0`.
- `tight_alert_pass`: Keeps the centered startup obstacle but reduces the
  `OBM_ALERT_REQUEST` range from 22 to 16 meters, testing whether the shorter
  warning still leaves enough room for avoidance; passes when `ARRIVED=true`
  and `OB_TOTAL_COLLISIONS=0`.
- `runtime_given_late_pass`: Posts `GIVEN_OBSTACLE` at three seconds, after the
  `OBM_ALERT_REQUEST` at one second, testing runtime obstacle insertion and
  alert generation; passes when `ARRIVED=true` and `OB_TOTAL_COLLISIONS=0`.
- `point_cluster_pass`: Posts four `TRACKED_FEATURE` points under the same key
  to test `pObstacleMgr`'s generated convex-hull path instead of its configured
  polygon path; passes when `ARRIVED=true` and `OB_TOTAL_COLLISIONS=0`.
- `two_sequential_fail`: Places two 8-by-8-meter obstacles in the transit lane
  at separate points along the route, testing consecutive obstacle encounters;
  passes when `ARRIVED=true`, `OB_TOTAL_ENCOUNTERS=2`, and
  `OB_TOTAL_COLLISIONS=1`.
- `wide_center_fail`: Replaces the baseline polygon with a 28-by-28-meter
  obstacle centered on the route, testing the collision outcome when the
  obstacle is too wide for this short corridor; passes when `ARRIVED=true`,
  `OB_TOTAL_ENCOUNTERS=1`, and `OB_TOTAL_COLLISIONS=1`.
- `avoid_disabled_fail`: Starts with an 18-by-16-meter obstacle centered on the
  route but directs the alert request to `DISABLED_ALERT` instead of the
  behavior's `OBSTACLE_ALERT` update variable, testing the missing behavior
  update path; passes when `ARRIVED=true`, `OB_TOTAL_ENCOUNTERS=1`, and
  `OB_TOTAL_COLLISIONS=1`.
- `no_alert_request_fail`: Starts with the same centered obstacle but sends no
  `OBM_ALERT_REQUEST`, testing that `pObstacleMgr` does not publish an alert
  without a request; passes when `ARRIVED=true`, `OB_TOTAL_ENCOUNTERS=1`, and
  `OB_TOTAL_COLLISIONS=1`.

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
