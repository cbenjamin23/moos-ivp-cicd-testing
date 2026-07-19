# mission_utility_unit

Single-community utility-app stem for `pMissionEval`, `pMissionHash`, and
`uMayFinish` testing.

The mission publishes deterministic baseline test mail with `uTimerScript`.
`pMissionHash` generates mission hash mail, `pMissionEval` evaluates scripted
conditions and writes `results.txt`, and the harness launches `uMayFinish`
directly so the process exit code remains part of the tested contract.

The mission wrapper validates exactly one `pMissionEval` result row for live
standalone runs and performs canonical root-scoped teardown. The mission-level
`launch.sh` still supports direct target generation for the `uMayFinish`
harness, where the utility's own return code is the subject evidence.

Direct runs default to the shared twelve-variable grading logger. Pass
`--log=full` to restore wildcard synchronous/asynchronous diagnostic logging.

Typical commands:

```bash
./zlaunch.sh --mmod=eval_baseline_pass --port_base=9000 10
./zlaunch.sh --just_make --port_base=9000 10
```
