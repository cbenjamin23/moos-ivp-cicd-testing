# H03-umayfinish_unit

Patch-driven unit harness for `uMayFinish`.

The harness uses isolated copies of the stem mission at
`missions/mission_utility_missions/mission_utility_unit`, but intentionally
launches `uMayFinish` directly with a unique alias per case so the process exit
code and timeout remain the tested contract. This direct path is the family's
CLI-subject exception to the ordinary `xlaunch` lifecycle.

Exported harness rows use `grade=<pass|fail>` for the harness verdict and
`subject=uMayFinish` for the utility under test. Expected timeout cases pass the
harness when `expected_subject=timeout` and the observed `subject_rc=1`.

The direct command passes `targ_shoreside.moos` first, as required by the
uMayFinish CLI, and allows 0.5 seconds for finish evidence to reach pLogger
before teardown. Logging defaults to the shared grading allowlist; use
`--log=full` for the original wildcard diagnostic logger.

## Current Matrix

- `mayfinish_default_exit_pass` Runs a directly launched, uniquely aliased `uMayFinish` with its default finish condition while pMissionEval posts `MISSION_EVALUATED=true`, testing normal completion; passes when `uMayFinish` exits with code 0 and the evaluator's result row grades pass.
- `mayfinish_custom_finish_pass` Configures the aliased process with `finish_var=CUSTOM_DONE` and `finish_val=complete`, then has pMissionEval post that exact pair, testing a custom finish condition; passes when the `.alog` contains `CUSTOM_DONE=complete`, `uMayFinish` exits with code 0, and the evaluator grades pass.
- `mayfinish_custom_value_timeout_fail` Configures `finish_var=CUSTOM_DONE` and `finish_val=complete` but has pMissionEval produce `CUSTOM_DONE=almost`, exercising rejection of a mismatched finish value; the harness passes when the directly launched `uMayFinish` reaches its six-second limit and exits with code 1.
- `mayfinish_timeout_fail` Configures pMissionEval behind the unmet lead condition `NEVER_READY=true`, so `MISSION_EVALUATED` is never intentionally posted, testing the direct `--max_time` path; the harness passes when `uMayFinish` reaches its two-second limit and exits with code 1.

## Running

```bash
./zlaunch.sh --case=mayfinish_default_exit_pass --port_base=9000 10
./zlaunch.sh --jobs=3 --port_base=9000 10
./zlaunch.sh --just_make --jobs=3 --port_base=9000 10
```

Serial and rolling runs use the same isolated-case path. Grouped runs use
30-port case blocks from `--port_base`; the default starts at `9000`.
