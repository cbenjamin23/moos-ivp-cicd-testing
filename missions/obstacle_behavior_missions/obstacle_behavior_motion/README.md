# obstacle_behavior_motion

Deterministic moving behavior-correctness stem mission for
`BHV_AvoidObstacleV24`.

This mission is the downstream behavior companion to the existing
`obmgr_motion` suite. The obstacle manager is still present, but the stem is
wired so the behavior owns the `OBM_ALERT_REQUEST` path and the evaluation
focus is on behavior signals rather than obstacle-manager parsing.

Scenario:

- one ownship starts at `(0,-60)` and drives east to `(70,-60)`
- one fixed rectangular obstacle sits in the lane near the middle
- `BHV_AvoidObstacleV24` auto-requests `OBSTACLE_ALERT`
- shoreside grades behavior-owned outcomes such as alert request seen,
  obstacle alert seen, `OBAVOIDING=end`, and clean arrival

The companion harness expands this stem into flag-path, launch-time polygon,
non-engagement, multi-obstacle, disabled-behavior, and invalid-configuration
variants.

Default pass rule:

- `TEST_EVAL_READY=true`
- `ALERT_REQUEST_SEEN=true`
- `OBSTACLE_ALERT_SEEN=true`
- `OBAVOIDING=end`
- `ARRIVED=true`
- `OB_TOTAL_COLLISIONS=0`

Entry points:

- `./launch.sh`
- `./launch.sh --just_make --nogui 10`
- `./zlaunch.sh`

Companion harness:

- [`/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/obstacle_behavior_harnesses/H01-obstacle_behavior_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/obstacle_behavior_harnesses/H01-obstacle_behavior_motion)
