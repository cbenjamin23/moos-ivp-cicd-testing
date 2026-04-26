# fixedturn_behavior_motion

Stem mission for moving correctness checks of `BHV_FixedTurn`.

The mission launches one simulated vehicle. The vehicle first runs a short
waypoint leg to establish a nonzero stem speed, then the waypoint behavior
raises `FIX_TURN=true` and hands control to `BHV_FixedTurn`.

Evaluation is mission-owned:

- `pAutoPoke` deploys the vehicle.
- `BHV_FixedTurn` posts `FT_DONE`, `TURNING_TIME`, `TURNING_DIST`, and
  `FT_REPORT`.
- `pMissionEval` writes the verdict to `results.txt`.
- `uMayFinish` is driven by `xlaunch.sh` through `zlaunch.sh`.

Typical commands:

```bash
./zlaunch.sh 10
./zlaunch.sh --just_make 10
./launch.sh --shore_mport=15000 --veh_mport=15001 --shore_pshare=16000 --veh_pshare=16001 --nogui 10
```

The companion harness lives at
`harnesses/fixedturn_behavior_harnesses/H01-fixedturn_behavior_motion`.
