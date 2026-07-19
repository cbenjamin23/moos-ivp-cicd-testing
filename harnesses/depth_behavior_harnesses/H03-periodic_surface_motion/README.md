# H03-periodic_surface_motion

Patch-driven moving correctness harness for `BHV_PeriodicSurface`.

The harness reuses
`missions/depth_behavior_missions/depth_behavior_motion`. Cases focus on
period timing, mark resets, acoustic-comms extensions, custom status variables,
ascent modes, timeout evidence, malformed config, and missing depth/speed
inputs.

## Current Matrix

- `periodic_surface_pass` Configures an eight-second period, two-meter zero-speed depth, three-second surface dwell, and full-speed ascent while a lower-weight constant-depth behavior keeps the UUV submerged between cycles. Passes when both `PERIODIC_ASCEND` and `PSF_AT_SURFACE_TIMEOUT` have been observed with no behavior error.
- `periodic_surface_status_vars_pass` Binds the short-form parameters to `SURFACE_WAIT` and `SURFACE_DWELL`. Passes after a surfacing cycle and timeout when `SURFACE_WAIT<=8`, `SURFACE_DWELL>=0`, and no behavior error is observed.
- `periodic_surface_status_variable_alias_pass` Binds the long-form aliases to `SURFACE_WAIT_LONG` and `SURFACE_DWELL_LONG`. Passes after a surfacing cycle and timeout when `SURFACE_WAIT_LONG<=8`, `SURFACE_DWELL_LONG>=0`, and no behavior error is observed.
- `periodic_surface_wait_window_pass` Sets `period=30` and evaluates at 12 seconds to test the pre-ascent wait window. Passes when neither `PERIODIC_ASCEND` nor `PSF_AT_SURFACE_TIMEOUT` has appeared and no behavior error is observed.
- `periodic_surface_acomms_extend_pass` Configures `acomms_mark_variable=ACOMMS_UPDATE,5,10` and posts `ACOMMS_UPDATE=ping` at eight seconds, just before the ten-second period expires. Passes at 15 seconds when neither ascent nor surface-timeout evidence has appeared and no behavior error is observed.
- `periodic_surface_mark_variable_reset_pass` Configures `period=20`, binds `mark_variable=SURFACE_RESET_MARK`, and posts marks at zero and eight seconds. Passes at 24 seconds when no ascent or surface timeout has occurred, `PENDING_SURFACE>=1`, and no behavior error is observed.
- `periodic_surface_quadratic_ascent_pass` Configures `ascent_grade=quadratic` with an eight-second period and one-meter-per-second ascent. Passes when both `PERIODIC_ASCEND` and `PSF_AT_SURFACE_TIMEOUT` have been observed with no behavior error.
- `periodic_surface_fullspeed_ascent_pass` Configures `ascent_grade=fullspeed` with an eight-second period and one-meter-per-second ascent. Passes when both `PERIODIC_ASCEND` and `PSF_AT_SURFACE_TIMEOUT` have been observed with no behavior error.
- `periodic_surface_timeout_reset_pass` Uses a four-second period, two-second maximum surface dwell, and two-meter-per-second full-speed ascent. After `PSF_AT_SURFACE_TIMEOUT` appears, the harness passes when `TIME_AT_SURFACE<=1` before the 45-second deadline and no behavior error is observed.
- `periodic_surface_current_speed_pass` Configures `ascent_speed=current` and `ascent_grade=quasi`. Passes when both `PERIODIC_ASCEND` and `PSF_AT_SURFACE_TIMEOUT` have been observed with no behavior error.
- `periodic_surface_speed_alias_pass` Configures the `speed_to_surface=1.2` alias with linear ascent. Passes when both `PERIODIC_ASCEND` and `PSF_AT_SURFACE_TIMEOUT` have been observed with no behavior error.
- `periodic_surface_bad_period_fail` Sets `period=0` to exercise rejection of a nonpositive surfacing period. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `periodic_surface_bad_ascent_grade_fail` Sets `ascent_grade=rocket` to exercise rejection of an unknown ascent profile. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `periodic_surface_bad_ascent_speed_fail` Sets `ascent_speed=-1` to exercise rejection of a negative ascent speed. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `periodic_surface_bad_zero_speed_depth_fail` Sets `zero_speed_depth=-2` to exercise rejection of a negative transition depth. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `periodic_surface_bad_max_time_fail` Sets `max_time_at_surface=-1` to exercise rejection of a negative surface dwell limit. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `periodic_surface_bad_acomms_mark_fail` Configures `acomms_mark_variable=ACOMMS_MARK,20,10`, where the extension interval exceeds its maximum. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`; this is the same fixture used by `periodic_surface_bad_acomms_max_fail`.
- `periodic_surface_bad_acomms_interval_fail` Configures `acomms_mark_variable=ACOMMS_MARK,0,10` to exercise rejection of a zero extension interval. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `periodic_surface_bad_acomms_max_fail` Configures `acomms_mark_variable=ACOMMS_MARK,20,10`, where the maximum extension is shorter than one interval. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`; this duplicates `periodic_surface_bad_acomms_mark_fail`.
- `periodic_surface_bad_mark_variable_fail` Supplies an empty `mark_variable` value. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `periodic_surface_bad_pending_status_var_fail` Supplies an empty `pending_status_var` value. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `periodic_surface_bad_atsurface_status_var_fail` Supplies an empty `atsurface_status_var` value. The harness passes when `IVPHELM_STATE` reports `HELM_MALCONFIG`.
- `periodic_surface_missing_nav_fail` Changes the simulator output prefix to `SIM`, leaving the helm without `NAV_X` or `NAV_DEPTH`. The harness passes at 45 seconds when both navigation variables remain absent, `DESIRED_ELEVATOR=0`, and no behavior error is observed.
- `periodic_surface_domain_missing_fail` Removes `depth` from the IvP helm domain while retaining the periodic-surface behavior. The harness passes when `ABE_BHV_ERROR` is observed.

