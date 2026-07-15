# pid_unit

Unit-style stem mission for `pMarinePIDV22`.

The mission runs a single MOOS community with `pMarinePIDV22`, scripted input
mail from `uTimerScript`, and mission-owned grading from `pMissionEval`. It is
intentionally not a closed-loop vehicle simulation. The goal is to prove the
PID app's direct input/output contract before adding any motion-level dynamics.

## Scenario

The default stem posts a manual-override release, desired heading/speed, and
current nav heading/speed. `pMarinePIDV22` should publish suffixed actuator
commands such as `DESIRED_RUDDER_PID` and `DESIRED_THRUST_PID`.

## Evaluation

The default case expects a positive rudder command and a thrust command near
`40` from `speed_factor=20` and `DESIRED_SPEED=2`.

## Typical Runs

```bash
./launch.sh 10
./zlaunch.sh 10
./zlaunch.sh --just_make 10
```

`zlaunch.sh` is the self-evaluating wrapper used by the harness. It accepts
explicit shoreside MOOSDB and reserved pShare ports, forwards `--max_time` and
display mode to `xlaunch.sh`, validates `targ_shoreside.moos` in
`--just_make`, and requires exactly one valid `pMissionEval` result row after
a live run. It preserves launch failure status and performs canonical
root-scoped teardown. The mission itself launches no pShare application.

The patch-driven harness at
[`harnesses/pid_harnesses/H01-pid_unit`](../../../harnesses/pid_harnesses/H01-pid_unit)
reuses this stem for heading, speed, depth, override, stale-input, and debug
output cases.
