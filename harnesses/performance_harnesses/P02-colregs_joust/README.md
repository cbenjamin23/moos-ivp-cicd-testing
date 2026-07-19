# P02-colregs_joust

Initial performance harness for the COLREGS joust stem.

## Current Matrix

- `baseline_colregs_pass` Runs the checked-in two-vehicle `baseline_2v.txt` crossing for a 280-second mission window, testing deterministic COLREGS interaction at two meters per second; passes with mission completion, no timeout or collision, closest range in 5–20 meters, 0–5 near misses, zero scanned warning matches, and wall time in 27.5–31.0 seconds.
- `dense_colregs_pass` Runs the checked-in three-vehicle `dense_3v.txt` crossing for 420 mission seconds, exercising simultaneous COLREGS pressure from three intersecting jousts; passes with completion, no timeout or collision, closest range in 6–20 meters, 0–8 near misses, zero scanned warnings, and wall time in 41.5–45.0 seconds.
- `endurance_colregs_pass` Runs the checked-in three-vehicle `endurance_3v.txt` layout for 1,200 mission seconds, exercising the same three-way interactions over a longer soak; passes with completion, no timeout or collision, closest range in 6–20 meters, 0–8 near misses, zero scanned warnings, and wall time in 118–124 seconds.

The harness checks:
- mission `grade`
- mission-side thresholds for MOOS-visible metrics in the selected case
- disallowed planner/runtime warning text across every vehicle `.alog`

For these joust cases, the main pressure metric is
`closest_range_ever`, derived from `UCD_CLOSEST_RANGE_EVER`. That is more
useful for this stem than `ENCOUNTER_TOTAL`, which may stay low or zero even
when the vehicles repeatedly operate inside a meaningful contact envelope.

Current split:
- `pMissionEval` owns the MOOS-visible verdict through one shoreside patch per
  launch
- shell still checks only:
  - `wall_time` bands
  - disallowed warning text across every vehicle `.alog`

Current mission-side case checks:
- `baseline_colregs_pass`
  - `MISSION_DONE=true`
  - `MISSION_TIMEOUT=false`
  - `COLLISION_TOTAL=0`
  - `UCD_CLOSEST_RANGE_EVER` in `[5,20]`
  - `NEAR_MISS_TOTAL` in `[0,5]`
- `dense_colregs_pass`
  - `MISSION_DONE=true`
  - `MISSION_TIMEOUT=false`
  - `COLLISION_TOTAL=0`
  - `UCD_CLOSEST_RANGE_EVER` in `[6,20]`
  - `NEAR_MISS_TOTAL` in `[0,8]`
- `endurance_colregs_pass`
  - `MISSION_DONE=true`
  - `MISSION_TIMEOUT=false`
  - `COLLISION_TOTAL=0`
  - `UCD_CLOSEST_RANGE_EVER` in `[6,20]`
  - `NEAR_MISS_TOTAL` in `[0,8]`

Current shell-side case checks:
- `baseline_colregs_pass`: `wall_time` in `[27.5,31.0]`, warning count `0`
- `dense_colregs_pass`: `wall_time` in `[41.5,45.0]`, warning count `0`
- `endurance_colregs_pass`: `wall_time` in `[118.0,124.0]`, warning count `0`

Typical runs:

```bash
./zlaunch.sh 10
./zlaunch.sh --port_base=31000 10
./zlaunch.sh --log=full --port_base=31000 10
./zlaunch.sh --case=baseline_colregs_pass 10
./zlaunch.sh --case=dense_colregs_pass 10
./zlaunch.sh --case=endurance_colregs_pass 10
```

Logging defaults to `minimal`: the shoreside logger is inactive and each
vehicle logs only `APP_LOG`, which preserves the warning-scan verdict. Use
`--log=full` to restore the original wildcard logs in every community. The
mode applies to the whole serial matrix or one explicit `--case`.

Execution notes:
- Performance cases run serially so concurrent load cannot distort their
  wall-clock gates.
- Each case uses an isolated mission copy and its own MOOSDB/pShare block
  (`case_base = port_base + case_idx*30`, pShare at `case_base + 15`).
- The harness explicitly maps each case to a human-facing mission `--scenario`
  and a shoreside evaluator patch.
- Separate invocations are not serialized. Callers must use non-overlapping
  port ranges and avoid concurrent load that would invalidate timing gates.
- Use `--keep_workdirs` to inspect isolated copies and generated targets.
