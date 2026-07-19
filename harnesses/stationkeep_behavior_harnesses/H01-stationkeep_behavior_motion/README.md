# H01-stationkeep_behavior_motion

Logging is minimal by default in both communities. Use `--log=full` for the
whole matrix or with `--case=NAME` for one diagnostic case.

Patch-driven matrix for
[`missions/stationkeep_behavior_missions/stationkeep_behavior_motion`](../../../missions/stationkeep_behavior_missions/stationkeep_behavior_motion).

This harness focuses on `BHV_StationKeep` as the behavior under test. The stem
mission keeps one simulated vehicle near a compact station point and grades on
behavior-owned outputs:

- `DIST_TO_STATION`
- `PSKEEP_MODE`
- `BHV_SETTINGS`
- `BHV_WARNING`
- `BHV_ERROR`

## Cases

- `static_station_pass` Configures station point `(12,-121)`, inner radius `4`, outer radius `15`, outer speed `1.2`, and the stock transit speed; passes at 35 seconds when `DIST_TO_STATION<=8` and no behavior warning or error occurred.
- `station_pt_alias_pass` Supplies `(12,-121)` through the `station_pt` alias instead of `point`, exercising alias parsing; passes at 35 seconds when `DIST_TO_STATION<=8` and no behavior warning or error occurred.
- `start_inside_hold_pass` Places the station at the vehicle's initial position `(-45,-121)`, exercising immediate hold without a transit; passes at 35 seconds when `DIST_TO_STATION<=8` and no behavior warning or error occurred.
- `center_activate_hold_pass` Omits a fixed point and sets `center_activate=true`, exercising creation of the station at activation; passes at 35 seconds when `DIST_TO_STATION<=8` and no behavior warning or error occurred.
- `center_activate_swing_pass` Sets `center_activate=true` and `swing_time=8`, exercising the activation-centered swing path; passes at 35 seconds when `DIST_TO_STATION<=8` and no behavior warning or error occurred.
- `wide_radius_pass` Sets `inner_radius=10` and `outer_radius=28`, exercising widened station rings; passes at 35 seconds when `DIST_TO_STATION<=10` and no `BHV_ERROR` occurred.
- `tight_radius_pass` Sets `inner_radius=2`, `outer_radius=8`, and `outer_speed=1.4`, exercising tighter station rings; passes at 35 seconds when `DIST_TO_STATION<=9` and no `BHV_ERROR` occurred.
- `inner_gt_outer_pass` Sets `inner_radius=18` and `outer_radius=8`, exercising the behavior's handling of an inner radius larger than the outer radius; passes at 35 seconds when `DIST_TO_STATION<=18` and no `BHV_ERROR` occurred.
- `outer_speed_slow_pass` Sets `outer_speed=0.35`, exercising slow correction inside the outer ring; passes at 50 seconds when `DIST_TO_STATION<=20` and no `BHV_ERROR` occurred.
- `outer_speed_update_slow_pass` Posts `outer_speed=0.35` through `STATIONKEEP_UPDATES` at two seconds, exercising a live correction-speed change; passes at 50 seconds when `10<DIST_TO_STATION<=22` and no `BHV_ERROR` occurred.
- `transit_speed_fast_pass` Sets `transit_speed=3.0`, exercising a faster approach from `(-45,-121)` to `(12,-121)`; passes at 35 seconds when `DIST_TO_STATION<=8` and no `BHV_ERROR` occurred.
- `extra_speed_alias_pass` Sets legacy alias `extra_speed=3.0`, exercising its use as the transit speed; passes at 20 seconds when `DIST_TO_STATION<=25` and no `BHV_ERROR` occurred.
- `transit_speed_slow_in_progress_pass` Sets `transit_speed=0.5`, exercising a slow approach; the harness passes at 15 seconds when `DIST_TO_STATION>20` and no `BHV_ERROR` occurred.
- `hibernation_seek_pass` Sets `hibernation_radius=25`, exercising the approach state outside that radius; passes at eight seconds when `DIST_TO_STATION>25`, `PSKEEP_MODE=SEEKING_STATION`, and no `BHV_ERROR` occurred.
- `hibernation_settle_pass` Sets `hibernation_radius=25`, exercising transition into the no-objective hibernation state; passes at 45 seconds when `DIST_TO_STATION<=25`, `PSKEEP_MODE=HIBERNATING`, and no `BHV_ERROR` occurred.
- `hibernation_radius_update_settle_pass` Posts `hibernation_radius=25` through `STATIONKEEP_UPDATES` at two seconds, exercising runtime hibernation enablement; passes at 45 seconds when `DIST_TO_STATION<=25`, `PSKEEP_MODE=HIBERNATING`, and no `BHV_ERROR` occurred.
- `hibernation_off_pass` Sets `hibernation_radius=off`, exercising the explicit disabled form; passes at 35 seconds when `DIST_TO_STATION<=8` and no behavior warning or error occurred.
- `point_update_retarget_pass` Changes the station from `(12,-121)` to `(24,-121)` through `STATIONKEEP_UPDATES` at eight seconds, exercising live retargeting; passes at 35 seconds when the behavior-owned `DIST_TO_STATION<=8` and no behavior warning or error occurred.
- `radius_update_expand_pass` Posts `inner_radius=10` and `outer_radius=28` at 12 and 13 seconds, exercising live ring expansion; passes at 35 seconds when `DIST_TO_STATION<=10` and no `BHV_ERROR` occurred.
- `radius_update_shrink_pass` Posts `inner_radius=2` and `outer_radius=8` at 12 and 13 seconds, exercising live ring contraction; passes at 35 seconds when `DIST_TO_STATION<=9` and no `BHV_ERROR` occurred.
- `speed_update_slow_progress_pass` Posts `transit_speed=0.45` at two seconds, exercising a live slowdown during approach; the harness passes at 15 seconds when `DIST_TO_STATION>20` and no `BHV_ERROR` occurred.
- `bad_point_update_recover_pass` Posts malformed `point=bad_point` at three seconds and valid `point=x=12,y=-121` at six seconds, exercising recovery after a rejected update; passes at 35 seconds when any warning was observed, `DIST_TO_STATION<=8`, and no `BHV_ERROR` occurred.
- `visual_hints_pass` Changes the station visualization to yellow vertices, cyan edges, and size `2`, exercising visual-hint parsing; passes with the unchanged `DIST_TO_STATION<=8` and no-warning/no-error motion grade.
- `missing_point_fail` Omits both `point` and `center_activate`, exercising a stationless configuration; the harness passes at 15 seconds when `DIST_TO_STATION>=100`, `PSKEEP_MODE=UNKNOWN`, and no `BHV_ERROR` occurred.
- `bad_point_fail` Sets `point=not_a_point`, exercising malformed point rejection; the harness passes at 15 seconds when `DIST_TO_STATION>=100`, `PSKEEP_MODE=UNKNOWN`, and no `BHV_ERROR` occurred.
- `bad_update_fail` Starts without a station point and posts malformed `point=bad_point` at three seconds, exercising failed runtime initialization; the harness passes at 15 seconds when `DIST_TO_STATION>=100`, `PSKEEP_MODE=UNKNOWN`, and no `BHV_ERROR` occurred.
- `bad_outer_radius_fail` Sets `outer_radius=-3`, exercising negative-radius rejection; the harness passes at 15 seconds when `DIST_TO_STATION>=100`, `PSKEEP_MODE=UNKNOWN`, and no `BHV_ERROR` occurred.
- `bad_inner_radius_fail` Sets `inner_radius=bad_radius`, exercising nonnumeric-radius rejection; the harness passes at 15 seconds when `DIST_TO_STATION>=100`, `PSKEEP_MODE=UNKNOWN`, and no `BHV_ERROR` occurred.
- `bad_hibernation_radius_fail` Sets `hibernation_radius=zero`, exercising invalid hibernation-radius rejection; the harness passes at 15 seconds when `DIST_TO_STATION>=100`, `PSKEEP_MODE=UNKNOWN`, and no `BHV_ERROR` occurred.
- `bad_outer_speed_fail` Sets `outer_speed=zero`, exercising nonnumeric correction-speed rejection; the harness passes at 15 seconds when `DIST_TO_STATION>=100`, `PSKEEP_MODE=UNKNOWN`, and no `BHV_ERROR` occurred.
- `bad_transit_speed_fail` Sets `transit_speed=-2`, exercising negative transit-speed rejection; the harness passes at 15 seconds when `DIST_TO_STATION>=100`, `PSKEEP_MODE=UNKNOWN`, and no `BHV_ERROR` occurred.
- `bad_extra_speed_fail` Sets alias `extra_speed=-2`, exercising negative alias-value rejection; the harness passes at 15 seconds when `DIST_TO_STATION>=100`, `PSKEEP_MODE=UNKNOWN`, and no `BHV_ERROR` occurred.
- `bad_center_activate_fail` Sets `center_activate=maybe`, exercising non-boolean activation-mode rejection; the harness passes at 15 seconds when `DIST_TO_STATION>=100`, `PSKEEP_MODE=UNKNOWN`, and no `BHV_ERROR` occurred.
- `bad_swing_time_fail` Sets `center_activate=true` with `swing_time=slow`, exercising nonnumeric swing-time rejection; the harness passes at 15 seconds when `DIST_TO_STATION>=100`, `PSKEEP_MODE=UNKNOWN`, and no `BHV_ERROR` occurred.

## Running

From this harness directory:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=static_station_pass 10
./zlaunch.sh --case=hibernation_seek_pass --gui 1
./zlaunch.sh --jobs=4 --port_base=34000 10
```

The full matrix currently has 34 cases. Serial and rolling modes both use
isolated mission copies and deterministic per-case port blocks. Rolling mode
starts the next pending case whenever an active slot finishes. Do not overlap
this harness with another MOOS harness on the same machine.

Latest validation:

- July 16, 2026
- generated-file matrix: `34/34` cases completed with `--just_make --jobs=4`
- three full rolling matrices: `102/102` mission-owned verdicts passed
- full serial matrix: `34/34` mission-owned verdicts passed
- warp: `10`
