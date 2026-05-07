# H02-goto_depth_motion

Patch-driven moving correctness harness for `BHV_GoToDepth`.

The harness reuses
`missions/depth_behavior_missions/depth_behavior_motion`. Cases focus on
stateful depth-level sequencing, arrival counters, repeat handling, vertical
target-depth crossings, malformed launch-time config, and missing depth inputs.

## Current Matrix

- `goto_depth_sequence_pass` Hits a three-level depth sequence and requires the behavior-owned arrival counter to reach the final level.
- `goto_depth_repeat_pass` Exercises repeated depth sequencing and grades on the repeated arrival counter.
- `goto_depth_crossing_pass` Commands a down-then-up depth sequence and requires crossing-triggered arrival evidence in both directions.
- `goto_depth_zero_delta_crossing_pass` Sets `arrival_delta=0` so target-depth crossings, not proximity tolerance, must produce arrival evidence.
- `goto_depth_single_level_pass` Holds a single long-plateau target and verifies one arrival without requiring sequence rollover.
- `goto_depth_time_gate_pass` Uses a long first-level dwell time and verifies the behavior does not advance early by time alone.
- `goto_depth_capture_delta_alias_pass` Uses the `capture_delta` alias while retaining the normal arrival flag evidence.
- `goto_depth_capture_flag_alias_pass` Uses the `capture_flag` alias while retaining the normal arrival counter evidence.
- `goto_depth_repeat_exhaustion_pass` Runs one finite repeat and verifies the arrival counter remains at the exhausted sequence count late in the mission.
- `goto_depth_bad_update_preserve_pass` Posts malformed runtime update mail and verifies the original depth sequence remains active.
- `goto_depth_bad_sequence_fail` Supplies malformed level syntax and expects the normal good-case verdict to fail.
- `goto_depth_negative_depth_fail` Supplies a negative level target and expects the normal good-case verdict to fail.
- `goto_depth_bad_repeat_fail` Supplies an invalid repeat count and expects the normal good-case verdict to fail.
- `goto_depth_bad_arrival_delta_fail` Supplies a negative arrival delta and expects the normal good-case verdict to fail.
- `goto_depth_bad_tuple_fail` Supplies too many fields in a depth tuple and expects the normal good-case verdict to fail.
- `goto_depth_bad_perpetual_fail` Supplies unsupported base `perpetual=true` config and expects helm configuration failure.
- `goto_depth_missing_nav_depth_fail` Removes usable ownship depth mail and expects the depth-sequence verdict to fail.
- `goto_depth_domain_missing_fail` Removes the helm depth domain and expects the depth-sequence verdict to fail.

## Running

```bash
./zlaunch.sh --case=goto_depth_sequence_pass 10
./zlaunch.sh --jobs=3 --port_base=42000 10
./zlaunch.sh --just_make 10
```

The wrapper supports `--jobs` and `--port_base` for isolated grouped local and
CI runs. The vertical target-crossing cases run in solo waves during grouped
execution because their later crossing hits are timing-sensitive when launched
beside other UUV depth sequence cases.
