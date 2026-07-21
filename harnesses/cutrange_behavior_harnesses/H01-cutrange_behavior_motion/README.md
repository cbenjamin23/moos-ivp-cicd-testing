# H01-cutrange_behavior_motion

Patch-driven matrix for
[`missions/cutrange_behavior_missions/cutrange_behavior_motion`](../../../missions/cutrange_behavior_missions/cutrange_behavior_motion).

This harness focuses on `BHV_CutRange` as the behavior under test. The stem uses
two simulated vehicles: `abe` owns `BHV_CutRange` and tries to reduce range to
`ben`, while `ben` owns simple waypoint or station-keep behaviors that create
controlled contact geometry. Grading is mission-owned through `pMissionEval` and
uses `pursueflag`/`giveupflag`, `uFldCollisionDetect` closest-range telemetry,
bridged NAV speed from both vehicles, and behavior warning/error mail.

## Cases

Deterministic `AOF_CutRangeCPA` and `BHV_CutRange` CTests separately grade
time-horizon utility, patience blending and clamping, runtime patience updates,
relevance weighting, and the `giveup_dist` alias. Their former broad closure
missions were removed; this matrix retains scenarios whose contract depends on
live pursuit, giveup, runtime relevance, or target motion.

- `settled_no_warning_pass`: Uses the stock configuration, clears accumulated startup warnings at 42 seconds, and observes the following three-second settled window; passes when pursuit has started and no new warning or behavior error occurs in that window.
- `static_cutrange_pass`: Starts Abe and Ben 90 meters apart on the same eastbound line with stock `pwt_inner_dist=15`, `pwt_outer_dist=70`, and `giveup_range=160`; passes at 65 seconds when pursuit is true, giveup is false, closest range is at most 50 meters, Ben's speed is at least 0.6, and no warning or behavior error was observed after the 55-second warning reset.
- `crossing_cutrange_pass`: Starts Ben at `(20,-140)` heading north toward `(20,60)` while Abe begins at `(-70,-80)` heading east; passes at 70 seconds when pursuit remains true without giveup, closest range is at most 60 meters, Ben is moving at least 0.6, and no settled warning or behavior error was observed.
- `headon_cutrange_pass`: Starts Ben at `(90,-80)` heading west at a configured 1.4 m/s toward `(-170,-80)`; passes at 60 seconds when pursuit is true, closest range is at most 45 meters, both vehicles are moving at least 0.8, and no settled warning or behavior error was observed.
- `angled_entry_pass`: Starts Abe at `(-95,-125)` heading 45 degrees and Ben at `(10,-80)` heading 80 degrees toward `(180,-40)`; passes at 70 seconds when pursuit remains true without giveup, closest range is at most 60 meters, Ben is moving at least 0.6, and no settled warning or behavior error was observed.
- `right_turn_target_pass`: Sends Ben through `115,-80:115,-10`; evaluation waits until Ben is within `105<=X<=125`, `Y>=-45`, and heading at most 45 degrees, then passes when pursuit is true without giveup, current range is at most 70 meters, Ben is moving at least 0.6, and no settled warning or behavior error was observed.
- `s_turn_target_pass`: Sends Ben through `55,-80:85,-45:115,-105:150,-80`; evaluation waits until Ben is at `X>=100`, `Y<=-88`, and heading 115 through 205 degrees, then passes when pursuit is true without giveup, current range is at most 75 meters, Ben is moving at least 0.6, and no settled warning or behavior error was observed.
- `slow_target_pass`: Sets Ben's waypoint speed to 0.5 m/s toward `(120,-80)`; passes at 65 seconds when pursuit is true, closest range is at most 35 meters, Abe is moving at least 0.8, Ben is moving at most 0.9, and no settled warning or behavior error was observed.
- `fast_target_pass`: Sets Ben's waypoint speed to 2.1 m/s toward `(220,-80)`; passes at 75 seconds when pursuit is true, closest range is at most 80 meters, Abe is moving at least 0.8, Ben is moving at least 1.7, and no settled warning or behavior error was observed.
- `stationary_target_pass`: Sets Ben's waypoint speed to 0.1 m/s toward `(45,-80)`; passes at 50 seconds when pursuit is true, closest range is at most 45 meters, Abe is moving at least 0.8, Ben is moving at most 0.35, and no settled warning or behavior error was observed.
- `high_patience_pass`: Sets `patience=85` and `max_patience=95`; passes at 65 seconds when pursuit is true without giveup, closest range is at most 50 meters, Ben is moving at least 0.6 m/s, and no settled warning or behavior error was observed.
- `pwt_inner_zero_relevance_pass`: Sets `pwt_inner_dist=105` and `pwt_outer_dist=120`, placing the initial 90-meter contact inside the zero-relevance region; passes at 50 seconds when pursuit is latched true but Abe's speed is at most 0.35, Ben is moving at least 0.6, closest range remains at least 70 meters, and no settled warning or behavior error was observed.
- `pwt_equal_zero_relevance_pass`: Sets both relevance distances to 90, exercising the zero-width relevance interval; passes under the same stopped-Abe, moving-Ben, minimum-range-at-least-70, and settled warning/error criteria.
- `giveup_start_far_pass`: Sets `giveup_range=120` and starts Ben at `(125,-80)`, 195 meters from Abe; passes at 45 seconds when pursuit and giveup are both false, Abe's speed is at most 0.35, closest range remains at least 150 meters, and no settled warning or behavior error was observed.
- `giveup_hysteresis_pass`: Sets `giveup_range=89.5` while stationary Ben begins 90 meters from Abe; passes at 30.5 seconds when pursuit is true, giveup is false, current range is 88 through 91 meters, and no settled warning or behavior error was observed.
- `runtime_pwt_on_pass`: Starts with `pwt_inner_dist=105,pwt_outer_dist=120`, enables CutRange at 30 seconds, then posts `pwt_inner_dist=15` and `pwt_outer_dist=70` at 37 and 38 seconds; passes at 80 seconds when pursuit is true without giveup, closest range is at most 65 meters, Abe is moving at least 0.6, and no settled warning or behavior error was observed.
- `runtime_pwt_off_pass`: Starts with the stock active relevance band, then posts `pwt_outer_dist=120` and `pwt_inner_dist=105` at 34 and 35 seconds; passes at 50 seconds when pursuit remains latched true but Abe is moving at most 0.35, Ben is moving at least 0.6, closest range remains at least 70 meters, and no settled warning or behavior error was observed.
- `runtime_giveup_update_pass`: Enables pursuit at 30 seconds and posts `giveup_range=20` at 34 seconds; passes at 55 seconds when both the earlier pursuit flag and the giveup flag are true, Abe's speed is at most 0.6, and no settled warning or behavior error was observed.
- `runtime_bad_update_recover_warn_pass`: Starts relevance off, posts invalid `pwt_outer_dist=-5` at 32 seconds, then restores `pwt_inner_dist=15,pwt_outer_dist=70` at 42 and 43 seconds; passes at 85 seconds when pursuit is true without giveup, current range is at most 90 meters, closest range is at most 65, Abe is moving at least 0.6, some behavior warning was observed, and no behavior error was observed.
- `missing_contact_warn_pass`: Sets `contact=ghost` with `on_no_contact_ok=true`; passes at 20 seconds when some behavior warning and no behavior error were observed.
- `missing_contact_fail`: Sets `contact=ghost` with `on_no_contact_ok=false`; after clearing startup errors at 35 seconds, this expected-negative case passes when a new `BHV_ERROR` arrives while Abe's helm remains in `DRIVE` and the 45-second deadline is still false.
- `missing_contact_param_fail`: Omits `contact` with `on_no_contact_ok=false`; after the same post-activation error reset, this expected-negative case passes when a new `BHV_ERROR` arrives in `DRIVE` before the deadline.
- `bad_pwt_outer_dist_fail`: Sets `pwt_outer_dist=-1`; this expected-negative case passes only when Abe's helm reaches `MALCONFIG` before the fallback deadline.
- `pwt_inner_normalizes_outer_pass`: Sets `pwt_outer_dist=70` followed by `pwt_inner_dist=90`, exercising the shared setter that raises the companion outer value to 90; over the bounded observation window, passes when pursuit is latched, Abe remains at or below 0.35 m/s, Ben moves at least 0.6 m/s, closest range stays at least 70 meters, and no settled warning or behavior error occurs.
- `bad_giveup_range_fail`: Sets `giveup_range=-1`; this expected-negative case passes only when Abe's helm reaches `MALCONFIG` before the fallback deadline.
- `bad_patience_fail`: Sets `patience=-1`; this expected-negative case passes only when Abe's helm reaches `MALCONFIG` before the fallback deadline.
- `bad_max_patience_fail`: Sets `max_patience=101`, above the accepted inclusive 0–100 range; this expected-negative case passes only when Abe's helm reaches `MALCONFIG` before the fallback deadline.
- `bad_time_on_leg_fail`: Sets `time_on_leg=-1`; this expected-negative case passes only when Abe's helm reaches `MALCONFIG` before the fallback deadline.
- `bad_decay_fail`: Sets `decay=nope`; this expected-negative case passes only when Abe's helm reaches `MALCONFIG` before the fallback deadline.
- `bad_legacy_util_param_fail`: Adds unsupported top-level `min_util_cpa_dist=10`; this expected-negative case passes only when Abe's helm reaches `MALCONFIG` before the fallback deadline.

