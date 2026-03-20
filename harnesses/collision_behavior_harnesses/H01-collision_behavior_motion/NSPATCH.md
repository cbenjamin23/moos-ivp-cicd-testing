# NSPATCH in H01-collision_behavior_motion

This harness uses `nspatch` to create temporary overlay files before each
case:

- `meta_shoreside.moosx`
- `meta_vehicle.moosx`
- `meta_vehicle.bhvx`

The stem files live in
[`missions/collision_behavior_missions/collision_behavior_motion`](../../../missions/collision_behavior_missions/collision_behavior_motion).
The patch files in this harness replace whole named `ProcessConfig` blocks in
those stems.

The key idea is:

- the stem mission owns the default behavior shape
- the harness owns case-specific tweaks
- the final runnable targ files are generated from the stem plus the overlay
  files created by `nspatch`

In this suite the common patch pattern is:

- `*.xmoos` patches for `uTimerScript`, `pMissionEval`, or other MOOS blocks
- `*.xbhv` patches for `BHV_AvoidCollision` block changes
- the harness clears any old overlays before each case so there is no leakage
  from one run to the next

The most important behavior-specific patches are:

- `no_alert_request=true`
  proves the behavior can be suppressed at the source
- `post_per_contact_info=true`
  proves the behavior can post the extra per-contact range and closing-speed
  outputs
- `match_type=...`
  proves the behavior-owned filter path works and can prevent spawning even
  when the contact itself is present

Case geometry is changed separately in `uTimerScript` patches when needed.
That keeps the behavior parameters and the motion geometry independent, which
is the cleanest way to test `BHV_AvoidCollision` itself.
