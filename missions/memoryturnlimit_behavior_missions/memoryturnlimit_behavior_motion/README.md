# memoryturnlimit_behavior_motion

Stem mission for moving correctness checks of `BHV_MemoryTurnLimit`.

The mission launches one simulated surface vehicle. `BHV_ConstantSpeed` keeps
the vehicle moving, `BHV_ConstantHeading` applies a sustained turn demand, and
`BHV_MemoryTurnLimit` constrains the course objective around the recent
ownship heading average.

Evaluation is mission-owned:

- `pAutoPoke` deploys the vehicle and resets observation variables.
- `BHV_MemoryTurnLimit` posts `MEM_HDG_AVG`.
- `BHV_ConstantHeading` posts `HEADING_MISMATCH`.
- `pMissionEval` writes the verdict to `results.txt`.
- `uMayFinish` is driven by `xlaunch.sh` through the harness launcher.

Typical commands:

```bash
./zlaunch.sh 10
./zlaunch.sh --just_make 10
./launch.sh --shore_mport=15000 --veh_mport=15001 --shore_pshare=15010 --veh_pshare=15011 --nogui 10
```

Logging is minimal by default. Add `--log=full` to either launcher to restore
the stem's original shoreside and vehicle `pLogger` configuration.

The companion harness lives at
`harnesses/memoryturnlimit_behavior_harnesses/H01-memoryturnlimit_behavior_motion`.
