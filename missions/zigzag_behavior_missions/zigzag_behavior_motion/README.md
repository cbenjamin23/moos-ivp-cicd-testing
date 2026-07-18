# zigzag_behavior_motion

Stem mission for moving correctness checks of `BHV_ZigZag`.

The mission launches one simulated surface vehicle. The vehicle first runs a
short waypoint leg to establish a nonzero navigation state, then the waypoint
behavior raises `ZIGZAG=true` and hands control to `BHV_ZigZag`.

Evaluation is mission-owned:

- `pAutoPoke` deploys the vehicle and resets behavior-observation variables.
- `BHV_ZigZag` posts `ZZ_DONE`, side-transition flags, zig/zag counts, and
  stem-line telemetry through behavior macros.
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
`harnesses/zigzag_behavior_harnesses/H01-zigzag_behavior_motion`.
