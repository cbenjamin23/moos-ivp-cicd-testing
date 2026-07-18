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

## Stem Mode Inventory

The shared `--mmod` inventory should be understood in four buckets:

- canonical reusable modes
  - stock two-vessel topologies that are already good shared sources for `H01`
    canonicals and for `H02-H04` overlays
- threshold-supporting modes
  - stock topologies that primarily exist because `H02` needs a clean source
    threshold cut, not because they are default H01 canonicals
- manual or calibration probes
  - topologies kept for source study or future harness work, but not yet part
    of a supported default harness gate
- legacy bad-fit modes
  - earlier stem modes that turned out to be the wrong shared source for a
    desired branch and should not be treated as authoritative building blocks

Practical ownership:

- `H01` should prefer canonical reusable modes
- `H02` can use canonical reusable modes or threshold-supporting modes
- `H03` and `H04` should usually reuse the same canonical or threshold stem and
  change only evaluation or parameters
- manual or calibration probes are allowed in the stem, but they are not
  automatically trusted as harness inputs

## When To Add A New `--mmod`

Adding a new `--mmod` in this shared stem is the intended extension path. It
is not the same thing as creating a new stem mission.

Reuse an existing `--mmod` when:

- the encounter topology is the same
- the harness only needs a different threshold-side assertion
- the difference is a small geometry nudge, not a behavior-parameter overlay

Add a new `--mmod` when:

- the source branch requires a genuinely different encounter topology
- the contact trajectory family is different, not just the threshold value
- reusing an old mode keeps producing the wrong source mode family
- the existing mode has become a known bad fit for the intended branch

