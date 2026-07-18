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
- `goto_depth_repeat_exhaustion_pass` Runs one finite repeat and requires both exactly four arrivals and the behavior's own completion flag.
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

The Bash 5.1 wrapper creates an isolated mission copy for every serial and
rolling case, refills rolling slots as cases finish, aggregates one strict
mission-owned result row per selected case, and performs root-scoped cleanup.
The vertical target-crossing cases retain exclusive solo slots because their
crossing evidence is sensitive to concurrent local simulation load.

### Migration evidence

Three untouched legacy `--jobs=4` matrices passed 54/54 rows in 90.80, 92.31,
and 92.18 seconds, for a 91.76-second mean. The legacy serial matrix finished
in 158.06 seconds but failed both repeat cases with only three of the required
four arrivals. Focused legacy sweeps reproduced each repeat failure once in
five attempts, establishing a pre-existing fixed-snapshot race.

The migration retains the original time gates where they carry test meaning
and adds each case's required arrival count as another lead condition. This
prevents pMissionEval from grading before the evidence under test exists.
`goto_depth_repeat_pass` now grades directly after four arrivals and the
existing `ABE_NAV_X >= 70` checkpoint. Its two target levels were narrowed
from 4/12 to 6/10 so the test remains a real two-level traversal while avoiding
unrelated vehicle-dynamics variance.

`goto_depth_repeat_exhaustion_pass` uses the same two levels, still traverses
them twice, and adds the behavior's standard `endflag` as `GTD_DONE`. A
case-specific vehicle and shoreside bridge carries that flag to pMissionEval;
the shared mission is unchanged. The evaluator now proves exactly four
arrivals, `GTD_DONE=true`, and the original horizontal checkpoint instead of
sampling the counter at an arbitrary late time. Both corrected repeat cases
passed 10/10 focused repetitions.

Three migrated rolling matrices passed 54/54 rows in 78, 83, and 79 seconds.
Their 80.0-second mean is 11.76 seconds, about 12.8 percent, faster than the
legacy rolling mean. A clean isolated serial matrix passed 18/18 in 179
seconds. This is 20.94 seconds longer than the 158.06-second legacy serial
attempt, but the legacy attempt failed two cases and is therefore not an
equivalent clean timing sample.

The first migrated serial matrix had one `goto_depth_bad_update_preserve_pass`
mission exit before writing a result. The unchanged case then passed 10/10
focused repetitions and the complete serial rerun; it had also passed all
three rolling matrices. This is recorded as an isolated launch/lifecycle
outlier, not hidden by a timing or coverage change.

Validation covered all-case generation, three complete rolling matrices, two
serial attempts, focused repeat and runtime-update sweeps, exact case and
patch-map reconciliation, solo-slot scheduling, 36 unique MOOSDB ports, 36
unique pShare ports, unknown-case rejection, active-lock behavior, Homebrew
Bash re-execution, and explicit Bash 3.2 rejection. Bash syntax, ShellCheck,
the harness checker, and all 18 generated-case evaluator checks pass. No
tested MOOS process survived cleanup.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case.
