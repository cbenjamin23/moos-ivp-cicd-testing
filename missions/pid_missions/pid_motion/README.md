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

Arrival-oriented evaluators use the existing `TEST_EVAL_READY` lead signal.
`pMissionEval` sets it when `ARRIVED` mail is received, and `uTimerScript` sets
the same signal at the deadline. This keeps the lead condition simple while
preserving arrival-or-deadline evaluation. `ARRIVED` currently has one one-shot
producer that only publishes `true`.

## Typical Runs

```bash
./launch.sh 10
./zlaunch.sh 10
./zlaunch.sh --just_make 10
./zlaunch.sh --mmod=baseline_transit_pass --shore_mport=26000 \
  --veh_mport=26001 --shore_pshare=26015 --veh_pshare=26016 10
```

The patch-driven harness at
[`harnesses/pid_harnesses/H02-pid_motion`](../../../harnesses/pid_harnesses/H02-pid_motion)
reuses this stem for closed-loop heading, speed, depth, and control-authority
checks.

The thin `zlaunch.sh` forwards `--mmod`, `--max_time`, and all four explicit
ports to the shared launch chain. In preparation mode it requires nonempty
`targ_shoreside.moos`, `targ_abe.moos`, and `targ_abe.bhv`. During a live run it
does not grade the mission: `pMissionEval` writes exactly one
`grade=pass|fail` row, while the wrapper validates that schema, preserves the
launch return code, and performs canonical root-scoped teardown.

The July 14, 2026 migration validation passed all thirteen cases in one
isolated serial matrix and three rolling two-worker matrices. No mission
condition, PID setting, behavior patch, or expected-degraded contract was
changed beyond the equivalent simple-lead normalization described above.

Logging is minimal by default and does not launch `pLogger`. Use `--log=full`
with `launch.sh` or `zlaunch.sh` to restore the pre-migration logging topology.
