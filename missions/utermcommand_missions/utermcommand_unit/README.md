# utermcommand_unit

Headless single-community stem for exercising `uTermCommand` as an
app-under-test. The harness runs `uTermCommand` externally with deterministic
stdin keystrokes, then `pMissionEval` grades the posted variables.

Typical harness run:

```sh
../../../harnesses/utermcommand_harnesses/H01-utermcommand_unit/zlaunch.sh --jobs=4 --port_base=16200 10
```

Run one inspectable case:

```sh
../../../harnesses/utermcommand_harnesses/H01-utermcommand_unit/zlaunch.sh --case=numeric_command_pass --port_base=16200 10
```
