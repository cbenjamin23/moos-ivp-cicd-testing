# H02-goto_depth_motion

Patch-driven moving correctness harness for `BHV_GoToDepth`.

The harness reuses
`missions/depth_behavior_missions/depth_behavior_motion`. Cases focus on
stateful depth-level sequencing, arrival counters, repeat handling, vertical
target-depth crossings, malformed launch-time config, and missing depth inputs.

## Current Matrix

- `goto_depth_sequence_pass` Runs `depth=6,1:14,1:9,999` to require three ordered depth arrivals and hold the final nine-meter level. Passes when the third arrival and `x>=60` have both occurred, `7<=NAV_DEPTH<=11`, the vehicle remains in the transit corridor, and no behavior error occurred.
- `goto_depth_crossing_pass` Runs `12,0:4,0:12,999` with a one-meter arrival delta to require a downward arrival at 12 meters followed by an upward arrival at four meters without plateau delay. Passes when the second arrival and `x>=60` have both occurred, the vehicle remains in the transit corridor, and no behavior error occurred.
- `goto_depth_zero_delta_crossing_pass` Runs the same 12-to-4-to-12 sequence with `arrival_delta=0`, so only crossing a target can post each arrival. Passes when the second crossing and `x>=60` have both occurred with no behavior error.
- `goto_depth_single_level_pass` Configures the single long-plateau level `depth=8,999`. Passes after the first arrival and `x>=45` when `6<=NAV_DEPTH<=10`, the vehicle remains in the transit corridor, and no behavior error occurred.
- `goto_depth_time_gate_pass` Configures `depth=4,25:16,999` to test that the first level remains active for its 25-second plateau. Passes at 18 seconds after `x>=25` when `GTD_LEVEL_HIT=1`, `2<=NAV_DEPTH<=6`, and no behavior error is observed.
- `goto_depth_capture_delta_alias_pass` Uses the nondefault alias `capture_delta=4.0` on a ten-meter target, testing that the alias changes the arrival threshold rather than being ignored in favor of the one-meter default. Passes when the first arrival is posted at `5.5<=NAV_DEPTH<=7.5` with no behavior error; changing the tolerance to one meter posts the arrival near nine meters and fails.
- `goto_depth_capture_flag_alias_pass` Uses `capture_flag=LEVEL_HIT` instead of `arrival_flag` on the three-level 6-to-14-to-9 sequence. Passes as soon as the alias-generated `GTD_LEVEL_HIT` counter reaches three at the final depth with no behavior error; a 70-second deadline exists only to grade a missing alias publication as failure.
- `goto_depth_repeat_exhaustion_pass` Runs `depth=6,1:10,1` with `repeat=1` to require exactly two traversals and then post `GTD_DONE=true`; a matching ten-meter guard takes over after the fourth arrival. Passes when `x>=70`, `GTD_LEVEL_HIT=4`, `GTD_DONE=true`, and no behavior error have all been observed.
- `goto_depth_bad_update_preserve_pass` Posts malformed `GTD_UPDATE="depth=bad"` at 12 seconds while running `6,1:14,1:9,999`, testing that the helm reports the rejected update and preserves the configured sequence. Passes when the post-update warning is observed, the original three arrivals complete, `x>=60`, `7<=NAV_DEPTH<=11`, and no behavior error occurred; replacing the malformed update with a valid one fails the warning assertion.
- `goto_depth_bad_sequence_fail` Gives GoToDepth `depth=5,1:bogus`, testing rejection of a nonnumeric sequence entry. Passes only when the first operational helm state is `MALCONFIG`; an ordinary `DRIVE` state fails.
- `goto_depth_negative_depth_fail` Gives GoToDepth `depth=-5,1`, testing rejection of a negative target depth. Passes only when the first operational helm state is `MALCONFIG`; an ordinary `DRIVE` state fails.
- `goto_depth_bad_repeat_fail` Gives the waypoint its valid `repeat=0` and GoToDepth the invalid `repeat=-1`, testing GoToDepth's repeat validation rather than the waypoint parser. Passes only when the first operational helm state is `MALCONFIG`; changing GoToDepth to `repeat=0` reaches `DRIVE` and fails.
- `goto_depth_bad_arrival_delta_fail` Gives GoToDepth `arrival_delta=-1`, testing rejection of a negative capture tolerance. Passes only when the first operational helm state is `MALCONFIG`; an ordinary `DRIVE` state fails.
- `goto_depth_bad_tuple_fail` Gives GoToDepth `depth=6,1,extra:14,1`, testing rejection of a tuple with three fields. Passes only when the first operational helm state is `MALCONFIG`; an ordinary `DRIVE` state fails.
- `goto_depth_perpetual_cycle_pass` Runs `depth=6,1:10,1` with the valid base parameter `perpetual=true`, testing that GoToDepth resets its arrival counter and starts the sequence again after the second level. Passes when the ordered counter changes from `2` at the end of the first cycle to `1` in the second cycle with no behavior error; disabling `perpetual` fails at the bounded missing-restart deadline.
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

### Validation notes

Ordinary sequence, crossing, single-level, and repeat-completion cases now
evaluate from the behavior's arrival/completion publications plus the stated
position threshold, not an arbitrary late snapshot. Time remains part of the
test only where the contract is temporal: the 25-second plateau, scheduled
runtime update, bounded proof that an expected publication never arrives, or
the missing-navigation observation window.

The weaker duplicate repeat case was removed. The former perpetual-malconfig
case was corrected to prove the supported `perpetual=true` cycle reset, and
the direct GoToDepth CTest now exercises the same base-parameter path used by
the helm. Focused mutations cover invalid repeat acceptance, repeat handoff,
capture tolerance, capture-flag publication, malformed-update warning and
plan preservation, and perpetual restart.

The complete 17-case matrix passes, and the `behaviors-marine` CTest family
passes all 203 tests.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case.
