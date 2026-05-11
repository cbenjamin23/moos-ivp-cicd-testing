# uField App Unit Mission

This shared stem mission supports app-level CI harnesses for `uFldPathCheck`,
`uFldMessageHandler`, and `uFldContactRangeSensor`. Each app has its own harness
family, but the families copy this single-community mission into isolated temp
directories, generate a case-specific `meta_shoreside.moosx`, and then grade the
app-under-test from the mission log.

Typical runs:

```bash
./launch.sh --just_make --xlaunched --nogui 10
../../harnesses/ufld_pathcheck_harnesses/H01-ufld_pathcheck_unit/zlaunch.sh --jobs=4
```

The mission itself is intentionally minimal: MOOSDB, `pLogger`, `pRealm`,
`uTimerScript`, `pMissionEval`, and the selected uField app. The app-specific
test traffic and pass/fail evidence are owned by the app-specific harness case
definition.
