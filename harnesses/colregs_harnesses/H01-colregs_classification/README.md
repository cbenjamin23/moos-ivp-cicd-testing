# H01-colregs_classification

TLDR:
- shared two-vessel classification harness on the `colregs_unit` stem
- checks canonical COLREGS mode/index selection before broader execution or parameter work
- does not claim maneuver safety; the H03 execution harness owns CPA, completion, side, crossing, and collision outcomes
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
- current wrapper support: isolated serial execution or Bash 5.1 rolling
  execution with `--jobs=<n>`, distinct two-vehicle port blocks, and
  root-scoped teardown
- long-term role: canonical classification suite for `BHV_AvdColregsV22`

## Cases

- `head_on_colregs_pass`: Sends equal-speed vessels directly toward one another on the `x=0` centerline, with ownship starting at `(0,-40)` on heading 180 and Ben at `(0,-130)` on heading 0. Passes as soon as `COLREGS_AVOID_IX_BEN=10` appears before the 70-second missing-classification deadline.
- `head_on_cpa_fallback_pass`: Sends ownship south from `(0,-70)` while Ben approaches from `(0,-120)` on heading 28, placing the encounter outside the head-on angular bucket. Passes as soon as CPA index 50 appears before the 70-second missing-classification deadline.
- `head_on_port_offset_pass`: Starts ownship at `(-6,-40)` heading south and Ben at `(6,-130)` heading north, with their destinations crossing the 12-meter lateral offset. Passes as soon as head-on index 10 appears before the 70-second missing-classification deadline.
- `head_on_starboard_offset_pass`: Mirrors the offset geometry with ownship at `(6,-40)` and Ben at `(-6,-130)`. Passes as soon as head-on index 10 appears before the 70-second missing-classification deadline.
- `crossing_starboard_giveway_pass`: Sends ownship east from `(-70,-80)` at 1.4 m/s while Ben moves north from `(10,-130)` at 1.3 m/s, placing Ben on ownship's starboard side. Passes as soon as give-way-stern index 20 appears before the 70-second missing-classification deadline.
- `crossing_starboard_giveway_far_pass`: Moves the same crossing starts outward to ownship `(-80,-80)` and Ben `(10,-150)`. Passes as soon as give-way-stern index 20 appears before the 70-second missing-classification deadline.
- `crossing_starboard_giveway_close_pass`: Moves the same crossing starts inward to ownship `(-60,-80)` and Ben `(10,-115)`. Passes as soon as give-way-stern index 20 appears before the 70-second missing-classification deadline.
- `crossing_port_standon_pass`: Sends ownship east from `(-70,-80)` while Ben moves south from `(10,-20)`, placing Ben on ownship's port side. Passes when the evolving encounter reaches stand-on-bow index 32 before the 70-second missing-classification deadline.
- `crossing_port_standon_unsure_bow_pass`: Uses the same geometry as `crossing_port_standon_pass` but grades the earlier stand-on transition. Passes as soon as `standon:unsure_bow` index 36 appears before the 70-second missing-classification deadline.
- `crossing_port_standon_stern_pass`: Starts Ben farther north at `(10,15)` and slower at 1.2 m/s against eastbound ownship at `(-70,-80)`. Passes as soon as stand-on-stern index 30 appears before the 70-second missing-classification deadline.
- `crossing_port_standon_far_pass`: Maps to the execution-far geometry: ownship starts eastbound at `(-76,-80)` and Ben southbound at `(10,-12)`. Passes when the encounter reaches stand-on-bow index 32 before the 70-second missing-classification deadline.
- `crossing_port_standon_close_pass`: Moves the port-crossing starts inward to ownship `(-60,-80)` and Ben `(10,-30)`. Passes when the encounter reaches stand-on-bow index 32 before the 70-second missing-classification deadline.
- `crossing_port_standon_close_unsure_bow_pass`: Uses the same close geometry as `crossing_port_standon_close_pass` but grades its earlier transition. Passes as soon as `standon:unsure_bow` index 36 appears before the 70-second missing-classification deadline.
- `crossing_port_standon_unsure_pass`: Offsets ownship's eastbound track to `y=-72`, starting at `(-55,-72)`, while Ben moves south from `(10,-18)`. Passes as soon as generic `standon:unsure` index 38 appears before the 70-second missing-classification deadline.
- `overtaking_starboard_pass`: Sends 1.6 m/s ownship east from `(-90,-80)` behind 1.0 m/s Ben at `(-35,-84)`, a four-meter pass on Ben's port side. Passes as soon as `overtaking:port` index 43 appears before the 70-second missing-classification deadline.
- `overtaking_starboard_small_gap_pass`: Narrows the same-side overtaking gap to two meters by placing Ben on `y=-82`. Passes as soon as `overtaking:port` index 43 appears before the 70-second missing-classification deadline.
- `overtaking_starboard_large_gap_pass`: Widens the same-side overtaking gap to eight meters by placing Ben on `y=-88`. Passes as soon as `overtaking:port` index 43 appears before the 70-second missing-classification deadline.
- `overtaking_starboard_mirror_pass`: Mirrors the four-meter geometry with ownship on `y=-84` and Ben on `y=-80`. Passes as soon as `overtaking:starboard` index 47 appears before the 70-second missing-classification deadline.
- `overtaking_starboard_mirror_small_gap_pass`: Narrows the mirrored overtaking gap to two meters with ownship on `y=-82`. Passes as soon as `overtaking:starboard` index 47 appears before the 70-second missing-classification deadline.
- `overtaking_starboard_mirror_large_gap_pass`: Widens the mirrored overtaking gap to eight meters with ownship on `y=-88`. Passes as soon as `overtaking:starboard` index 47 appears before the 70-second missing-classification deadline.
- `overtaken_port_standon_pass`: Sends 1.0 m/s ownship east from `(-35,-80)` while 1.6 m/s Ben overtakes from `(-70,-76)` on ownship's port side. Passes as soon as `COLREGS_AVOID_MODE_BEN=standon_ot:port` appears before the 70-second missing-classification deadline.
- `overtaken_starboard_standon_pass`: Mirrors the overtaken geometry with Ben starting at `(-70,-84)` on ownship's starboard side. Passes as soon as `COLREGS_AVOID_MODE_BEN=standon_ot:starboard` appears before the 70-second missing-classification deadline.

