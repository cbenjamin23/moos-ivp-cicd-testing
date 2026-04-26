# H01-convoy_behavior_motion

Patch-driven matrix for
[`/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/convoy_behavior_missions/convoy_behavior_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/convoy_behavior_missions/convoy_behavior_motion).

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

- `static_convoy_pass`
  Baseline convoy following with 5 meter mark spacing and an 80 meter queue cap.
- `fine_mark_spacing_pass`
  Shrinks `inter_mark_range` and grades a larger mark queue.
- `coarse_mark_spacing_pass`
  Expands `inter_mark_range` and grades a smaller mark queue.
- `short_mark_queue_pass`
  Shrinks `max_mark_range` and grades queue trimming through `MXRNG` and `TLEN`.
- `long_mark_queue_pass`
  Expands `max_mark_range` and verifies the configured cap is reported.
- `tight_radius_pass`
  Shrinks waypoint capture radius while preserving stable convoy following.
- `wide_radius_pass`
  Expands waypoint capture radius while preserving stable convoy following.
- `cruise_speed_pass`
  Uses explicit `cruise_speed` and grades the chaser speed branch.
- `cruise_speed_cap_warn_pass`
  Sets `cruise_speed` above `spd_max` and expects the behavior warning path.
- `safety_range_autoadjust_warn_pass`
  Gives inconsistent safety ranges and expects auto-adjust warning behavior.
- `safety_off_bad_ranges_pass`
  Disables range safety and verifies the same inconsistent ranges remain runnable.
- `tailgating_speed_slow_pass`
  Forces the tailgating branch and grades slower physical chaser speed.
- `lagging_speed_fast_pass`
  Forces the lagging branch and grades faster physical chaser speed.
- `estop_speed_zero_pass`
  Forces the emergency-stop branch and grades near-zero chaser speed.
- `range_aliases_pass`
  Uses the `range_estop`, `range_tailgating`, and `range_lagging` aliases.
- `nm_radius_zero_pass`
  Verifies zero `nm_radius` is accepted without breaking queue following.
- `view_point_post_pass`
  Verifies `BHV_Convoy` posts visual breadcrumb points through `VIEW_POINT`.
- `angled_entry_pass`
  Starts the chaser southwest of the convoy line with a northeast heading and
  grades recovery into the target's breadcrumb queue.
- `cross_track_entry_pass`
  Starts the chaser well below the target leg with a northward heading and
  grades lateral convergence back toward the convoy line.
- `opposite_heading_recover_pass`
  Starts the chaser behind the target but pointed west, then grades recovery
  into eastbound convoy following.
- `close_offset_tailgate_pass`
  Starts the chaser close and laterally offset from the target, then grades
  tailgate-safe convergence into the convoy queue.
- `lead_right_turn_pass`
  Replaces the target's straight waypoint leg with a right-angle dogleg and
  grades convoy following after the target has turned north.
- `lead_s_turn_pass`
  Gives the target a compact S-turn path and grades convoy following after the
  target has completed two heading changes.
- `short_queue_turn_pass`
  Combines a right-angle target turn with a shortened `max_mark_range`, grading
  queue trimming during non-straight contact motion.
- `slow_follower_pass`
  Uses conservative speed multipliers and grades slower physical chaser speed.
- `fast_follower_pass`
  Uses aggressive speed multipliers and grades faster physical chaser speed.
- `runtime_inter_mark_coarse_pass`
  Changes `inter_mark_range` through `CONVOY_UPDATES`.
- `runtime_max_mark_short_pass`
  Changes `max_mark_range` through `CONVOY_UPDATES`.
- `runtime_cruise_speed_pass`
  Changes `cruise_speed` through `CONVOY_UPDATES` and grades chaser speed.
- `runtime_cruise_cap_warn_pass`
  Applies high cruise-speed/runtime speed-cap updates and expects warning-safe
  operation.
- `runtime_bad_update_recover_pass`
  Sends a malformed runtime mark-spacing update, then verifies a later valid
  update recovers without a behavior error.
- `no_extrapolate_pass`
  Verifies clean following with contact extrapolation disabled.
- `missing_contact_warn_pass`
  Uses a missing contact with `on_no_contact_ok=true` and expects warning-only
  behavior.
- `missing_contact_fail`
  Uses a missing contact with `on_no_contact_ok=false` and expects failure.
- `missing_contact_param_fail`
  Omits `contact` with `on_no_contact_ok=false` and expects failure.
- `bad_inter_mark_range_fail`
  Rejects negative `inter_mark_range`.
- `bad_max_mark_range_fail`
  Rejects negative `max_mark_range`.
- `bad_radius_fail`
  Rejects a negative `radius` configuration and expects the behavior error path
  to make the mission fail.
- `bad_nm_radius_fail`
  Rejects a negative `nm_radius` configuration and expects the behavior error
  path to make the mission fail.
- `bad_spd_slower_fail`
  Rejects `spd_slower` outside its accepted range.
- `bad_spd_faster_fail`
  Rejects `spd_faster` outside its accepted range.
- `bad_spd_max_fail`
  Rejects zero `spd_max`, which would make the convoy speed cap invalid, and
  expects mission failure.
- `bad_rng_estop_fail`
  Rejects a negative `rng_estop` safety range and expects the behavior error
  path to make the mission fail.
- `bad_rng_safety_fail`
  Rejects malformed `rng_safety`.

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
./zlaunch.sh --jobs=4 10
```

From the paired mission directory, named cases are forwarded to this harness:

```bash
./zlaunch.sh --case=runtime_max_mark_short_pass --gui 1
```

The geometry-entry cases are evaluated at 65 seconds and require `QLEN >= 2`,
`TLEN` between 10 and 95 meters, `MXRNG = 80`, `AVG2 >= 1`, a moving target
and chaser, chaser `Y` between -102 and -58, and no behavior error.

The full matrix currently has 44 cases. Wave mode uses isolated temp mission
copies and deterministic two-vehicle port blocks, so do not overlap it with
other MOOS harnesses on the same machine.
