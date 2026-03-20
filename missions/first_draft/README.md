# first_draft

Single-vehicle obstacle-avoidance mission for validating:

- `pAutoPoke`
- `pMissionEval`
- `uMayFinish`

Scenario:

- the vehicle starts on the left at `(0,-60)`
- the goal waypoint is on the right at `(170,-60)`
- one static obstacle sits directly in the transit lane
- `pAutoPoke` auto-deploys once the vehicle connects
- `pAutoPoke` also publishes the static `KNOWN_OBSTACLE` used by shoreside grading
- `pMissionEval` marks the mission evaluated when the vehicle posts `ARRIVED=true`
- the mission passes only if `OB_TOTAL_COLLISIONS=0`

Implementation notes:

- the vehicle avoids a static `given_obstacle` configured in `pObstacleMgr`
- shoreside grading uses `uFldCollObDetect` fed by the same obstacle spec
- this mission does not use `uFldObstacleSim`

Entry points:

- `./launch.sh` for interactive runs
- `./launch.sh --nogui --xlaunched 10` for headless scripted launch
- `./zlaunch.sh` for automated launch via the generic MOOS-IvP `xlaunch.sh`
