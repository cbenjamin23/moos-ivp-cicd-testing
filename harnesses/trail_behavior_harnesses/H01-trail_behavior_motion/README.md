# H01-trail_behavior_motion

Logging is minimal by default in both communities. Use `--log=full` for the
whole matrix or with `--case=NAME` for one diagnostic case.

Patch-driven matrix for
[`missions/trail_behavior_missions/trail_behavior_motion`](../../../missions/trail_behavior_missions/trail_behavior_motion).

This harness focuses on `BHV_Trail` as the behavior under test. The stem uses
two simulated vehicles: `abe` owns `BHV_Trail` and chases `ben`, while `ben`
owns a simple waypoint leg. Grading is mission-owned through `pMissionEval` and
uses behavior outputs such as:

- `TRAIL_DISTANCE`
- `TRAIL_RELEVANCE`
- `TRAIL_RANGE`
- `PURSUIT`
- `REGION`
- `BHV_WARNING`
- `BHV_ERROR`

## Cases

- `static_trail_pass` Configures a relative `180`-degree trail point `45` meters behind Ben with `radius=8` and `nm_radius=18`; passes at 45 seconds when `TRAIL_DISTANCE<=20`, `PURSUIT=1`, relevance is positive, and no `BHV_ERROR` occurred.
- `absolute_west_pass` Sets `trail_angle_type=absolute` and `trail_angle=270`, exercising a world-frame trail point west of Ben; passes with the default `TRAIL_DISTANCE<=20`, active-pursuit, positive-relevance, and no-error grade.
- `relative_port_pass` Sets relative `trail_angle=135` and `nm_radius=22`, exercising a port-quarter trail point; passes at 45 seconds when `TRAIL_DISTANCE<=30`, pursuit is active, relevance is positive, and no error occurred.
- `relative_starboard_pass` Sets relative `trail_angle=-135` and `nm_radius=22`, exercising a starboard-quarter trail point; passes at 45 seconds when `TRAIL_DISTANCE<=30`, pursuit is active, relevance is positive, and no error occurred.
- `lead_right_turn_pass` Replaces Ben's straight leg with points `(55,-80):(55,-35):(125,-35)`, exercising pursuit through a right-angle target turn; passes at 80 seconds when Ben has reached `x>=50,y>=-45`, `TRAIL_DISTANCE<=45`, pursuit and relevance are active, and no error occurred.
- `lead_port_turn_pass` Uses the same target dogleg with relative `trail_angle=135` and `nm_radius=22`, exercising port-quarter trailing through the turn; passes at 80 seconds with Ben beyond `x=50,y=-45`, `TRAIL_DISTANCE<=45`, active pursuit and relevance, and no error.
- `lead_turn_angle_update_pass` Uses the target dogleg and posts `trail_angle=135` at 12 seconds, exercising a live trail-point reframe before the turn; passes at 80 seconds with Ben beyond `x=50,y=-45`, `TRAIL_DISTANCE<=45`, active pursuit and relevance, and no error.
- `short_trail_range_pass` Sets `trail_range=28`, `radius=5`, and `nm_radius=14`, exercising a shorter desired separation; passes at 45 seconds when `TRAIL_DISTANCE<=18`, pursuit and relevance are active, and no error occurred.
- `long_trail_range_pass` Sets `trail_range=70`, `radius=10`, and `nm_radius=28`, exercising a longer desired separation; passes at 60 seconds when `TRAIL_DISTANCE<=40`, pursuit and relevance are active, and no error occurred.
- `tight_radius_pass` Sets `radius=3`, `nm_radius=14`, and `trail_range=35`, exercising tighter trail-point capture bands; passes at 45 seconds when `TRAIL_DISTANCE<=18`, pursuit and relevance are active, and no error occurred.
- `wide_radius_pass` Sets `radius=16` and `nm_radius=25`, exercising wider trail-point capture bands; passes at 45 seconds when `TRAIL_DISTANCE<=30`, pursuit and relevance are active, and no error occurred.
- `inside_nm_radius_pass` Sets `nm_radius=45`, placing the chaser inside the near-mode radius for the default geometry; passes at 45 seconds when `TRAIL_DISTANCE<=45`, pursuit and relevance are active, and no error occurred.
- `outside_nm_radius_pass` Sets both `radius` and `nm_radius` to zero, exercising continuous pursuit outside the near-mode radius; passes at 45 seconds when `TRAIL_DISTANCE>0`, pursuit and relevance are active, and no error occurred.
- `pwt_outer_active_pass` Sets `pwt_outer_dist=250` and `nm_radius=22`, exercising outer-distance relevance while the contact remains inside the boundary; passes when `TRAIL_RANGE<250`, `TRAIL_DISTANCE<=30`, pursuit and relevance are active, and no error occurred.
- `pwt_outer_inactive_pass` Sets `pwt_outer_dist=30`, below the observed contact range; passes when `TRAIL_RANGE>30`, `PURSUIT=0`, relevance is zero, and no error occurred.
- `mod_trail_range_plus_pass` Starts from `trail_range=45` and sets `mod_trail_range=15`, exercising an additive desired-range increase; passes at 45 seconds when `TRAIL_DISTANCE<=30`, pursuit and relevance are active, and no error occurred.
- `mod_trail_range_pct_pass` Starts from `trail_range=50` and sets `mod_trail_range_pct=50`, exercising percentage range reduction; passes at 45 seconds when `TRAIL_DISTANCE<=18`, pursuit and relevance are active, and no error occurred.
- `mod_trail_range_floor_pass` Starts from `trail_range=12` and applies `mod_trail_range=-50`, exercising the 10-meter minimum range clamp; passes at 45 seconds when `TRAIL_DISTANCE<=14`, pursuit and relevance are active, and no error occurred.
- `runtime_range_extend_pass` Posts `trail_range=70` through `TRAIL_UPDATES` at 12 seconds, exercising live desired-range extension; passes at 60 seconds when `TRAIL_DISTANCE<=40`, pursuit and relevance are active, and no error occurred.
- `runtime_mod_range_plus_pass` Posts `mod_trail_range=15` at 12 seconds, exercising a live additive range change; passes at 45 seconds when `TRAIL_DISTANCE<=30`, pursuit and relevance are active, and no error occurred.
- `runtime_mod_range_pct_pass` Posts `mod_trail_range_pct=50` at 12 seconds, exercising a live percentage range change; passes at 45 seconds when `TRAIL_DISTANCE<=18`, pursuit and relevance are active, and no error occurred.
- `runtime_mod_range_floor_pass` Posts `mod_trail_range_pct=10` at 12 seconds, exercising the 10-meter minimum clamp through a runtime update; passes at 60 seconds when `TRAIL_DISTANCE<=14`, pursuit and relevance are active, and no error occurred.
- `runtime_angle_update_pass` Posts `trail_angle=135` at 12 seconds, exercising a live change from astern to port-quarter trailing; passes at 60 seconds when `TRAIL_DISTANCE<=30`, pursuit and relevance are active, and no error occurred.
- `runtime_relevance_off_pass` Posts `pwt_outer_dist=5` at 12 seconds, exercising runtime relevance shutdown; passes at 35 seconds when `TRAIL_RANGE>5`, `PURSUIT=0`, relevance is zero, and no error occurred.
- `runtime_bad_update_recover_pass` Posts invalid `trail_range=-5` at 12 seconds and valid `trail_range=45` at 15 seconds, exercising continued pursuit after a rejected update; passes at 25 seconds when any warning was observed, `TRAIL_DISTANCE<=30`, pursuit and relevance are active, and no error occurred.
- `idle_post_distance_pass` Sets `post_trail_dist_on_idle=true` and turns `TRAIL=false` at 25 seconds, exercising distance publication while idle; passes at 35 seconds when `PURSUIT=0`, `TRAIL_DISTANCE<999`, and no error occurred.
- `idle_no_post_distance_pass` Sets `post_trail_dist_on_idle=false`, turns `TRAIL=false` at 25 seconds, and writes sentinel `TRAIL_DISTANCE=999` at 28 seconds; passes at 35 seconds when the sentinel remains `999`, `PURSUIT=0`, and no error occurred.
- `no_extrapolate_pass` Sets `extrapolate=false`, exercising pursuit from the latest contact report without projection; passes at 60 seconds when `TRAIL_DISTANCE<=30`, pursuit and relevance are active, and no error occurred.
- `no_alert_request_pass` Sets `no_alert_request=true`, exercising suppression of Trail's contact-manager alert request; passes with the default `TRAIL_DISTANCE<=20`, active-pursuit, positive-relevance, and no-error grade.
- `auto_alert_request_pass` Uses a spawn-templated Trail with `no_alert_request=false`, exercising automatic contact-manager subscription; passes at 60 seconds when any `BCM_ALERT_REQUEST` was observed, `TRAIL_DISTANCE<=30`, pursuit and relevance are active, and no error occurred.
- `missing_contact_warn_pass` Sets `contact=ghost` with `on_no_contact_ok=true`, exercising warning-only handling of an unavailable contact; passes at eight seconds when any `BHV_WARNING` was observed and no `BHV_ERROR` occurred.
- `missing_contact_fail` Sets `contact=ghost` with `on_no_contact_ok=false`, exercising the required-contact error path; the harness passes as soon as any `BHV_ERROR` publication is observed.
- `missing_contact_param_fail` Omits `contact` while setting `on_no_contact_ok=false`, exercising the missing required parameter path; the harness passes as soon as any `BHV_ERROR` publication is observed.
- `bad_trail_angle_type_fail` Sets `trail_angle_type=diagonal`, exercising invalid angle-type rejection; the harness passes at 20 seconds when Trail remains inactive (`TRAIL_DISTANCE>=900`, zero relevance, `PURSUIT=-1`, `REGION=UNKNOWN`) and no error occurred.
- `bad_trail_angle_fail` Sets `trail_angle=west`, exercising nonnumeric angle rejection; the harness passes at 20 seconds when the same inactive sentinel outputs remain and no error occurred.
- `bad_trail_range_fail` Sets `trail_range=-1`, exercising negative range rejection; the harness passes at 20 seconds when the inactive sentinel outputs remain and no error occurred.
- `bad_mod_trail_range_pct_fail` Sets `mod_trail_range_pct=0`, exercising rejection of a nonpositive percentage modifier; the harness passes at 20 seconds when the inactive sentinel outputs remain and no error occurred.
- `bad_radius_fail` Sets `radius=-1`, exercising negative capture-radius rejection; the harness passes at 20 seconds when the inactive sentinel outputs remain and no error occurred.
- `bad_nm_radius_fail` Sets `nm_radius=-1`, exercising negative near-mode-radius rejection; the harness passes at 20 seconds when the inactive sentinel outputs remain and no error occurred.
- `bad_pwt_outer_dist_fail` Sets `pwt_outer_dist=-1`, exercising negative relevance-distance rejection; the harness passes at 20 seconds when the inactive sentinel outputs remain and no error occurred.
- `bad_decay_fail` Sets `decay=60,30`, exercising rejection of a decay start greater than its end; the harness passes at 20 seconds when the inactive sentinel outputs remain and no error occurred.
- `bad_time_on_leg_fail` Sets inherited `time_on_leg=-1`, exercising rejection of a negative leg duration; the harness passes at 20 seconds when the inactive sentinel outputs remain and no error occurred.

