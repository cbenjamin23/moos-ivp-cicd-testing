# H01-constant_depth_motion

Patch-driven moving correctness harness for `BHV_ConstantDepth`.

The harness reuses
`missions/depth_behavior_missions/depth_behavior_motion`. A single simulated
UUV runs a horizontal waypoint only to keep realistic helm and simulator mail
flowing; the verdict is owned by the constant-depth behavior, depth telemetry,
and integration evidence from `pMarinePIDV22`.

## Current Matrix

- `constant_depth_hold_pass` Baseline hold at the stock target depth with bounded mismatch and clean behavior state.
- `constant_depth_surface_pass` Holds the lower depth boundary at the surface and verifies zero-depth mismatch.
- `constant_depth_negative_clip_pass` Supplies a negative configured depth and verifies mission-time clipping to the surface.
- `constant_depth_shape_params_pass` Uses tight ZAIC shaping parameters and verifies the vehicle still settles inside the intended depth band.
- `constant_depth_summitdelta_clip_pass` Supplies an over-range summit delta and verifies the behavior clips it while still holding depth.
- `constant_depth_no_mismatch_var_pass` Omits the optional mismatch telemetry variable and verifies the behavior still controls depth.
- `constant_depth_update_pass` Posts a runtime `DEPTH_VALUE` update and requires the behavior to settle near the updated target.
- `constant_depth_bad_update_preserve_pass` Posts a valid runtime update followed by malformed update mail and verifies the valid target remains in force.
- `constant_depth_duration_complete_pass` Gives the behavior a finite duration and verifies its completion flag is posted in the moving mission.
- `constant_depth_bad_depth_fail` Supplies malformed depth config and expects the normal good-case verdict to fail.
- `constant_depth_bad_peakwidth_fail` Supplies malformed ZAIC width config and expects the normal good-case verdict to fail.
- `constant_depth_bad_basewidth_fail` Supplies malformed ZAIC basewidth config and expects the normal good-case verdict to fail.
- `constant_depth_bad_summitdelta_fail` Supplies malformed summit-delta config and expects the normal good-case verdict to fail.
- `constant_depth_bad_mismatch_var_fail` Supplies a whitespace-bearing mismatch variable name and expects the normal good-case verdict to fail.
- `constant_depth_missing_duration_fail` Omits the required duration override and expects the behavior to time out before holding depth.
- `constant_depth_missing_nav_depth_fail` Removes usable ownship depth mail and expects the constant-depth good-case verdict to fail.
- `constant_depth_domain_missing_fail` Removes the helm depth domain and expects the constant-depth good-case verdict to fail.
- `constant_depth_low_elevator_authority_fail` Limits PID elevator authority while commanding a deep target; the good-case depth band should not be reached.
- `constant_depth_control_disabled_fail` Disables PID depth control while commanding a dive; the good-case depth band should not be reached.

## Running

```bash
./zlaunch.sh --case=constant_depth_hold_pass 10
./zlaunch.sh --jobs=3 --port_base=41000 10
./zlaunch.sh --just_make 10
```

The wrapper supports `--jobs` and `--port_base` for isolated grouped local and
CI runs.
