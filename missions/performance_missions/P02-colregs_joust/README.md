# P02-colregs_joust

This stem mission is the first COLREGS/contact performance baseline. It was
copied from `missions-swarm/S51-joust` and then adapted to this repo's
auto-deploy, self-evaluating test flow.

The mission now has three checked-in scenarios:
- `baseline_colregs`
  - two-vehicle deterministic joust
- `dense_colregs`
  - three-vehicle deterministic continuous joust
- `endurance_colregs`
  - a milder three-vehicle joust evaluated over a longer window

The copied swarm stem has been trimmed so the vehicles keep moving through the
joust instead of stopping to synchronize at each turn. The mission evaluates
after a mode-specific runtime window and writes `results.txt`.

Pass contract:
- `MISSION_DONE = true`
- `COLLISION_TOTAL = 0`
- `MISSION_TIMEOUT = false`
- `UCD_CLOSEST_RANGE_EVER > 0`
- `UCD_CLOSEST_RANGE_EVER <= 40`

The joust stem does not naturally produce a reliable encounter-count threshold
for this repo-style performance contract. Instead, the mission now uses the
closest contact range ever posted by `uFldCollisionDetect`, which captures
whether the run actually generated meaningful COLREGS/contact pressure.

The checked-in joust baseline is also intentionally contact-dense. It is
collision-free when healthy, but it is not expected to be a zero-near-miss
scenario.

Typical runs:

```bash
./launch.sh --just_make --scenario=baseline_colregs --nogui 10
./zlaunch.sh --scenario=dense_colregs --nogui 10
./zlaunch.sh --scenario=endurance_colregs --nogui 10
```
