# H01-collision_behavior_motion

Patch-driven moving correctness harness for
[`missions/collision_behavior_missions/collision_behavior_motion`](../../../missions/collision_behavior_missions/collision_behavior_motion).

This harness is the moving behavior companion to the contact-manager and
obstacle-manager suites already in the repo. The stem mission tests
`BHV_AvoidCollision` itself: it requests alerts on its own, spawns on contact
info, posts lifecycle flags, and resolves when the contact is handled.

The stem keeps the same lower-band MIT_SP framing as the other motion suites:
the ownship starts at `(0,-60)` and drives a short eastbound corridor to
`(70,-60)`.

## Current Matrix

- `default_resolve_pass`
  Baseline on-lane contact. The behavior should auto-request, spawn, resolve,
  and finish with no collision.
- `no_alert_request_absent_pass`
  The same behavior is run with `no_alert_request=true`, but the contact is
  moved off-lane so the mission stays clean without engaging the avoid
  behavior.
- `post_per_contact_info_pass`
  The avoid behavior is configured to post per-contact info and the mission
  confirms the range and closing-speed outputs show up.
- `behavior_filter_absent_pass`
  The behavior requests an alert with a type filter that excludes the spoofed
  contact, so the avoid behavior stays idle and the mission still finishes
  cleanly.
- `head_on_resolve_pass`
  This case moves the intruder head-on. This is a stronger geometry than
  the baseline crossing case, but the mission should still resolve cleanly.
- `pwt_outer_too_small_fail`
  The collision-relevance outer distance is made too small for the on-lane
  encounter. The behavior may resolve administratively, but the mission should
  grade pass only when the expected collision evidence is observed.
- `pwt_grade_quadratic_pass`
  The objective-function grade is switched to `quadratic`. The behavior should
  still resolve the baseline encounter with no collision.
- `pwt_grade_quasi_pass`
  The objective-function grade is switched to `quasi`. The behavior should
  still resolve the baseline encounter with no collision.
- `use_refinery_pass`
  Enables the IvP refinery and verifies the behavior still builds a usable
  function, resolves the contact, arrives, and avoids collision.
- `contact_type_required_absent_pass`
  The legacy `contact_type_required` alias is set to a type that excludes the
  spoofed contact while the contact is moved off-lane. The behavior should stay
  idle and the transit should finish cleanly.
- `no_extrapolate_pass`
  Contact extrapolation is disabled while the contact feed remains fresh. The
  behavior should still resolve the encounter cleanly.
- `no_alert_request_fail`
  The behavior is told not to request an alert while the contact remains
  challenging. The mission should grade pass only when the avoid behavior stays
  idle, no contact info arrives, and a collision is observed.
- `bad_pwt_inner_dist_fail`
  `pwt_inner_dist` is set outside the valid relationship to `pwt_outer_dist`.
  The mission should grade pass only when the helm reports malconfiguration.
- `bad_pwt_outer_dist_fail`
  `pwt_outer_dist` is set inside the configured inner distance. The mission
  should grade pass only when the helm reports malconfiguration.
- `bad_min_util_cpa_dist_fail`
  `min_util_cpa_dist` is set above `max_util_cpa_dist`. The mission should
  grade pass only when the helm reports malconfiguration.
- `bad_max_util_cpa_dist_fail`
  `max_util_cpa_dist` is set below `min_util_cpa_dist`. The mission should
  grade pass only when the helm reports malconfiguration.
- `bad_pwt_grade_fail`
  `pwt_grade` is set to an unsupported token. The behavior configuration
  should be rejected and the mission should grade pass only when the helm
  reports malconfiguration.
- `bad_completed_dist_fail`
  `completed_dist` is set negative. The behavior configuration should be
  rejected and the mission should grade pass only when the helm reports
  malconfiguration.
- `bad_time_on_leg_fail`
  The inherited `time_on_leg` setting is set negative. The mission should
  grade pass only when the helm reports malconfiguration.
- `bad_decay_fail`
  The inherited contact-decay window is malformed by making the stale threshold
  lower than the linear threshold. The mission should grade pass only when the
  helm reports malconfiguration.
- `bad_collision_depth_fail`
  `collision_depth` is enabled in this 2D mission, where the source requires a
  depth domain. The mission should grade pass only when the helm reports
  malconfiguration.

## Results Lines

Typical pass line:

```text
case=default_resolve_pass  grade=pass  form=collision_behavior_motion_tests  mmod=default_resolve_pass  eval=true  avoiding=end  alert_req_seen=false  contact_seen=true  range_seen=false  clsg_seen=false  resolved=intruder  arrived=true  encounters=1  near_misses=0  collisions=0  mhash=[...]
```

Typical expected-negative pass line:

```text
case=no_alert_request_fail  grade=pass  form=collision_behavior_motion_tests  mmod=no_alert_request_fail  eval=true  avoiding=idle  alert_req_seen=false  contact_seen=false  resolved=none  arrived=true  collisions=1  mhash=[...]
```

Field anatomy:

- `case`: harness case name
- `grade`: mission-owned pass/fail from `pMissionEval`; expected-negative
  cases grade `pass` when their negative evidence is observed
- `form`: mission family name
- `mmod`: mission mode for the selected case
- `eval`: whether the evaluation checkpoint fired
- `avoiding`: behavior lifecycle state at grading time
- `alert_req_seen`: whether `BCM_ALERT_REQUEST` was seen on shoreside. This is
  report-only because the request is posted during helm startup and may race
  the shore bridge path.
- `contact_seen`: whether `CONTACT_INFO` was seen
- `range_seen`: whether `RANGE_AVD_intruder` was seen
- `clsg_seen`: whether `CLSG_SPD_AVD_intruder` was seen
- `resolved`: value posted by the behavior `endflag`
- `arrived`: whether the ownship reached the goal waypoint
- `encounters`: `uFldCollisionDetect` encounter total, when reported by the
  case
- `near_misses`: `uFldCollisionDetect` near-miss total, when reported by the
  case
- `collisions`: `uFldCollisionDetect` collision total
- `mhash`: short mission hash

## Running

```bash
./zlaunch.sh
./zlaunch.sh --jobs=4 --port_base=17000 10
./zlaunch.sh --case=head_on_resolve_pass 10
./zlaunch.sh --just_make 10
```

Wave mode notes:

- `--jobs=<N>` runs the matrix in waves of up to `N` isolated case copies
- each live case in a wave gets its own temp mission directory and unique port
  block
- the harness uses scoped teardown between waves
- this mode is intended for CI wall-clock reduction, not for interactive use
  alongside other MOOS missions

Latest validation:

- May 21, 2026
- full case sweep: `21/21` passing
- warp: `10`
- command: individual `--case=<name>` runs with `--port_base` values from
  `36100` through `36500`
