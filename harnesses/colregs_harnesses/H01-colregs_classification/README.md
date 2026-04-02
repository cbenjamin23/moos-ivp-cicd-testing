# H01-colregs_classification

TLDR:
- shared two-vessel classification harness on the `colregs_unit` stem
- checks canonical COLREGS mode/index selection before broader execution or parameter work
- current implemented cases: 22 stable canonicals spanning head-on, CPA fallback, give-way stern, stand-on bow/stern/unsure_bow/unsure plus overtaken-vessel stand-on, and overtaking on both passing sides

This harness is the first implemented COLREGS correctness suite. It uses the
shared two-vessel `colregs_unit` stem mission and focuses on classification
correctness before broader motion or performance work.

Status:
- current implemented canonical cases:
  - `head_on_colregs_pass`
  - `head_on_cpa_fallback_pass`
  - `head_on_port_offset_pass`
  - `head_on_starboard_offset_pass`
  - `crossing_starboard_giveway_pass`
  - `crossing_starboard_giveway_far_pass`
  - `crossing_starboard_giveway_close_pass`
  - `crossing_port_standon_pass`
  - `crossing_port_standon_unsure_bow_pass`
  - `crossing_port_standon_stern_pass`
  - `crossing_port_standon_far_pass`
  - `crossing_port_standon_close_pass`
  - `crossing_port_standon_close_unsure_bow_pass`
  - `crossing_port_standon_unsure_pass`
  - `overtaking_starboard_pass`
  - `overtaking_starboard_small_gap_pass`
  - `overtaking_starboard_large_gap_pass`
  - `overtaking_starboard_mirror_pass`
  - `overtaking_starboard_mirror_small_gap_pass`
  - `overtaking_starboard_mirror_large_gap_pass`
  - `overtaken_port_standon_pass`
  - `overtaken_starboard_standon_pass`
- current harness model: case-owned shoreside `nspatch` overlays
- current wrapper support: serial or wave-batched execution with `--jobs=<n>`
- long-term role: canonical classification suite for `BHV_AvdColregsV22`

## Case Guide

- `head_on_colregs_pass`: clean symmetric head-on encounter that should classify as true COLREGS head-on.
- `head_on_cpa_fallback_pass`: near-head-on geometry intentionally outside the head-on bucket so the app should fall back to CPA mode.
- `head_on_port_offset_pass`: head-on encounter with a left-right offset, still expected to stay in the head-on family.
- `head_on_starboard_offset_pass`: mirrored offset head-on encounter, also expected to stay in the head-on family.
- `crossing_starboard_giveway_pass`: canonical starboard crossing where ownship should be the give-way vessel.
- `crossing_starboard_giveway_far_pass`: same starboard crossing family with a longer setup and more spacing.
- `crossing_starboard_giveway_close_pass`: same starboard crossing family with a tighter setup and earlier interaction.
- `crossing_port_standon_pass`: canonical port-side crossing where ownship should stand on and settle to the bow submode.
- `crossing_port_standon_unsure_bow_pass`: port-side crossing that first enters the stand-on `unsure_bow` branch before later resolving.
- `crossing_port_standon_stern_pass`: port-side crossing that settles to the stand-on stern branch.
- `crossing_port_standon_far_pass`: longer-spacing version of the port-side stand-on bow case.
- `crossing_port_standon_close_pass`: tighter-spacing version of the port-side stand-on bow case.
- `crossing_port_standon_close_unsure_bow_pass`: tighter-spacing port-side case that still first enters `unsure_bow`.
- `crossing_port_standon_unsure_pass`: port-side crossing that first enters the generic stand-on `unsure` branch.
- `overtaking_starboard_pass`: canonical overtaking geometry on one passing side.
- `overtaking_starboard_small_gap_pass`: overtaking with a smaller lateral gap to confirm the same overtaking family still holds.
- `overtaking_starboard_large_gap_pass`: overtaking with a larger lateral gap to confirm the same overtaking family still holds.
- `overtaking_starboard_mirror_pass`: mirrored overtaking geometry on the opposite passing side.
- `overtaking_starboard_mirror_small_gap_pass`: mirrored overtaking with a smaller lateral gap.
- `overtaking_starboard_mirror_large_gap_pass`: mirrored overtaking with a larger lateral gap.
- `overtaken_port_standon_pass`: ownship is the vessel being overtaken, with the contact overtaking on ownship's port side.
- `overtaken_starboard_standon_pass`: mirrored overtaken-vessel case with the contact overtaking on ownship's starboard side.

Primary intent:
- prove the correct COLREGS mode is selected for a given encounter geometry
- prove the behavior stays collision-free in the canonical two-vessel cases
- record geometry in a way that makes manual runs visually intuitive

## How This Harness Works

- the shared stem mission supplies the geometry and manual `--mmod` path
- this harness applies case-owned shoreside `nspatch` overlays
- each case passes when the expected live COLREGS classification appears
  without collision or timeout

