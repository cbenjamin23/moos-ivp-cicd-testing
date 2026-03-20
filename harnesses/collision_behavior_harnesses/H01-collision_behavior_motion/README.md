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
  The intruder is patched to move head-on. This is a stronger geometry than
  the baseline crossing case, but the mission should still resolve cleanly.
- `no_alert_request_fail`
  The behavior is told not to request an alert while the contact remains
  challenging. The mission should fail because the avoid behavior never
  engages and the required resolution path does not complete.

## Results Lines

Typical pass line:

```text
case=default_resolve_pass  expected=pass  actual=pass  status=ok  grade=pass  form=collision_behavior_motion_tests  mmod=default_resolve_pass  eval=true  avoiding=end  alert_req_seen=false  contact_seen=true  range_seen=false  clsg_seen=false  resolved=intruder  arrived=true  collisions=0  mhash=[WARM-ELMO]
```

Typical fail line:

```text
case=no_alert_request_fail  expected=fail  actual=fail  status=ok  grade=fail  form=collision_behavior_motion_tests  mmod=no_alert_request_fail  eval=true  avoiding=idle  alert_req_seen=false  contact_seen=false  range_seen=false  clsg_seen=false  resolved=none  arrived=true  collisions=0  mhash=[...]
```

Field anatomy:

- `case`: harness case name
- `expected`: what the harness expected the mission to grade
- `actual`: what the mission actually graded
- `status`: `ok` when expected equals actual
- `grade`: mission-level pass/fail from `pMissionEval`
- `form`: mission family name
- `mmod`: case-specific mission mode
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
- `collisions`: `uFldCollisionDetect` collision total
- `mhash`: short mission hash

## Running

```bash
./zlaunch.sh
./zlaunch.sh --jobs=2 10
./zlaunch.sh --case=head_on_resolve_pass 10
./zlaunch.sh --just_make 10
```

Wave mode notes:

- `--jobs=<N>` runs the matrix in waves of up to `N` isolated case copies
- each live case in a wave gets its own temp mission directory and unique port
  block
- the harness uses one `ktm` barrier between waves, not after every case
- this mode is intended for CI wall-clock reduction, not for interactive use
  alongside other MOOS missions

Latest validation:

- March 20, 2026
- full matrix: `6/6` passing
- warp: `10`
- serial wall clock: `63.82` seconds
- `--jobs=2` wall clock: `43.62` seconds
