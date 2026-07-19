# P01 Obstacle Gauntlet Harness

Performance harness for the first deterministic gauntlet mission family.

## Current Matrix

- `baseline_field_pass` Traverses the checked-in six-obstacle `baseline_field.txt` for one waypoint loop, testing the deterministic reference gauntlet; passes with one cycle, zero collisions, 0–6 near misses, 1–8 encounters, `OBAVOIDING=end`, no timeout, zero scanned warning matches, and wall time in 16.5–20.0 seconds locally or 80–95 seconds under the CI profile.
- `dense_field_pass` Traverses the checked-in eight-obstacle `dense_field.txt` for one waypoint loop, exercising a denser deterministic fixture; passes on the same one-cycle, zero-collision, 0–6 near-miss, 1–8 encounter, lifecycle-end, no-timeout, zero-warning, and profile-specific wall-time conditions as the baseline.
- `endurance_random_pass` Generates seven obstacles at launch and repeats the loop until at least ten cycles, exercising a long randomized avoidance sample; passes with zero collisions, 20–70 near misses, 50–80 encounters, `OBAVOIDING=end`, `DB_UPTIME<=1800`, no timeout, zero scanned warnings, and wall time in 145–180 seconds locally or 780–900 seconds under the CI profile.

The harness currently expects the gauntlet mission to:

- in fixed-field cases, complete one full loop
- in the endurance case, complete 10 full loops before the fail-safe timeout
- incur zero collisions
- avoid timing out
- satisfy the mission-side MOOS-visible thresholds for the selected case
- avoid disallowed planner/runtime warning text in the vehicle `.alog`

It preserves the mission-owned report columns and wraps them in the usual harness summary line:

- `case`
- `grade`
- `perf_status`
- `perf_notes`
- `warning_count`

## Typical Runs

```bash
./zlaunch.sh 10
PERF_PROFILE=ci ./zlaunch.sh
./zlaunch.sh --profile=ci
./zlaunch.sh --port_base=30000 10
./zlaunch.sh --log=full --port_base=30000 10
./zlaunch.sh --case=baseline_field_pass 10
./zlaunch.sh --just_make 10
```

Logging defaults to `minimal`: the shoreside logger is inactive and each
vehicle logs only `APP_LOG`, the artifact required by the warning scan. Use
`--log=full` for the original shoreside and vehicle wildcard logs. The mode
applies to the whole serial matrix or one explicit `--case`.

## Notes

- All cases run the same loop-course mission. The harness explicitly maps each
  case to a mission `--scenario` and a shoreside evaluator patch.
- Each case now launches with one shoreside patch, and `pMissionEval` owns the MOOS-visible verdict.
- Two cases are deterministic comparison runs, and one case is a longer randomized endurance run.
- Performance cases run serially so concurrent system load cannot distort their
  wall-clock gates. Every case still uses an isolated mission copy and unique
  port block (`case_base = port_base + case_idx*30`, pShare at
  `case_base + 15`).
- A performance-family lock prevents P01, P02, and P03 from overlapping in the
  same checkout.
- The endurance case automatically uses a larger launcher `--max_time` budget than the fixed-field cases.
- The harness has two timing profiles:
  - `local` is the default and keeps the original warp `10` timing bands.
  - `ci` is what GitHub Actions sets through `PERF_PROFILE=ci`; it uses warp `2` and the relaxed wall-time bands below.
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
  - `local`
    - `baseline_field_pass`: `wall_time` in `[16.5,20.0]`, warning count `0`
    - `dense_field_pass`: `wall_time` in `[16.5,20.0]`, warning count `0`
    - `endurance_random_pass`: `wall_time` in `[145.0,180.0]`, warning count `0`
  - `ci`
    - `baseline_field_pass`: `wall_time` in `[80.0,95.0]`, warning count `0`
    - `dense_field_pass`: `wall_time` in `[80.0,95.0]`, warning count `0`
    - `endurance_random_pass`: `wall_time` in `[780.0,900.0]`, warning count `0`
- See [BASELINES.md](BASELINES.md) for why wall time and warning scans stay in shell.
- See [NSPATCH.md](NSPATCH.md) for the current split between stem-owned scenario defaults and harness-owned semantic overrides.
- GitHub Actions runs this harness with `PERF_PROFILE=ci`.
