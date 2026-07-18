# timer_behavior_motion

Stem mission for moving correctness checks of `BHV_Timer`.

The mission launches one simulated vehicle on a short waypoint leg. A timer
behavior is configured with `TIMER_ACTIVE=false` at startup and is activated by
the shoreside script after deployment. The mission grades the behavior-owned
idle and running duration counters rather than vehicle arrival.

Evaluation is mission-owned:

- `pAutoPoke` deploys the vehicle and initializes the timer flags.
- `uTimerScript` switches `TIMER_ACTIVE=true` and later posts
  `TEST_EVAL_READY=true`.
- `BHV_Timer` posts idle and running duration counters.
- `pMissionEval` writes the verdict to `results.txt`.
- `uMayFinish` is driven by `xlaunch.sh` through `zlaunch.sh`.

Typical commands:

```bash
./zlaunch.sh 10
./zlaunch.sh --just_make 10
./launch.sh --shore_mport=15000 --veh_mport=15001 --shore_pshare=16000 --veh_pshare=16001 --nogui 10
```

Logging is minimal by default. Add `--log=full` to either launcher to restore
the stem's original shoreside and vehicle `pLogger` configuration.

The companion harness lives at
`harnesses/timer_behavior_harnesses/H01-timer_behavior_motion`.