This is the right interpretation for `standon:unsure_stern`. The source branch
exists in
[BHV_AvdColregsV22.cpp](https://github.com/moos-ivp/moos-ivp/blob/main/ivp/src/lib_behaviors-colregs/BHV_AvdColregsV22.cpp#L1009),
but the original reused horizontal stem mode has not proven to be the right
shared source. That family needs a dedicated stern-side trajectory mode rather
than more attempts to force the old one.

## COLREGS Index Meanings

`BHV_AvdColregsV22` posts `COLREGS_AVOID_IX_<CONTACT>` as a compact numeric
classification code. The harnesses use these values because they come directly
from the upstream source in
[BHV_AvdColregsV22.cpp](https://github.com/moos-ivp/moos-ivp/blob/main/ivp/src/lib_behaviors-colregs/BHV_AvdColregsV22.cpp).

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

The stem also now bridges `COLREGS_AVOID_PWT_<CONTACT>` to shoreside. H04 uses
that posted avoidance weight for parameter families like `pwt_inner_dist` and
`pwt_grade`, where the source effect is relevance shaping rather than a clean
mode-entry split.

The stem also now bridges refinery diagnostics per ownship:
- `PLATEAU_CHECK_OK_<vname>`
- `BASIN_CHECK_OK_<vname>`
- `PLATEAU_LOGIC_CASE_<vname>`

H04 uses those bridged variables to make `use_refinery` a real mission-visible
diagnostic family instead of only a CPA stability check.

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
- `overtaking_thresh_below_pass`
- `overtaking_thresh_edge_pass`
- `overtaking_thresh_above_pass`
- `overtaking_thresh_below_mirror_pass`
- `overtaking_thresh_edge_mirror_pass`
- `overtaking_thresh_above_mirror_pass`
- `crossing_starboard_giveway_pass`
- `crossing_starboard_giveway_far_pass`
- `crossing_starboard_giveway_close_pass`
- `crossing_starboard_giveway_bow_pass`
- `crossing_port_standon_pass`
- `crossing_port_standon_unsure_bow_pass`
- `crossing_port_standon_stern_pass`
- `crossing_port_standon_far_pass`
- `crossing_port_standon_exec_far_pass`
- `crossing_port_standon_far_unsure_bow_pass`
- `crossing_port_standon_close_pass`
- `crossing_port_standon_close_unsure_bow_pass`
- `crossing_port_standon_unsure_pass`
- `crossing_port_standon_southwest_unsure_bow_pass`
- `crossing_port_standon_southwest_unsure_pass`
- `crossing_port_standon_southwest_stern_pass`
- `crossing_port_standon_turn_unsure_stern_below_pass`
- `crossing_port_standon_turn_unsure_stern_edge_pass`
- `crossing_port_standon_turn_unsure_stern_above_pass`
- `crossing_port_standon_unsure_stern_diag_probe`
- `overtaking_starboard_pass`
- `overtaking_starboard_range_far_pass`
- `overtaking_starboard_range_far_small_gap_pass`
- `overtaking_starboard_range_far_large_gap_pass`
- `overtaking_starboard_small_gap_pass`
- `overtaking_starboard_large_gap_pass`
- `overtaking_starboard_mirror_pass`
- `overtaking_starboard_mirror_range_far_pass`
- `overtaking_starboard_mirror_range_far_small_gap_pass`
- `overtaking_starboard_mirror_range_far_large_gap_pass`
- `overtaking_starboard_mirror_small_gap_pass`
- `overtaking_starboard_mirror_large_gap_pass`
- `overtaken_port_standon_pass`
- `overtaken_port_standon_midrange_pass`
- `overtaken_port_standon_range_far_pass`
- `overtaken_port_standon_range_edge_pass`
- `overtaken_port_standon_range_close_pass`
- `overtaken_port_standon_cpa_wide_pass`
- `overtaken_port_standon_cpa_edge_pass`
- `overtaken_port_standon_cpa_above_pass`
- `overtaken_port_standon_gate_below_pass`
- `overtaken_port_standon_gate_edge_pass`
- `overtaken_port_standon_gate_above_pass`
- `crossing_port_standon_cpa_wide_pass`
- `crossing_port_standon_cpa_edge_pass`
- `crossing_port_standon_cpa_above_pass`
- `crossing_port_standon_band315_unsure_pass`
- `crossing_port_standon_band315_unsure_bow_pass`
- `crossing_port_standon_band315_bow_pass`
- `crossing_port_standon_band270_bow_probe`
- `crossing_port_standon_band270_mid_probe`
- `crossing_port_standon_band270_near_probe`
- `overtaken_starboard_standon_pass`
- `overtaken_starboard_standon_midrange_pass`
- `overtaken_starboard_standon_range_far_pass`
- `overtaken_starboard_standon_gate_below_pass`
- `overtaken_starboard_standon_gate_edge_pass`
- `overtaken_starboard_standon_gate_above_pass`

Notes:
- the shared unit stem currently has stable canonicals for `headon`, `cpa`,
  `giveway:stern`, `standon:bow`, `standon:stern`,
  `standon:unsure_bow`, `standon:unsure`, `standon_ot` on both passing
  sides, and overtaking on both passing sides
- some overtaking/overtaken canonicals are intentionally compact and therefore
  already largely side-resolved at mission start; if a harness needs stronger
  pass-side evidence, prefer a longer-range stem geometry such as
  `crossing_port_standon_exec_far_pass`,
  `overtaking_starboard_range_far_pass`,
  `overtaking_starboard_range_far_small_gap_pass`,
  `overtaking_starboard_range_far_large_gap_pass`,
  `overtaking_starboard_mirror_range_far_pass`,
  `overtaking_starboard_mirror_range_far_small_gap_pass`,
  `overtaking_starboard_mirror_range_far_large_gap_pass`,
  `overtaken_port_standon_midrange_pass`, or
  `overtaken_starboard_standon_midrange_pass`
- `crossing_port_standon_exec_far_pass` is an H03-oriented longer-range
  stand-on sibling kept because the exact H01 far canonical was not
  completion-stable enough for the current execution harness
- `crossing_starboard_giveway_bow_pass` remains useful as a manual/source
  probe, but it has not yet proven stable enough to stay in the default H01
  pass set under repeated serial runs
- `crossing_port_standon_far_unsure_bow_pass` remains useful as a manual/source
  probe, but it can degrade into `cpa` in isolated runs and is therefore not
  part of the default H01 pass set
- `crossing_port_standon_unsure_stern_pass` is now treated as a legacy bad-fit
  stem mode. It was an early attempt to reuse the horizontal stand-on family
  for the `standon:unsure_stern` branch, but source-backed runtime traces have
  not shown it producing `34`
- `crossing_port_standon_unsure_stern_diag_probe` is the first dedicated
  stern-side trajectory probe for that branch. It is manual-only for now: it is
  a better encounter topology than the legacy horizontal mode, but it is not
  yet a supported H02 threshold case
- the shared stem now has a turn-driven Band #1 `unsure_stern` probe family:
  `crossing_port_standon_turn_unsure_stern_below_pass`,
  `crossing_port_standon_turn_unsure_stern_edge_pass`, and
  `crossing_port_standon_turn_unsure_stern_above_pass`
- the shared stem now also has a dedicated turn-based CPA pocket:
  `crossing_port_standon_cpa_edge_pass` and
  `crossing_port_standon_cpa_above_pass`
- `crossing_port_standon_cpa_wide_pass` remains the stable stern-side anchor
  for the stand-on CPA below case if H02 needs to revisit the wider offset
- the shared stem now also has a supported Band #2 (`315 < rel_bng <= 350`)
  family:
  `crossing_port_standon_band315_unsure_pass`,
  `crossing_port_standon_band315_unsure_bow_pass`, and
  `crossing_port_standon_band315_bow_pass`
- the shared stem now also has a Band #3 (`270 <= rel_bng <= 315`)
  probe family kept for calibration:
  `crossing_port_standon_band270_bow_probe`,
  `crossing_port_standon_band270_mid_probe`, and
  `crossing_port_standon_band270_near_probe`
- H02 uses the stern anchor for CPA-below and the turn-based pocket for
  CPA edge/above so the split stays geometry-backed instead of reusing the
  same stern probe for every label
- the H02 overtaken-entry threshold aliases are geometry-only and keep the
  stock utility defaults fixed, rather than changing `min_util_cpa_dist` or
  `max_util_cpa_dist`
- that family is the first source-backed stem geometry in this repo to hit the
  real `standon:unsure_stern` (`34`) branch; it avoids the old dead-end
  straight-line geometry by starting just outside the head-on window and then
  turning into the stern-crossing branch
- repeated cold-start runs still split between early `34` hits and later
  `complete`, so this family remains manual-only for now even though it is the
  best current source study direction
- the shared stem now also has a low-speed moving stern probe:
  `crossing_port_standon_speed_unsure_stern_probe`
- that probe keeps BEN on a straight `244`-degree course with a reduced stock
  speed (`0.7`) and can hit `standon:unsure_stern` after BEN is already moving
  under the stock source
- repeated stock-overlay reruns are still not stable enough for CI promotion:
  the same cold-start geometry still splits across `standon:stern` (`30`),
  `standon:unsure_stern` (`34`), and occasional `cpa` (`50`), and a delayed
  deploy overlay did not cleanly stabilize it
- a later isolated rerun sweep around the apparent best pocket
  `B_POS=-5,-86,244` showed that the cleaner-looking edge candidate was not
  reproducible under hard-clean restarts: nearby `241/0.69`, `244/0.69`,
  `244/0.70`, and `245/0.69` variants mostly stayed in `none` with only
  intermittent `34` hits
- delayed `AVOID_ALL` activation was also tested on the moving speed probe so
  the encounter could enter the narrow stern-side geometry before COLREGS mode
  selection began, but `6`, `8`, and `10` second activation delays also stayed
  effectively manual-only and did not yield a stable gateable family
- the shared stem now also has a supported southwest-track stern-adjacent
  family:
  `crossing_port_standon_southwest_unsure_bow_pass`,
  `crossing_port_standon_southwest_unsure_pass`, and
  `crossing_port_standon_southwest_stern_pass`
- that southwest family remains honest supported coverage for stern-adjacent
  `unsure_bow -> unsure -> stern` progression under the stock source, but it is
  still the only supported stern-side family in the current H02 gate
- the shared unit stem now also has stable timing/offset breadth within those
  families, so the harness can test canonical geometry variation without
  leaving the default source-backed mode family
- `overtaken_port_standon_range_edge_pass` is the H02 range-edge pocket;
  `overtaken_port_standon_range_close_pass` is the above pocket
- the first overtaking threshold cut now also shows that falling out of the
  overtaking aft-sector does not necessarily mean a handoff to `cpa`; on the
  upper-side threshold the same closing geometry can hand off to `giveway:bow`
- the shared stem now also has a supported stock turn-gap threshold family:
  `giveway_turngap_below_pass`, `giveway_turngap_edge_pass`,
  `giveway_turngap_above_pass`, and the 180-degree mirror variants
- that family uses a diagonal `115 / 330` give-way geometry instead of the
  earlier orthogonal bad-fit probe, so the first useful Rule 16
  classification lands with `xcn_bow_dist < 10` while `xcn_turn_gap` walks
  the real stock `< 10` turn-gap cutoff
- the first stand-on `unsure_bow` threshold cut is best treated as an
  early-phase progression family under this shared stem: the clean current
  representative cases hit `standon:unsure`, `standon:unsure_bow`, and
  `standon:bow` as first useful stand-on classifications rather than holding
  those modes as terminal end states
- `giveway:bow_must` appears unreachable in the current upstream source:
  `bow_must` is indexed in
  [BHV_AvdColregsV22.cpp](https://github.com/moos-ivp/moos-ivp/blob/main/ivp/src/lib_behaviors-colregs/BHV_AvdColregsV22.cpp)
  but is not assigned anywhere in the mode-selection logic
- `standon_ot` is a real source-defined mode family, but it does not currently
  receive a dedicated nonzero `COLREGS_AVOID_IX`; harnesses should assert on
  the posted mode string for those cases rather than treating `0` as
  ambiguous failure

Typical runs:

```bash
./launch.sh --just_make --nogui 10
./launch.sh --mmod=crossing_starboard_giveway_pass --nogui 10
./zlaunch.sh 10
```

Logging is minimal by default and does not launch `pLogger`. Use `--log=full`
with `launch.sh` or `zlaunch.sh` to restore the pre-migration logging topology
for every consumer of this shared stem.
