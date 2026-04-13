# P03-colregs_traffic_ring

Seeded five-vehicle COLREGS traffic-ring harness.

Current case set:
- `baseline_circle_pass`
- `mixed_speed_circle_pass`
- `endurance_circle_pass`

What it tests:
- continuous five-vehicle COLREGS pressure under repeated seeded reassignment
- no collisions over a fixed runtime window
- no planner/runtime warning text in the newest vehicle `.alog`
- case-specific floors on assignment activity

Current split:
- `pMissionEval` owns the MOOS-visible verdict through one case-specific
  shoreside patch per launch
- shell still checks:
  - `wall_time` bands
  - disallowed warning text in the newest `.alog`

Current supported envelopes:
- `baseline_circle_pass`: `collisions=0`, `batches>=25`
- `mixed_speed_circle_pass`: `collisions=0`, `batches>=25`
- `endurance_circle_pass`: `collisions=0`, `batches>=80`

Notes:
- `assign_fails` is still reported, but it is treated as coordinator health rather than the primary traffic verdict.
- `closest_range_ever` and `near_misses` are still reported as telemetry, but they are not the primary gate for the async random traffic mission.
- If a harness batch fails, the temp case directories are preserved and the wrapper prints the retained root path.

This is a seeded-random performance harness, not a correctness harness like
`H01-H04`. The mission is randomized within a fixed seed so failures are
replayable.
