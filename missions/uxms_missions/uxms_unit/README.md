# uxms_unit

Headless single-community stem for exercising `uXMS` terminal scoping behavior.
The harness launches a controlled publisher mission, runs bounded `uXMS`
captures, and grades the terminal output for scoped variables, columns,
history, truncation, and mission-file configuration behavior.
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
