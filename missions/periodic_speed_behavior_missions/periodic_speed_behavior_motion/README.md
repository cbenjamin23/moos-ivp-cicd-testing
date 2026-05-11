# periodic_speed_behavior_motion

Deterministic moving stem mission for `BHV_PeriodicSpeed`.

The vehicle runs a constant-heading behavior so the periodic speed behavior can
own the speed objective. The shoreside evaluator grades the behavior-owned
status variables (`PS_BUSY_COUNT`, `PS_PENDING_ACTIVE`,
`PS_PENDING_INACTIVE`, and `PERIODIC_SPEED`) plus observed `NAV_SPEED`.

Typical commands:

```bash
./launch.sh --just_make --nogui 10
./zlaunch.sh 10
```

Harness cases are in
`harnesses/periodic_speed_behavior_harnesses/H01-periodic_speed_behavior_motion`.
