# pid_motion

Closed-loop motion stem mission for `pMarinePIDV22`.

The mission runs one simulated vehicle community and one shoreside evaluation
community. The vehicle uses `pHelmIvP` to publish desired heading, speed, and
optional depth setpoints, `pMarinePIDV22` to convert those setpoints into
actuator commands, and `uSimMarineV22` to close the loop. Shoreside
`pMissionEval` grades the observed motion through bridged nav and actuator
variables.

## Scenario

The default stem starts vehicle `abe` at `(0,-60)` heading east toward a waypoint
at `(70,-60)`. It auto-deploys, releases manual override, and evaluates after a
short fixed window.

## Evaluation

The default case expects the simulated vehicle to arrive near the target
corridor without excessive speed. Harness patches may change the starting
heading, PID speed mode, depth-control configuration, or evaluation conditions.

## Typical Runs

```bash
./launch.sh 10
./zlaunch.sh 10
./zlaunch.sh --just_make 10
```

The patch-driven harness at
[`harnesses/pid_harnesses/H02-pid_motion`](../../../harnesses/pid_harnesses/H02-pid_motion)
reuses this stem for closed-loop heading, speed, depth, and control-authority
checks.
