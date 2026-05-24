# H03-umayfinish_unit

Patch-driven unit harness for `uMayFinish`.

The harness uses the stem mission at
`missions/mission_utility_missions/mission_utility_unit`, but launches
`uMayFinish` directly with a unique alias per case so the process exit code is
part of the tested contract.

Exported harness rows use `grade=<pass|fail>` for the harness verdict and
`subject=uMayFinish` for the utility under test. Expected timeout cases pass the
harness when `expected_subject=timeout` and the observed `subject_rc=1`.

## Current Matrix

- `mayfinish_default_exit_pass` Direct default-exit case. The harness requires `uMayFinish` to exit with code 0 when `MISSION_EVALUATED=true` arrives.
- `mayfinish_custom_finish_pass` Custom finish-variable case. A uniquely aliased `uMayFinish` reads a matching config block and exits on `CUSTOM_DONE=complete`.
- `mayfinish_custom_value_timeout_fail` Custom finish-value mismatch case. `CUSTOM_DONE` is posted with the wrong value, so the custom `uMayFinish` instance must ignore the default mission-evaluated flag and time out.
- `mayfinish_timeout_fail` Direct timeout case. The mission never posts the finish variable, so `uMayFinish --max_time` must exit with code 1.

## Running

```bash
./zlaunch.sh --case=mayfinish_default_exit_pass --port_base=7400 10
./zlaunch.sh --jobs=3 --port_base=7400 10
./zlaunch.sh --just_make --jobs=3 --port_base=7400 10
```

Grouped runs use 10-port case blocks from `--port_base`. The default starts at
`7400`.
