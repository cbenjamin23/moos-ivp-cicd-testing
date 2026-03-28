# NSPATCH in P01-obstacle_gauntlet

This harness uses `nspatch` more narrowly than the correctness suites.

The mission stem in
[`missions/performance_missions/P01-obstacle_gauntlet`](../../../missions/performance_missions/P01-obstacle_gauntlet)
still owns the common loop-course environment and the obstacle-source choice
through `--mmod`. The harness owns the one case whose evaluation semantics
materially differ from the stem default.

Current split:

- stem mission default:
  - one-loop fixed-field performance evaluation
  - manual `--mmod` support for `baseline_field_pass`, `dense_field_pass`,
    and `endurance_random_pass`
- harness overlay:
  - `endurance-random-pass-shoreside.xmoos`
  - replaces `uTimerScript`, `pMissionEval`, and `uFldObstacleSim`
  - turns the endurance case into a 10-cycle run with obstacle resets enabled

Why this hybrid model:

- fixed-field cases differ mainly by obstacle data, so `mmod` is a reasonable
  scenario selector there
- endurance differs in mission semantics, not just geometry, so the harness
  owns that change with `nspatch`

The launchers in the mission stem are run with `nsplug -x`, so when the harness
creates `meta_shoreside.moosx`, the generated targ file automatically uses the
overlay without changing ordinary manual launch behavior.
