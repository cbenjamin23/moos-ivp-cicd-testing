# H02-pid_motion

Patch-driven closed-loop motion harness for `pMarinePIDV22`.

This harness reuses the `missions/pid_missions/pid_motion` two-community stem.
Each case runs `pMarinePIDV22` with `pHelmIvP` and `uSimMarineV22`, then lets
shoreside `pMissionEval` grade bridged motion and actuator evidence. H02 is the
integration companion to H01: H01 proves direct PID publications; H02 proves
that those publications can drive simulated motion.

## Current Matrix

- `baseline_transit_pass`: Starts at `(0,-60)` heading east and follows a `BHV_Waypoint` to `(70,-60)` using `speed_factor=20`, testing the normal helm-to-PID-to-simulator chain; passes on `ARRIVED=true` with `55<=x<=80`, `-72<=y<=-48`, heading in `[85,95]`, and speed at most `3.0`.
- `turn_recover_pass`: Starts at `(0,-60)` heading north while the waypoint remains east at `(70,-60)`, testing closed-loop correction from a 90-degree initial heading error; passes on arrival with `55<=x<=80`, `-76<=y<=-44`, heading in `[80,115]`, and speed at most `3.0`.
- `hard_turn_recover_pass`: Starts at `(0,-60)` heading west, opposite the eastbound waypoint, testing recovery from a 180-degree initial heading error; passes on arrival with `55<=x<=80`, `-76<=y<=-44`, heading in `[70,115]`, and speed at most `3.0`.
- `rudder_bias_recover_pass`: Sets `sim_instability=18` in `pMarinePIDV22`, exercising the controller's simulated rudder disturbance while following the eastbound waypoint; passes on arrival with `55<=x<=80`, `-76<=y<=-44`, heading in `[75,125]`, and speed at most `3.0`.
- `speed_pid_transit_pass`: Sets `speed_factor=0` and `speed_pid_kp=40`, selecting closed-loop speed PID control for the eastbound transit; passes on arrival with `55<=x<=80`, `-72<=y<=-48`, heading in `[85,95]`, and speed at most `3.0`.
- `runtime_speed_factor_update_pass`: Starts with `speed_factor=4` and posts `SPEED_FACTOR=20` at eight seconds, testing that an underway factor update restores enough speed to reach `(70,-60)` by the 48-second deadline; passes on arrival with `55<=x<=80`, `-72<=y<=-48`, heading in `[85,95]`, and speed at most `3.0`.
- `depth_step_pass`: Extends the waypoint to `(140,-60)`, enables PID depth control, and commands `BHV_ConstantDepth` to `10` meters, testing closed-loop elevator and depth response during transit; at 45 seconds, passes with `x>=50`, `-74<=y<=-46`, heading in `[85,95]`, and depth in `[8,13]`.
- `override_release_pass`: Starts deployed under manual override, releases `MOOS_MANUAL_OVERRIDE_ALL` at eight seconds, and continues toward `(70,-60)`, exercising post-override control reacquisition; passes on arrival with `55<=x<=80`, `-72<=y<=-48`, heading in `[85,95]`, and speed at most `3.0`. The evaluator does not grade pre-release immobility.
- `low_rudder_authority_fail`: Starts heading north and limits `maxrudder=2`, testing the degraded turn response under insufficient rudder authority; at 42 seconds, the harness passes when `x<55`, `y>-48`, heading is below `80`, and `DESIRED_RUDDER<=2.5`.
- `low_thrust_authority_fail`: Limits `maxthrust=8` on the eastbound transit, testing the degraded progress caused by insufficient thrust authority; at the inherited 45-second evaluation, the harness passes when `x<55`, speed is at most `0.8`, and `DESIRED_THRUST<=8.5`.
- `low_elevator_authority_fail`: Extends the waypoint to `(140,-60)`, commands depth `30`, and limits `maxelevator=0.25`, testing the degraded dive under insufficient elevator authority; at 45 seconds, the harness passes with `x>=50`, `-74<=y<=-46`, heading in `[85,95]`, depth below `25`, and `DESIRED_ELEVATOR>=-0.5`.
- `depth_control_disabled_fail`: Extends the waypoint to `(140,-60)` and commands depth `10` while `pMarinePIDV22` keeps `depth_control=false`, testing that the helm's depth request cannot move the simulator vertically without PID elevator control; at 45 seconds, the harness passes with `x>=50`, `-74<=y<=-46`, heading in `[85,95]`, and depth at most `1`.
- `manual_override_fail`: Deploys the helm while holding `MOOS_MANUAL_OVERRIDE_ALL=true`, testing the expected no-motion state under sustained override; at the inherited 45-second evaluation, the harness passes when `x<=5`, speed is at most `0.1`, and `DESIRED_THRUST=0`.

## Edge Audit

Covered in H02:

- Closed-loop heading correction through `uSimMarineV22`.
- Large heading-error recovery and rudder-bias disturbance recovery.
- Closed-loop speed-factor and speed-PID modes.
- Runtime `SPEED_FACTOR` mail updates while underway.
- Depth-control actuation through `BHV_ConstantDepth`, `pMarinePIDV22`, and simulated depth response.
- PID control reacquisition after manual override release.
- Expected-negative sentinels for insufficient rudder, thrust, and elevator authority, disabled depth control, and held manual override.

Deferred beyond this first H02 layer:

- Long-duration oscillation scoring and formal settling-time metrics.
- Disturbance or current modeling.
- Multi-leg waypoint courses and repeated depth setpoint sequences.

