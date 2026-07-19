# H01-convoy_behavior_motion

Patch-driven matrix for
[`missions/convoy_behavior_missions/convoy_behavior_motion`](../../../missions/convoy_behavior_missions/convoy_behavior_motion).

This harness focuses on `BHV_Convoy` as the behavior under test. The stem uses
two simulated vehicles: `abe` owns `BHV_Convoy` and chases `ben`, while `ben`
owns a simple waypoint leg. Grading is mission-owned through `pMissionEval` and
uses behavior outputs plus chaser/target speed aliases:

- `QLEN`
- `TLEN`
- `MXRNG`
- `AVG2`
- `AVG5`
- `WPTX`
- `WPTY`
- `ABE_NAV_SPEED`
- `BEN_NAV_SPEED`
- `ABE_NAV_X`
- `ABE_NAV_Y`
- `BEN_NAV_X`
- `BEN_NAV_Y`
- `BHV_WARNING`
- `BHV_ERROR`

## Cases

- `static_convoy_pass`: Uses the stock five-meter breadcrumb spacing and 80-meter queue cap while Abe follows eastbound Ben; passes at 45 seconds when `QLEN>=2`, `10<=TLEN<=95`, `MXRNG=80`, `AVG2>=1`, `AVG5>=1`, Abe's speed is at least 0.4, Ben's is at least 0.9, and no behavior error was observed.
- `fine_mark_spacing_pass`: Sets `inter_mark_range=3` to retain more breadcrumbs; passes when `QLEN>=12`, `30<=TLEN<=95`, `MXRNG=80`, `AVG2>=1`, and no behavior error was observed.
- `coarse_mark_spacing_pass`: Sets `inter_mark_range=12` to retain fewer breadcrumbs; passes at 40 seconds when `2<=QLEN<=6`, `20<=TLEN<=95`, `MXRNG=80`, `AVG2>=1`, and no behavior error was observed.
- `short_mark_queue_pass`: Sets `max_mark_range=30` to trim the convoy trail; passes at 25 seconds when `QLEN>=2`, `TLEN<=38`, `MXRNG=30`, `AVG2>=1`, and no behavior error was observed.
- `long_mark_queue_pass`: Sets `max_mark_range=140` to permit a longer trail; passes when `QLEN>=6`, `TLEN>=35`, `MXRNG=140`, `AVG2>=1`, and no behavior error was observed.
- `tight_radius_pass`: Shrinks the breadcrumb capture `radius` from 8 to 3; passes under the stock 45-second queue, trail-length, reported-cap, average-speed, vehicle-speed, and no-error bounds.
- `wide_radius_pass`: Expands the breadcrumb capture `radius` from 8 to 16; passes under the same stock 45-second queue, trail-length, reported-cap, average-speed, vehicle-speed, and no-error bounds.
- `cruise_speed_pass`: Configures `cruise_speed=1.2` with `spd_max=4`; passes at 35 seconds when `QLEN>=2`, `TLEN>=20`, `MXRNG=80`, `AVG2>=1`, Abe's speed is at most 1.25, Ben's is at least 0.9, and no behavior error was observed.
- `cruise_speed_cap_warn_pass`: Configures `cruise_speed=5` above `spd_max=2`; passes at 20 seconds when `QLEN>=2`, `TLEN>=10`, `AVG2>=1`, at least one `BHV_WARNING` was observed, and no behavior error was observed.
- `safety_range_autoadjust_warn_pass`: Configures the compressed safety ranges `rng_estop=2`, `rng_tgating=5`, and `rng_lagging=6` with safety enabled; passes at 20 seconds when `QLEN>=2`, `TLEN>=10`, `AVG2>=1`, at least one behavior warning was observed, and no behavior error was observed.
- `safety_off_bad_ranges_pass`: Uses the same `2,5,6` ranges with `rng_safety=false`; passes under the stock 45-second queue, trail-length, reported-cap, average-speed, vehicle-speed, and no-error bounds.
- `tailgating_speed_slow_pass`: Sets `rng_estop=5`, `rng_tgating=200`, `rng_lagging=220`, and `spd_slower=0.5` so Abe remains in the tailgating speed regime; passes when `QLEN>=2`, `TLEN>=10`, Abe's speed is at most 0.8, Ben's is at least 0.9, and no behavior error was observed.
- `lagging_speed_fast_pass`: Sets the safety ranges to `5,10,20` and `spd_faster=2` so Abe remains in the lagging regime; passes when `QLEN>=4`, `TLEN>=20`, `MXRNG=80`, `AVG2>=1`, Abe's speed is at least 1.4, Ben's is at least 0.9, and no behavior error was observed.
- `estop_speed_zero_pass`: Raises the safety ranges to `rng_estop=200`, `rng_tgating=210`, and `rng_lagging=220` so Abe remains inside the emergency-stop range; passes when `QLEN>=2`, `TLEN>=10`, Abe's speed is at most 0.25, Ben's is at least 0.9, and no behavior error was observed.
- `range_aliases_pass`: Replaces the stock range keys with the legacy aliases `range_estop=15`, `range_tailgating=25`, and `range_lagging=45`; passes under the stock 45-second queue, trail-length, reported-cap, average-speed, vehicle-speed, and no-error bounds.
- `nm_radius_zero_pass`: Sets `nm_radius=0` while leaving breadcrumb `radius=8`; passes under the stock 45-second queue, trail-length, reported-cap, average-speed, vehicle-speed, and no-error bounds.
- `view_point_post_pass`: Uses the stock convoy configuration and watches `VIEW_POINT`; passes when at least one view point was posted, `QLEN>=2`, `TLEN>=10`, and no behavior error was observed.
- `angled_entry_pass`: Starts Abe at `(-85,-125)` heading 40 degrees toward Ben's eastbound track; passes at 65 seconds when `QLEN>=2`, `10<=TLEN<=95`, `MXRNG=80`, `AVG2>=1`, both vehicles are moving, Abe has converged to `-102<=Y<=-58`, and no behavior error was observed.
- `cross_track_entry_pass`: Starts Abe at `(-35,-145)` heading north across Ben's eastbound track; passes at 65 seconds under the same queue, trail, speed, `-102<=ABE_NAV_Y<=-58`, and no-error convergence bounds.
- `opposite_heading_recover_pass`: Starts Abe behind Ben at `(-55,-80)` but heading west; passes at 65 seconds under the same queue, trail, speed, `-102<=ABE_NAV_Y<=-58`, and no-error convergence bounds.
- `close_offset_tailgate_pass`: Starts Abe close to Ben's track at `(3,-60)` heading south; passes at 65 seconds under the same queue, trail, speed, `-102<=ABE_NAV_Y<=-58`, and no-error convergence bounds.
- `lead_right_turn_pass`: Replaces Ben's straight leg with `55,-80:55,-35:125,-35`; passes at 85 seconds when Ben has reached `X>=50,Y>=-45`, `QLEN>=2`, `10<=TLEN<=130`, `MXRNG=80`, `AVG2>=0.6`, Abe's speed is at least 0.4, Ben's is at least 0.8, and no behavior error was observed.
- `lead_s_turn_pass`: Starts Abe at `(-20,-80)`, changes the convoy trail to `radius=3.5`, `inter_mark_range=1.5`, and `max_mark_range=40`, and sends Ben through `25,-80:55,-50:70,-50:70,-90:105,-90`; passes at 145 seconds when `QLEN>=2`, `10<=TLEN<=55`, `MXRNG=40`, `AVG2>=0.6`, Abe is moving within `55<=X<=68,-58<=Y<=-48`, Ben is at `X>=72,Y<=-90`, the active breadcrumb is at `X>=70,Y>=-68`, and no behavior error was observed.
- `short_queue_turn_pass`: Sets `max_mark_range=30` while Ben follows `40,-80:40,-40:105,-40`; passes at 85 seconds when `2<=QLEN<=6`, `10<=TLEN<=55`, `MXRNG=30`, `AVG2>=0.6`, both vehicles are moving, Ben has reached `X>=35,Y>=-50`, and no behavior error was observed.
- `slow_follower_pass`: Sets `spd_slower=0.5`, `spd_faster=1`, and `max_mark_range=100`, exercising conservative speed multipliers; passes when `QLEN>=6`, `TLEN>=35`, `MXRNG=100`, `AVG2>=1`, and no behavior error was observed.
- `fast_follower_pass`: Sets `spd_slower=1` and `spd_faster=2`; passes when `QLEN>=4`, `TLEN>=20`, `MXRNG=80`, `AVG2>=1`, Abe's speed is at least 1.4, Ben's is at least 0.9, and no behavior error was observed.
- `runtime_inter_mark_coarse_pass`: Posts `CONVOY_UPDATES=inter_mark_range=12` at 12 seconds; passes at 40 seconds when `2<=QLEN<=6`, `20<=TLEN<=95`, `MXRNG=80`, `AVG2>=1`, and no behavior error was observed.
- `runtime_max_mark_short_pass`: Posts `CONVOY_UPDATES=max_mark_range=30` at 12 seconds; passes at 25 seconds when `QLEN>=2`, `TLEN<=38`, `MXRNG=30`, `AVG2>=1`, and no behavior error was observed.
- `runtime_cruise_speed_pass`: Posts `CONVOY_UPDATES=cruise_speed=1.2` at 12 seconds; passes at 35 seconds when `QLEN>=2`, `TLEN>=20`, `MXRNG=80`, `AVG2>=1`, Abe's speed is at most 1.25, Ben's is at least 0.9, and no behavior error was observed.
- `runtime_cruise_cap_warn_pass`: Posts `cruise_speed=5` at 12 seconds and `spd_max=2` at 13 seconds through `CONVOY_UPDATES`; passes at 20 seconds when `QLEN>=2`, `TLEN>=10`, `AVG2>=1`, at least one behavior warning was observed, and no behavior error was observed.
- `runtime_estop_speed_zero_pass`: Posts `rng_estop=200`, `rng_tgating=210`, and `rng_lagging=220` at 12, 13, and 14 seconds; passes when `QLEN>=2`, `TLEN>=10`, Abe's speed is at most 0.25, Ben's is at least 0.9, and no behavior error was observed.
- `runtime_bad_update_recover_pass`: Posts invalid `inter_mark_range=-1` at 12 seconds and restores `inter_mark_range=5` at 15 seconds; passes under the stock 45-second queue, trail-length, reported-cap, average-speed, vehicle-speed, and final no-error bounds.
- `no_extrapolate_pass`: Sets `extrapolate=false` while Ben continues publishing fresh motion reports; passes under the stock 45-second queue, trail-length, reported-cap, average-speed, vehicle-speed, and no-error bounds.
- `missing_contact_warn_pass`: Sets `contact=ghost` with `on_no_contact_ok=true`; passes at eight seconds when `QLEN=0`, `TLEN=0`, a behavior warning was observed, and no behavior error was observed.
- `missing_contact_fail`: Sets `contact=ghost` with `on_no_contact_ok=false`; this expected-negative case passes at the 20-second checkpoint when either helm malconfiguration or any `BHV_ERROR` was observed.
- `missing_contact_param_fail`: Omits `contact` and sets `on_no_contact_ok=false`; this expected-negative case passes at the 20-second checkpoint when either helm malconfiguration or any behavior error was observed.
- `bad_inter_mark_range_fail`: Sets `inter_mark_range=-1`; this expected-negative case passes at 20 seconds when either helm malconfiguration or any behavior error was observed.
- `bad_max_mark_range_fail`: Sets `max_mark_range=-1`; this expected-negative case passes at 20 seconds when either helm malconfiguration or any behavior error was observed.
- `bad_radius_fail`: Sets `radius=-1`; this expected-negative case passes at 20 seconds when either helm malconfiguration or any behavior error was observed.
- `bad_nm_radius_fail`: Sets `nm_radius=-1`; this expected-negative case passes at 20 seconds when either helm malconfiguration or any behavior error was observed.
- `bad_spd_slower_fail`: Sets `spd_slower=1.5`, above the accepted multiplier range; this expected-negative case passes at 20 seconds when either helm malconfiguration or any behavior error was observed.
- `bad_spd_faster_fail`: Sets `spd_faster=0.5`, below the accepted multiplier range; this expected-negative case passes at 20 seconds when either helm malconfiguration or any behavior error was observed.
- `bad_spd_max_fail`: Sets `spd_max=0`; this expected-negative case passes at 20 seconds when either helm malconfiguration or any behavior error was observed.
- `bad_rng_estop_fail`: Sets `rng_estop=-1`; this expected-negative case passes at 20 seconds when either helm malconfiguration or any behavior error was observed.
- `bad_rng_tgating_fail`: Sets `rng_tgating=-1`; this expected-negative case passes at 20 seconds when either helm malconfiguration or any behavior error was observed.
- `bad_rng_lagging_fail`: Sets `rng_lagging=-1`; this expected-negative case passes at 20 seconds when either helm malconfiguration or any behavior error was observed.
- `bad_rng_safety_fail`: Sets the boolean `rng_safety=maybe`; this expected-negative case passes at 20 seconds when either helm malconfiguration or any behavior error was observed.
- `bad_cruise_speed_fail`: Sets `cruise_speed=-1`; this expected-negative case passes at 20 seconds when either helm malconfiguration or any behavior error was observed.

