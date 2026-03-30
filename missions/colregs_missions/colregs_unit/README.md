# colregs_unit

TLDR:
- shared two-vessel stem for `H01-H04`
- manual `--mmod` is for human-friendly scenario launching
- harnesses still own formal case semantics through `nspatch` overlays

This stem mission is the first COLREGS correctness baseline. It is derived from
`missions-auto/11-colavd` and now supports a small deterministic two-vessel
classification matrix for `BHV_AvdColregsV22`.

This stem is intentionally the canonical two-vessel version of the family and
is the planned shared base for the four unit-oriented correctness suites:
- `H01-colregs_classification`
- `H02-colregs_thresholds`
- `H03-colregs_execution`
- `H04-colregs_parameters`

`colregs_motion` remains the broader multi-vessel integration stem derived from
`08-colavd-lanes`, so the two stems are meant to stay different rather than be
collapsed into one mission.

The mission auto-deploys, self-evaluates, and writes `results.txt`. Harnesses
own the case semantics through `nspatch`, while the mission owns the common
geometry plumbing and manual `--mmod` launch path.

## How This Stem Is Used

There are two valid ways this stem gets used:

- manual mission runs:
  - launch the stem directly with `--mmod=<case>`
  - good for visual inspection, debugging, and sanity checks
- harness runs:
  - launch the same stem, but apply `nspatch` overlays for case-specific
    evaluation or behavior changes
  - good for repeatable correctness testing

This is intentional. `--mmod` is the manual convenience layer. `nspatch` is
the harness/testing layer. They do not conflict as long as the harness remains
the owner of formal assertions.

## COLREGS Index Meanings

`BHV_AvdColregsV22` posts `COLREGS_AVOID_IX_<CONTACT>` as a compact numeric
classification code. The harnesses use these values because they come directly
from the upstream source in
[BHV_AvdColregsV22.cpp](/Users/charlesbenjamin/moos-ivp/ivp/src/lib_behaviors-colregs/BHV_AvdColregsV22.cpp).

The main values currently used in this repo are:

- `10` = `headon`
- `20` = `giveway:stern`
- `22` = `giveway:bow`
- `25` = `giveway:bow_must`
- `30/31/32/34/36/38/39` = `standon` submodes
- `43/47` = `overtaking` submodes
- `50` = `cpa`

So when a harness says `COLREGS_AVOID_IX_BEN = 20`, it means:
- the case passes only if Ben entered the exact source-defined
  `giveway:stern` classification

That check is not replacing pass/fail. It is the case-specific condition that
defines pass/fail.

Design expectations for this stem:
- keep the geometry simple and comparable across many overlays
- support `nspatch`-driven case variation rather than mission forks
- preserve good visual cues for manual runs, including route/track lines that
  make encounter geometry intuitive

Pass contract:
- `COLLISION_TOTAL = 0`
- `MISSION_TIMEOUT = false`
- case-specific expected `COLREGS_AVOID_IX_BEN` via harness overlay

Implemented manual `--mmod` values:
- `head_on_colregs_pass`
- `head_on_centerline_pass`
- `head_on_cpa_fallback_pass`
- `head_on_port_offset_pass`
- `head_on_starboard_offset_pass`
- `head_on_thresh_below_pass`
- `head_on_thresh_edge_pass`
- `head_on_thresh_above_pass`
- `head_on_thresh_below_mirror_pass`
- `head_on_thresh_edge_mirror_pass`
- `head_on_thresh_above_mirror_pass`
- `crossing_starboard_giveway_pass`
- `crossing_starboard_giveway_far_pass`
- `crossing_starboard_giveway_close_pass`
- `crossing_port_standon_pass`
- `crossing_port_standon_unsure_bow_pass`
- `crossing_port_standon_stern_pass`
- `crossing_port_standon_far_pass`
- `crossing_port_standon_far_unsure_bow_pass`
- `crossing_port_standon_close_pass`
- `crossing_port_standon_close_unsure_bow_pass`
- `crossing_port_standon_unsure_pass`
- `overtaking_starboard_pass`
- `overtaking_starboard_small_gap_pass`
- `overtaking_starboard_large_gap_pass`
- `overtaking_starboard_mirror_pass`
- `overtaking_starboard_mirror_small_gap_pass`
- `overtaking_starboard_mirror_large_gap_pass`

Notes:
- the shared unit stem currently has stable canonicals for `headon`, `cpa`,
  `giveway:stern`, `standon:bow`, `standon:stern`, `standon:unsure_bow`,
  `standon:unsure`, and overtaking on both passing sides
- the shared unit stem now also has stable timing/offset breadth within those
  families, so the harness can test canonical geometry variation without
  leaving the default source-backed mode family
- `giveway:bow_must` appears unreachable in the current upstream source:
  `bow_must` is indexed in
  [BHV_AvdColregsV22.cpp](/Users/charlesbenjamin/moos-ivp/ivp/src/lib_behaviors-colregs/BHV_AvdColregsV22.cpp)
  but is not assigned anywhere in the mode-selection logic
- `giveway:bow` has not yet been promoted into the stable canonical harness
  because the tested default-parameter geometries did not produce a repeatable
  live `COLREGS_AVOID_IX = 22`

Typical runs:

```bash
./launch.sh --just_make --nogui 10
./launch.sh --mmod=crossing_starboard_giveway_pass --nogui 10
./zlaunch.sh 10
```
