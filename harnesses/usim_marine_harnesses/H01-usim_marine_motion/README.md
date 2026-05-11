# H01 uSimMarineV22 Motion

This harness validates `uSimMarineV22` as an app-level simulator dependency. The stem mission runs a single MOOS community with `uSimMarineV22`, `uTimerScript`, `pMissionEval`, `pAutoPoke`, and standard logging/watchdog apps. Cases patch the simulator configuration, scripted actuator/runtime mail, and grading conditions.

Run all cases:

```bash
./zlaunch.sh
```

Run one case for manual inspection:

```bash
./zlaunch.sh --case=thrust_forward_pass --gui
```

Run a grouped local batch on isolated ports:

```bash
./zlaunch.sh --jobs=4 --port_base=30000
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
