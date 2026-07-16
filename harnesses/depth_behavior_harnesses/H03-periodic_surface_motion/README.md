# H03-periodic_surface_motion

Patch-driven moving correctness harness for `BHV_PeriodicSurface`.

The harness reuses
`missions/depth_behavior_missions/depth_behavior_motion`. Cases focus on
period timing, mark resets, acoustic-comms extensions, custom status variables,
ascent modes, timeout evidence, malformed config, and missing depth/speed
inputs.

## Current Matrix

- `periodic_surface_pass` Requires ascent and at-surface timeout evidence while a lower-priority constant-depth behavior keeps the UUV submerged between surfacing cycles.
- `periodic_surface_status_vars_pass` Uses custom pending and at-surface status variables and verifies they bridge to shoreside.
- `periodic_surface_status_variable_alias_pass` Uses the long-form status-variable parameter names and verifies they bridge to shoreside.
- `periodic_surface_wait_window_pass` Uses a long surface period and verifies the behavior remains in its pending wait window before ascent.
- `periodic_surface_acomms_extend_pass` Posts the acoustic-comms mark variable and verifies the pending surface window is extended.
- `periodic_surface_mark_variable_reset_pass` Posts a custom mark variable update and verifies it resets the pending surface timer before ascent.
- `periodic_surface_quadratic_ascent_pass` Uses the quadratic ascent grade and requires surfacing evidence.
- `periodic_surface_fullspeed_ascent_pass` Uses explicit fullspeed ascent and requires surfacing and at-surface timeout evidence.
- `periodic_surface_timeout_reset_pass` Verifies the behavior posts an at-surface timeout and resets surface dwell time before continuing the next cycle.
- `periodic_surface_current_speed_pass` Uses `ascent_speed=current` with a non-default grade and requires surfacing evidence.
- `periodic_surface_speed_alias_pass` Uses the `speed_to_surface` alias with linear ascent and requires surfacing evidence.
- `periodic_surface_bad_period_fail` Supplies an invalid surface period and expects the normal good-case verdict to fail.
- `periodic_surface_bad_ascent_grade_fail` Supplies an invalid ascent grade and expects the normal good-case verdict to fail.
- `periodic_surface_bad_ascent_speed_fail` Supplies an invalid ascent-speed value and expects the normal good-case verdict to fail.
- `periodic_surface_bad_zero_speed_depth_fail` Supplies a negative zero-speed depth and expects the normal good-case verdict to fail.
- `periodic_surface_bad_max_time_fail` Supplies a negative max-at-surface time and expects the normal good-case verdict to fail.
- `periodic_surface_bad_acomms_mark_fail` Supplies an invalid acoustic-comms mark tuple and expects the normal good-case verdict to fail.
- `periodic_surface_bad_acomms_interval_fail` Supplies a zero acoustic-comms extension interval and expects the normal good-case verdict to fail.
- `periodic_surface_bad_acomms_max_fail` Supplies an acoustic-comms max extension shorter than the interval and expects the normal good-case verdict to fail.
- `periodic_surface_bad_mark_variable_fail` Supplies an empty mark variable and expects helm configuration failure.
- `periodic_surface_bad_pending_status_var_fail` Supplies an empty pending-status variable and expects helm configuration failure.
- `periodic_surface_bad_atsurface_status_var_fail` Supplies an empty at-surface status variable and expects helm configuration failure.
- `periodic_surface_missing_nav_fail` Removes usable ownship depth/speed mail and expects the surfacing verdict to fail.
- `periodic_surface_domain_missing_fail` Removes the helm depth domain and expects the surfacing verdict to fail.

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
unique pShare ports, intended sidecars, unknown-case rejection, active-lock
behavior, Homebrew Bash re-execution, and explicit Bash 3.2 rejection. Bash
syntax, ShellCheck, the harness checker, and all 24 generated-case evaluator
checks pass. No tested MOOS process survived cleanup, and the shared depth stem
was not changed.