## Running

```bash
./zlaunch.sh --case=periodic_surface_pass 10
./zlaunch.sh --jobs=3 --port_base=43000 10
./zlaunch.sh --just_make 10
```

The Bash 5.1 wrapper creates an isolated mission copy for every serial and
rolling case, refills rolling slots as cases finish, aggregates one strict
mission-owned result row per selected case, and performs root-scoped cleanup.

### Migration evidence

Two precisely timed clean untouched legacy `--jobs=4` matrices passed 48/48
rows in 77.692 and 77.808 seconds, for a 77.750-second mean. An additional
untimed console-output matrix passed 24/24, and the untouched serial matrix
passed 24/24 in 199.477 seconds.

A third precisely timed untouched rolling matrix finished in 77.358 seconds
but had
`periodic_surface_status_variable_alias_pass` grade before its required
at-surface timeout flag arrived. The mission had already surfaced, and the
unchanged case then passed 10/10 focused legacy repetitions. The outlier is a
pre-existing fixed-snapshot race and is excluded from the clean timing mean.

The migrated positive surfacing evaluators retain their existing time gates
and additionally wait for the already-required sticky surfacing and timeout
evidence. No behavior parameter, stimulus, threshold, or event time changed.
The long-form status-variable alias case passed 10/10 focused migrated runs.

`periodic_surface_timeout_reset_pass` now evaluates the behavior's real order:
first the timeout must be posted, then the at-surface counter must reset to at
most one. The existing timer is retained as the reset deadline, so a behavior
that never resets still produces a mission-owned failure. This avoids sampling
the counter between the timeout post and the reset on the next helm iteration.
The ordered case passed 10/10 focused repetitions with `timeout_seen=true` and
`surface_time=0` in every result.

The first migrated rolling matrix exposed the same one-iteration sampling
problem in the timeout-reset case: it observed the timeout while
`TIME_AT_SURFACE` was still two. The ordered evaluator above is the resulting
correction; no test threshold was relaxed.

Three clean migrated rolling matrices passed 72/72 rows in 64, 63, and 63
seconds. Their 63.33-second mean is 14.42 seconds, about 18.5 percent, faster
than the legacy wave mean. The isolated serial matrix passed 24/24 in 222
seconds, 22.52 seconds or about 11.3 percent slower than legacy, roughly 0.94
seconds per case for isolated copying and verified cleanup.

Validation covered all-case generation, three clean rolling matrices, one
complete migrated serial matrix, focused alias and reset sweeps, exact matrix
and patch-map reconciliation, rolling refill, 48 unique MOOSDB ports, 48
unique pShare ports, intended sidecars, unknown-case rejection, Homebrew Bash
re-execution, and explicit Bash 3.2 rejection. Bash
syntax, ShellCheck, the harness checker, and all 24 generated-case evaluator
checks pass. No tested MOOS process survived cleanup, and the shared depth stem
was not changed.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case.
