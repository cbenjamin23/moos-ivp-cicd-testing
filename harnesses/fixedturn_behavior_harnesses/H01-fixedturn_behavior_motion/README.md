# H01-fixedturn_behavior_motion

Patch-driven moving correctness harness for
`missions/fixedturn_behavior_missions/fixedturn_behavior_motion`.

This harness focuses on `BHV_FixedTurn` as the behavior under test. The stem
mission first establishes a short moving leg, then activates the fixed-turn
behavior and grades mission-owned evidence:

- `FT_DONE`
- final `NAV_HEADING` for directional 90-degree cases
- `TURNING_TIME` and `TURNING_DIST`
- scheduled-turn flags such as `TURN_ONE` and `TURN_TWO`
- clean behavior state through `BHV_ERROR_SEEN=false`

## Current Matrix

- `starboard_90_pass`: After the approach leg leaves the vehicle heading about 45 degrees, runs the stock `fix_turn=90`, `mod_hdg=25`, `turn_dir=star`, and `speed=1.4`; passes when `FT_DONE=true`, final heading is 105 through 170 degrees, turn time exceeds one second, turn distance exceeds three meters, and no behavior error was observed.
- `port_90_pass`: Changes only `turn_dir=port`; passes when `FT_DONE=true`, final heading is above 285 degrees, turn time exceeds one second, turn distance exceeds three meters, and no behavior error was observed.
- `starboard_360_pass`: Sets `fix_turn=355`, `mod_hdg=90`, `turn_dir=star`, and `speed=1.8`; passes when `FT_DONE=true`, turn time exceeds 15 seconds, turn distance exceeds 25 meters, and no behavior error was observed, while heading and radial `FT_REPORT` contents are report-only.
- `port_360_pass`: Uses the same 355-degree configuration with `turn_dir=port`; passes on the same completion, time, distance, and no-error conditions, without a graded direction, heading, or radius assertion.
- `speed_auto_pass`: Sets `speed=auto` for the stock starboard 90-degree turn; passes on the stock completion, 105-to-170-degree heading, positive time/distance, and no-error conditions, while the inherited speed shown in `FT_REPORT` is report-only.
- `fixed_speed_pass`: Sets `speed=1.3` for the stock starboard 90-degree turn; passes on the same completion, heading, time/distance, and no-error conditions without grading the selected speed.
- `turn_delay_pass`: Adds `turn_delay=4` to the stock starboard 90-degree turn; passes when the turn eventually completes with time above one second, distance above three meters, and no behavior error, without grading the four-second hold interval.
- `timeout_complete_pass`: Configures `fix_turn=355`, `mod_hdg=10`, `speed=1`, and behavior `timeout=4`; passes when timeout-driven `FT_DONE=true` arrives with turn time between three and eight seconds, turn distance above two meters, and no behavior error.
- `turn_spec_sequence_pass`: Queues a 45-degree starboard turn followed by a 45-degree port turn, both at speed 1.4 and `mod_hdg=45`; passes when `TURN_ONE`, `TURN_TWO`, and `FT_DONE` are true, the final reported turn has time above one second and distance above three meters, and no behavior error was observed.
- `turn_spec_fallback_pass`: Queues `spd=-1,mhdg=-1,fix=-1,turn=auto` so the entry inherits static `speed=1.4,mod_hdg=25,fix_turn=90,turn_dir=star`; passes when `TURN_ONE` and `FT_DONE` are true, heading is 105 through 170 degrees, turn time and distance are positive, and no behavior error was observed.
- `turn_spec_key_update_pass`: Starts with keyed entry `alpha`, then at one second replaces it with a 45-degree port turn at speed 1.4 and `mod_hdg=45`, appending `TURN_TWO` and `FT_DONE` flags; passes when retained `TURN_ONE`, appended `TURN_TWO`, and `FT_DONE` are true, heading is above 285 degrees, time and distance are positive, and no behavior error was observed.
- `turn_spec_bad_update_recover_pass`: Sends malformed keyed update `key=alpha,bogus=oops` at one second and a valid 45-degree starboard replacement at two seconds; passes when `TURN_ONE`, `TURN_TWO`, and `FT_DONE` are true, heading is 70 through 130 degrees, time and distance are positive, and no behavior error was observed, without grading a rejection warning for the malformed update.
- `turn_spec_clear_pass`: Queues a near-full `TURN_ONE` entry, then posts `turn_spec=clear` at one second so the static starboard 90-degree turn runs instead; passes when `TURN_ONE=false`, `FT_DONE=true`, heading is 105 through 170 degrees, time and distance are positive, and no behavior error was observed.
- `turn_spec_timeout_pass`: Queues a 355-degree starboard turn with `mod_hdg=10`, speed 1, and entry `timeout=4`; passes when `TURN_ONE` and `FT_DONE` are true, turn time is three through eight seconds, turn distance exceeds two meters, and no behavior error was observed.
- `zero_fix_turn_pass`: Sets `fix_turn=0`; passes when `FT_DONE=true`, turn time is below two seconds, and no behavior error was observed, while turn distance and heading are report-only.
- `runtime_static_update_pass`: Posts `turn_dir=port` at one second and `speed=1.6` at 1.2 seconds before the static turn activates; passes when `FT_DONE=true`, heading is above 285 degrees, time and distance are positive, and no behavior error was observed, without grading the updated speed.
- `turn_spec_clear_add_pass`: Replaces a queued near-full `TURN_ONE` entry with `turn_spec=clear` plus a 45-degree port `TURN_TWO` entry at one second; passes when `TURN_ONE=false`, `TURN_TWO=true`, `FT_DONE=true`, heading is above 285 degrees, time and distance are positive, and no behavior error was observed.
- `turn_spec_aliases_pass`: Queues `fhdg=90,tdir=star` instead of `fix` and `turn`, overriding opposite static values; passes when `TURN_ONE` and `FT_DONE` are true, heading is 105 through 170 degrees, time and distance are positive, and no behavior error was observed.
- `bad_fix_turn_fail`: Sets `fix_turn=-5`; this expected-negative case passes at the 15-second mission timeout when `FT_DONE=false`, speed is below 0.1, heading remains 44 through 46 degrees, and no behavior error was observed.
- `bad_stale_nav_thresh_fail`: Sets `stale_nav_thresh=0`; this expected-negative case passes on the same timeout, no-completion, stopped, unchanged-heading, and no-error conditions.
- `bad_turn_dir_fail`: Sets `turn_dir=banana`; this expected-negative case passes on the same timeout, no-completion, stopped, unchanged-heading, and no-error conditions.
- `bad_speed_fail`: Sets `speed=-1`; this expected-negative case passes on the same timeout, no-completion, stopped, unchanged-heading, and no-error conditions.
- `bad_turn_spec_fail`: Adds `bogus=oops` to a scheduled turn; this expected-negative case passes on the same timeout, no-completion, stopped, unchanged-heading, and no-error conditions.
- `bad_schedule_repeat_fail`: Sets `schedule_repeat=maybe`; this expected-negative case passes on the same timeout, no-completion, stopped, unchanged-heading, and no-error conditions.

The stem also posts viewer-only context markers: a white approach-leg segment
and an orange fixed-turn start point. These markers do not participate in
grading. Near-360-degree cases additionally produce the `BHV_FixedTurn`
radial `VIEW_SEGLIST` report after completion; 90-degree turns do not produce
that built-in radial report because the behavior has no opposite-heading
samples from a quarter turn.

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=port_90_pass 10
./zlaunch.sh --jobs=4 --port_base=29400 --port_stride=12 10
./zlaunch.sh --just_make --jobs=4 --port_base=29400 --port_stride=12 10
```

The default `--port_base` is `15000` to avoid the repository's older 9000-range
defaults during active development. Grouped runs use 30-port case blocks by
default so each live case gets isolated MOOSDB and pShare ports. Use
`--port_stride=12` when a validation window must keep the full 24-case matrix
inside a 300-port range; each case uses only the `+0`, `+1`, `+10`, and `+11`
offsets within its block.

Latest validation:

- May 20, 2026
- focused expected-negative cases: `6/6` mission `grade=pass`
- wave matrix: `24/24` mission `grade=pass` with `--jobs=4 --port_base=29400 --port_stride=12`
- port range used by grouped run: `29400-29687`
- warp: `10`

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for a fully logged diagnostic case.