For example:
- `head_on_colregs_pass` expects `COLREGS_AVOID_IX_BEN = 10`
- `head_on_cpa_fallback_pass` expects `COLREGS_AVOID_IX_BEN = 50`
- `head_on_port_offset_pass` expects `COLREGS_AVOID_IX_BEN = 10`
- `head_on_starboard_offset_pass` expects `COLREGS_AVOID_IX_BEN = 10`
- `crossing_starboard_giveway_pass` expects `COLREGS_AVOID_IX_BEN = 20`
- `crossing_port_standon_pass` expects `COLREGS_AVOID_IX_BEN = 32`
- `crossing_port_standon_unsure_bow_pass` expects `COLREGS_AVOID_IX_BEN = 36`
- `crossing_port_standon_stern_pass` expects `COLREGS_AVOID_IX_BEN = 30`
- `crossing_port_standon_unsure_pass` expects `COLREGS_AVOID_IX_BEN = 38`
- `overtaking_starboard_pass` expects `COLREGS_AVOID_IX_BEN = 43`
- `overtaking_starboard_mirror_pass` expects `COLREGS_AVOID_IX_BEN = 47`
- `overtaken_port_standon_pass` expects `COLREGS_AVOID_MODE_BEN = standon_ot:port`
- `overtaken_starboard_standon_pass` expects `COLREGS_AVOID_MODE_BEN = standon_ot:starboard`

Those numbers come directly from the upstream `BHV_AvdColregsV22` source
mapping, documented in the shared
[colregs_unit README](/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/colregs_missions/colregs_unit/README.md).

Current assertions:
- `COLREGS_AVOID_MODE_<CONTACT>`
- `COLREGS_AVOID_IX_<CONTACT>`
- `COLREGS_SUMMARY_<CONTACT>`
- `COLLISION_TOTAL = 0`
- `MISSION_TIMEOUT = false`
- no explicit encounter-count dependency

Current expected mode index checks:
- `head_on_colregs_pass` -> `COLREGS_AVOID_IX_BEN = 10`
- `head_on_cpa_fallback_pass` -> `COLREGS_AVOID_IX_BEN = 50`
- `head_on_port_offset_pass` -> `COLREGS_AVOID_IX_BEN = 10`
- `head_on_starboard_offset_pass` -> `COLREGS_AVOID_IX_BEN = 10`
- `crossing_starboard_giveway_pass` -> `COLREGS_AVOID_IX_BEN = 20`
- `crossing_starboard_giveway_far_pass` -> `COLREGS_AVOID_IX_BEN = 20`
- `crossing_starboard_giveway_close_pass` -> `COLREGS_AVOID_IX_BEN = 20`
- `crossing_port_standon_pass` -> `COLREGS_AVOID_IX_BEN = 32`
- `crossing_port_standon_unsure_bow_pass` -> `COLREGS_AVOID_IX_BEN = 36`
- `crossing_port_standon_stern_pass` -> `COLREGS_AVOID_IX_BEN = 30`
- `crossing_port_standon_far_pass` -> `COLREGS_AVOID_IX_BEN = 32`
- `crossing_port_standon_close_pass` -> `COLREGS_AVOID_IX_BEN = 32`
- `crossing_port_standon_close_unsure_bow_pass` -> `COLREGS_AVOID_IX_BEN = 36`
- `crossing_port_standon_unsure_pass` -> `COLREGS_AVOID_IX_BEN = 38`
- `overtaking_starboard_pass` -> `COLREGS_AVOID_IX_BEN = 43`
- `overtaking_starboard_mirror_pass` -> `COLREGS_AVOID_IX_BEN = 47`
- `overtaking_starboard_small_gap_pass` -> `COLREGS_AVOID_IX_BEN = 43`
- `overtaking_starboard_large_gap_pass` -> `COLREGS_AVOID_IX_BEN = 43`
- `overtaking_starboard_mirror_small_gap_pass` -> `COLREGS_AVOID_IX_BEN = 47`
- `overtaking_starboard_mirror_large_gap_pass` -> `COLREGS_AVOID_IX_BEN = 47`
- `overtaken_port_standon_pass` -> `COLREGS_AVOID_MODE_BEN = standon_ot:port`
- `overtaken_starboard_standon_pass` -> `COLREGS_AVOID_MODE_BEN = standon_ot:starboard`

Current visual/manual-run instrumentation:
- draw the ownship and contact tracks in the viewer
- show the contact leg or waypoint line so the expected encounter is obvious
- show enough geometry markers to make port/starboard pass side intuitive
- keep geometry variation within stable source-backed families rather than
  padding the harness with cases that never actually classify

Next planned case groups:
- additional head-on offsets if they remain meaningfully distinct from the current center/offset trio
- dynamic stand-on progression states such as `unsure_stern`, `neither`, and `inextremis` if they can be made source-stable without turning H01 into a threshold suite
- all-clear completion and broader non-COLREGS fallback families

Current known gaps:
- `crossing_starboard_giveway_bow_pass` remains available as a manual exploratory
  case, but it is intentionally not part of the default H01 pass set because
  repeated serial runs showed it can fall into `giveway:stern` or release to
  `complete` depending on startup timing under the shared stem
- `crossing_port_standon_far_unsure_bow_pass` remains available as a manual
  exploratory case, but it is intentionally not part of the default H01 pass
  set because repeated isolated runs can fall through to `cpa` instead of
  holding a stable `standon:unsure_bow` classification under the shared stem
- `giveway:bow_must` appears unreachable in the current upstream source path
- `standon_ot` is covered by mode-string assertions because the current source
  does not assign a dedicated nonzero `COLREGS_AVOID_IX` to that mode family
- the stand-on uncertainty family now has stable `unsure_bow` and `unsure` coverage
- `unsure_stern`, `neither`, and `inextremis` remain intentionally outside the
  current canonical H01 cut because they are more transition-sensitive than the
  source-stable center-of-bin cases above

Typical runs:

```bash
./zlaunch.sh 10
./zlaunch.sh --jobs=2 10
./zlaunch.sh --case=head_on_colregs_pass 10
./zlaunch.sh --case=head_on_cpa_fallback_pass 10
./zlaunch.sh --case=overtaking_starboard_mirror_pass 10
```
