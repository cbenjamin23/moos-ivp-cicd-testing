# P03-colregs_traffic_ring

Seeded five-vehicle COLREGS traffic-ring harness.

## Current Matrix

- `baseline_circle_pass`
  Baseline fixed-seed five-vehicle traffic ring. The mission should maintain
  assignment activity, run warning-free, and finish the fixed window with zero
  collisions.
- `mixed_speed_circle_pass`
  Traffic ring with mixed vehicle speeds. The mission should keep assignments
  active and remain collision-free despite speed asymmetry.
- `endurance_circle_pass`
  Longer traffic-ring run. The mission should sustain repeated assignments over
  the endurance window with zero collisions and clean warning logs. This case
  uses a wider assignment ring and stricter predicted-CPA filtering than the
  shorter cases to keep the long CI soak stable.
- `noncoop_circle_pass`
  Non-cooperative traffic-ring variant. Vehicle `eve` runs with `AVOID=false`,
  while the overall traffic ring should still remain collision-free.

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
./zlaunch.sh --case=baseline_circle_pass 10
./zlaunch.sh --just_make --port_base=32000 10
```

Performance cases run serially so concurrent system load cannot distort their
wall-clock gates. Every case still uses an isolated mission copy and a unique
30-port block, with pShare ports starting at `case_base + 15`. A
performance-family lock prevents P01, P02, and P03 from overlapping in the same
checkout.

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
