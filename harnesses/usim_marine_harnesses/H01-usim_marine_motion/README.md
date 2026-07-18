# H01 uSimMarineV22 Motion

This harness validates `uSimMarineV22` as an app-level simulator dependency. The stem mission runs a single MOOS community with `uSimMarineV22`, `uTimerScript`, `pMissionEval`, `pAutoPoke`, and standard logging/watchdog apps. Cases patch the simulator configuration, scripted actuator/runtime mail, and grading conditions.

Run all cases:

```bash
./zlaunch.sh
```

Run one isolated case and retain its generated mission for inspection:

```bash
./zlaunch.sh --case=thrust_forward_pass --keep_workdirs
```

Run two cases at a time with rolling refill on isolated ports:

```bash
./zlaunch.sh --jobs=2 --port_base=22000
```

## Current Matrix

- `thrust_forward_pass` Straight positive thrust reaches the expected forward speed and eastward displacement while holding heading.
- `rudder_turn_starboard_pass` Positive rudder under forward thrust produces a clear starboard heading change while maintaining forward motion.
- `differential_turn_pass` Differential left/right thrust switches the simulator into differential mode and produces a heading change without ordinary rudder mail.
- `start_pos_seed_pass` A non-zero `start_pos` seeds initial x/y, heading, speed, and subsequent motion as expected.
- `max_speed_clamp_pass` High thrust with a low `max_speed` cap keeps published speed bounded.
- `acceleration_limit_pass` A low `max_acceleration` prevents immediate jump to the requested thrust-derived speed.
- `drift_x_pass` Positive `DRIFT_X` moves the vehicle east even without commanded thrust.
- `rotate_speed_pass` Runtime `ROTATE_SPEED` changes heading without rudder input.
- `sim_pause_holds_pass` Runtime `USM_SIM_PAUSED=true` prevents continued motion under commanded thrust.
- `reset_count_pass` Runtime `USM_RESET` increments the simulator reset counter and returns position near the configured start.
- `disabled_nav_seed_pass` When `USM_ENABLED=false`, runtime `NAV_X`, `NAV_Y`, and `NAV_HEADING` mail seeds simulator state.
- `reverse_thrust_pass` Negative thrust produces reverse speed and westward displacement from an east-facing start.
- `elevator_dive_pass` Positive elevator under forward thrust increases published depth while maintaining bounded forward motion.
- `runtime_turn_rate_pass` Runtime `USM_TURN_RATE` lowers rudder turn authority and produces a bounded heading change.
- `obstacle_hit_stop_pass` Runtime `OBSTACLE_HIT=true` forces the simulator to publish a stopped vehicle state.
- `current_y_alias_pass` Runtime `CURRENT_Y` acts as a drift-y alias and moves the vehicle laterally without commanded thrust.
- `drift_vector_mult_pass` Runtime `DRIFT_VECTOR` plus `DRIFT_VECTOR_MULT` scales an external drift vector before evaluation.
- `water_depth_altitude_pass` A configured water depth and start depth produce the expected published altitude.
- `thrust_map_interpolation_pass` A custom `thrust_map` maps midrange thrust to bounded speed without using `thrust_factor`.
- `max_deceleration_pass` A low `max_deceleration` keeps an initially moving vehicle from stopping abruptly.
- `embedded_pid_speed_factor_pass` Embedded PID mode maps desired speed through `speed_factor` and publishes custom desired-thrust/rudder evidence.
- `drift_vector_add_pass` Runtime `DRIFT_VECTOR_ADD` composes a second drift vector with the existing one and produces two-axis displacement.
- `runtime_buoyancy_rate_pass` Runtime `BUOYANCY_RATE` changes vertical motion while the vehicle is otherwise stationary.
- `rudder_slew_limit_pass` `max_rudder_degs_per_sec` limits how quickly a large rudder command can take effect.
- `turn_speed_map_pass` A configured turn-speed map shapes turn authority as a function of vehicle speed.
- `wind_polar_speed_cap_pass` Configured wind and polar-plot data cap high commanded thrust at the wind-relative sailing limit.
- `dual_state_reset_nav_pass` In dual-state mode, runtime `USM_RESET_NAV` snaps navigation state back to ground truth after drift separates them.
- `wormhole_transport_pass` A configured wormhole transports the vehicle between convex polygons and updates published navigation position.
- `runtime_wind_conditions_pass` Runtime `WIND_CONDITIONS` activates an existing polar plot and caps high commanded thrust.
- `runtime_water_depth_pass` Runtime `WATER_DEPTH` updates published altitude without moving the vehicle vertically.
- `runtime_thrust_mode_reverse_pass` Runtime `THRUST_MODE_REVERSE` flips the effective thrust/heading mode under positive thrust.
- `max_depth_rate_limit_pass` A zero `max_depth_rate` suppresses elevator-driven diving under forward motion.
- `positive_buoyancy_surface_pass` Positive buoyancy moves a submerged stationary vehicle back to the surface clamp.
- `embedded_pid_depth_control_pass` Embedded PID depth control publishes simulation-mode depth state while driving toward a desired depth.
- `buoyancy_control_report_pass` `BUOYANCY_CONTROL` completes through the depth-control report path and posts the expected completion report.
- `trim_control_report_pass` `TRIM_CONTROL` completes through the depth-control report path and posts the expected trim report.

