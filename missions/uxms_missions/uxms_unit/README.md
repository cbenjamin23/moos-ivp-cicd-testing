# uxms_unit

Headless single-community publisher stem for exercising `uXMS` terminal
scoping behavior. `uTimerScript` publishes the deterministic XMS fixture, and
`pMissionEval` confirms `XMS_ALPHA=alpha` and `XMS_NUM=42` after the existing
`TEST_EVAL_READY` gate. The stem therefore owns fixture readiness while the
harness checks terminal output, which is the behavior `uXMS` exposes.

The thin `zlaunch.sh` wrapper forwards ports and `--max_time` through
`xlaunch.sh`, validates exactly one pMissionEval grade, waits briefly for a
late report write, and applies canonical scoped teardown. Its ordinary
standalone path posts `TEST_EVAL_READY=true`; `--external_client` leaves that
existing gate to the harness until the terminal capture finishes. No helper
application or new grading variable is involved. `form` and `mmod` are regular
report columns so pMissionEval writes the complete result row atomically during
xlaunch shutdown.

The case matrix also covers shortcut flags, alternate host/port option aliases,
process aliasing, event/paused/streaming display modes, all/alll display modes,
short option aliases, clean CLI overrides, no-match filters, and color-map
parsing while keeping the oracle to concrete terminal tokens plus the
mission-level readiness grade.

Typical harness run:

```sh
../../../harnesses/uxms_harnesses/H01-uxms_unit/zlaunch.sh --jobs=4 --port_base=15200 10
```

Run one inspectable case:

```sh
../../../harnesses/uxms_harnesses/H01-uxms_unit/zlaunch.sh --case=scoped_var_pass --port_base=15200 10
```

Run the publisher stem alone on an explicit port with:

```sh
./zlaunch.sh --shore_mport=15200 --shore_pshare=15215 --max_time=30 10
```

Logging defaults to `minimal`, with no active `pLogger`; terminal output and
the mission result provide the grading evidence. Pass `--log=full` to either
mission launcher to restore the previous logger configuration.