## Running

From this harness directory:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=static_cutrange_pass 10
./zlaunch.sh --case=headon_cutrange_pass --gui 1
./zlaunch.sh --case=right_turn_target_pass --gui 1
./zlaunch.sh --case=s_turn_target_pass --gui 1
./zlaunch.sh --case=runtime_giveup_update_pass --gui 1
./zlaunch.sh --jobs=4 --port_base=25000 10
```

The paired mission launcher remains a single-scenario launcher. It accepts the
mission modifier, both vehicle-start overrides, and all six MOOSDB/pShare ports
directly:

```bash
./zlaunch.sh --mmod=angled_entry_pass \
  --vpos1=x=-95,y=-125,heading=45 --vpos2=x=10,y=-80,heading=80 \
  --shore_mport=25000 --veh1_mport=25001 --veh2_mport=25002 \
  --shore_pshare=25010 --veh1_pshare=25011 --veh2_pshare=25012 10
```

The full matrix currently has 38 cases. Normal expected-pass cases clear the
known first-active contact warm-up warning before evaluation, then require no
settled chaser-side behavior warnings or errors; explicitly named `*_warn_pass`
cases require the expected warning and `BHV_ERROR_SEEN=false`. Serial and
rolling modes both use one isolated mission copy and one deterministic
two-vehicle port block per case. Each case uses `case_base + 0..2` for MOOSDB
and `case_base + 10..12` for pShare inside a 30-port stride. The harness
requires Bash 5.1 or newer and prevents a second invocation while one is
active; keep `--port_base` separated from any other live harness.

The dogleg and S-turn geometry cases use evaluator gates on `BEN_NAV_X`,
`BEN_NAV_Y`, and `BEN_NAV_HEADING`, then grade current range at that moment.
That keeps them from passing solely because the vehicles happened to get close
before the target reached the turn geometry under test.

Latest validation:

- July 19, 2026 contract-strengthening sweep: `38/38` cases passed with
  minimal logging at `--jobs=4` and `38/38` passed serially with full logging

- July 16, 2026
- generated-file matrix: `39/39` isolated cases completed
- migrated rolling matrices: `117/117` rows passed in 141.66, 138.08, and
  140.40 seconds
- migrated isolated serial matrix: `39/39` rows passed in 519.74 seconds
- untouched legacy rolling mean: 169.91 seconds; legacy serial: 500.78 seconds

The migration preserved all 39 unique case mappings, patches, evaluator
conditions, event times, behavior values, grading variables, and four custom
vehicle-start configurations. Strict result handling exposed that the existing
120-second ceiling ended `right_turn_target_pass` and `s_turn_target_pass`
before their geometry lead conditions became true. The legacy launcher had
continued the missions after that nominal timeout while waiting for a late
row. The existing harness and mission `MAX_TIME` defaults are now 180 seconds;
no case condition, event, position, stimulus, or behavior value changed, and
ordinary cases still finish as soon as pMissionEval grades them.

Validation also covered nominal, two-position geometry, turn geometry, and
expected-inactive verdicts, standalone generation and live execution, exact
case order, rolling refill, 117 unique MOOSDB ports and 117 non-overlapping
pShare ports, intended sidecars, unknown-case rejection,
and Bash 3.2 rejection. Disposable fault injection verified normalized
`missing_result`, `duplicate_results`, and `prepare_error` rows. Bash syntax,
ShellCheck, and both skill static checkers pass. No tested MOOS process
survived cleanup.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for a fully logged diagnostic case. Full mode
restores one shoreside and both vehicle loggers before case overlays.
