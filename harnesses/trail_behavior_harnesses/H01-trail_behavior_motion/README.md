# H01-trail_behavior_motion

Patch-driven matrix for
[`/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/trail_behavior_missions/trail_behavior_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/trail_behavior_missions/trail_behavior_motion).

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

- `static_trail_pass`
  Baseline relative trailing at 180 degrees and 45 meters.
- `absolute_west_pass`
  Uses `trail_angle_type=absolute` with a westward trail point.
- `relative_port_pass`
  Keeps the chaser on the target's port quarter, proving `BHV_Trail` can hold
  an offset relative trail angle instead of only following directly astern.
- `relative_starboard_pass`
  Trails off the contact's starboard quarter with a relative -135 degree angle.
- `lead_right_turn_pass`
  Replaces the target's straight waypoint leg with a right-angle dogleg and
  grades pursuit after the target changes course.
- `lead_port_turn_pass`
  Combines the dogleg target path with a port-quarter relative trail angle.
- `lead_turn_angle_update_pass`
  Follows a target through a dogleg while changing `trail_angle` at runtime,
  proving the chaser can reframe the desired trail point during a turn.
- `short_trail_range_pass`
  Shortens `trail_range` and grades tighter trail-point convergence.
- `long_trail_range_pass`
  Extends `trail_range` and evaluates after the longer geometry settles.
- `tight_radius_pass`
  Shrinks the inner radius while keeping pursuit active.
- `wide_radius_pass`
  Expands the inner radius and grades bounded trail-point convergence.
- `inside_nm_radius_pass`
  Exercises the inside-`nm_radius` branch.
- `outside_nm_radius_pass`
  Forces the outside-`nm_radius` branch with zero radius settings.
- `pwt_outer_active_pass`
  Sets `pwt_outer_dist` high enough that relevance remains active.
- `pwt_outer_inactive_pass`
  Sets `pwt_outer_dist` below contact range and grades zero relevance/pursuit.
- `mod_trail_range_plus_pass`
  Uses `mod_trail_range` to increase configured trailing distance.
- `mod_trail_range_pct_pass`
  Uses `mod_trail_range_pct` to shrink configured trailing distance.
- `mod_trail_range_floor_pass`
  Drives `mod_trail_range` below the source minimum and verifies the configured
  trail range clamps to the 10 meter floor while pursuit stays active.
- `runtime_range_extend_pass`
  Increases `trail_range` through `TRAIL_UPDATES`, proving the behavior can
  accept a runtime range change and settle cleanly at the longer separation.
- `runtime_mod_range_plus_pass`
  Applies `mod_trail_range` through `TRAIL_UPDATES`.
- `runtime_mod_range_pct_pass`
  Applies `mod_trail_range_pct` through `TRAIL_UPDATES`.
- `runtime_mod_range_floor_pass`
  Applies a runtime `mod_trail_range_pct` update below the source minimum and
  verifies the clamped trail range still produces valid pursuit.
- `runtime_angle_update_pass`
  Changes `trail_angle` through `TRAIL_UPDATES`.
- `runtime_relevance_off_pass`
  Lowers `pwt_outer_dist` through `TRAIL_UPDATES` and grades pursuit shutoff.
- `runtime_bad_update_recover_pass`
  Sends a malformed runtime trail-range update, then verifies a later valid
  update recovers without a behavior error.
- `idle_post_distance_pass`
  Idles the trail behavior after pursuit and verifies distance reporting can
  continue with pursuit off.
- `idle_no_post_distance_pass`
  Idles the trail behavior with `post_trail_dist_on_idle=false` and verifies a
  sentinel `TRAIL_DISTANCE` value is not overwritten while pursuit is off.
- `no_extrapolate_pass`
  Verifies clean pursuit with contact extrapolation disabled.
- `no_alert_request_pass`
  Verifies `no_alert_request=true` is accepted without breaking pursuit.
- `auto_alert_request_pass`
  Runs a spawnable trail behavior with `no_alert_request=false` and verifies an
  automatic `BCM_ALERT_REQUEST` is posted before pursuit becomes active.
- `missing_contact_warn_pass`
  Uses a missing contact with `on_no_contact_ok=true` and expects warning-only
  behavior.
- `missing_contact_fail`
  Uses a missing contact with `on_no_contact_ok=false` and expects failure.
- `missing_contact_param_fail`
  Omits `contact` with `on_no_contact_ok=false` and expects failure.
- `bad_trail_angle_type_fail`
  Rejects malformed `trail_angle_type`.
- `bad_trail_angle_fail`
  Rejects malformed `trail_angle`.
- `bad_trail_range_fail`
  Rejects negative `trail_range`.
- `bad_mod_trail_range_pct_fail`
  Rejects zero `mod_trail_range_pct`.
- `bad_radius_fail`
  Rejects a negative `radius` configuration and expects the behavior error path
  to make the mission fail.
- `bad_nm_radius_fail`
  Rejects a negative `nm_radius` configuration and expects the behavior error
  path to make the mission fail.
- `bad_pwt_outer_dist_fail`
  Rejects negative `pwt_outer_dist`.
- `bad_decay_fail`
  Rejects malformed `decay` input and expects the behavior error path to make
  the mission fail.
- `bad_time_on_leg_fail`
  Rejects negative `time_on_leg`.

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

From the paired mission directory, named cases are forwarded to this harness:

```bash
./zlaunch.sh --case=runtime_angle_update_pass --gui 1
```

The full matrix currently has 42 cases. Wave mode uses isolated temp mission
copies and deterministic two-vehicle port blocks, so do not overlap it with
other MOOS harnesses on the same machine.
