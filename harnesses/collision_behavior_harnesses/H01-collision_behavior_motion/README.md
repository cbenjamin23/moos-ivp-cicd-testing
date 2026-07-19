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

- `default_resolve_pass`: Places a stationary kayak named `intruder` at `(30,-60)`, directly in Abe's eastbound lane, to exercise the stock spawned `BHV_AvoidCollision`; passes at the evaluation checkpoint when contact info was received, `AVOIDING=end`, `CONTACT_RESOLVED=intruder`, `ARRIVED=true`, and `COLLISION_TOTAL=0`.
- `no_alert_request_absent_pass`: Sets `no_alert_request=true` and moves the stationary intruder five meters off the lane to `(30,-55)`; passes when the behavior remains idle with no resolved contact, Abe arrives, and collision total is zero.
- `post_per_contact_info_pass`: Sets `post_per_contact_info=true` against the baseline on-lane intruder; passes when both `RANGE_AVD_intruder` and `CLSG_SPD_AVD_intruder` were published, contact info was received, the behavior ends with `intruder` resolved, Abe arrives, and collision total is zero.
- `behavior_filter_absent_pass`: Sets `match_type=submarine` while the off-lane intruder is a kayak, exercising the behavior-owned type filter; passes when the behavior remains idle with no resolved contact, Abe arrives, and collision total is zero.
- `head_on_resolve_pass`: Starts the intruder at `(70,-60)` moving west at 1.2 m/s directly toward eastbound Abe; passes when contact info was received, the behavior ends with `intruder` resolved, Abe arrives, and collision total is zero.
- `pwt_outer_too_small_fail`: Shrinks the relevance distances to `pwt_outer_dist=5`, `pwt_inner_dist=2`, `min_util_cpa_dist=1`, and `max_util_cpa_dist=3` against the baseline on-lane intruder; this expected-negative case passes when `COLLISION_TOTAL>0` at the evaluation checkpoint.
- `pwt_grade_quadratic_pass`: Sets `pwt_grade=quadratic` against the baseline on-lane intruder, exercising quadratic relevance shaping; passes when contact info was received, the behavior ends with `intruder` resolved, Abe arrives, and collision total is zero.
- `pwt_grade_quasi_pass`: Sets `pwt_grade=quasi` against the baseline on-lane intruder, exercising quasi relevance shaping; passes when contact info was received, the behavior ends with `intruder` resolved, Abe arrives, and collision total is zero.
- `use_refinery_pass`: Sets `use_refinery=true` against the baseline on-lane intruder, exercising refined IvP-function construction; passes when contact info was received, the behavior ends with `intruder` resolved, Abe arrives, and collision total is zero.
- `contact_type_required_absent_pass`: Uses the legacy `contact_type_required=submarine` alias while the off-lane intruder is a kayak; passes when the behavior remains idle with no resolved contact, Abe arrives, and collision total is zero.
- `no_extrapolate_pass`: Sets `extrapolate=false` while retaining the fresh stationary on-lane intruder feed; passes when contact info was received, the behavior ends with `intruder` resolved, Abe arrives, and collision total is zero.
- `no_alert_request_fail`: Sets `no_alert_request=true` while leaving the stationary intruder directly in Abe's lane; this expected-negative case passes when the behavior remains idle, `CONTACT_INFO` is never observed, no contact is resolved, Abe arrives, and at least one collision is recorded.
- `bad_pwt_inner_dist_fail`: Sets `pwt_inner_dist=30` above `pwt_outer_dist=24`; this expected-negative case passes when `IVPHELM_STATE` reports `HELM_MALCONFIG=true` before the 18-second checkpoint.
- `bad_pwt_outer_dist_fail`: Sets `pwt_outer_dist=5` below `pwt_inner_dist=10`; this expected-negative case passes when `IVPHELM_STATE` reports `HELM_MALCONFIG=true` before the 18-second checkpoint.
- `bad_min_util_cpa_dist_fail`: Sets `min_util_cpa_dist=20` above `max_util_cpa_dist=16`; this expected-negative case passes when `IVPHELM_STATE` reports `HELM_MALCONFIG=true` before the 18-second checkpoint.
- `bad_max_util_cpa_dist_fail`: Sets `max_util_cpa_dist=4` below `min_util_cpa_dist=6`; this expected-negative case passes when `IVPHELM_STATE` reports `HELM_MALCONFIG=true` before the 18-second checkpoint.
- `bad_pwt_grade_fail`: Sets the unsupported `pwt_grade=banana`; this expected-negative case passes when `IVPHELM_STATE` reports `HELM_MALCONFIG=true` before the 18-second checkpoint.
- `bad_completed_dist_fail`: Sets `completed_dist=-1`; this expected-negative case passes when `IVPHELM_STATE` reports `HELM_MALCONFIG=true` before the 18-second checkpoint.
- `bad_time_on_leg_fail`: Sets inherited `time_on_leg=-1`; this expected-negative case passes when `IVPHELM_STATE` reports `HELM_MALCONFIG=true` before the 18-second checkpoint.
- `bad_decay_fail`: Sets the inherited contact decay window to `30,20`, placing the stale threshold below the linear threshold; this expected-negative case passes when `IVPHELM_STATE` reports `HELM_MALCONFIG=true` before the 18-second checkpoint.
- `bad_collision_depth_fail`: Sets `collision_depth=2` in a course/speed-only IvP domain; this expected-negative case passes when `IVPHELM_STATE` reports `HELM_MALCONFIG=true` before the 18-second checkpoint.

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

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for a fully logged diagnostic case.
