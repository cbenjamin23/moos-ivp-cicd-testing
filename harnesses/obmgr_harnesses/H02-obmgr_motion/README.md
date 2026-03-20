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

The stem uses the lower-band Forrest 19 / MIT_SP framing, with the ownship
starting at `(0,-60)` and heading east along a `70m` corridor.

Run the full matrix:

```bash
cd /Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/obmgr_harnesses/H02-obmgr_motion
./zlaunch.sh 10
```

Run one case:

```bash
./zlaunch.sh --case=baseline_center_pass 10
```

Current cases:
- `baseline_center_pass`: one obstacle blocks the lane and the vehicle is expected to arrive cleanly with zero collisions.
- `offset_clear_pass`: one obstacle sits well above the transit lane, the vehicle should arrive with zero encounters.
- `tight_alert_pass`: the alert range is reduced but still large enough for a clean one-obstacle avoidance transit.
- `two_sequential_fail`: two obstacles are placed in sequence as a stress case; the short stem is expected to fail its normal clean-transit criteria.
- `wide_center_fail`: a wider centered obstacle exceeds what this short corridor stem can safely clear, so the baseline mission criteria are expected to fail.
- `avoid_disabled_fail`: obstacle alerts are redirected onto an unused variable instead of `OBSTACLE_ALERT`, so no avoid behavior spawns and the vehicle is expected to fail the normal mission criteria.

Typical results line:

```text
case=baseline_center_pass  expected=pass  actual=pass  status=ok  grade=pass  form=obavoid_tests_harness  mmod=baseline_center_pass  eval=true  arrived=true  collisions=0  near_misses=0  encounters=1  mhash=[NEAR-TODD]
```

Key fields:
- `expected` is the harness expectation for the case.
- `actual` is the mission grade parsed from the mission `results.txt`.
- `status` is the harness comparison result.
- `grade`, `form`, `mmod`, and the trailing metrics come from the mission-level `pMissionEval` report.
