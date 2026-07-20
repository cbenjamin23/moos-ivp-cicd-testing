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
- normalized `BHV_SETTINGS` and `VIEW_POLYGON` fields
- `DESIRED_SPEED`
- exact `IVPHELM_UPDATE_RESULT` outcomes
- `IVPHELM_STATE`, `BHV_WARNING`, and `BHV_ERROR`

## Cases

- `static_station_pass` Configures station point `(12,-121)`, inner radius `4`, outer radius `15`, outer speed `1.2`, and the stock transit speed; passes at 35 seconds when `DIST_TO_STATION<=8` and no behavior warning or error occurred.
- `station_pt_alias_pass` Supplies `(12,-121)` through the `station_pt` alias instead of `point`, exercising alias parsing; passes at 35 seconds when `DIST_TO_STATION<=8` and no behavior warning or error occurred.
- `start_inside_hold_pass` Places the station at the vehicle's initial position `(-45,-121)`, exercising immediate hold without a transit; passes at 35 seconds when `DIST_TO_STATION<=8` and no behavior warning or error occurred.
- `center_activate_hold_pass` Omits a fixed point and sets `center_activate=true`, exercising creation of the station at activation; passes at 35 seconds when `DIST_TO_STATION<=8` and no behavior warning or error occurred.
- `wide_radius_pass` Sets `inner_radius=10` and `outer_radius=28`; passes after the normalized settings report those exact radii and the vehicle reaches `DIST_TO_STATION<=10` before the deadline.
- `tight_radius_pass` Sets `inner_radius=2`, `outer_radius=8`, and `outer_speed=1.4`; passes after the normalized settings report all three values and the seeking vehicle reaches `DIST_TO_STATION<=9` before the deadline.
- `inner_gt_outer_pass` Sets `inner_radius=18` and `outer_radius=8`; the live case requires those exact settings and `DIST_TO_STATION<=18`, while a direct CTest verifies that StationKeep promotes the effective outer ring to 18 meters.
- `outer_speed_slow_pass` Sets `outer_speed=0.35`; passes when the reported setting rounds to `0.3` and, at `9<DIST_TO_STATION<11`, StationKeep commands `0.15<DESIRED_SPEED<0.25` while seeking.
- `outer_speed_update_slow_pass` Posts `outer_speed=0.35` through `STATIONKEEP_UPDATES`; passes after the exact update is accepted, settings report `outer_speed=0.3`, and StationKeep commands `0.15<DESIRED_SPEED<0.25` at `9<DIST_TO_STATION<11` while seeking.
- `transit_speed_fast_pass` Sets `transit_speed=3.0`; passes when StationKeep commands `2.95<DESIRED_SPEED<3.05` while seeking outside the 15-meter outer ring.
- `extra_speed_alias_pass` Sets alias `extra_speed=3.0`; passes when StationKeep likewise commands `2.95<DESIRED_SPEED<3.05` while seeking outside the outer ring.
- `transit_speed_slow_in_progress_pass` Sets `transit_speed=0.5`; passes when the seeking vehicle has moved from its initial 57-meter range to `15<DIST_TO_STATION<56` while commanding `0.45<DESIRED_SPEED<0.55`.
- `hibernation_seek_pass` Sets `hibernation_radius=25`, exercising the approach state outside that radius; passes at eight seconds when `DIST_TO_STATION>25`, `PSKEEP_MODE=SEEKING_STATION`, and no `BHV_ERROR` occurred.
- `hibernation_settle_pass` Sets `hibernation_radius=25`, exercising transition into the no-objective hibernation state; passes at 45 seconds when `DIST_TO_STATION<=25`, `PSKEEP_MODE=HIBERNATING`, and no `BHV_ERROR` occurred.
- `hibernation_radius_update_settle_pass` Posts `hibernation_radius=25` through `STATIONKEEP_UPDATES` at two seconds, exercising runtime hibernation enablement; passes at 45 seconds when `DIST_TO_STATION<=25`, `PSKEEP_MODE=HIBERNATING`, and no `BHV_ERROR` occurred.
- `hibernation_off_pass` Sets `hibernation_radius=off`; passes when settings report the disabled value `hiber=-1.0` and StationKeep remains `SEEKING_STATION` while more than 25 meters from the station.
- `point_update_retarget_pass` Posts `point=x=24,y=-121`; passes after that exact update is accepted, settings report station `(24,-121)`, and the seeking vehicle reaches `DIST_TO_STATION<=8` from the new point.
- `radius_update_expand_pass` Posts `inner_radius=10` and `outer_radius=28`; passes after update success is observed, settings report both exact radii, and the vehicle subsequently reaches `DIST_TO_STATION<=10`.
- `radius_update_shrink_pass` Posts `inner_radius=2` and `outer_radius=8`; passes after update success is observed, settings report both exact radii, and the vehicle subsequently reaches `DIST_TO_STATION<=9`.
- `speed_update_slow_progress_pass` Posts `transit_speed=0.45`; passes after the exact update is accepted and the seeking vehicle advances to `15<DIST_TO_STATION<56` while the helm's speed grid commands `0.35<DESIRED_SPEED<0.55`.
- `bad_point_update_recover_pass` Starts at station `(12,-121)`, rejects `point=bad_point`, then accepts the distinct recovery point `(24,-121)`; passes after the exact rejection result and a warning, exact accepted update and final settings, and `DIST_TO_STATION<=8` from the recovered point.
- `visual_hints_pass` Sets vertex color yellow, edge color cyan, and both sizes to `2`; passes only when the normalized inner-ring `VIEW_POLYGON` reports those fields, with white label color and no behavior error.
- `missing_point_fail` Omits both `point` and `center_activate`, exercising a stationless configuration; the harness passes at 15 seconds when `DIST_TO_STATION>=100`, `PSKEEP_MODE=UNKNOWN`, and no `BHV_ERROR` occurred.
- `bad_point_fail` Sets `point=not_a_point`, exercising malformed point rejection; the harness passes at 15 seconds when `DIST_TO_STATION>=100`, `PSKEEP_MODE=UNKNOWN`, and no `BHV_ERROR` occurred.
- `bad_update_fail` Starts without a station and posts `point=bad_point`; the harness passes only when the exact update reports `param_error`, a warning is published, and StationKeep remains inactive with `DIST_TO_STATION>=100`, `PSKEEP_MODE=UNKNOWN`, and no behavior error.
- `bad_outer_radius_fail` Sets `outer_radius=-3`; the harness passes only when the armed helm reaches `MALCONFIG` before the missing-state deadline, while a direct CTest requires this value to be rejected by StationKeep.
- `bad_inner_radius_fail` Sets `inner_radius=bad_radius`; the harness passes only when the armed helm reaches `MALCONFIG` before the missing-state deadline, while a direct CTest requires this value to be rejected by StationKeep.
- `bad_hibernation_radius_fail` Sets `hibernation_radius=zero`; the harness passes only when the armed helm reaches `MALCONFIG` before the missing-state deadline, while a direct CTest requires this value to be rejected by StationKeep.
- `bad_outer_speed_fail` Sets `outer_speed=zero`; the harness passes only when the armed helm reaches `MALCONFIG` before the missing-state deadline, while a direct CTest requires this value to be rejected by StationKeep.
- `bad_transit_speed_fail` Sets `transit_speed=-2`; the harness passes only when the armed helm reaches `MALCONFIG` before the missing-state deadline, while a direct CTest requires this value to be rejected by StationKeep.
- `bad_extra_speed_fail` Sets alias `extra_speed=-2`; the harness passes only when the armed helm reaches `MALCONFIG` before the missing-state deadline, while a direct CTest requires this value to be rejected by StationKeep.
- `bad_center_activate_fail` Sets `center_activate=maybe`; the harness passes only when the armed helm reaches `MALCONFIG` before the missing-state deadline, while a direct CTest requires this value to be rejected by StationKeep.
- `bad_swing_time_fail` Sets `center_activate=true` with `swing_time=slow`; the harness passes only when the armed helm reaches `MALCONFIG` before the missing-state deadline, while a direct CTest requires this value to be rejected by StationKeep.