## Launcher Contract

Every selected case runs from its own copy of the `usim_marine_motion` stem,
including single-case and serial runs. `thrust_forward_pass` uses the unpatched
stem. Every other case applies one exact shoreside `nspatch` overlay inside its
copy, then calls that copy's mission `zlaunch.sh`. The source stem is never
patched in place.

The launcher requires Bash 5.1 or newer for rolling `--jobs` scheduling. On
macOS it safely re-executes a suitable Homebrew or Linuxbrew Bash when one is
available. Each case receives a stride-30 port block with its one MOOSDB at
`+0` and a reserved shoreside pShare slot at `+15`. This mission does not
launch pShare; the slot is forwarded only for a consistent launcher and port
allocation contract.

`pMissionEval` is the sole owner of the live verdict. The shell requires
exactly one mission row with exactly one `grade=pass|fail`, adds the selected
`case=`, and writes one aggregate row per selected case in declared order. It
does not compare the result with a shell-side expected value. Ordinary case
failures do not stop later cases. Cleanup is limited to the case root; if
cleanup cannot be proved, the scheduler stops launching new cases and retains
the run root and safety lock for diagnosis.

`--gui` remains accepted for single-case wrapper parity, but this app-level
mission contains no viewer. Use `--keep_workdirs` to inspect generated targets,
applied sidecars, case logs, and intermediate result rows under
`.harness_runs`. See [`NSPATCH.md`](NSPATCH.md) for the patch contract.

The harness defaults to `--log=minimal`, which runs without `pLogger` because
all grading evidence is live. Use `--log=full` for the complete matrix, or
combine it with `--case=<name>` for one inspectable fully logged case.

## Harness-skill Migration Validation

Validation on July 14, 2026 preserved all thirty-six case tokens in their
declared order, the unpatched baseline, all thirty-five shoreside mappings,
every `mission_mod`, every scripted input, every evaluator condition, and every
report column. No stem template, case overlay, simulator setting, grading
variable, application, or coverage assertion changed.

Untouched legacy measurements at warp 10 and `--max_time=45` were:

- three two-worker batch-wave matrices: 108/108 rows passing in 124.23, 134.17,
  and 119.89 seconds, for a 126.10-second mean
- one shared-stem serial matrix: 36/36 passing in 191.91 seconds

Migrated validation included both skill static checkers, Bash syntax,
ShellCheck, Bash 3.2 rejection and Homebrew re-execution, invalid CLI and port
probes, a retained full generation matrix, an independent stem pass, isolated
nominal and obstacle-stop cases, one complete serial matrix, three complete
rolling matrices, and missing-file, missing-row, duplicate-row, invalid-grade,
duplicate-grade, nonzero-launch, preparation, timeout, and teardown failure
paths.

Complete migrated measurements were:

- isolated serial: 36/36 passing in 217.00 seconds
- rolling `--jobs=2`: 108/108 rows passing in 111.09, 111.03, and 107.75
  seconds, for a 109.96-second mean
- final checkout-state rolling matrix: 36/36 passing in 114.14 seconds from
  assigned MOOSDB and reserved pShare slots verified clear afterward

The rolling mean is 16.14 seconds, about 12.8 percent, faster than the matched
legacy mean. Its 3.34-second range is also smaller than the legacy 14.28-second
range. Serial is 25.09 seconds, about 13.1 percent, slower than the one legacy
sample because it now pays the same isolated-copy and scoped-cleanup cost for
every case.

The retained rolling matrix had thirty-six targets, the intended thirty-five
sidecars, thirty-six original `.alog` files, one mission row per case, exact
selected-order results, and no runtime error flags or pShare blocks. Verbose
events proved immediate refill after each completed case. Every assigned
MOOSDB and reserved pShare slot was clear afterward.

The migration deliberately leaves the existing fixed-time snapshot coverage
strategy unchanged. Any later effort to strengthen temporal, history, or
absence assertions should be handled as separate case-quality work.
