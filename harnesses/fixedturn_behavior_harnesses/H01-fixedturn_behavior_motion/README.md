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

- `starboard_90_pass`: baseline starboard 90-degree turn.
- `port_90_pass`: mirrored 90-degree port turn.
- `starboard_360_pass`: long starboard turn with distance/time sanity checks.
- `port_360_pass`: long port turn with distance/time sanity checks.
- `speed_auto_pass`: turn speed follows the stem speed branch.
- `fixed_speed_pass`: explicit fixed turn speed branch.
- `turn_delay_pass`: delayed-turn branch with ordinary fixed-turn completion.
- `timeout_complete_pass`: timeout completion branch.
- `turn_spec_sequence_pass`: two scheduled turns with per-turn flags.
- `bad_fix_turn_fail`: invalid `fix_turn` configuration should fail.
- `bad_stale_nav_thresh_fail`: invalid stale-nav threshold should fail.

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=port_90_pass 10
./zlaunch.sh --jobs=4 --port_base=15000 10
./zlaunch.sh --just_make --jobs=4 --port_base=15000 10
```

The default `--port_base` is `15000` to avoid the repository's older 9000-range
defaults during active development.

Latest validation:

- April 26, 2026
- full matrix: `11/11` expected outcomes matched
- wave matrix: `11/11` expected outcomes matched with `--jobs=4`
- port base: `15000`
- warp: `10`
