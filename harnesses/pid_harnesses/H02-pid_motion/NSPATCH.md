# NSPATCH in H02-pid_motion

This harness uses `nspatch` to build case-local sidecars from the dedicated
`missions/pid_missions/pid_motion` stem:

- shoreside `.xmoos` files replace timer or `pMissionEval` blocks
- vehicle `.xmoos` files replace startup, PID, simulator, or timer blocks
- three vehicle `.xbhv` files replace waypoint/depth behavior blocks
- `nsplug -x` consumes the generated `meta_shoreside.moosx`,
  `meta_vehicle.moosx`, and `meta_vehicle.bhvx` files

The thirteen-case mapping contains exactly twenty-two `.xmoos` and three
`.xbhv` overlays. `baseline_transit_pass` uses the unpatched stem. Every other
mapping is declared explicitly in `zlaunch.sh`.

Every selected case starts from a separate cleaned copy of the stem, including
serial and one-case runs. Sidecars are therefore written only inside that case
copy and cannot leak into a later case or the source mission. The copied stem
also receives `--mmod=<case_name>`, so the mission-owned result identifies the
selected case even when no shoreside overlay is present.

Seven arrival-oriented evaluators use this simple-lead form:

```text
mailflag = @ARRIVED#TEST_EVAL_READY=true
lead_condition = TEST_EVAL_READY = true
```

The existing timer independently posts `TEST_EVAL_READY=true` at each case's
deadline. Under the current mission contract, `ARRIVED` has one one-shot
producer and only publishes `true`, so the mailflag preserves the former
arrival-or-deadline behavior without adding an application or variable. A
future `ARRIVED=false` initializer would also trigger a mailflag and must not be
added without revisiting this design.
