# H03-periodic_surface_motion

Patch-driven moving correctness harness for `BHV_PeriodicSurface`.

The harness reuses
`missions/depth_behavior_missions/depth_behavior_motion`. Cases focus on
period timing, mark resets, acoustic-comms extensions, custom status variables,
ascent modes, timeout evidence, malformed config, and missing depth/speed
inputs.

## Current Matrix

- `periodic_surface_pass` Configures an eight-second wait, a two-meter zero-speed depth, a three-second surface dwell, and full-speed ascent to test one complete periodic-surface cycle; passes as soon as an ascent and its `PSF_AT_SURFACE_TIMEOUT` have both occurred with no behavior error.
- `periodic_surface_status_vars_pass` Binds the short parameter names to `SURFACE_WAIT` and `SURFACE_DWELL` to test their waiting-phase values; passes when the pending countdown reaches three through five seconds while the surface-dwell counter remains zero, with no behavior error.
- `periodic_surface_status_variable_alias_pass` Binds the long parameter aliases to `SURFACE_WAIT_LONG` and `SURFACE_DWELL_LONG` to test their at-surface values; passes during the first surface dwell when the dwell counter is one through three, the pending counter is nonpositive, an ascent has occurred, and no behavior error is observed.
- `periodic_surface_wait_window_pass` Sets `period=30` and evaluates at 12 seconds to test the pre-ascent wait window. Passes when neither `PERIODIC_ASCEND` nor `PSF_AT_SURFACE_TIMEOUT` has appeared and no behavior error is observed.
- `periodic_surface_acomms_extend_pass` Configures `acomms_mark_variable=ACOMMS_UPDATE,5,10` and posts `ACOMMS_UPDATE=ping` at eight seconds, just before the ten-second period expires. Passes at 15 seconds when neither ascent nor surface-timeout evidence has appeared and no behavior error is observed.
- `periodic_surface_mark_variable_reset_pass` Configures `period=20`, binds `mark_variable=SURFACE_RESET_MARK`, and posts marks at zero and eight seconds. Passes at 24 seconds when no ascent or surface timeout has occurred, `PENDING_SURFACE>=1`, and no behavior error is observed.
- `periodic_surface_quadratic_ascent_pass` Starts at 12 meters and configures `ascent_grade=quadratic`, `ascent_speed=1`, and `zero_speed_depth=2` to test the squared speed ramp; passes while ascending through six to eight meters when `ABE_DESIRED_SPEED` is 0.1–0.5 m/s and no behavior error is observed.
- `periodic_surface_fullspeed_ascent_pass` Starts at 12 meters and configures `ascent_grade=fullspeed` with `ascent_speed=1` to test that the requested speed is held throughout ascent; passes while ascending through six to eight meters when `ABE_DESIRED_SPEED` is 0.9–1.1 m/s and no behavior error is observed.
- `periodic_surface_timeout_reset_pass` Uses a four-second period, two-second maximum surface dwell, and two-meter-per-second full-speed ascent. After `PSF_AT_SURFACE_TIMEOUT` appears, the harness passes when `TIME_AT_SURFACE<=1` before the 45-second deadline and no behavior error is observed.
- `periodic_surface_current_speed_pass` Starts at 12 meters with the vehicle moving near 2 m/s and configures `ascent_speed=current` with a quasi ramp to test capture of the live speed at ascent start; passes while ascending through 10–11.5 meters when `ABE_DESIRED_SPEED` is 1.5–2.1 m/s and no behavior error is observed.
- `periodic_surface_speed_alias_pass` Starts at 12 meters and configures the legacy `speed_to_surface=1.2` alias with a linear ramp to test that the alias supplies the ramp's starting speed; passes while ascending through six to eight meters when `ABE_DESIRED_SPEED` is 0.4–0.8 m/s and no behavior error is observed.
- `periodic_surface_bad_period_fail` Sets `period=0` to test rejection of a nonpositive surfacing period; passes only when `IVPHELM_STATE=MALCONFIG`.
- `periodic_surface_bad_ascent_grade_fail` Sets `ascent_grade=rocket` to test rejection of an unknown ascent profile; passes only when `IVPHELM_STATE=MALCONFIG`.
- `periodic_surface_bad_ascent_speed_fail` Sets `ascent_speed=-1` to test rejection of a negative ascent speed; passes only when `IVPHELM_STATE=MALCONFIG`.
- `periodic_surface_bad_zero_speed_depth_fail` Sets `zero_speed_depth=-2` to test rejection of a negative transition depth; passes only when `IVPHELM_STATE=MALCONFIG`.
- `periodic_surface_bad_max_time_fail` Sets `max_time_at_surface=-1` to test rejection of a negative surface dwell limit; passes only when `IVPHELM_STATE=MALCONFIG`.
- `periodic_surface_bad_acomms_mark_fail` Configures `acomms_mark_variable=BAD MARK,5,10` to test rejection of whitespace in the acoustic mark variable name; passes only when `IVPHELM_STATE=MALCONFIG`.
- `periodic_surface_bad_acomms_interval_fail` Configures `acomms_mark_variable=ACOMMS_MARK,0,10` to test rejection of a zero extension interval; passes only when `IVPHELM_STATE=MALCONFIG`.
- `periodic_surface_bad_acomms_max_fail` Configures `acomms_mark_variable=ACOMMS_MARK,20,10` to test rejection when one extension interval exceeds the maximum accumulated extension; passes only when `IVPHELM_STATE=MALCONFIG`.
- `periodic_surface_bad_mark_variable_fail` Supplies an empty `mark_variable` value to test rejection of a missing reset source; passes only when `IVPHELM_STATE=MALCONFIG`.
- `periodic_surface_bad_pending_status_var_fail` Supplies an empty `pending_status_var` value to test rejection of a missing pending-status output name; passes only when `IVPHELM_STATE=MALCONFIG`.
- `periodic_surface_bad_atsurface_status_var_fail` Supplies an empty `atsurface_status_var` value to test rejection of a missing surface-dwell output name; passes only when `IVPHELM_STATE=MALCONFIG`.
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

### Validation

The strengthened 24-case rolling matrix passes 24/24. Focused mutations that
swap the quadratic and full-speed profiles, replace `current` with a fixed
speed, alter `speed_to_surface`, or redirect either custom status output all
fail their intended case. Replacing the invalid acoustic variable token with
the valid `ACOMMS_MARK,5,10` tuple also fails its negative case. The
`behaviors-marine` CTest family passes 207/207, including direct checks of the
quadratic profile, one-time current-speed capture, long status aliases, and
the `speed_to_surface` alias.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case.
