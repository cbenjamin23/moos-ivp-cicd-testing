# periodic_speed_behavior_motion

Deterministic moving stem mission for `BHV_PeriodicSpeed`.

The vehicle runs a constant-heading behavior and a low-priority zero-speed
objective so the helm remains fully defined during PeriodicSpeed's lazy window;
the higher-priority periodic behavior owns speed during its busy window. At a
scheduled clock sample, the shoreside evaluator grades the case-specific
`PS_BUSY_COUNT`, pending-time, and `NAV_SPEED` contract, requires the sample
before its fallback deadline, and rejects a behavior error or any helm state
other than `DRIVE`.

Typical commands:

```bash
./launch.sh --just_make --nogui 10
./zlaunch.sh 10
```

Logging is minimal by default. Add `--log=full` to either launcher to restore
the stem's original shoreside and vehicle `pLogger` configuration.

Harness cases are in
`harnesses/periodic_speed_behavior_harnesses/H01-periodic_speed_behavior_motion`.