These deferred items are not missing basic `pMarinePIDV22` coverage. H01 owns
direct app-contract and source-branch checks, while H02 owns closed-loop
mission correctness with the helm and simulator in the loop. A future H03
should therefore be framed as a dynamics or quality harness, not another
correctness expansion: smaller case count, longer runs, and explicit evaluation
of settling, oscillation, repeated setpoint changes, multi-leg recovery, or
disturbance rejection. With the current `pMissionEval`-only structure, those
criteria should stay window/corridor based unless a separate metric source is
added.

Candidate H03 quality metrics include time-to-arrival or time-to-depth-band,
maximum cross-track error, heading overshoot and final settling band, depth
overshoot and steady-state error, actuator saturation duration, and oscillation
count or RMS error over a final evaluation window. Those metrics are valuable
for PID tuning regression, but they are intentionally separate from H02's
closed-loop correctness contract.

## Quality Criteria

Each case grades motion outcome through `pMissionEval`, not by parsing alogs in
the harness. Arrival-oriented cases evaluate on arrival or deadline and use
endpoint, corridor, heading, and bounded-output checks. Depth-oriented cases use
underway evaluation windows with tighter depth and heading bands.
Expected-negative cases are named with `_fail`, but now emit `grade=pass` when
their degraded-motion evidence is observed, using `expected=<reason>` plus the
supporting motion and actuator fields.

## Typical Runs

```bash
./zlaunch.sh --case=baseline_transit_pass 10
./zlaunch.sh --case=depth_step_pass 10
./zlaunch.sh --jobs=2 --port_base=26000 10
./zlaunch.sh --just_make --keep_workdirs --jobs=2 --port_base=26000 10
```

The harness defaults to headless runs and requires Bash 5.1 or newer for its
rolling scheduler. It safely re-executes Homebrew or Linuxbrew Bash when the
system Bash is older.

Execution contract:

- every selected case, including serial and one-case runs, gets its own mission
  copy beneath a unique harness-owned `.harness_runs` root
- every case gets a stride-30 port block: MOOSDB at `case_base + 0..1` and
  pShare at `case_base + 15..16`
- the default base is the skill-standard `9000`; use `--port_base` for a fresh
  local or CI allocation
- `--jobs=<N>` is rolling, so the next pending case starts when any active case
  finishes
- the copied stem `zlaunch.sh` receives `--mmod`, `--max_time`, and all four
  explicit ports
- `pMissionEval` writes the authoritative grade; the harness preserves that row
  and synthesizes failures only for preparation, launch, result-schema,
  scheduler, or teardown errors
- cleanup uses the canonical root-scoped teardown helper; there is no global
  `ktm`, `pkill`, watchdog, or process-group extension

## Harness-skill migration validation

Validation on July 14, 2026 preserved all thirteen case names, all twenty-two
MOOS overlays, all three behavior overlays, the case-to-patch mapping, PID and
helm configuration, evaluator conditions, report columns, and expected-
degraded semantics. No application or grading variable was added. Seven
arrival-or-deadline evaluators now use the existing `TEST_EVAL_READY` signal as
a simple lead: `ARRIVED` sets it through a `pMissionEval` mailflag, while the
existing timer still sets it at the deadline.

Untouched legacy measurements at warp 10 and `--max_time=80` were:

- one shared-stem serial matrix: 13/13 passing in 122.79 seconds
- three two-worker batch-wave matrices: 39/39 passing in 82.91, 84.69, and
  81.80 seconds, for an 83.13-second mean

Migrated validation included both skill static checkers, Bash syntax,
ShellCheck, Bash 3.2 re-execution and rejection, invalid argument and port
probes, a retained full preparation matrix, a direct stem preparation, nominal
and live pass, nominal and expected-degraded harness cases, a complete serial
matrix, three complete
rolling matrices, and missing, duplicate, invalid-grade, launch, preparation,
timeout, and teardown failure paths. The retained generation matrix had 26
unique MOOSDB ports, 26 unique pShare ports, 26 MOOS targets, 13 behavior
targets, and exactly the intended 22 `.moosx` plus 3 `.bhvx` sidecars.

Complete migrated measurements were:

- isolated serial: 13/13 passing in 142.47 seconds
- rolling `--jobs=2`: 39/39 passing in 79.41, 81.55, and 84.10 seconds, for an
  81.69-second mean
- final checkout-state rolling matrix: 13/13 passing in 74.37 seconds from a
  port block verified clear on both TCP and UDP before and after the run

Serial is 19.68 seconds, or about 16 percent, slower than the one legacy sample
because serial now uses the same copied, scoped-cleanup path as parallel runs.
The intended two-worker mode is 1.45 seconds, or about 1.7 percent, faster on
the matched three-run mean. Every rolling completion refilled in the same
recorded second, result order remained exact, and every assigned port was clear
after each run.

The migration did not redesign case coverage. Retained legacy logs contained a
transient, retracted `No waypts given` startup advisory in all cases that spawn
the transit behavior. Two depth cases also emitted
`MissingDecVars:speed,course` only after their passing verdict, when a later
waypoint completed during the legacy shutdown tail. These are recorded for
later lifecycle or case-quality work and are not part of grading.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case.

`runtime_speed_factor_update_pass` replaces the vehicle ANTLER block to launch
its case-only `uTimerScript`. Minimal mode keeps that stimulus without
`pLogger`; a separate full-only case patch restores the original logged
topology.
