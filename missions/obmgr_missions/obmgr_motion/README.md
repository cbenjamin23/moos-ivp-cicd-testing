# obmgr_motion

Deterministic moving obstacle-avoidance stem mission.

This mission is the moving integration companion to the app-level
`obmgr_unit` suite. It keeps the obstacle-avoidance stack small and
deterministic:

- one ownship with `pHelmIvP`, `pMarinePIDV22`, and `BHV_AvoidObstacleV24`
- one short left-to-right transit
- one fixed obstacle in the transit lane by default
- shoreside grading with `uFldCollObDetect` and `pMissionEval`

Scenario:

- the vehicle starts at `(0,-60)`
- the goal waypoint is `(70,-60)`
- a single rectangular obstacle sits in the lane near the middle of that
  corridor
- the shoreside viewer uses `MIT_SP.tif` with `set_pan_x=0`, `set_pan_y=-300`,
  and `zoom=1.0`
- `pAutoPoke` auto-deploys and initializes the mission counters
- a vehicle-local `uTimerScript` posts the `OBM_ALERT_REQUEST` that binds
  `pObstacleMgr` to `OBSTACLE_ALERT`
- a short shoreside `uTimerScript` publishes the matching `KNOWN_OBSTACLE`
  after deploy so `uFldCollObDetect` is already running when it receives the
  obstacle spec
- `pMissionEval` grades at a timer checkpoint rather than waiting forever for
  arrival in failing cases

Default pass rule:

- `TEST_EVAL_READY=true`
- `ARRIVED=true`
- `OB_TOTAL_COLLISIONS=0`

The default stem reports `OB_TOTAL_ENCOUNTERS`, but it does not require a fixed
encounter count in the default pass rule. The moving harness uses encounter
count only in cases where that count proved stable enough to be worth grading.

Implementation notes:

- this stem is derived from the moving obstacle-avoidance family, but it uses a
  fixed obstacle specification rather than `uFldObstacleSim` so the harness can
  patch geometry deterministically
- the companion harness patches `pObstacleMgr`, `pAutoPoke`, `uTimerScript`,
  and `pMissionEval` to vary obstacle geometry, alert timing, and grading rules
- the companion harness is
  [`/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/obmgr_harnesses/H02-obmgr_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/obmgr_harnesses/H02-obmgr_motion)

Entry points:

- `./launch.sh` for interactive runs
- `./launch.sh --just_make --nogui 10` for target generation
- `./zlaunch.sh` for one automated headless run through shared `xlaunch.sh`;
  it accepts explicit shoreside/vehicle MOOSDB and pShare ports, verifies that
  `pMissionEval` wrote a `grade=`, and performs root-scoped teardown
