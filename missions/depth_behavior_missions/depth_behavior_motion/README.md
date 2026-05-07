# depth_behavior_motion

Single-vehicle UUV/depth behavior stem for `BHV_ConstantDepth`,
`BHV_GoToDepth`, `BHV_PeriodicSurface`, `BHV_MaxDepth`, and
`BHV_MinAltitude` motion tests.

The mission runs one simulated vehicle with `pHelmIvP`, `pMarinePIDV22`, and
`uSimMarineV22`. The waypoint behavior keeps the vehicle moving east through a
short corridor; the depth behavior is the behavior under test. Shoreside
`pMissionEval` grades bridged navigation, depth telemetry, behavior-owned flags,
and actuator evidence.

Typical runs:

```bash
./launch.sh --just_make 10
./zlaunch.sh 10
```

The harnesses under `harnesses/depth_behavior_harnesses/` apply
behavior-specific patches and evaluator checks used in CI. Each behavior has its
own harness, while all five share this mission stem.
