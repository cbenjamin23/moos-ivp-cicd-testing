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

- `baseline_memory_pass`
  Requests a turn from 90 to 180 degrees at 1.3 m/s with
  `memory_time=8`, `turn_range=18`, and limiter weight 150; passes at 16
  seconds when `MEM_HDG_AVG` is 95 to 175, heading is 105 to 180, speed is at
  least 0.7 m/s, mismatch is at most 75 degrees, and no warning or error occurs.
- `tight_window_pass`
  Holds limiter weight at 150 and narrows `turn_range` from 18 to 6, isolating
  the smaller peak width around the recent-heading average; passes at 16
  seconds when `MEM_HDG_AVG` is 90 to 100 degrees, heading is 95 to 105,
  mismatch is at least 75 degrees, speed is at least 0.7 m/s, and no warning or
  error occurs.
- `relaxed_window_pass`
  Holds limiter weight at 150 and widens `turn_range` from 18 to 120, isolating
  the wider peak that lets the 180-degree heading demand advance farther;
  passes at 16 seconds when `MEM_HDG_AVG` is 100 to 180 degrees, heading is 125
  to 185, mismatch is at most 60 degrees, speed is at least 0.7 m/s, and no
  warning or error occurs.
- `short_memory_pass`
  Shortens `memory_time` from 8 to 2 seconds so the average retains two seconds
  of trailing headings; passes at the shared 16-second turn checkpoint when
  `MEM_HDG_AVG` is 110 to 114 degrees, heading is 112 to 116, mismatch is 55 to
  70 degrees, speed is at least 0.7 m/s, and no warning or error occurs.
- `long_memory_pass`
  Lengthens `memory_time` from 8 to 20 seconds; passes when the memory average
  is 85 to 120 degrees, heading is 90 to 130, mismatch is at least 50, speed is
  at least 0.7 m/s, and no warning or error occurs.
- `zero_memory_pass`
  Sets `memory_time=0`, causing each iteration to discard every earlier heading
  sample so the posted average tracks the current heading; passes at the shared
  16-second turn checkpoint when `MEM_HDG_AVG` and heading are each 115 to 120
  degrees, mismatch is 60 to 70 degrees, speed is at least 0.7 m/s, and no
  warning or error occurs.
- `weak_priority_pass`
  Lowers the limiter weight from 150 to 40 while retaining `turn_range=18`;
  passes when the memory average is 120 to 190 degrees, heading is 140 to 190,
  mismatch is at most 50, speed is at least 0.7 m/s, and no warning or error
  occurs.
- `very_slow_speed_floor_pass`
  Commands 0.1 m/s, below the limiter's minimum-speed floor; passes when speed
  is 0.05 to 0.2 m/s, memory average and heading are each 88 to 105 degrees,
  mismatch is 75 to 95, and no warning or error occurs.
- `slow_speed_scaled_pass`
  Commands 0.4 m/s to exercise speed-scaled peak width; passes when speed is
  0.25 to 0.7 m/s, memory average is 88 to 115 degrees, heading is 88 to 120,
  mismatch is 60 to 92, and no warning or error occurs.
- `fast_speed_unscaled_pass`
  Commands 2.0 m/s above the reference-speed branch; passes when speed is 1.5
  to 2.2 m/s, memory average is 95 to 125 degrees, heading is 100 to 135,
  mismatch is 45 to 85, and no warning or error occurs.
- `moderate_turn_pass`
  Changes the constant-heading demand from 180 to 135 degrees; passes when the
  memory average is 95 to 125 degrees, heading is 100 to 135, mismatch is 15
  to 45, speed is at least 0.7 m/s, and no warning or error occurs.
- `starboard_turn_pass`
  Changes the heading demand from 180 to 0 degrees to exercise the opposite
  turn direction; passes when the memory average is 60 to 90 degrees, heading
  is 45 to 85, mismatch is 45 to 90, speed is at least 0.7 m/s, and no warning
  or error occurs.
- `max_turn_range_pass`
  Sets the accepted upper boundary `turn_range=180`; passes when the memory
  average is 125 to 190 degrees, heading is 145 to 190, mismatch is at most 45,
  speed is at least 0.7 m/s, and no warning or error occurs.
- `zero_turn_range_pass`
  Sets the accepted lower boundary `turn_range=0`; passes when the memory
  average and heading are each 88 to 105 degrees, mismatch is 75 to 95, speed
  is at least 0.7 m/s, and no warning or error occurs.
- `runtime_widen_range_pass`
  Starts at `turn_range=6` and posts `turn_range=120` after five seconds;
  passes when the memory average is 110 to 170 degrees, heading is 125 to 185,
  mismatch is at most 65, speed is at least 0.7 m/s, and no warning or error
  occurs.
- `runtime_tighten_range_pass`
  Starts at `turn_range=120` and posts `turn_range=0` after five seconds;
  passes when the memory average is 95 to 130 degrees, heading is 95 to 140,
  mismatch is 40 to 85, speed is at least 0.7 m/s, and no warning or error
  occurs.
- `bad_memory_time_fail`
  Sets invalid `memory_time=-1`; the harness passes when the helm reaches
  `MALCONFIG` and vehicle speed remains at or below 0.05 m/s.
- `bad_turn_range_high_fail`
  Sets invalid `turn_range=181`; the harness passes when the helm reaches
  `MALCONFIG` and vehicle speed remains at or below 0.05 m/s.
- `bad_turn_range_text_fail`
  Sets nonnumeric `turn_range=wide`; the harness passes when the helm reaches
  `MALCONFIG` and vehicle speed remains at or below 0.05 m/s.
- `missing_memory_time_fail`
  Omits required `memory_time`; the harness passes when any `BHV_WARNING` is
  observed at evaluation time and no `BHV_ERROR` is observed.
- `missing_turn_range_fail`
  Omits required `turn_range`; the harness passes when any `BHV_WARNING` is
  observed at evaluation time and no `BHV_ERROR` is observed.

## Wave Notes

The invalid and missing configuration cases are listed in `SOLO_CASES` in
`zlaunch.sh`. They intentionally exercise failure paths where the limiter does
not produce the same normal motion telemetry as the pass cases, so grouped runs
isolate them while still preserving caller-controlled `--jobs` and
`--port_base` behavior for the rest of the matrix.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for a fully logged diagnostic case.
