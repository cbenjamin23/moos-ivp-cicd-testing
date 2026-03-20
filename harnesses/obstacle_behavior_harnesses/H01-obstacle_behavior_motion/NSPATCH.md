# NSPATCH in H01-obstacle_behavior_motion

This harness uses `nspatch` to create temporary overlay files before each
case:

- `meta_shoreside.moosx`
- `meta_vehicle.moosx`
- `meta_vehicle.bhvx`

The stem files live in
[`missions/obstacle_behavior_missions/obstacle_behavior_motion`](../../../missions/obstacle_behavior_missions/obstacle_behavior_motion).
The patch files in this harness replace whole named blocks in those stems.

The key idea is:

- the stem mission owns the default behavior shape
- the harness owns case-specific tweaks
- the final runnable targ files are generated from the stem plus the overlay
  files created by `nspatch`

In this suite the common patch pattern is:

- `*.xmoos` patches for `uTimerScript`, `pMissionEval`, or obstacle geometry
- `*.xbhv` patches for `BHV_AvoidObstacleV24` block changes
- the harness clears any old overlays before each case so there is no leakage
  from one run to the next

The most important behavior-specific patches are:

- `rng_flag = <... AVOID_RANGE_SEEN=true`
  proves the behavior can post range-threshold flags
- `cpa_flag = AVOID_CPA_SEEN=true`
  proves the behavior can post CPA-event flags
- altered `condition = AVOID = ...`
  proves the behavior can be effectively disabled without changing the rest of
  the mission stack

Geometry patches are kept separate in `.xmoos` files so the behavior parameter
changes and the obstacle layout changes remain independent.