## Running

From this harness directory:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=static_convoy_pass 10
./zlaunch.sh --case=fine_mark_spacing_pass --gui 1
./zlaunch.sh --case=angled_entry_pass --gui 1
./zlaunch.sh --case=cross_track_entry_pass --gui 1
./zlaunch.sh --case=lead_right_turn_pass --gui 1
./zlaunch.sh --case=lead_s_turn_pass --gui 1
./zlaunch.sh --case=short_queue_turn_pass --gui 1
./zlaunch.sh --case=fast_follower_pass --gui 1
./zlaunch.sh --case=short_mark_queue_pass --gui 1
./zlaunch.sh --jobs=4 --port_base=26000 10
```

The paired mission launcher remains a single-scenario launcher. It accepts the
mission modifier, custom chaser start, and all six MOOSDB/pShare ports directly:

```bash
./zlaunch.sh --mmod=static_convoy_pass --vpos1=x=-20,y=-80,heading=90 \
  --shore_mport=26000 --veh1_mport=26001 --veh2_mport=26002 \
  --shore_pshare=26010 --veh1_pshare=26011 --veh2_pshare=26012 10
```

The geometry-entry cases are evaluated at 65 seconds and require `QLEN >= 2`,
`TLEN` between 10 and 95 meters, `MXRNG = 80`, `AVG2 >= 1`, a moving target
and chaser, chaser `Y` between -102 and -58, and no behavior error.

The full matrix currently has 48 cases. Serial and rolling modes both use one
isolated mission copy and one deterministic two-vehicle port block per case:
`case_base = port_base + case_idx*PORT_STRIDE`, with MOOSDB ports at
`case_base + 0..2` and pShare ports at `case_base + 10..12`. The harness
requires Bash 5.1 or newer and prevents a second invocation from starting
while one is active. Do not overlap it with other MOOS harnesses on the same
machine.

Latest validation:

- July 16, 2026
- generated-file matrix: `48/48` isolated cases completed
- migrated rolling matrices: `144/144` rows passed in 150.14, 157.45, and
  148.09 seconds
- migrated isolated serial matrix: `48/48` rows passed in 564.49 seconds
- untouched legacy rolling mean: 186.59 seconds; legacy serial: 552.52 seconds

The migration preserved every case mapping, patch, evaluator condition, event
time, behavior value, grading variable, custom start position, and the
`lead_s_turn_pass` 170-second case ceiling. One of four untouched legacy full
matrices narrowly failed that S-turn case with `ABE_NAV_SPEED=0.34` against its
existing 0.35 condition. The unchanged case then passed 5/5 focused legacy
repetitions, its migrated focused run, all three migrated rolling matrices,
and migrated serial. The outlier is recorded rather than hidden or fixed by
weakening the test.

Validation also covered standalone generation and live execution, exact case
order, rolling refill, 144 unique MOOSDB ports and 144 non-overlapping pShare
ports, intended sidecars, unknown-case rejection, and
Bash 3.2 rejection. Disposable fault injection verified normalized
`missing_result`, `duplicate_results`, and `prepare_error` rows. Bash syntax,
ShellCheck, and both skill static checkers pass. No tested MOOS process
survived cleanup.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for a fully logged diagnostic case. Full mode
restores one shoreside and both vehicle loggers before case overlays.
