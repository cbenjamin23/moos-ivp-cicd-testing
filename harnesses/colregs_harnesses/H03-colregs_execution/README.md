# H03-colregs_execution

TLDR:
- execution-quality harness on the shared `colregs_unit` stem
- checks realized CPA band, relative side outcome, and loose completion time per canonical case
- sits after `H01/H02`: mode selection is already assumed correct and now we check maneuver quality

Execution-quality harness built on the shared `colregs_unit` stem mission.

Primary intent:
- keep the same canonical two-vessel cases as `H01`
- validate that Ben completes the encounter cleanly
- record realized closest range and final relative-side state

## How This Harness Works

- reuse the same canonical geometry from `H01`
- let the mission run through completion instead of grading at first
  classification
- check realized maneuver quality in the harness:
  - CPA band
  - relative side outcome
  - loose completion time

Current implemented case list:
- `head_on_execution_pass`
- `crossing_starboard_giveway_execution_pass`
- `crossing_port_standon_execution_pass`
- `overtaking_starboard_execution_pass`

Current mission-side assertions:
- `ARRIVED_ben = true`
- `COLLISION_TOTAL = 0`
- `MISSION_TIMEOUT = false`
- `UCD_CLOSEST_RANGE_EVER > 0`

Current harness-side checks:
- `head_on_execution_pass`: `closest_range_ever` in `[16,20]`, `cn_port=1`, `wall_time<=12`
- `crossing_starboard_giveway_execution_pass`: `closest_range_ever` in `[18,24]`, `cn_port=1`, `wall_time<=12`
- `crossing_port_standon_execution_pass`: `closest_range_ever` in `[14,20]`, `cn_port=0`, `wall_time<=12`
- `overtaking_starboard_execution_pass`: `closest_range_ever` in `[45,65]`, `cn_port=0`, `wall_time<=12`

Current reported execution metrics:
- `closest_range_ever`
- `cn_port`
- `cn_fore`
- `cn_crossed`
- `near_misses`
- `collisions`
- `wall_time`

This is the first execution-focused cut. It establishes clean completion and
now checks a first per-case execution envelope in the harness.
