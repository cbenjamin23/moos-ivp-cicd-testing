# H02-obmgr_motion

Patch-driven harness for the moving obstacle-avoidance stem in
[`/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/obmgr_missions/obmgr_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/obmgr_missions/obmgr_motion).

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
./zlaunch.sh --jobs=4 --port_base=28500 10
```

Run one case:

```bash
./zlaunch.sh --case=baseline_center_pass 10
```

Wave mode notes:

- `--jobs=<N>` runs the matrix in waves of up to `N` isolated case copies
- each live case in a wave gets its own temp mission directory and unique port
  block
- the harness uses scoped mission teardown between waves rather than global
  `ktm`
- this mode is intended for CI wall-clock reduction, not for interactive use
  alongside other MOOS missions

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

Latest validation:

- April 27, 2026
- full matrix: `10/10` expected outcomes matched
- command: `./zlaunch.sh --jobs=4 --port_base=15000 10`
- warp: `10`
