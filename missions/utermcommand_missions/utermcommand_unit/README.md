# utermcommand_unit

Headless single-community stem for exercising `uTermCommand` as an
app-under-test. The harness runs `uTermCommand` externally with deterministic
stdin keystrokes, posts the existing evaluation-ready event, and then
`pMissionEval` grades the posted variables. The mission wrapper uses the shared
`xlaunch.sh` lifecycle and scoped teardown helper.

The stem can also validate its default numeric case directly:

```sh
./zlaunch.sh --shore_mport=16200 --shore_pshare=16215 --max_time=180 10
```

Typical harness run:

```sh
../../../harnesses/utermcommand_harnesses/H01-utermcommand_unit/zlaunch.sh --jobs=4 --port_base=16200 10
```

Run one inspectable case:

```sh
../../../harnesses/utermcommand_harnesses/H01-utermcommand_unit/zlaunch.sh --case=numeric_command_pass --port_base=16200 10
```

Logging defaults to `minimal`, with no active `pLogger`; terminal stimulus and
the mission result provide the grading evidence. Pass `--log=full` to either
mission launcher to restore the previous logger configuration.
