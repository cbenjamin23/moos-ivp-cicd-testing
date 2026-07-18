# upokedb_unit

Headless single-community stem for exercising `uPokeDB` as an app-under-test.
The harness launches this mission, runs `uPokeDB` against the live MOOSDB, and
lets `pMissionEval` grade the variables that should have been posted.
The case matrix covers direct host/port arguments, mission-file connection,
cache mode, host/server aliases, short option aliases, forced string posts,
numeric posts, time macros, duplicate overwrite ordering, mixed cache/CLI
pokes, and repeated batch behavior.

Typical harness run:

```sh
../../../harnesses/upokedb_harnesses/H01-upokedb_unit/zlaunch.sh --jobs=4 --port_base=15000 10
```

Run one inspectable case:

```sh
../../../harnesses/upokedb_harnesses/H01-upokedb_unit/zlaunch.sh --case=numeric_direct_pass --port_base=15000 10
```

The mission-local `zlaunch.sh` is a thin automation wrapper around
`xlaunch.sh`. It forwards the mission modifier, one-community MOOSDB port,
reserved pShare port, display mode, time warp, and `--max_time`; validates one
and only one pMissionEval result row; and uses the canonical root-scoped
teardown helper. A standalone run performs the baseline numeric `uPokeDB`
stimulus itself. The harness uses `--external_stimulus` so it can invoke each
case's exact CLI or cache-mode command after the copied MOOSDB starts without
duplicating `xlaunch.sh`/`uMayFinish` lifecycle logic.

`TEST_EVAL_READY=true` is posted by the tested `uPokeDB` command together with
the value under evaluation. No separate initializer app or additional grading
variable is needed.

Logging defaults to `minimal`, with no active `pLogger`; no log artifact is
part of grading. Pass `--log=full` to either mission launcher to restore the
previous logger configuration.
