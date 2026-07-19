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

- `thrust_forward_pass`: Starts at `(0,0)` heading `90` and applies thrust `50` with zero rudder, testing the standard `thrust_factor=20` forward-motion path; at eight seconds, passes with `14<=NAV_X<=25`, `-2<=NAV_Y<=2`, speed in `[2.2,2.8]`, and heading in `[88,92]`.
- `rudder_turn_starboard_pass`: Applies thrust `60` and rudder `45` from an east-facing stop, testing positive-rudder turning under forward power; at six seconds, passes with `NAV_X>=5`, speed at least `1.8`, and heading in `[135,230]`.
- `differential_turn_pass`: Enables `THRUST_MODE_DIFFERENTIAL` and posts left/right thrust `80/20` without ordinary rudder mail, testing differential propulsion and steering; at six seconds, passes with `NAV_X>=4`, speed at least `1.5`, and heading in `[120,220]`.
- `start_pos_seed_pass`: Configures `start_pos=x=20,y=-10,heading=180,speed=1,depth=3` and applies thrust `20`, testing nonzero planar state and speed initialization before southbound motion; at three seconds, passes with `18<=NAV_X<=22`, `NAV_Y<=-12`, heading in `[178,182]`, and speed at least `0.8`. Initial depth is reported but not graded.
- `max_speed_clamp_pass`: Configures `max_speed=2` and applies thrust `100`, whose factor-based target is `5`, testing the simulator speed cap; at seven seconds, passes with `20<=NAV_X<=35` and speed in `[1.8,2.1]`.
- `acceleration_limit_pass`: Configures `max_acceleration=0.2` and applies thrust `100` from rest, testing gradual acceleration toward the five-meter-per-second target; at five seconds, passes with `0.8<=NAV_X<=5` and speed in `[0.3,0.8]`.
- `drift_x_pass`: Posts `DRIFT_X=1.0` while thrust and rudder remain zero, testing runtime eastward drift independent of through-water speed; at seven seconds, passes with `5<=NAV_X<=9`, `-1<=NAV_Y<=1`, and speed within `[-0.1,0.1]`.
- `rotate_speed_pass`: Applies thrust `30` with zero rudder, then posts `ROTATE_SPEED=8`, testing direct runtime heading rotation; at six seconds, passes with speed in `[1.2,1.8]` and heading in `[120,150]`.
- `sim_pause_holds_pass`: Applies thrust `50`, then posts `USM_SIM_PAUSED=true` at 1.5 seconds, testing that position integration stops while the existing speed state is retained; at seven seconds, passes with `0<=NAV_X<=5`, speed in `[2.2,2.8]`, and heading in `[88,92]`.
- `reset_count_pass`: Applies thrust `50`, posts `USM_RESET=true` at four seconds, and allows two seconds of post-reset motion, testing reset accounting and return to the configured start state; passes when `USM_RESET_COUNT>=1`, `0<=NAV_X<=7`, and heading is in `[88,92]`.
- `disabled_nav_seed_pass`: Posts `USM_ENABLED=false`, followed by `NAV_X=42`, `NAV_Y=-17`, and `NAV_HEADING=275`, exercising the disabled-state nav-seeding inputs; at three seconds, passes when those same MOOS variables remain within one unit and heading is in `[274,276]`. Because the evaluator sees the injected mail directly, it does not prove the simulator consumed it.
- `reverse_thrust_pass`: Starts heading east and applies thrust `-40` with zero rudder, testing negative thrust and signed speed; at six seconds, passes with `NAV_X<=-3`, `-2<=NAV_Y<=2`, speed in `[-1.7,-1.2]`, and heading in `[88,92]`.
- `elevator_dive_pass`: Applies thrust `40` and elevator `60` with zero buoyancy, testing elevator-driven descent during forward motion; at six seconds, passes with `8<=NAV_X<=16`, `-2<=NAV_Y<=2`, speed in `[1.6,2.4]`, and depth in `[7,12]`.
- `runtime_turn_rate_pass`: Applies thrust `50` and rudder `60`, then lowers `USM_TURN_RATE` to `20`, testing runtime turn-authority reconfiguration; at six seconds, passes with speed in `[1.4,2.5]` and heading in `[110,150]`.
- `obstacle_hit_stop_pass`: Applies thrust `50`, then posts `OBSTACLE_HIT=true` at 1.5 seconds, testing the collision-stop input; at six seconds, passes with `1<=NAV_X<=6`, speed within `[-0.1,0.1]`, and heading in `[88,92]`.
- `current_y_alias_pass`: Posts `CURRENT_Y=1.2` with zero thrust and rudder, testing the runtime alias for y-axis drift; at seven seconds, passes with `-1<=NAV_X<=1`, `6<=NAV_Y<=10`, and speed within `[-0.1,0.1]`.
- `drift_vector_mult_pass`: Posts `DRIFT_VECTOR="90,1"`, then `DRIFT_VECTOR_MULT=2`, testing scalar multiplication of a runtime drift vector; at seven seconds, passes with `10<=NAV_X<=14`, `-1<=NAV_Y<=1`, and speed within `[-0.1,0.1]`.
- `water_depth_altitude_pass`: Starts at depth `12` with `default_water_depth=40` and zero buoyancy, testing altitude publication as water depth minus vehicle depth; at three seconds, passes with depth in `[11.5,12.5]` and altitude in `[27,29]`.
- `thrust_map_interpolation_pass`: Replaces `thrust_factor` with `thrust_map=0:0,50:1,100:4` and applies thrust `50`, testing lookup/interpolation at the map's midpoint; at seven seconds, passes with `5<=NAV_X<=9`, speed in `[0.8,1.2]`, and heading in `[88,92]`.
- `max_deceleration_pass`: Starts at speed `3`, configures `max_deceleration=0.1`, and commands zero thrust, testing rate-limited slowing instead of an immediate stop; at six seconds, passes with `12<=NAV_X<=28`, speed in `[2.2,2.8]`, and heading in `[88,92]`.
- `embedded_pid_speed_factor_pass`: Enables embedded simulation PID mode, requests speed `1.5` and heading `90`, and uses `speed_factor=20`, testing conversion of helm setpoints inside `uSimMarineV22`; at six seconds, passes with `28<=USM_PID_THRUST<=32`, `-2<=USM_PID_RUDDER<=2`, speed in `[1.3,1.7]`, and heading in `[88,92]`.
- `drift_vector_add_pass`: Posts eastward `DRIFT_VECTOR="90,1"`, then adds northward `DRIFT_VECTOR_ADD="0,1"`, testing vector composition; at seven seconds, passes with `5<=NAV_X<=8`, `4<=NAV_Y<=7`, and speed within `[-0.1,0.1]`.
- `runtime_buoyancy_rate_pass`: Starts stationary at depth `5` with zero configured buoyancy, then posts `BUOYANCY_RATE=-1`, testing runtime negative buoyancy; at five seconds, passes with depth in `[8,11]` and speed within `[-0.1,0.1]`.
- `rudder_slew_limit_pass`: Configures `max_rudder_degs_per_sec=5` and commands thrust `50` with rudder `100`, testing gradual rudder application; at six seconds, passes with speed in `[0.7,1.4]`, heading in `[155,180]`, `NAV_X>=3`, and `NAV_Y<=-2`.
- `turn_speed_map_pass`: Configures turn-rate interpolation from `5` degrees per second at zero speed to `70` at speed `4`, then commands thrust `30` and rudder `80`, testing speed-dependent turn authority; at six seconds, passes with speed in `[0.6,1.0]`, heading in `[125,140]`, `NAV_X>=3`, and `NAV_Y<=-1`.
- `wind_polar_speed_cap_pass`: Configures wind `spd=3,dir=180` and a polar plot, then commands thrust `100` while heading east, testing the wind-relative sailing-speed limit; at six seconds, passes with speed in `[2.2,2.6]`, `10<=NAV_X<=16`, and heading in `[88,92]`.
- `dual_state_reset_nav_pass`: Enables dual-state mode with `drift_x=1` and posts `USM_RESET_NAV=true` at three seconds, exercising resynchronization of navigation state to ground truth; at 3.4 seconds, passes with both `NAV_X` and `NAV_GT_X` in their configured `[4.8,7.5]` bands, y near zero, and speed near zero. The evaluator does not observe the pre-reset separation.
- `wormhole_transport_pass`: Starts inside a wormhole polygon centered at `(0,0)` with a one-way connection to a polygon centered at `(100,100)`, testing stationary spatial transport; at seven seconds, passes with both nav coordinates in `[98,102]` and speed within `[-0.1,0.1]`.
- `runtime_wind_conditions_pass`: Configures only the polar plot, commands thrust `100`, then posts `WIND_CONDITIONS="spd=3, dir=180"` at 0.8 seconds, testing runtime activation of the wind-relative speed limit; at six seconds, passes with speed in `[2.2,2.6]`, `10<=NAV_X<=16`, and heading in `[88,92]`.
- `runtime_water_depth_pass`: Starts stationary at depth `10`, then posts `WATER_DEPTH=40`, testing runtime recomputation of altitude without vertical motion; at three seconds, passes with depth in `[9.5,10.5]` and altitude in `[29,31]`.
- `runtime_thrust_mode_reverse_pass`: Enables `THRUST_MODE_REVERSE`, then applies positive thrust `40` with zero rudder, testing reverse-mode signed propulsion; at six seconds, passes with `-1<=NAV_X<=1`, `3<=NAV_Y<=5`, and speed in `[-1.8,-1.2]`.
- `max_depth_rate_limit_pass`: Configures `max_depth_rate=0` and commands thrust `50` with elevator `100`, testing suppression of vertical motion without suppressing propulsion; at six seconds, passes with speed in `[2.2,2.8]`, depth in `[0,0.5]`, and `NAV_X>=12`.
- `positive_buoyancy_surface_pass`: Starts stationary at depth `5` with `buoyancy_rate=1`, testing ascent and the zero-depth surface clamp; at six seconds, passes with depth in `[0,0.5]`, speed within `[-0.1,0.1]`, and `NAV_X` within `[-0.5,0.5]`.
- `embedded_pid_depth_control_pass`: Enables embedded simulation PID and depth control, then requests speed `2`, heading `90`, and depth `5`, testing internal thrust, heading, and depth actuation; at seven seconds, passes with `SIMULATION_MODE=TRUE`, speed in `[1.7,2.3]`, depth in `[1.0,5.5]`, `NAV_Z<=-1`, and heading in `[88,92]`.
- `buoyancy_control_report_pass`: Enables depth control with `buoyancy_rate=0.5` and posts `BUOYANCY_CONTROL=true`, testing the buoyancy-control completion protocol; at 6.5 seconds, passes when `BUOYANCY_REPORT` exactly equals `status=2,error=0,completed,buoyancy=0.0` and depth is in `[0,0.5]`.
- `trim_control_report_pass`: Enables depth control with `max_trim_delay=6` and posts `TRIM_CONTROL=true`, testing the trim-control completion protocol; at 6.5 seconds, passes when `TRIM_REPORT` exactly equals `status=2,error=0,completed,trim_pitch=0,trim_roll=0.0` and pitch is in `[-0.1,0.1]`.

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
