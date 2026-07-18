# NSPATCH in P01-obstacle_gauntlet

This harness uses `nspatch` more narrowly than the correctness suites.

The mission stem in
[`missions/performance_missions/P01-obstacle_gauntlet`](../../../missions/performance_missions/P01-obstacle_gauntlet)
still owns the common loop-course environment and scenario construction through
`--scenario`. The harness owns each case's explicit evaluation overlay.

Current split:

- stem scenario selector:
  - `baseline_field`, `dense_field`, or `endurance_random`
  - selects fixed versus generated obstacle data and endurance launch behavior
- harness overlays:
  - one explicitly mapped shoreside patch per case
  - fixed cases replace the `pMissionEval` block
  - the endurance patch also replaces `uTimerScript` and `uFldObstacleSim`

Why this hybrid model:

- scenario construction includes files and launch-time behavior, so it remains
  a human-facing mission argument rather than being forced into a MOOS patch
- case-specific grading stays visible in the harness-owned patches

The launchers in the mission stem are run with `nsplug -x`, so when the harness
creates `meta_shoreside.moosx`, the generated targ file automatically uses the
overlay without changing ordinary manual launch behavior.
