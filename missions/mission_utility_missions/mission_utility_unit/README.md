# mission_utility_unit

Single-community utility-app stem for `pMissionEval`, `pMissionHash`, and
`uMayFinish` testing.

The mission publishes deterministic baseline test mail with `uTimerScript`.
`pMissionHash` generates mission hash mail, `pMissionEval` evaluates scripted
conditions and writes `results.txt`, and the harness launches `uMayFinish`
directly so the process exit code remains part of the tested contract.

Typical commands:

```bash
./zlaunch.sh --mmod=eval_baseline_pass --port_base=7100 10
./zlaunch.sh --just_make --port_base=7100 10
```