## Running

From this harness directory:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=static_trail_pass 10
./zlaunch.sh --case=pwt_outer_inactive_pass --gui 1
./zlaunch.sh --case=outside_nm_radius_pass --gui 1
./zlaunch.sh --case=lead_right_turn_pass --gui 1
./zlaunch.sh --case=lead_port_turn_pass --gui 1
./zlaunch.sh --case=lead_turn_angle_update_pass --gui 1
./zlaunch.sh --case=runtime_bad_update_recover_pass --gui 1
./zlaunch.sh --case=idle_post_distance_pass --gui 1
./zlaunch.sh --jobs=4 --port_base=15000 10
```

The paired mission launcher remains a single-scenario launcher. It accepts the
mission modifier and all six MOOSDB/pShare ports directly:

```bash
./zlaunch.sh --mmod=static_trail_pass --shore_mport=15000 \
  --veh1_mport=15001 --veh2_mport=15002 --shore_pshare=15010 \
  --veh1_pshare=15011 --veh2_pshare=15012 10
```

The full matrix currently has 42 cases. Serial and rolling modes both use one
isolated mission copy and one deterministic two-vehicle port block per case.
The harness requires Bash 5.1 or newer for rolling scheduling and prevents a
second invocation from starting while one is active. Do not overlap it with
other MOOS harnesses on the same machine.

## Migration validation

The skill 1.4.3 migration preserved all 42 case mappings, patches, evaluator
conditions, event times, behavior values, grading variables, and coverage
claims. Three clean `--jobs=4` matrices passed 126/126 rows in 132.18, 131.90,
and 131.63 seconds. The isolated serial matrix passed 42/42 in 498.45 seconds.
The untouched legacy rolling mean was 159.71 seconds and its serial matrix
took 446.54 seconds.

One additional migrated rolling attempt had a single `bad_decay_fail` mission
exit before writing a pMissionEval row. The case passed immediately with its
workdir retained, then passed five of five focused repetitions, the retained
full matrix, and the final full matrix without any test change. This is
recorded as an isolated launch/lifecycle outlier rather than converted into a
passing result.

Validation also covered all-case generation, nominal, warning-only, and
expected-inactive verdicts, standalone generation and live execution, exact
case order, rolling refill, 126 unique MOOSDB ports and the paired pShare
ports, intended sidecars, unknown-case rejection, active-lock behavior, and
Bash 3.2 rejection. Disposable fault injection verified normalized
`missing_result`, `duplicate_results`, and `prepare_error` rows. Bash syntax,
ShellCheck, and both skill static checkers pass. No tested MOOS process
survived cleanup.