Primary intent:
- prove the correct COLREGS mode is selected for a given encounter geometry
- stop at the first expected classification; completed-maneuver safety belongs to `H03-colregs_execution`
- record geometry in a way that makes manual runs visually intuitive

## How This Harness Works

- the shared stem mission supplies the geometry and manual `--mmod` path
- this harness applies case-owned shoreside `nspatch` overlays
- each case passes when the expected live COLREGS classification appears
  before the 70-second deadline
- the deadline is only the failure path when the expected classification never appears

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
[colregs_unit README](../../../missions/colregs_missions/colregs_unit/README.md).

Current assertions:
- the exact expected `COLREGS_AVOID_MODE_<CONTACT>` or `COLREGS_AVOID_IX_<CONTACT>`
- `MISSION_TIMEOUT = false`

Diagnostic report columns only:
- `COLREGS_SUMMARY_<CONTACT>` and the observed mode/index
- range and contact-relative geometry
- arrival, encounter, near-miss, and collision totals

H01 intentionally has no completion, CPA, encounter-count, or collision
assertion. Those are evaluated by `H03-colregs_execution`, after the encounter
has had time to unfold.

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
- the shared mission retains developer calibration geometries for
  `giveway:bow` and a far-range `standon:unsure_bow` transition, but H01 does
  not expose them as harness cases because neither has a repeatable
  classification contract
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
./zlaunch.sh --jobs=2 --port_base=22000 10
./zlaunch.sh --case=head_on_colregs_pass 10
./zlaunch.sh --case=head_on_cpa_fallback_pass 10
./zlaunch.sh --case=overtaking_starboard_mirror_pass 10
```

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case.
