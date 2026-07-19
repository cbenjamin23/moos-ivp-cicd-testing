# H02-goto_depth_motion

Patch-driven moving correctness harness for `BHV_GoToDepth`.

The harness reuses
`missions/depth_behavior_missions/depth_behavior_motion`. Cases focus on
stateful depth-level sequencing, arrival counters, repeat handling, vertical
target-depth crossings, malformed launch-time config, and missing depth inputs.

## Current Matrix

- `goto_depth_sequence_pass` Runs `depth=6,1:14,1:9,999` with a one-meter arrival delta to exercise three ordered target levels. Passes after `x>=60` when `GTD_LEVEL_HIT>=3`, `7<=NAV_DEPTH<=11`, and the vehicle remains in the transit corridor.
- `goto_depth_repeat_pass` Runs the two-level sequence `6,1:10,1` with `repeat=1` to exercise a second traversal. Passes after `x>=70` when `GTD_LEVEL_HIT>=4`.
- `goto_depth_crossing_pass` Runs `12,0:4,0:12,999` with a one-meter arrival delta to exercise downward and upward target crossings without plateau delay. Passes after `x>=60` in the transit corridor when `GTD_LEVEL_HIT>=2`.
- `goto_depth_zero_delta_crossing_pass` Runs the same 12-to-4-to-12 sequence with `arrival_delta=0`, requiring target crossings rather than proximity to increment the arrival flag. Passes after `x>=60` when `GTD_LEVEL_HIT>=2`.
- `goto_depth_single_level_pass` Configures the single long-plateau level `depth=8,999`. Passes after `x>=45` when `GTD_LEVEL_HIT>=1`, `6<=NAV_DEPTH<=10`, and the vehicle remains in the transit corridor.
- `goto_depth_time_gate_pass` Configures `depth=4,25:16,999` to test that the first level remains active for its 25-second plateau. Passes at 18 seconds after `x>=25` when `GTD_LEVEL_HIT=1`, `2<=NAV_DEPTH<=6`, and no behavior error is observed.
- `goto_depth_capture_delta_alias_pass` Uses `capture_delta=1.0` instead of `arrival_delta` on the three-level 6-to-14-to-9 sequence. Passes after `x>=60` when `GTD_LEVEL_HIT>=3` and `7<=NAV_DEPTH<=11`.
- `goto_depth_capture_flag_alias_pass` Uses `capture_flag=LEVEL_HIT` instead of `arrival_flag` on the three-level 6-to-14-to-9 sequence. Passes after `x>=60` when the resulting `GTD_LEVEL_HIT` counter reaches at least three and `7<=NAV_DEPTH<=11`.
- `goto_depth_repeat_exhaustion_pass` Runs `depth=6,1:10,1` with `repeat=1` and `endflag=GTD_DONE=true` to test finite repeat exhaustion. Passes after `x>=70` when `GTD_LEVEL_HIT=4` and `GTD_DONE=true`.
- `goto_depth_bad_update_preserve_pass` Posts malformed `GTD_UPDATE="depth=bad"` at 12 seconds while running `6,1:14,1:9,999`, testing preservation of the configured sequence. Passes after `x>=60` when `GTD_LEVEL_HIT>=3` and `7<=NAV_DEPTH<=11`.
- `goto_depth_bad_sequence_fail` Configures `depth=5,1:bogus` to exercise rejection of a nonnumeric sequence entry. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `goto_depth_negative_depth_fail` Configures `depth=-5,1` to exercise rejection of a negative target depth. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `goto_depth_bad_repeat_fail` Configures the single timed level `depth=5,1` with `repeat=0`. The harness currently passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`; this fixture does not contain the invalid repeat value implied by its name.
- `goto_depth_bad_arrival_delta_fail` Configures `arrival_delta=-1` on a two-level sequence to exercise rejection of a negative capture tolerance. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `goto_depth_bad_tuple_fail` Configures `depth=6,1,extra:14,1` to exercise rejection of a depth tuple with three fields. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `goto_depth_bad_perpetual_fail` Adds `perpetual=true` to a single-level `BHV_GoToDepth` configuration. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `goto_depth_missing_nav_depth_fail` Changes the simulator output prefix to `SIM`, leaving the helm without `NAV_X` or `NAV_DEPTH`. The harness passes at 45 seconds when both navigation variables remain absent, `DESIRED_ELEVATOR=0`, and no behavior error is observed.
- `goto_depth_domain_missing_fail` Removes `depth` from the IvP helm domain while retaining the three-level behavior. The harness passes when `ABE_BHV_ERROR` is observed.

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