## Direct CTest coverage

The `BHVStationKeepTest` target verifies deterministic contracts that live
vehicle motion cannot isolate reliably: an eight-second swing follows ownship
before freezing its station, an inner radius greater than the outer radius
promotes the effective outer ring, aliases are accepted, and every malformed
value used by this harness is rejected. The former live
`center_activate_swing_pass` case was removed because its ordinary final hold
grade could not distinguish `swing_time=8` from zero.

## Running

From this harness directory:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=static_station_pass 10
./zlaunch.sh --case=hibernation_seek_pass --gui 1
./zlaunch.sh --jobs=4 --port_base=34000 10
```

The full matrix currently has 33 cases. Serial and rolling modes both use
isolated mission copies and deterministic per-case port blocks. Rolling mode
starts the next pending case whenever an active slot finishes. Do not overlap
this harness with another MOOS harness on the same machine.

Latest validation:

- July 20, 2026
- generated-file matrix: `33/33` cases completed with `--just_make --jobs=4`
- minimal and full rolling matrices: `66/66` mission-owned verdicts passed
- focused `behaviors-marine` CTests: `214/214` passed, including all eight
  `BHVStationKeepTest` cases
- five deliberate mutations failed for the intended reason: ignored slow
  correction speed, wrong recovery point, wrong visual color, repaired invalid
  startup radius, and disabled swing interval
- warp: `10`
