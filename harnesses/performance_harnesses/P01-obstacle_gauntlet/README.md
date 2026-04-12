# P01 Obstacle Gauntlet Harness

Performance harness for the first deterministic gauntlet mission family.

## Current Case

- `baseline_field_pass`
- `dense_field_pass`
- `endurance_random_pass`

The harness currently expects the gauntlet mission to:

- in fixed-field cases, complete one full loop
- in the endurance case, complete 10 full loops before the fail-safe timeout
- incur zero collisions
- avoid timing out
- satisfy the case-specific mission-side MOOS-visible thresholds
- avoid disallowed planner/runtime warning text in the vehicle `.alog`

It preserves the mission-owned report columns and wraps them in the usual harness summary line:

- `case`
- `expected`
- `actual`
- `status`
- `perf_status`
- `perf_notes`
- `warning_count`

## Typical Runs

```bash
./zlaunch.sh 10
./zlaunch.sh --jobs=2 10
./zlaunch.sh --case=baseline_field_pass 10
./zlaunch.sh --just_make 10
```

## Notes

- All cases run the same loop-course mission and select obstacle source through `--mmod`.
- Each case now launches with one case-specific shoreside patch, and `pMissionEval` owns the MOOS-visible verdict.
- Two cases are deterministic comparison runs, and one case is a longer randomized endurance run.
- The harness supports wave-batch execution with `--jobs=<n>` using isolated temp mission copies and per-wave port blocks.
- The endurance case automatically uses a larger launcher `--max_time` budget than the fixed-field cases.
- Current mission-side checks:
  - `baseline_field_pass`
    - `WPT_CYCLES=1`
    - `OB_TOTAL_COLLISIONS=0`
    - `OB_TOTAL_NEAR_MISSES` in `[0,6]`
    - `OB_TOTAL_ENCOUNTERS` in `[1,8]`
    - `OBAVOIDING=end`
    - `MISSION_TIMEOUT=false`
  - `dense_field_pass`
    - `WPT_CYCLES=1`
    - `OB_TOTAL_COLLISIONS=0`
    - `OB_TOTAL_NEAR_MISSES` in `[0,6]`
    - `OB_TOTAL_ENCOUNTERS` in `[1,8]`
    - `OBAVOIDING=end`
    - `MISSION_TIMEOUT=false`
  - `endurance_random_pass`
    - `WPT_CYCLES>=10`
    - `OB_TOTAL_COLLISIONS=0`
    - `OB_TOTAL_NEAR_MISSES` in `[20,70]`
    - `OB_TOTAL_ENCOUNTERS` in `[50,80]`
    - `OBAVOIDING=end`
    - `DB_UPTIME<=1800`
    - `MISSION_TIMEOUT=false`
- Current shell-side checks:
  - `baseline_field_pass`: `wall_time` in `[16.5,20.0]`, warning count `0`
  - `dense_field_pass`: `wall_time` in `[16.5,20.0]`, warning count `0`
  - `endurance_random_pass`: `wall_time` in `[145.0,180.0]`, warning count `0`
- See [BASELINES.md](/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/performance_harnesses/P01-obstacle_gauntlet/BASELINES.md) for why wall time and warning scans stay in shell.
- See [NSPATCH.md](/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/performance_harnesses/P01-obstacle_gauntlet/NSPATCH.md) for the current split between stem-owned scenario defaults and harness-owned semantic overrides.
