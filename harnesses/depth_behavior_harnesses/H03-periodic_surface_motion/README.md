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

The wrapper supports `--jobs` and `--port_base` for isolated grouped local and
CI runs.
