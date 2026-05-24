# H01-memoryturnlimit_behavior_motion

This harness focuses on `BHV_MemoryTurnLimit` as the behavior under test. The
stem uses one simulated vehicle. A constant-speed behavior keeps the vehicle
moving while a constant-heading behavior asks for a sustained turn. The memory
turn limiter constrains the helm around the recent ownship heading average.

Evaluation is mission-owned. `pMissionEval` checks the reported memory heading
average, the resulting ownship heading, speed, heading mismatch, and behavior
warning/error status.

Typical commands:

```bash
./zlaunch.sh --case=baseline_memory_pass 10
./zlaunch.sh --jobs=4 --port_base=15000 --port_stride=30 10
./zlaunch.sh --just_make 10
```

## Current Matrix

- `baseline_memory_pass` Runs the stock limiter and expects the memory heading average, ownship heading, and heading mismatch to remain in the nominal damped-turn window.
- `tight_window_pass` Narrows `turn_range` and increases limiter weight, proving the commanded heading change is held near the recent heading memory.
- `relaxed_window_pass` Widens `turn_range` and lowers limiter weight, proving the same turn demand can advance farther when the memory constraint is loose.
- `short_memory_pass` Shortens `memory_time`, proving the recent-heading average advances with the turn while speed and behavior health stay nominal.
- `long_memory_pass` Lengthens `memory_time`, proving older headings keep the average and final heading closer to the initial course.
- `zero_memory_pass` Exercises the `memory_time=0` boundary and expects the limiter to track the current heading without warnings or errors.
- `weak_priority_pass` Lowers limiter priority with the stock window and expects the constant-heading demand to advance farther.
- `very_slow_speed_floor_pass` Lowers vehicle speed below the limiter's minimum speed floor and expects a near-initial heading response at the very slow reported speed.
- `slow_speed_scaled_pass` Lowers vehicle speed to exercise the speed-scaled peak-width branch and expects a slower, tighter heading response.
- `fast_speed_unscaled_pass` Raises vehicle speed above the reference speed and expects nominal unscaled limiter behavior at the faster reported speed.
- `moderate_turn_pass` Changes the requested heading to a smaller port turn and checks that the memory limiter damps that smaller heading change.
- `starboard_turn_pass` Requests a starboard turn toward north and checks that the remembered heading average and final heading move in the opposite direction.
- `max_turn_range_pass` Exercises the legal `turn_range=180` boundary and expects a loose memory constraint with no behavior warnings.
- `zero_turn_range_pass` Exercises the legal `turn_range=0` boundary and expects a very tight memory constraint with no behavior warnings.
- `runtime_widen_range_pass` Starts with a narrow `turn_range`, widens it through `MEMORY_TURN_UPDATES`, and expects the turn to advance without behavior warnings.
- `runtime_tighten_range_pass` Starts with a loose `turn_range`, tightens it through `MEMORY_TURN_UPDATES`, and expects the final heading to remain in a constrained band.
- `bad_memory_time_fail` Sets a negative `memory_time` and expects `pMissionEval` to pass only when `HELM_MALCONFIG` proves the bad launch-time behavior configuration was rejected.
- `bad_turn_range_high_fail` Sets `turn_range` above 180 and expects `pMissionEval` to pass only when `HELM_MALCONFIG` proves the bad launch-time behavior configuration was rejected.
- `bad_turn_range_text_fail` Sets a nonnumeric `turn_range` and expects `pMissionEval` to pass only when `HELM_MALCONFIG` proves the bad launch-time behavior configuration was rejected.
- `missing_memory_time_fail` Omits `memory_time` and expects `pMissionEval` to pass only when a behavior warning proves the required limiter parameter was rejected.
- `missing_turn_range_fail` Omits `turn_range` and expects `pMissionEval` to pass only when a behavior warning proves the required limiter parameter was rejected.

## Wave Notes

The invalid and missing configuration cases are listed in `SOLO_CASES` in
`zlaunch.sh`. They intentionally exercise failure paths where the limiter does
not produce the same normal motion telemetry as the pass cases, so grouped runs
isolate them while still preserving caller-controlled `--jobs` and
`--port_base` behavior for the rest of the matrix.
