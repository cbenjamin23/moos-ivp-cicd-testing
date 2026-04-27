# H01-stationkeep_behavior_motion

Patch-driven matrix for
[`/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/stationkeep_behavior_missions/stationkeep_behavior_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/stationkeep_behavior_missions/stationkeep_behavior_motion).

This harness focuses on `BHV_StationKeep` as the behavior under test. The stem
mission keeps one simulated vehicle near a compact station point and grades on
behavior-owned outputs:

- `DIST_TO_STATION`
- `PSKEEP_MODE`
- `BHV_SETTINGS`
- `BHV_WARNING`
- `BHV_ERROR`

## Cases

- `static_station_pass`
  Baseline station point, default radii, and default settle grading.
- `station_pt_alias_pass`
  Uses the `station_pt` parameter alias instead of `point`.
- `start_inside_hold_pass`
  Starts at the configured station point and verifies it can hold immediately.
- `center_activate_hold_pass`
  Exercises `center_activate=true` with the station anchored at activation.
- `center_activate_swing_pass`
  Combines `center_activate=true` with swing behavior.
- `wide_radius_pass`
  Expands the inner and outer station-keeping rings.
- `tight_radius_pass`
  Tightens the station-keeping rings and grades tighter station proximity.
- `inner_gt_outer_pass`
  Exercises the parameter path where the inner radius is greater than the
  outer radius.
- `outer_speed_slow_pass`
  Slows the outer correction speed and verifies bounded progress.
- `outer_speed_update_slow_pass`
  Slows the outer correction speed through `STATIONKEEP_UPDATES` and grades a
  farther bounded approach than the default controller would produce.
- `transit_speed_fast_pass`
  Raises transit speed and verifies quicker station arrival.
- `extra_speed_alias_pass`
  Uses the `extra_speed` alias for transit speed and evaluates early arrival.
- `transit_speed_slow_in_progress_pass`
  Lowers transit speed and evaluates early to confirm the vehicle is still
  seeking the station.
- `hibernation_seek_pass`
  Enables hibernation and evaluates early while `PSKEEP_MODE=SEEKING_STATION`.
- `hibernation_settle_pass`
  Enables hibernation and waits for `PSKEEP_MODE=HIBERNATING`.
- `hibernation_radius_update_settle_pass`
  Enables hibernation through `STATIONKEEP_UPDATES` and waits for
  `PSKEEP_MODE=HIBERNATING`.
- `hibernation_off_pass`
  Explicitly disables hibernation and verifies clean station arrival.
- `point_update_retarget_pass`
  Retargets the station point through `STATIONKEEP_UPDATES`.
- `radius_update_expand_pass`
  Expands station-keeping radii through runtime updates.
- `radius_update_shrink_pass`
  Shrinks station-keeping radii through runtime updates.
- `speed_update_slow_progress_pass`
  Slows speed through runtime updates and grades early in-progress behavior.
- `bad_point_update_recover_pass`
  Sends a malformed update, then confirms the behavior continues with warning
  visibility and no fatal error.
- `visual_hints_pass`
  Exercises StationKeep visual-hint configuration.
- `missing_point_fail`
  Removes the station point and expects mission failure.
- `bad_point_fail`
  Provides malformed point configuration and expects mission failure.
- `bad_update_fail`
  Starts without a valid point, sends a malformed update, and expects failure.
- `bad_outer_radius_fail`
  Provides malformed outer radius configuration and expects failure.
- `bad_inner_radius_fail`
  Provides malformed inner radius configuration and expects failure.
- `bad_hibernation_radius_fail`
  Provides malformed hibernation radius configuration and expects failure.
- `bad_outer_speed_fail`
  Provides malformed outer speed configuration and expects failure.
- `bad_transit_speed_fail`
  Provides malformed transit speed configuration and expects failure.
- `bad_extra_speed_fail`
  Provides malformed `extra_speed` alias configuration and expects failure.
- `bad_center_activate_fail`
  Provides non-boolean `center_activate` configuration and expects failure.
- `bad_swing_time_fail`
  Provides malformed swing-time configuration and expects failure.

## Running

From this harness directory:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=static_station_pass 10
./zlaunch.sh --case=hibernation_seek_pass --gui 1
./zlaunch.sh --jobs=4 --port_base=34000 10
```

From the paired mission directory, named cases are forwarded to this harness:

```bash
./zlaunch.sh --case=point_update_retarget_pass --gui 1
```

The full matrix currently has 34 cases. Wave mode uses isolated temp mission
copies and deterministic per-case port blocks, so do not overlap it with other
MOOS harnesses on the same machine.

Latest validation:

- April 27, 2026
- generated-file matrix: `34/34` cases completed with `--just_make --jobs=4 --port_base=15000`
- full wave matrix: `34/34` expected outcomes matched with `--jobs=4 --port_base=15000`
- warp: `10`
