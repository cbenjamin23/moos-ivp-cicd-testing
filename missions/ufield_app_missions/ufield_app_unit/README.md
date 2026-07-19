# uField App Unit Mission

This shared stem mission supports app-level CI harnesses for `uFldPathCheck`,
`uFldMessageHandler`, `uFldContactRangeSensor`, `uFldBeaconRangeSensor`,
`uFldCollisionDetect`, `uFldCollObDetect`, and `uFldScope`. Each app has its own
harness family, but the families copy this single-community mission into
isolated temp directories and generate a case-specific
`meta_shoreside.moosx`.

Typical runs:

```bash
./zlaunch.sh --just_make --nogui 10
../../harnesses/ufld_pathcheck_harnesses/H01-ufld_pathcheck_unit/zlaunch.sh --jobs=4
```

The mission itself is intentionally minimal: MOOSDB, `pRealm`,
`uTimerScript`, `pMissionEval`, and the selected uField app. Migrated consumers
supply their test traffic and substantive `pMissionEval` conditions through
app-specific case overlays; their harness launchers only validate and aggregate
the mission-owned result.

Logging is minimal by default. The six ordinary app harnesses run without
`pLogger`; the `uFldScope` harness retains only `APPCAST` because its grading
contract inspects the latest Scope table. Use `--log=full` on a stem or harness
command to restore the complete pre-migration wildcard logger configuration.
