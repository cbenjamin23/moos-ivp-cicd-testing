# H01-constant_depth_motion

Patch-driven moving correctness harness for `BHV_ConstantDepth`.

The harness reuses
`missions/depth_behavior_missions/depth_behavior_motion`. A single simulated
UUV runs a horizontal waypoint only to keep realistic helm and simulator mail
flowing; the verdict is owned by the constant-depth behavior, depth telemetry,
and integration evidence from `pMarinePIDV22`.

## Current Matrix

- `constant_depth_hold_pass` Commands the baseline 12-meter depth with `peakwidth=4`, `basewidth=10`, and `summitdelta=15`. Passes after the eastbound transit reaches `x>=45` with `10<=NAV_DEPTH<=14`, `DEPTH_MISMATCH<=4`, the expected corridor and heading, and no behavior error.
- `constant_depth_surface_pass` Commands the valid lower boundary `depth=0` with a one-meter peak width and four-meter base width. Passes after `x>=40` with `NAV_DEPTH<=2`, `DEPTH_MISMATCH<=2`, and no behavior error.
- `constant_depth_negative_clip_pass` Configures `depth=-5` to test clamping a negative target to the surface. Passes after `x>=40` with `NAV_DEPTH<=2`, `DEPTH_MISMATCH<=2`, and no behavior error.
- `constant_depth_shape_params_pass` Configures `depth=10`, `peakwidth=0`, `basewidth=2`, and `summitdelta=100` to exercise the narrowest accepted ZAIC shape. Passes after `x>=45` with `8<=NAV_DEPTH<=12`, `DEPTH_MISMATCH<=2`, the expected corridor and heading, and no behavior error.
- `constant_depth_summitdelta_clip_pass` Configures `summitdelta=250` at an 11-meter target to exercise the over-range summit-delta path. Passes after `x>=45` with `9<=NAV_DEPTH<=13`, `DEPTH_MISMATCH<=3`, and no behavior error.
- `constant_depth_no_mismatch_var_pass` Omits the optional `depth_mismatch_var` while retaining the baseline 12-meter target. Passes after `x>=45` with `10<=NAV_DEPTH<=14`, the expected transit corridor, and no behavior error.
- `constant_depth_update_pass` Posts `DEPTH_VALUE="depth=18"` at 10 seconds to change the active target from 12 to 18 meters. Passes after `x>=55` with `16<=NAV_DEPTH<=20`, `DEPTH_MISMATCH<=4`, the expected corridor and heading, and no behavior error.
- `constant_depth_bad_update_preserve_pass` Posts the valid 18-meter update at 10 seconds and malformed `depth=bad` mail at 22 seconds to test preservation of the last valid target. Passes after `x>=55` with `16<=NAV_DEPTH<=20`, `DEPTH_MISMATCH<=4`, the expected corridor and heading, and no behavior error.
- `constant_depth_duration_complete_pass` Sets `duration=8` and `endflag=CD_DONE=true` to exercise finite behavior completion during the transit. Passes at 22 seconds when `CD_DONE=true` and `NAV_X>=20`; behavior-error state is recorded but not graded.
- `constant_depth_bad_depth_fail` Sets `depth=deep` to exercise rejection of a nonnumeric depth. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `constant_depth_bad_peakwidth_fail` Sets `peakwidth=wide` to exercise rejection of a nonnumeric peak width. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `constant_depth_bad_basewidth_fail` Sets `basewidth=wide` to exercise rejection of a nonnumeric base width. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `constant_depth_bad_summitdelta_fail` Sets `summitdelta=wide` to exercise rejection of a nonnumeric summit delta. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `constant_depth_bad_mismatch_var_fail` Sets `depth_mismatch_var=BAD VAR` to exercise rejection of a MOOS variable name containing whitespace. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `constant_depth_missing_duration_fail` Omits the required `duration` parameter from `BHV_ConstantDepth`. The harness passes when `ABE_BHV_ERROR` is observed.
- `constant_depth_missing_nav_depth_fail` Changes the simulator output prefix to `SIM`, leaving the helm without `NAV_X` or `NAV_DEPTH`. The harness passes at 45 seconds when both navigation variables remain absent, `DESIRED_ELEVATOR=0`, and no behavior error is observed.
- `constant_depth_domain_missing_fail` Removes `depth` from the IvP helm domain while retaining the constant-depth behavior. The harness passes when `ABE_BHV_ERROR` is observed.
- `constant_depth_low_elevator_authority_fail` Commands 24 meters while limiting `pMarinePIDV22` to `maxelevator=0.05`, exercising an underpowered depth controller. The harness passes after `x>=45` when `NAV_DEPTH<=22`, `DESIRED_ELEVATOR<=0.05`, and no behavior error is observed; reaching `NAV_DEPTH>=22.5` fails the case.
- `constant_depth_control_disabled_fail` Commands 16 meters with `depth_control=false` in `pMarinePIDV22`. The harness passes after `x>=45` when `NAV_DEPTH<=2`, `DEPTH_MISMATCH>=14`, and no behavior error is observed; reaching `NAV_DEPTH>=14` fails the case.

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
case order, uses root-scoped teardown, and continues after ordinary case
failures. Separate invocations are allowed; callers must choose non-overlapping
port ranges. All nineteen case
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
non-overlapping pShare ports, intended sidecars, unknown-case rejection, and
Bash 3.2 rejection. Failure probes verified
normalized `missing_result`, `duplicate_results`, and `prepare_error` rows.
Bash syntax, ShellCheck, and both skill static checkers pass. No tested MOOS
process survived cleanup.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case.
