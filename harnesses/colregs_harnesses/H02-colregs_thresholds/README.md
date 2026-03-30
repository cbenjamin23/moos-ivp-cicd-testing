# H02-colregs_thresholds

TLDR:
- threshold-edge harness on the shared `colregs_unit` stem
- proves that small geometry changes flip classification where the source says they should
- current implemented focus: head-on cutoff and mirrored head-on cutoff

This harness uses the shared `colregs_unit` stem mission to probe
the exact geometry and parameter boundaries that decide COLREGS
classification.

Primary intent:
- catch regressions at the decision boundaries rather than only at canonical
  center-of-bin encounters
- verify that small geometry changes produce the expected mode transition and
  not a spurious or unstable classification

Why a separate suite:
- threshold behavior is where subtle regressions usually hide
- this keeps the classification suite readable while still making the boundary
  coverage explicit

## How This Harness Works

- keep the same shared two-vessel stem
- change only the encounter geometry around a decision boundary
- assert that the live `COLREGS_AVOID_IX_BEN` flips where the source says it
  should

So this harness is where geometry changes are tested most aggressively.

Current implemented focus areas:
- head-on threshold near the relative-bearing cutoff
- mirrored head-on threshold on the opposite pass side
- overtaking threshold near the aft-sector cutoff
- additional threshold families still planned:
  - give-way bow/stern split near `giveway_bow_dist`
  - activation and completion around `pwt_inner_dist`, `pwt_outer_dist`, and
    `completed_dist`
  - CPA utility transitions near `min_util_cpa_dist` and `max_util_cpa_dist`

Planned case pattern:
- `*_below_pass`
- `*_edge_pass`
- `*_above_pass`

Current implemented case list:
- `head_on_thresh_below_pass`
- `head_on_thresh_edge_pass`
- `head_on_thresh_above_pass`
- `head_on_thresh_below_mirror_pass`
- `head_on_thresh_edge_mirror_pass`
- `head_on_thresh_above_mirror_pass`

Current assertion style:
- head-on below expects live `COLREGS_AVOID_IX_BEN = 10`
- head-on edge/above expect transition to `COLREGS_AVOID_IX_BEN = 50`
- mirrored head-on cases reuse the same expected index behavior with the
  contact mirrored to the opposite side of the centerline

Planned assertions:
- expected `COLREGS_AVOID_MODE_<CONTACT>`
- expected `COLREGS_AVOID_IX_<CONTACT>`
- stable no-collision outcome
- no classification oscillation near the threshold
- future overtaking and give-way threshold families will be added after their
  geometry is calibrated against the source logic

Planned visual/manual-run instrumentation:
- draw the encounter track and threshold-sensitive geometry in the viewer
- expose enough geometry cues that a human can see why the case is just below
  or just above a threshold
