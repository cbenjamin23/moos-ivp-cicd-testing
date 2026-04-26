# P03-colregs_traffic_ring

Seeded five-vehicle COLREGS traffic-ring harness.

## Current Matrix

- `baseline_circle_pass`
  Baseline seeded five-vehicle traffic ring. The mission should maintain
  assignment activity, run warning-free, and finish the fixed window with zero
  collisions.
- `mixed_speed_circle_pass`
  Traffic ring with mixed vehicle speeds. The mission should keep assignments
  active and remain collision-free despite speed asymmetry.
- `endurance_circle_pass`
  Longer traffic-ring run. The mission should sustain repeated assignments over
  the endurance window with zero collisions and clean warning logs.
- `noncoop_circle_pass`
  Non-cooperative traffic-ring variant. Vehicle `eve` runs with `AVOID=false`,
  while the overall traffic ring should still remain collision-free.

What it tests:
- continuous five-vehicle COLREGS pressure under repeated seeded reassignment
- one non-cooperative traffic-ring variant with `eve` launched at `AVOID=false`
- no collisions over a fixed runtime window
- no planner/runtime warning text in the newest vehicle `.alog`
- case-specific floors on assignment activity

Current split:
- `pMissionEval` owns the MOOS-visible verdict through one case-specific
  shoreside patch per launch
- shell still checks:
  - `wall_time` bands
  - disallowed warning text in the newest `.alog`

The harness has two timing profiles:
- `local` is the default and keeps the original warp `10` timing bands.
- `ci` is what GitHub Actions sets through `PERF_PROFILE=ci`; it uses warp `5`
  and the relaxed wall-time bands below.

Current supported envelopes:
- `baseline_circle_pass`: `collisions=0`, `batches>=25`
- `mixed_speed_circle_pass`: `collisions=0`, `batches>=25`
- `endurance_circle_pass`: `collisions=0`, `batches>=80`
- `noncoop_circle_pass`: `collisions=0`, `batches>=25`, `dumb_vname=eve`

Notes:
- `assign_fails` is still reported, but it is treated as coordinator health rather than the primary traffic verdict.
- `closest_range_ever` and `near_misses` are still reported as telemetry, but they are not the primary gate for the async random traffic mission.
- If a harness batch fails, the temp case directories are preserved and the wrapper prints the retained root path.
- GitHub Actions runs this harness with `PERF_PROFILE=ci`.

This is a seeded-random performance harness, not a correctness harness like
`H01-H04`. The mission is randomized within a fixed seed so failures are
replayable.

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
