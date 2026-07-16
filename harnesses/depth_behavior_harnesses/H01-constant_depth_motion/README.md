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

The wrapper requires Bash 5.1 or newer and supports `--jobs`, `--port_base`,
and `--port_stride` for isolated serial and rolling local or CI runs.

## Scheduling Notes

Five existing timing-sensitive cases remain solo-slot cases:
`constant_depth_shape_params_pass`,
`constant_depth_summitdelta_clip_pass`,
`constant_depth_no_mismatch_var_pass`,
`constant_depth_bad_update_preserve_pass`, and
`constant_depth_low_elevator_authority_fail`. A rolling run drains active
workers before starting one of these cases, then resumes rolling refill for
the rest of the matrix. This preserves the legacy isolation policy without
weakening any evaluator condition.

The migrated launcher uses one isolated mission copy and one dedicated MOOSDB
and pShare port block per case in both serial and rolling modes. It aggregates
exactly one mission-owned pMissionEval row per selected case in deterministic
case order, uses root-scoped teardown, rejects concurrent invocations with a
harness lock, and continues after ordinary case failures. All nineteen case
names, explicit patch mappings, evaluator conditions, behavior values, event
times, grading variables, and coverage claims are unchanged.

Three untouched legacy `--jobs=4` matrices passed 57/57 rows in 122.06,
123.24, and 123.30 seconds, for a 122.87-second mean. The untouched serial
matrix passed 19/19 in 155.30 seconds. Three migrated rolling matrices passed
57/57 rows in 116.66, 116.16, and 116.48 seconds, for a 116.43-second mean,
about 5.2 percent faster. The isolated migrated serial matrix passed 19/19 in
181.14 seconds, 25.84 seconds or about 16.6 percent slower, roughly 1.36
seconds per case.

A controlled serial run using the teardown helper's supported one-second INT
and TERM grace overrides passed in 164.97 seconds. Thus about 16.17 seconds of
the default serial difference came from the canonical three-second graceful
cleanup budgets; the remaining 9.67 seconds, about 0.51 seconds per case,
came from isolated copying and the standard mission-wrapper lifecycle. The
canonical cleanup defaults remain unchanged.

Validation covered all-case generation, three full rolling matrices, one full
serial matrix, nominal depth hold, expected helm-malconfiguration, all five
solo-slot cases, standalone stem generation and live execution with both the
preserved 55-second default and an explicit 80-second ceiling, exact matrix
and patch-map reconciliation, rolling refill, 38 unique MOOSDB ports, 38
non-overlapping pShare ports, intended sidecars, unknown-case rejection,
active-lock behavior, and Bash 3.2 rejection. Failure probes verified
normalized `missing_result`, `duplicate_results`, and `prepare_error` rows.
Bash syntax, ShellCheck, and both skill static checkers pass. No tested MOOS
process survived cleanup.
