# collision_behavior_motion

This mission is the moving correctness stem for `BHV_AvoidCollision`.
It is intentionally small and deterministic:

- ownship starts at `(0,-60)`
- ownship drives east to `70,-60`
- the map uses the MIT_SP / lower-band framing
- contact input is synthetic and comes from `pSpoofNode`
- the avoid behavior owns its own alert request path through `BCM_ALERT_REQUEST`

The mission is graded on behavior-owned lifecycle flags and a few helper
booleans that capture whether the request and per-contact info path fired.
The mission does not rely on harness-side string matching.

One nuance: `BCM_ALERT_REQUEST` is posted during helm startup, so the
shoreside `alert_req_seen` helper is useful as a report column but not as a
strict graded field. The actual pass/fail logic relies on downstream behavior
evidence like `CONTACT_INFO`, `AVOIDING`, `CONTACT_RESOLVED`, arrival, and
collision outcome.

## Case Family

- `default_resolve_pass`
  Baseline on-lane contact. The behavior should request an alert, spawn, run,
  resolve the contact, and finish cleanly with no collision.
- `no_alert_request_absent_pass`
  The same corridor is run with `no_alert_request=true`, but the contact is
  moved off the lane so the mission stays clean without spawning the avoid
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
- `pwt_outer_too_small_fail`
  A too-small collision-relevance outer distance should be exposed as an unsafe
  collision outcome, not counted as a clean success.
- `pwt_grade_quadratic_pass`
  The quadratic objective-function grade should still resolve the baseline
  encounter cleanly.
- `pwt_grade_quasi_pass`
  The quasi objective-function grade should still resolve the baseline
  encounter cleanly.
- `use_refinery_pass`
  The refinery branch should still produce a safe resolving behavior.
- `contact_type_required_absent_pass`
  The legacy `contact_type_required` alias should filter out a non-matching
  off-lane contact and allow clean idle transit.
- `no_extrapolate_pass`
  Disabling extrapolation should still work while the contact report is fresh.
- `no_alert_request_fail`
  The behavior is told not to request an alert while the contact remains
  challenging. The mission should fail because the avoid behavior never
  engages and the required resolution path does not complete.
- `bad_pwt_inner_dist_fail`
  Invalid inner/outer relevance ordering should grade fail.
- `bad_pwt_outer_dist_fail`
  Invalid outer/inner relevance ordering should grade fail.
- `bad_min_util_cpa_dist_fail`
  Invalid minimum/maximum CPA utility ordering should grade fail.
- `bad_max_util_cpa_dist_fail`
  Invalid maximum/minimum CPA utility ordering should grade fail.
- `bad_pwt_grade_fail`
  Unsupported `pwt_grade` values should grade fail.
- `bad_completed_dist_fail`
  Negative `completed_dist` should grade fail.
- `bad_time_on_leg_fail`
  Negative inherited `time_on_leg` should grade fail.
- `bad_decay_fail`
  Malformed inherited contact decay should grade fail.
- `bad_collision_depth_fail`
  Depth-specific collision settings should grade fail in this 2D domain.

## Results

Each mission run writes one line to `results.txt`.
The harness reads that line, checks the `grade=` field, and compares it to the
expected result for the case.

## Running

```bash
./launch.sh --just_make --nogui 10
./zlaunch.sh 10
```

Logging is minimal by default. Add `--log=full` to either launcher to restore
the stem's original shoreside and vehicle `pLogger` configuration.
