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
