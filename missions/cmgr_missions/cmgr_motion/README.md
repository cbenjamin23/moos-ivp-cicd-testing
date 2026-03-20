# cmgr_motion

Deterministic moving contact-avoidance stem mission.

This mission is the moving integration companion to the app-level
`cmgr_unit` suite. It keeps the stack small and deterministic:

- one ownship with `pHelmIvP`, `pMarinePIDV22`, and `BHV_AvoidCollision`
- one short left-to-right transit
- one default spoofed on-lane contact from `pSpoofNode`
- onboard `pContactMgrV20` generating `CONTACT_INFO`
- shoreside grading with `uFldCollisionDetect` and `pMissionEval`

Scenario:

- the vehicle starts at `(0,-60)`
- the goal waypoint is `(70,-60)`
- a default intruder appears near the middle of the lane
- the shoreside viewer uses `MIT_SP.tif` with `set_pan_x=0`, `set_pan_y=-300`,
  and `zoom=1.0`
- `pAutoPoke` auto-deploys and initializes the collision counters
- a vehicle-local `uTimerScript` publishes the default spoof event
- `pContactMgrV20` turns the encounter into `CONTACT_INFO`
- `BHV_AvoidCollision` spawns from `CONTACT_INFO`
- `pMissionEval` grades at a timer checkpoint rather than waiting forever in
  failing cases

Config note:

- `pSpoofNode` uses `default_vtype` and `default_vsource` in this stem. Those
  are the supported parameter names in the local app source and avoid the
  warning that the older `default_type` / `default_source` names would produce.

Default pass rule:

- `TEST_EVAL_READY=true`
- `ARRIVED=true`
- `COLLISION_TOTAL=0`
- `CONTACT_DETECTED=true`

Implementation notes:

- this stem uses a real moving ownship, but it is still intentionally small
  and corridor-like so the harness can patch geometry deterministically
- the companion harness patches `pContactMgrV20`, `uTimerScript`, and
  `pMissionEval` to vary encounter geometry, timing, and grading rules
- the companion harness is
  [`/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/cmgr_harnesses/H02-cmgr_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/cmgr_harnesses/H02-cmgr_motion)

Current moving harness summary:

- harness size: `9` cases
- latest full run on March 20, 2026: `9/9` passing at warp `10`
- latest wall clock for the full harness: about `97` seconds

Entry points:

- `./launch.sh` for interactive runs
- `./launch.sh --just_make --nogui 10` for target generation
- `./zlaunch.sh` for automated headless runs through shared `xlaunch.sh`
