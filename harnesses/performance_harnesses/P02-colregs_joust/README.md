# P02-colregs_joust

Initial performance harness for the COLREGS joust stem.

## Current Matrix

- `baseline_colregs_pass`
  Mild two-vehicle deterministic crossing. The mission should complete with
  zero collisions, clean warning logs, and closest-range telemetry inside the
  baseline envelope.
- `dense_colregs_pass`
  Denser three-vehicle continuous joust. The mission should preserve
  collision-free completion and safe closest-range telemetry under added
  COLREGS pressure.
- `endurance_colregs_pass`
  Longer-running three-vehicle joust. The mission should preserve the same
  zero-collision and zero-warning contract over the endurance window.

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
./zlaunch.sh --case=baseline_colregs_pass 10
./zlaunch.sh --case=dense_colregs_pass 10
./zlaunch.sh --case=endurance_colregs_pass 10
```

Execution notes:
- Performance cases run serially so concurrent load cannot distort their
  wall-clock gates.
- Each case uses an isolated mission copy and its own MOOSDB/pShare block
  (`case_base = port_base + case_idx*30`, pShare at `case_base + 15`).
- The harness explicitly maps each case to a human-facing mission `--scenario`
  and a shoreside evaluator patch.
- A performance-family lock prevents P01, P02, and P03 from overlapping in the
  same checkout.
- Use `--keep_workdirs` to inspect isolated copies and generated targets.
