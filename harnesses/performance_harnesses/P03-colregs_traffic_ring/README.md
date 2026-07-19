# P03-colregs_traffic_ring

Seeded five-vehicle COLREGS traffic-ring harness.

## Current Matrix

- `baseline_circle_pass` Uses seed 17, five vehicles at speed 1.75, a 36-meter field ring, and a 300-second assignment window, testing the fixed-seed traffic-ring baseline; passes with mission completion, no timeout or collision, closest range in `(0,30]`, at least 25 assignment batches, zero scanned warnings, and wall time in 30–32 seconds locally or 58–70 seconds in CI.
- `mixed_speed_circle_pass` Uses seed 23 and five speeds from 1.55 through 2.03 over the same 300-second window, exercising assignment and COLREGS behavior under speed asymmetry; passes with completion, no timeout or collision, closest range in `(0,30]`, at least 25 batches, zero scanned warnings, and the baseline profile-specific wall-time band.
- `endurance_circle_pass` Uses seed 31, the five mixed speeds, a 42-meter field ring, 45-meter assignment radius, minimum target separation 18, predicted CPA 14, and a 900-second window, exercising a longer and wider traffic-ring soak; passes with completion, no timeout or collision, closest range in `(0,30]`, at least 80 batches, zero scanned warnings, and wall time in 89.5–92.5 seconds locally or 175–190 seconds in CI.
- `noncoop_circle_pass` Uses seed 41, five vehicles at speed 1.5, and launches `eve` with `AVOID=false`, exercising a non-cooperative participant in the 300-second ring; passes on the same completion, no-timeout, zero-collision, closest-range, 25-batch, zero-warning, and profile-specific wall-time conditions as the short cooperative cases, while `DUMB_VNAME=eve` is only reported.

What it tests:
- continuous five-vehicle COLREGS pressure under repeated seeded reassignment
- one non-cooperative traffic-ring variant with `eve` launched at `AVOID=false`
- no collisions over a fixed runtime window
- no planner/runtime warning text across any vehicle `.alog`
- assignment-activity floors for the selected case

Current split:
- `pMissionEval` owns the MOOS-visible verdict through one shoreside patch per
  launch
- shell still checks:
  - `wall_time` bands
  - disallowed warning text across every vehicle `.alog`

The harness has two timing profiles:
- `local` is the default and keeps the original warp `10` timing bands.
- `ci` is what GitHub Actions sets through `PERF_PROFILE=ci`; it uses warp `5`
  and the relaxed wall-time bands below.

## Running

```bash
./zlaunch.sh 10
PERF_PROFILE=ci ./zlaunch.sh
./zlaunch.sh --port_base=32000 10
./zlaunch.sh --log=full --port_base=32000 10
./zlaunch.sh --case=baseline_circle_pass 10
./zlaunch.sh --just_make --port_base=32000 10
```

Logging defaults to `minimal`: the shoreside logger is inactive and each
vehicle logs only `APP_LOG`, which preserves the warning-scan verdict. Use
`--log=full` to restore the original wildcard logs in every community. The
mode applies to the whole serial matrix or one explicit `--case`.

Performance cases run serially so concurrent system load cannot distort their
wall-clock gates. Every case still uses an isolated mission copy and a unique
30-port block, with pShare ports starting at `case_base + 15`. Separate
invocations are not serialized; callers must use non-overlapping port ranges
and avoid concurrent load that would invalidate timing gates.

Latest validation:

- April 27, 2026
- full matrix: `4/4` expected outcomes matched
- command: `./zlaunch.sh --port_base=32000 10`

Current supported envelopes:
- `baseline_circle_pass`: `collisions=0`, `batches>=25`
- `mixed_speed_circle_pass`: `collisions=0`, `batches>=25`
- `endurance_circle_pass`: `collisions=0`, `batches>=80`
- `noncoop_circle_pass`: `collisions=0`, `batches>=25`, `dumb_vname=eve`

Notes:
- `assign_fails` is still reported, but it is treated as coordinator health rather than the primary traffic verdict.
- `closest_range_ever` and `near_misses` are still reported as telemetry, but they are not the primary gate for the async random traffic mission.
- Use `--keep_workdirs` when generated targets or logs should be preserved for inspection.
- The harness explicitly maps each case to a mission `--scenario` and a
  shoreside evaluator patch.
- GitHub Actions runs this harness with `PERF_PROFILE=ci`.

This is a fixed-seed randomized performance harness, not a correctness harness
like `H01-H04`. Asynchronous waypoint completion can change assignment order,
so repeated runs are comparable samples rather than exact event replays.

Current shell-side checks:
- `local`
  - `baseline_circle_pass`: `wall_time` in `[30.0,32.0]`, warning count `0`
  - `mixed_speed_circle_pass`: `wall_time` in `[30.0,32.0]`, warning count `0`
  - `endurance_circle_pass`: `wall_time` in `[89.5,92.5]`, warning count `0`
  - `noncoop_circle_pass`: `wall_time` in `[30.0,32.0]`, warning count `0`
- `ci`
  - `baseline_circle_pass`: `wall_time` in `[58.0,70.0]`, warning count `0`
  - `mixed_speed_circle_pass`: `wall_time` in `[58.0,70.0]`, warning count `0`
  - `endurance_circle_pass`: `wall_time` in `[175.0,190.0]`, warning count `0`
  - `noncoop_circle_pass`: `wall_time` in `[58.0,70.0]`, warning count `0`
