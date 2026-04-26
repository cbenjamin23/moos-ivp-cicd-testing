# H02-colregs_thresholds

TLDR:
- threshold-edge harness on the shared `colregs_unit` stem
- proves that small geometry changes flip classification where the source says they should
- intended to absorb the unstable edge-space that does not belong in `H01`
- current active gate is a geometry-first 58-case threshold sweep with wave-mode support

This harness uses the shared `colregs_unit` stem mission to probe the exact
geometry boundaries that decide COLREGS classification in
`BHV_AvdColregsV22`.

Primary intent:
- catch regressions at decision boundaries rather than only at canonical
  center-of-bin encounters
- verify that small input changes produce the expected mode transition and not
  a spurious or sticky classification
- take ownership of the edge-space intentionally excluded from `H01`, including
  `giveway:bow`-adjacent and `unsure_bow`-adjacent cases

Why a separate suite:
- threshold behavior is where subtle regressions usually hide
- this keeps `H01` readable and stable while still making boundary coverage
  explicit
- this is the right home for cases that may require synchronized release,
  staged deploy, or tighter telemetry inspection

Developer note:
- use `./zlaunch.sh --group=headon 10`, `--group=overtaking`,
  `--group=overtaken`, `--group=overtaken_mirror`, `--group=giveway`,
  `--group=turngap`, or `--group=standon` to iterate on one family without
  running the full H02 gate
- `./zlaunch.sh --jobs=2 10` runs isolated wave batches with unique port blocks;
  a small set of known-sensitive supported edge cases currently run as solo
  waves, with a short settle pause between waves, to keep the stock geometry
  honest instead of retuning it into different scenarios

## How To Read Case Names

Most H02 case names encode both the source threshold being probed and the side
of the threshold the geometry is expected to land on.

- `*_below_pass`: just below the source cutoff; should stay on the pre-flip side
- `*_edge_pass`: on the currently calibrated stock edge
- `*_above_pass`: just above the cutoff; should land on the post-flip side
- `*_mirror_pass`: same idea as the base case, but rotated or mirrored to prove
  the opposite-side geometry is behaving symmetrically
- `band350`: supported band-specific stand-on cases pinned to the source
  bearing band in `BHV_AvdColregsV22`
- `band270`: the stern reference is currently supported in the active gate;
  the bow/unsure variants remain calibration-only
- `band315`: supported geometry-backed stand-on band #2 cases pinned to the
  source bearing band in `BHV_AvdColregsV22`
- `southwest`: a custom stern-adjacent contact track added after calibration
  showed the older stern probes were a poor fit for the stock source

## Active Case Guide

- `giveway_turngap_edge_pass`
  Probes the calibrated give-way turn-gap edge; expected classification should
  stay on the stock stern-side branch.
- `giveway_turngap_above_pass`
  Probes just beyond the give-way turn-gap edge; expected classification should
  flip to the stock bow-side branch.
- `giveway_turngap_edge_mirror_pass`
  Mirrored turn-gap edge case; expected classification should match the
  non-mirrored edge behavior.
- `giveway_turngap_above_mirror_pass`
  Mirrored turn-gap above-edge case; expected classification should match the
  non-mirrored bow-side behavior.
- `head_on_thresh_below_pass`
  Probes below the head-on entry cutoff; expected classification should remain
  true head-on.
- `head_on_thresh_edge_pass`
  Probes the calibrated head-on entry edge; expected classification should land
  on the calibrated edge behavior.
- `head_on_thresh_above_pass`
  Probes above the head-on entry cutoff; expected classification should fall
  back to CPA.
- `head_on_thresh_below_mirror_pass`
  Mirrored below-threshold head-on case; expected classification should remain
  true head-on.
- `head_on_thresh_edge_mirror_pass`
  Mirrored head-on edge case; expected classification should match the
  non-mirrored edge behavior.
- `head_on_thresh_above_mirror_pass`
  Mirrored above-threshold head-on case; expected classification should fall
  back to CPA.
- `overtaking_thresh_below_pass`
  Probes below the overtaking classification threshold; expected behavior
  should remain on the calibrated pre-flip side.
- `overtaking_thresh_edge_pass`
  Probes the overtaking threshold edge; expected behavior should match the
  calibrated edge branch.
- `overtaking_thresh_above_pass`
  Probes above the overtaking threshold; expected behavior should land on the
  post-flip side.
- `overtaking_thresh_below_mirror_pass`
  Mirrored below-threshold overtaking case; expected behavior should match the
  non-mirrored pre-flip side.
- `overtaking_thresh_edge_mirror_pass`
  Mirrored overtaking edge case; expected behavior should match the calibrated
  edge branch.
- `overtaking_thresh_above_mirror_pass`
  Mirrored above-threshold overtaking case; expected behavior should land on
  the post-flip side.
- `overtaken_thresh_below_pass`
  Probes below the overtaken-vessel threshold; expected behavior should remain
  on the calibrated stand-on overtaken side.
- `overtaken_thresh_edge_pass`
  Probes the overtaken-vessel threshold edge; expected behavior should match
  the calibrated edge branch.
- `overtaken_thresh_above_pass`
  Probes above the overtaken-vessel threshold; expected behavior should land on
  the calibrated fallback side.
- `overtaken_thresh_below_mirror_pass`
  Mirrored below-threshold overtaken-vessel case; expected behavior should
  match the non-mirrored stand-on overtaken side.
- `overtaken_thresh_edge_mirror_pass`
  Mirrored overtaken-vessel edge case; expected behavior should match the
  calibrated edge branch.
- `overtaken_thresh_above_mirror_pass`
  Mirrored above-threshold overtaken-vessel case; expected behavior should land
  on the calibrated fallback side.
- `giveway_bowdist_below_pass`
  Probes below the give-way bow-distance split; expected classification should
  stay in `giveway:stern`.
- `giveway_bowdist_edge_pass`
  Probes the calibrated give-way bow-distance edge; expected classification
  should still stay in `giveway:stern`.
- `giveway_bowdist_above_pass`
  Probes above the give-way bow-distance split; expected classification should
  flip to `giveway:bow`.
- `giveway_bowdist_below_mirror_pass`
  Mirrored below-threshold bow-distance case; expected classification should
  stay in `giveway:stern`.
- `giveway_bowdist_edge_mirror_pass`
  Mirrored bow-distance edge case; expected classification should still stay in
  `giveway:stern`.
- `giveway_bowdist_above_mirror_pass`
  Mirrored above-threshold bow-distance case; expected classification should
  flip to `giveway:bow`.
- `giveway_turngap_below_pass`
  Probes below the give-way turn-gap split; expected classification should stay
  on the stern-side branch.
- `giveway_turngap_below_mirror_pass`
  Mirrored below-threshold turn-gap case; expected classification should stay
  on the stern-side branch.
- `standon_unsurebow_below_pass`
  Probes below the stand-on unsure-bow transition; expected classification
  should remain on the calibrated pre-flip side.
- `standon_unsurebow_edge_pass`
  Probes the stand-on unsure-bow edge; expected classification should match the
  calibrated edge behavior.
- `standon_unsurebow_above_pass`
  Probes above the stand-on unsure-bow transition; expected classification
  should land on the post-flip side.
- `standon_band270_stern_pass`
  Band-270 stand-on geometry expected to resolve to the stern branch.
- `standon_band350_unsurebow_pass`
  Band-350 stand-on geometry expected to remain in the unsure-bow branch.
- `standon_band350_unsurebow_alt_pass`
  Alternate band-350 geometry that should also remain in the unsure-bow branch.
- `standon_band350_bow_pass`
  Band-350 stand-on geometry expected to resolve to the bow branch.
- `standon_band315_unsure_pass`
  Band-315 stand-on geometry expected to enter the generic unsure branch.
- `standon_band315_unsure_bow_pass`
  Band-315 stand-on geometry expected to enter the unsure-bow branch.
- `standon_band315_bow_pass`
  Band-315 stand-on geometry expected to resolve to the bow branch.
- `standon_southwest_unsurebow_pass`
  Southwest stand-on probe expected to enter the unsure-bow branch.
- `standon_southwest_unsure_pass`
  Southwest stand-on probe expected to enter the generic unsure branch.
- `standon_southwest_stern_pass`
  Southwest stand-on probe expected to resolve to the stern branch.
- `outer_dist_below_pass`
  Probes below the COLREGS outer-distance relevance gate; expected behavior
  should remain inactive or fallback as calibrated.
- `outer_dist_edge_pass`
  Probes the outer-distance relevance edge; expected behavior should match the
  calibrated edge branch.
- `outer_dist_above_pass`
  Probes above the outer-distance relevance gate; expected COLREGS behavior
  should activate.
- `standon_inextremis_range_below_pass`
  Probes below the stand-on in-extremis range cutoff; expected classification
  should follow the calibrated range branch.
- `standon_inextremis_range_edge_pass`
  Probes the stand-on in-extremis range edge; expected classification should
  match the calibrated edge branch.
- `standon_inextremis_range_above_pass`
  Probes above the stand-on in-extremis range cutoff; expected classification
  should follow the post-edge range branch.
- `standon_inextremis_cpa_below_pass`
  Probes below the stand-on in-extremis CPA cutoff; expected classification
  should follow the calibrated CPA branch.
- `standon_inextremis_cpa_edge_pass`
  Probes the stand-on in-extremis CPA edge; expected classification should
  match the calibrated edge branch.
- `standon_inextremis_cpa_above_pass`
  Probes above the stand-on in-extremis CPA cutoff; expected classification
  should follow the post-edge CPA branch.
- `standon_ot_inextremis_range_below_pass`
  Probes below the overtaken-vessel in-extremis range cutoff; expected
  classification should follow the calibrated range branch.
- `standon_ot_inextremis_range_edge_pass`
  Probes the overtaken-vessel in-extremis range edge; expected classification
  should match the calibrated edge branch.
- `standon_ot_inextremis_range_above_pass`
  Probes above the overtaken-vessel in-extremis range cutoff; expected
  classification should follow the post-edge range branch.
- `standon_ot_inextremis_cpa_below_pass`
  Probes below the overtaken-vessel in-extremis CPA cutoff; expected
  classification should follow the calibrated CPA branch.
- `standon_ot_inextremis_cpa_edge_pass`
  Probes the overtaken-vessel in-extremis CPA edge; expected classification
  should match the calibrated edge branch.
- `standon_ot_inextremis_cpa_above_pass`
  Probes above the overtaken-vessel in-extremis CPA cutoff; expected
  classification should follow the post-edge CPA branch.

## Scope Boundary

`H01` answers:
- what are the stable source-backed canonical bins?

`H02` answers:
- exactly where do the source thresholds flip?
- do edge cases land on the expected side of the threshold?
- do near-boundary cases remain deterministic enough to be useful as tests?

`H03` answers:
- once classification is correct, does the realized maneuver quality stay
  acceptable?

## Design Rules

- keep the same shared two-vessel stem whenever possible
- change one thing at a time:
  - geometry
  - speed ratio
- prefer `below / edge / above` triplets
- mirror families when port/starboard symmetry matters
- allow `H02` to use stronger fixture control than `H01` when needed:
  - delayed `DEPLOY_ALL`
  - synchronized release after contact establishment
  - short pre-motion hold with simultaneous breakaway

That last point matters. Some threshold cases are not failing because the
harness launches differently from a manual run. They fail because they sit near
real source thresholds and the first accepted evidence can land on either side
of the branch. A staged-release fixture may reduce startup jitter, but it does
not magically convert a threshold case into an `H01` canonical.

## Source-Backed Threshold Families

The current upstream source exposes several distinct threshold classes that
deserve explicit coverage.

### 1. Head-On Entry Cutoff

Purpose:
- verify the relative-bearing cutoff that separates true `headon` from `cpa`

Current implemented cases:
- `head_on_thresh_below_pass`
- `head_on_thresh_edge_pass`
- `head_on_thresh_above_pass`
- `head_on_thresh_below_mirror_pass`
- `head_on_thresh_edge_mirror_pass`
- `head_on_thresh_above_mirror_pass`

Expected behavior:
- below threshold: `COLREGS_AVOID_IX_BEN = 10`
- edge and above: `COLREGS_AVOID_IX_BEN = 50`

Status:
- implemented and passing

### 2. Give-Way Stern vs Bow Split

Purpose:
- verify the Rule 16 submode split between `giveway:stern` and `giveway:bow`
- verify the stock bow-crossing-distance criterion
- verify the stock turn-gap criterion under the shared stem

Why it matters:
- this is the uncovered edge-space behind the manual
  `crossing_starboard_giveway_bow_pass` probe

Current implemented cases:
- `giveway_bowdist_below_pass`
- `giveway_bowdist_edge_pass`
- `giveway_bowdist_above_pass`
- `giveway_bowdist_below_mirror_pass`
- `giveway_bowdist_edge_mirror_pass`
- `giveway_bowdist_above_mirror_pass`
- `giveway_turngap_below_pass`
- `giveway_turngap_edge_pass`
- `giveway_turngap_above_pass`
- `giveway_turngap_below_mirror_pass`
- `giveway_turngap_edge_mirror_pass`
- `giveway_turngap_above_mirror_pass`

What each implemented case means:
- `giveway_bowdist_below_pass`: the contact still crosses far enough astern
  that the stock classifier stays in `giveway:stern`
- `giveway_bowdist_edge_pass`: the calibrated edge case for the stock
  `giveway_bow_dist` cutoff; it still belongs to `giveway:stern`
- `giveway_bowdist_above_pass`: a small geometry change pushes the contact onto
  the bow side and should flip to `giveway:bow`
- `giveway_bowdist_below_mirror_pass`: mirrored version of the below case
- `giveway_bowdist_edge_mirror_pass`: mirrored version of the edge case
- `giveway_bowdist_above_mirror_pass`: mirrored version of the above case
- `giveway_turngap_below_pass`: diagonal give-way case where
  `xcn_bow_dist < 10` but `xcn_turn_gap` is still above the stock cutoff, so
  the classifier stays in `giveway:stern`
- `giveway_turngap_edge_pass`: last calibrated stern-side turn-gap case under
  the supported diagonal family
- `giveway_turngap_above_pass`: same family with `xcn_turn_gap < 10`, which
  should flip to `giveway:bow` while staying below the ordinary bow-distance
  cutoff
- `giveway_turngap_below_mirror_pass`: 180-degree mirror of the below case
- `giveway_turngap_edge_mirror_pass`: 180-degree mirror of the edge case
- `giveway_turngap_above_mirror_pass`: 180-degree mirror of the above case

Expected behavior:
- `giveway_bowdist_below_pass` -> `giveway:stern` (`20`)
- `giveway_bowdist_edge_pass` -> `giveway:stern` (`20`)
- `giveway_bowdist_above_pass` -> `giveway:bow` (`22`)
- `giveway_bowdist_below_mirror_pass` -> `giveway:stern` (`20`)
- `giveway_bowdist_edge_mirror_pass` -> `giveway:stern` (`20`)
- `giveway_bowdist_above_mirror_pass` -> `giveway:bow` (`22`)
- `giveway_turngap_below_pass` -> `giveway:stern` (`20`)
- `giveway_turngap_edge_pass` -> `giveway:stern` (`20`)
- `giveway_turngap_above_pass` -> `giveway:bow` (`22`)
- `giveway_turngap_below_mirror_pass` -> `giveway:stern` (`20`)
- `giveway_turngap_edge_mirror_pass` -> `giveway:stern` (`20`)
- `giveway_turngap_above_mirror_pass` -> `giveway:bow` (`22`)

Status:
- direct `giveway_bow_dist` family now implemented in both the base and
  180-degree rotated mirror geometries
- the stock turn-gap family is now also implemented and supported in both the
  base and mirror geometries

Notes:
- turn-gap values are now carried through `XCN_BOW_TURN_GAP_BEN` in the
  mission and harness reports
- the current stock behavior uses a strict `>` comparison against
  `giveway_bow_dist`, so the edge case currently still belongs to the stern side
- the source turn-gap branch uses a strict `<` comparison, so low turn-gap
  cases belong to `giveway:bow`
- the older orthogonal turn-gap probes were a bad fit for the stock source:
  they entered give-way too early and never got `xcn_turn_gap` near the real
  cutoff before other geometry dominated
- the supported family now uses a diagonal `115 / 330` base geometry and its
  180-degree mirror so the first useful Rule 16 classification lands with
  `xcn_bow_dist < 10` while `xcn_turn_gap` walks the real stock threshold
- the active gate keeps a longer pre-motion hold on this family so the
  turn-gap split is sampled after contact exchange settles under both serial
  and wave-mode validation
- repeated cold starts on the calibrated base family held the split at:
  - below / edge -> `20`
  - above -> `22`
- repeated cold starts on the mirror family held the same split

### 3. Stand-On `unsure_bow` vs `bow` Split

Purpose:
- verify the stand-on progression boundaries driven by crossing distance and
  bearing band

Why it matters:
- this is the uncovered edge-space behind the manual
  `crossing_port_standon_far_unsure_bow_pass` probe

Current implemented cases:
- `standon_unsurebow_below_pass`
- `standon_unsurebow_edge_pass`
- `standon_unsurebow_above_pass`
- `standon_band350_unsurebow_pass`
- `standon_band350_unsurebow_alt_pass`
- `standon_band350_bow_pass`

What each implemented case means:
- `standon_unsurebow_below_pass`: representative near-zero bow-crossing case
  that still belongs to `standon:unsure`
- `standon_unsurebow_edge_pass`: representative zero-line case that now enters
  `standon:unsure_bow`
- `standon_unsurebow_above_pass`: small increase on the bow side that reaches
  `standon:bow`
- `standon_band350_unsurebow_pass`: Band #1 (`rel_bng > 350`) case pinned in
  `standon:unsure_bow`
- `standon_band350_unsurebow_alt_pass`: alternate Band #1 geometry that lands
  in the same branch and helps keep this narrow slice from being under-sampled
- `standon_band350_bow_pass`: Band #1 case pinned in `standon:bow`

Current implemented cases:
- Band #2 (`315 < rel_bng <= 350`) cases pinned in each submode:
  - `standon_band315_unsure_pass`
  - `standon_band315_unsurebow_pass`
  - `standon_band315_bow_pass`
- Band #3 (`270 <= rel_bng <= 315`) cases pinned in each submode:
  - `standon_band270_stern_pass`

Note: earlier drafts of this harness included band270 cases, but they used
axis-aligned geometries (`crossing_port_standon_unsure_pass`,
`crossing_port_standon_close_pass`, `crossing_port_standon_far_pass`) that
were not verified to land in the claimed band. Inspection confirmed they
produced the same `mmod`, the same patch, and the same `colregs_ix` as the
`standon_unsurebow_*` family — making them unverified duplicates. They remain
removed until geometry that genuinely pins each bearing band is calibrated.
The Band #3 stern case was preserved separately (see section 4).

Expected behavior:
- first representative progression family:
  - `standon_unsurebow_below_pass` -> `standon:unsure` (`38`)
  - `standon_unsurebow_edge_pass` -> `standon:unsure_bow` (`36`)
  - `standon_unsurebow_above_pass` -> `standon:bow` (`32`)
- Band #1 cases:
- `standon_band350_unsurebow_pass` -> `standon:unsure_bow` (`36`)
- `standon_band350_unsurebow_alt_pass` -> `standon:unsure_bow` (`36`)
- `standon_band350_bow_pass` -> `standon:bow` (`32`)
- `standon_band315_unsure_pass` -> `standon:unsure` (`38`)
- `standon_band315_unsurebow_pass` -> `standon:unsure_bow` (`36`)
- `standon_band315_bow_pass` -> `standon:bow` (`32`)

Status:
- first representative `unsure -> unsure_bow -> bow` family implemented
- Band #1 family implemented
- Band #2 family implemented
- only additional Band #3 bow/unsure/unsure_bow representatives remain removed
  pending verified geometry

### 4. Stand-On `unsure` vs `stern` and `unsure_stern`

Purpose:
- verify the stern-side stand-on branches that are still uncovered in `H01`

Current implemented supported cases:
- `standon_band270_stern_pass`
- `standon_southwest_unsurebow_pass`
- `standon_southwest_unsure_pass`
- `standon_southwest_stern_pass`

Exploratory cases kept off the supported gate:
- `crossing_port_standon_band270_bow_probe`
- `crossing_port_standon_band270_mid_probe`
- `crossing_port_standon_band270_near_probe`
- turn-driven stem probes:
  - `crossing_port_standon_turn_unsure_stern_below_pass`
  - `crossing_port_standon_turn_unsure_stern_edge_pass`
  - `crossing_port_standon_turn_unsure_stern_above_pass`
- `standon_band350_unsure_stern_pass`

Still-planned cases:
- `standon_band315_stern_pass`: Band #2 stern-side reference case — removed
  pending a geometry that is verified to land in Band #2 (`315 < rel_bng <= 350`)
  at the time of classification. Earlier drafts used an axis-aligned geometry
  that was not confirmed to be in that band.
- `standon_stern_direct_pass`: simple direct stern anchor case — removed because
  it was using the same mmod as the southwest family without offering distinct
  coverage. Will be revisited if a genuinely distinct direct-stern geometry is
  calibrated.

What each supported case means:
- `standon_band270_stern_pass`: Band #3 stern-side reference case; the contact
  approaches from the southwest quadrant and the classifier resolves to
  `standon:stern` (`30`) within the `270 <= rel_bng <= 315` bearing band
- `standon_southwest_unsurebow_pass`: custom southwest-track geometry that
  approaches the stern-side transition but still lands in `standon:unsure_bow`
- `standon_southwest_unsure_pass`: small southwest-track shift that lands in
  `standon:unsure`
- `standon_southwest_stern_pass`: another small southwest-track shift that
  lands in `standon:stern`

What the unsupported exploratory cases are trying to do:
- the turn-driven `crossing_port_standon_turn_unsure_stern_*` family is the
  first source-backed geometry in this repo that can hit the real
  `standon:unsure_stern` (`34`) branch
- the intended progression in that family is:
  - below probe -> `standon:unsure` (`38`)
  - edge probe -> `standon:unsure_stern` (`34`)
  - above probe -> `standon:stern` (`30`)
- repeated cold-start runs are still not stable enough for CI promotion:
  the current edge candidates only hit `34` intermittently and otherwise fall
  through to later `complete`
- the new manual-only `crossing_port_standon_speed_unsure_stern_probe`
  establishes that `34` is also reachable after BEN is already moving, not
  only in the earlier zero-speed startup window
- the best moving pocket found so far is roughly `B_POS=-6,-86,244` with BEN
  speed in the `0.68-0.70` range, but repeated stock-overlay reruns still
  split across `30`, `34`, and occasional `50`
- a delayed-deploy overlay did not stabilize that low-speed moving pocket, so
  it remains calibration-only rather than a supported H02 threshold family
- a later follow-up search found a more promising apparent edge pocket around
  `B_POS=-5,-86,244`, but that point did not survive hard-clean isolated reruns
  under the normal H02 timing budget
- in isolated reruns, nearby candidates such as `241/0.69`, `244/0.69`,
  `244/0.70`, and `245/0.69` mostly fell back to `none`, with only occasional
  `34` or `50` hits, so there is still no honest `38 -> 34 -> 30` threshold
  family to promote
- two fixture-control ideas were also tried and rejected:
  - delayed `DEPLOY_ALL` on the moving speed probe
  - delayed `AVOID_ALL` so the encounter could develop before the behavior
    classified it
- neither timing control stabilized `34`; the delayed-deploy variants mostly
  stayed in `none`, and delayed-avoid activation at `6`, `8`, and `10` seconds
  also failed to produce a CI-honest family
- `standon_band350_unsure_stern_pass`: attempt to reach `34` inside the narrow
  `rel_bng > 350` source band with a straight-line geometry that turned out to
  be a bad fit for the stock source

Expected behavior:
- edge progression among:
  - `standon:unsure`
  - `standon:unsure_stern`
  - `standon:stern`

Notes:
- this family matters because `unsure_stern` is source-defined and was one of
  the last missing stock stand-on submodes in the repo
- the best current search direction is turn-driven rather than straight-line:
  the old straight-line Band #1 stern probes repeatedly fell into `headon`,
  `cpa`, or plain `stern` before they could prove the real `34` branch
- `standon_band315_stern_pass` and `standon_stern_direct_pass` were both
  removed from the active harness: the former used an unverified axis-aligned
  geometry that may not actually land in Band #2, and the latter duplicated
  coverage already provided by the southwest family without adding a distinct
  geometry anchor; both are planned for restoration once calibrated alternatives
  are found

Status:
- the supported southwest-track stern-adjacent family covers the honest
  reachable progression under the stock source:
  - `standon_southwest_unsurebow_pass` -> `standon:unsure_bow` (`36`)
  - `standon_southwest_unsure_pass` -> `standon:unsure` (`38`)
  - `standon_southwest_stern_pass` -> `standon:stern` (`30`)
- `standon_band270_stern_pass` is the one Band #3 stern reference kept because
  it uses the same well-understood geometry (`crossing_port_standon_stern_pass`)
  that H01 already validates
- the new turn-driven stern probes are the first ones that can hit `34`, but
  they are still calibration-only because repeatability is not yet honest
  enough for the supported H02 gate
- the `standon_unsurestern_*` / `standon_band350_unsure_stern_pass` overlays
  remain on disk as exploratory material and regression breadcrumbs for the
  older bad-fit search direction

## Upper-Ceiling Stand-On Band Inventory

This section is intentionally expansive. It is the planning ceiling for the
stand-on side of `H02`, not a commitment that every listed case must survive
calibration.

Why this section exists:
- the upstream stand-on logic is explicitly split by relative-bearing band in
  [BHV_AvdColregsV22.cpp](/Users/charlesbenjamin/moos-ivp/ivp/src/lib_behaviors-colregs/BHV_AvdColregsV22.cpp#L1008)
- future chats or models should not have to reconstruct those bands from
  source before continuing the harness
- some of the cases below will likely collapse or be pruned later, but the
  upper ceiling should be written down now

Important scope note:
- the explicit bearing-band decomposition currently matters most for the
  `standon` branch
- other families in `H02` may still use angle-sensitive geometry, but the
  source does not present them in the same clean band-by-band form

### Stand-On Source Bands

The current source uses four stand-on bearing bands:
- Band #1: `os_cn_rel_bng > 350`
- Band #2: `315 < os_cn_rel_bng <= 350`
- Band #3: `270 <= os_cn_rel_bng <= 315`
- Band #4: `180 < os_cn_rel_bng < 270`

Each band has different bow/stern/unsure branching behavior and, in some
cases, different crossing-distance thresholds or turn-rate qualifiers.

### Band #1 Ceiling: `os_cn_rel_bng > 350`

Primary source transitions:
- `bow` vs `unsure_bow` around `cn_crosses_os_bow_dist >= min_util_cpa_dist`
- `unsure_bow` vs `unsure` around `cn_crosses_os_bow_dist > 0`
- `unsure_stern` on the stern side of the bow-stern line

Upper-ceiling case options:
- `standon_band350_bow_minutil_below_pass`
- `standon_band350_bow_minutil_edge_pass`
- `standon_band350_bow_minutil_above_pass`
- `standon_band350_unsurebow_zero_below_pass`
- `standon_band350_unsurebow_zero_edge_pass`
- `standon_band350_unsurebow_zero_above_pass`
- `standon_band350_unsurestern_zero_below_pass`
- `standon_band350_unsurestern_zero_edge_pass`
- `standon_band350_unsurestern_zero_above_pass`

Optional turn-rate-qualified expansions:
- `standon_band350_bow_minutil_turnneg_below_pass`
- `standon_band350_bow_minutil_turnneg_edge_pass`
- `standon_band350_bow_minutil_turnneg_above_pass`
- `standon_band350_unsurebow_zero_turnneg_below_pass`
- `standon_band350_unsurebow_zero_turnneg_edge_pass`
- `standon_band350_unsurebow_zero_turnneg_above_pass`

### Band #2 Ceiling: `315 < os_cn_rel_bng <= 350`

Primary source transitions:
- `bow` vs `unsure_bow` around `cn_crosses_os_bow_dist >= half_util_cpa_dist`
- another `bow` vs `unsure_bow` split around
  `cn_crosses_os_bow_dist >= min_util_cpa_dist`
- `unsure_bow` vs `unsure` around `cn_crosses_os_bow_dist > 0`
- `stern` on the stern side of the bow-stern line

Upper-ceiling case options:
- `standon_band315_bow_halfutil_below_pass`
- `standon_band315_bow_halfutil_edge_pass`
- `standon_band315_bow_halfutil_above_pass`
- `standon_band315_bow_minutil_below_pass`
- `standon_band315_bow_minutil_edge_pass`
- `standon_band315_bow_minutil_above_pass`
- `standon_band315_unsurebow_zero_below_pass`
- `standon_band315_unsurebow_zero_edge_pass`
- `standon_band315_unsurebow_zero_above_pass`
- `standon_band315_stern_zero_below_pass`
- `standon_band315_stern_zero_edge_pass`
- `standon_band315_stern_zero_above_pass`

Optional turn-rate-qualified expansions:
- `standon_band315_bow_halfutil_turn3_below_pass`
- `standon_band315_bow_halfutil_turn3_edge_pass`
- `standon_band315_bow_halfutil_turn3_above_pass`
- `standon_band315_bow_minutil_turn01_below_pass`
- `standon_band315_bow_minutil_turn01_edge_pass`
- `standon_band315_bow_minutil_turn01_above_pass`
- `standon_band315_unsurebow_zero_turnneg_below_pass`
- `standon_band315_unsurebow_zero_turnneg_edge_pass`
- `standon_band315_unsurebow_zero_turnneg_above_pass`

### Band #3 Ceiling: `270 <= os_cn_rel_bng <= 315`

Primary source transitions:
- `bow` vs `unsure_bow` around `cn_crosses_os_bow_dist >= max_util_cpa_dist`
- `bow` vs `unsure_bow` around `cn_crosses_os_bow_dist >= half_util_cpa_dist`
- `unsure_bow` vs `unsure` around `cn_crosses_os_bow_dist > 0`
- `stern` on the stern side of the bow-stern line

Upper-ceiling case options:
- `standon_band270_bow_maxutil_below_pass`
- `standon_band270_bow_maxutil_edge_pass`
- `standon_band270_bow_maxutil_above_pass`
- `standon_band270_bow_halfutil_below_pass`
- `standon_band270_bow_halfutil_edge_pass`
- `standon_band270_bow_halfutil_above_pass`
- `standon_band270_unsurebow_zero_below_pass`
- `standon_band270_unsurebow_zero_edge_pass`
- `standon_band270_unsurebow_zero_above_pass`
- `standon_band270_stern_zero_below_pass`
- `standon_band270_stern_zero_edge_pass`
- `standon_band270_stern_zero_above_pass`

Optional turn-rate-qualified expansions:
- `standon_band270_bow_halfutil_turn01_below_pass`
- `standon_band270_bow_halfutil_turn01_edge_pass`
- `standon_band270_bow_halfutil_turn01_above_pass`
- `standon_band270_unsurebow_zero_turnneg_below_pass`
- `standon_band270_unsurebow_zero_turnneg_edge_pass`
- `standon_band270_unsurebow_zero_turnneg_above_pass`

### Band #4 Ceiling: `180 < os_cn_rel_bng < 270`

Primary source transitions:
- `unsure_bow` vs `unsure` around `cn_crosses_os_bow_dist >= half_util_cpa_dist`
- `unsure` vs `stern` around `cn_crosses_os_bow_dist > 0`

Upper-ceiling case options:
- `standon_band180_270_unsurebow_halfutil_below_pass`
- `standon_band180_270_unsurebow_halfutil_edge_pass`
- `standon_band180_270_unsurebow_halfutil_above_pass`
- `standon_band180_270_unsure_zero_below_pass`
- `standon_band180_270_unsure_zero_edge_pass`
- `standon_band180_270_unsure_zero_above_pass`
- `standon_band180_270_stern_zero_below_pass`
- `standon_band180_270_stern_zero_edge_pass`
- `standon_band180_270_stern_zero_above_pass`

Optional turn-rate-qualified expansions:
- `standon_band180_270_unsurebow_halfutil_turn01_below_pass`
- `standon_band180_270_unsurebow_halfutil_turn01_edge_pass`
- `standon_band180_270_unsurebow_halfutil_turn01_above_pass`

### Stand-On Cross-Band Meta Families

If the harness grows large enough, these are the obvious cross-band families to
build after the first per-band cuts:
- `standon_band_350_to_315_transition_below_pass`
- `standon_band_350_to_315_transition_edge_pass`
- `standon_band_350_to_315_transition_above_pass`
- `standon_band_315_to_270_transition_below_pass`
- `standon_band_315_to_270_transition_edge_pass`
- `standon_band_315_to_270_transition_above_pass`
- `standon_band_270_to_180_transition_below_pass`
- `standon_band_270_to_180_transition_edge_pass`
- `standon_band_270_to_180_transition_above_pass`

These are not first-priority cases, but they are legitimate ceiling options
because the source switches rule logic at those bearing boundaries.

### 5. Stand-On `neither` Transition

Purpose:
- exploratory release-controlled search for the terminal branch where ownship
  has crossed to the contact's port side
- keep the release timing honest so the branch is reached as a transition case,
  not as a static offset trick
- this family was repeatedly probed and rejected as a supported H02 threshold
  family; it remains calibration-only

Exploratory cases kept off the supported gate:
- `standon_neither_below_pass`
- `standon_neither_edge_pass`
- `standon_neither_above_pass`

What each exploratory case was trying to mean:
- `standon_neither_below_pass`: stable stern-side anchor that should remain in
  `standon:stern` (`30`) under the delayed release
- `standon_neither_edge_pass`: release-controlled pocket that was intended to
  land in `standon:neither` (`39`)
- `standon_neither_above_pass`: earlier-release version of the same pocket
  that was also intended to land in `standon:neither` (`39`)

Notes:
- this family deliberately used delayed release on the shared stem so the
  transition could settle before the behavior was sampled
- the search tried multiple pockets and release points, including southwest,
  turn-driven, and band315 midpoints; none produced a clean, repeatable
  `standon:neither` gate under hard reruns
- release points tested during the search included 0, 1, 1.5, 1.75, 1.9, and
  2 seconds, plus the older 4/8 second variants; no combination was promoted
- the best stable anchor remained `crossing_port_standon_southwest_stern_pass`

### 6. In-Extremis Entry

Purpose:
- verify entry into `standon:inextremis` and `standon_ot:inextremis` when range
  and current CPA collapse together
- the stock `min_util_cpa_dist` and `max_util_cpa_dist` defaults stay fixed;
  geometry carries the below/edge/above split

Current implemented cases:
- `standon_inextremis_range_below_pass`
- `standon_inextremis_range_edge_pass`
- `standon_inextremis_range_above_pass`
- `standon_inextremis_cpa_below_pass`
- `standon_inextremis_cpa_edge_pass`
- `standon_inextremis_cpa_above_pass`
- `standon_ot_inextremis_range_below_pass`
- `standon_ot_inextremis_range_edge_pass`
- `standon_ot_inextremis_range_above_pass`
- `standon_ot_inextremis_cpa_below_pass`
- `standon_ot_inextremis_cpa_edge_pass`
- `standon_ot_inextremis_cpa_above_pass`

Expected behavior:
- standon below threshold: `standon:stern`
- standon edge/above threshold: `standon:inextremis`
- standon CPA below threshold: `standon:stern`
- standon CPA edge/above threshold: `standon:inextremis`
- standon_ot below threshold: `standon_ot:port`
- standon_ot edge/above threshold: `standon_ot:inextremis`

Notes:
- the supported split keeps the stock utility defaults fixed at
  `min_util_cpa_dist=10` and `max_util_cpa_dist=18`
- the standon bank uses the turn-driven stern probes:
  `crossing_port_standon_stern_pass`,
  `crossing_port_standon_turn_unsure_stern_below_pass`,
  `crossing_port_standon_turn_unsure_stern_edge_pass`, and
  `crossing_port_standon_turn_unsure_stern_above_pass`
- the standon CPA below case anchors on the stable stern seed:
  `crossing_port_standon_stern_pass`
- the standon CPA edge/above cases use the dedicated turn-based CPA pocket:
  `crossing_port_standon_cpa_edge_pass` and
  `crossing_port_standon_cpa_above_pass`
- `crossing_port_standon_cpa_wide_pass` remains available as a calibration
  alias if the wider CPA offset needs to be revisited later
- the `standon_ot` range bank uses
  `overtaken_port_standon_range_far_pass`,
  `overtaken_port_standon_range_edge_pass`, and
  `overtaken_port_standon_range_close_pass`
- `overtaken_port_standon_range_edge_pass` is the H02 range-edge pocket;
  `overtaken_port_standon_range_close_pass` is the above pocket
- the `standon_ot` CPA bank uses
  `overtaken_port_standon_cpa_wide_pass`,
  `overtaken_port_standon_cpa_edge_pass`, and
  `overtaken_port_standon_cpa_above_pass`
- the `standon_ot` range and CPA banks are both clean geometry-backed
  threshold families
- this family is more useful here than trying to force `inextremis` into `H01`

Status:
- implemented and passing under serial cold-start validation

### 7. Overtaking Aft-Sector Entry

Purpose:
- verify the Rule 13 entry cutoff for overtaking and the split between
  overtaking and non-overtaking classifications

Already scaffolded on disk:
- `overtaking-thresh-below-shoreside.xmoos`
- `overtaking-thresh-edge-shoreside.xmoos`
- `overtaking-thresh-above-shoreside.xmoos`

Planned harness cases:
- `overtaking_thresh_below_pass`
- `overtaking_thresh_edge_pass`
- `overtaking_thresh_above_pass`
- `overtaking_thresh_below_mirror_pass`
- `overtaking_thresh_edge_mirror_pass`
- `overtaking_thresh_above_mirror_pass`
- `giveway_bowdist_below_pass`
- `giveway_bowdist_edge_pass`
- `giveway_bowdist_above_pass`
- `giveway_bowdist_below_mirror_pass`
- `giveway_bowdist_edge_mirror_pass`
- `giveway_bowdist_above_mirror_pass`
- `standon_unsurebow_below_pass`
- `standon_unsurebow_edge_pass`
- `standon_unsurebow_above_pass`
- `standon_band315_unsure_pass`
- `standon_band315_unsurebow_pass`
- `standon_band315_bow_pass`
- `standon_band270_stern_pass`
- `standon_band350_unsurebow_pass`
- `standon_band350_unsurebow_alt_pass`
- `standon_band350_bow_pass`
- `standon_southwest_unsurebow_pass`
- `standon_southwest_unsure_pass`
- `standon_southwest_stern_pass`
- `standon_stern_direct_pass`

Expected behavior:
- upper-bound family:
  - `overtaking_thresh_below_pass` -> `overtaking:port` (`43`)
  - `overtaking_thresh_edge_pass` -> `giveway:bow` (`22`)
  - `overtaking_thresh_above_pass` -> `giveway:bow` (`22`)
- lower-bound mirror family:
  - `overtaking_thresh_below_mirror_pass` -> `cpa` (`50`)
  - `overtaking_thresh_edge_mirror_pass` -> `cpa` (`50`)
  - `overtaking_thresh_above_mirror_pass` -> `overtaking:starboard` (`47`)

Status:
- implemented and passing

### 8. Overtaken-Vessel Entry

Purpose:
- verify the CPA gate that turns on `standon_ot`

Current implemented cases:
- `overtaken_thresh_below_pass`
- `overtaken_thresh_edge_pass`
- `overtaken_thresh_above_pass`
- `overtaken_thresh_below_mirror_pass`
- `overtaken_thresh_edge_mirror_pass`
- `overtaken_thresh_above_mirror_pass`

Expected behavior:
- below threshold: `cpa`
- edge/above threshold: `standon_ot:port` or `standon_ot:starboard`

Notes:
- this gives `standon_ot` a true boundary suite instead of only H01 canonicals
- the stock utility defaults stay fixed; the split is carried by geometry only
- the H02 checkpoint treats both the port and mirror families as supported
  gate cases
- the geometry anchors behind this family are the new overtaken-entry pockets:
  `overtaken_port_standon_gate_below_pass`,
  `overtaken_port_standon_gate_edge_pass`,
  `overtaken_port_standon_gate_above_pass`,
  `overtaken_starboard_standon_gate_below_pass`,
  `overtaken_starboard_standon_gate_edge_pass`, and
  `overtaken_starboard_standon_gate_above_pass`
- current calibrated split:
  - `overtaken_thresh_below_pass` and `overtaken_thresh_below_mirror_pass`
    cleanly remain in `cpa`
  - edge and above cases cleanly enter `standon_ot:port` or
    `standon_ot:starboard`

### 9. Outer Range Gate

Purpose:
- verify the stock `pwt_outer_dist` cutoff using geometry only
- keep the below/edge/above split honest without changing behavior
  parameters

Current implemented cases:
- `outer_dist_below_pass`
- `outer_dist_edge_pass`
- `outer_dist_above_pass`

Expected behavior:
- below and edge: `standon:stern`
- above: `cpa`

Notes:
- the geometry anchor is the southwest stern pocket that already sits almost
  exactly on the 40-unit outer range cutoff
- the below/edge/above split comes from a 0.5 m translation of that pocket,
  not from any `BHV_PATCH` or utility-threshold override
- `outer_dist_edge_pass` is the current southwest stern pocket
- `outer_dist_above_pass` is the slightly more distant translation that
  should fall back to `cpa` instead of holding stand-on

### 10. Deferred Activation and Release Gates

Purpose:
- these remain outside the supported geometry-only H02 pattern

Deferred families:
- `inner_dist_below_pass`
- `inner_dist_edge_pass`
- `inner_dist_above_pass`
- `completed_dist_below_pass`
- `completed_dist_edge_pass`
- `completed_dist_above_pass`

Expected behavior:
- inside `pwt_inner_dist`: expected mode should activate
- beyond `completed_dist`: contact should release cleanly

Notes:
- if any of these ever become supported H02 cases, they must be rewritten as
  geometry-first families rather than parameter overlays
- otherwise they belong in a parameter-oriented harness, not H02

### 11. Deferred CPA Utility Boundaries

Purpose:
- these are not part of the supported geometry-only H02 pattern

Deferred cases:
- `cpa_minutil_below_pass`
- `cpa_minutil_edge_pass`
- `cpa_minutil_above_pass`
- `cpa_maxutil_below_pass`
- `cpa_maxutil_edge_pass`
- `cpa_maxutil_above_pass`
- `giveway_release_to_cpa_pass`
- `standon_release_to_cpa_pass`
- `overtaking_reject_to_cpa_pass`

Expected behavior:
- controlled transitions into or out of `cpa`
- no spurious persistence of a prior COLREGS mode once utility is gone

Notes:
- if a case can be expressed geometrically, it should stay in the geometry
  pattern instead of becoming a parameter sweep
- otherwise it belongs outside the supported H02 geometry harness

## Current Implemented Case List

Implemented and supported families:

- Head-on cutoff:
  `head_on_thresh_below_pass`, `head_on_thresh_edge_pass`,
  `head_on_thresh_above_pass`, and the three mirror variants
- Overtaking aft-sector cutoff:
  `overtaking_thresh_below_pass`, `overtaking_thresh_edge_pass`,
  `overtaking_thresh_above_pass`, and the three mirror variants
- Overtaken-vessel CPA gate:
  `overtaken_thresh_below_pass`, `overtaken_thresh_edge_pass`,
  `overtaken_thresh_above_pass`, and the three mirror variants
- Give-way bow-distance split:
  `giveway_bowdist_below_pass`, `giveway_bowdist_edge_pass`,
  `giveway_bowdist_above_pass`, and the three mirror variants
- Give-way turn-gap split:
  `giveway_turngap_below_pass`, `giveway_turngap_edge_pass`,
  `giveway_turngap_above_pass`, and the three mirror variants
- Stand-on representative `unsure -> unsure_bow -> bow` progression:
  `standon_unsurebow_below_pass`, `standon_unsurebow_edge_pass`,
  `standon_unsurebow_above_pass`
- Stand-on band350 representatives:
  `standon_band350_unsurebow_pass`, `standon_band350_unsurebow_alt_pass`,
  `standon_band350_bow_pass`
- Stand-on band315 representatives:
  `standon_band315_unsure_pass`, `standon_band315_unsure_bow_pass`,
  `standon_band315_bow_pass`
- Stand-on band270 stern reference:
  `standon_band270_stern_pass`
- Stand-on stern-adjacent southwest family:
  `standon_southwest_unsurebow_pass`, `standon_southwest_unsure_pass`,
  `standon_southwest_stern_pass`
- Outer range gate:
  `outer_dist_below_pass`, `outer_dist_edge_pass`, `outer_dist_above_pass`
- In-extremis entry family:
  `standon_inextremis_range_below_pass`, `standon_inextremis_range_edge_pass`,
  `standon_inextremis_range_above_pass`, `standon_inextremis_cpa_below_pass`,
  `standon_inextremis_cpa_edge_pass`, `standon_inextremis_cpa_above_pass`,
  `standon_ot_inextremis_range_below_pass`,
  `standon_ot_inextremis_range_edge_pass`,
  `standon_ot_inextremis_range_above_pass`,
  `standon_ot_inextremis_cpa_below_pass`,
  `standon_ot_inextremis_cpa_edge_pass`,
  `standon_ot_inextremis_cpa_above_pass`

## Current Assertion Style

- head-on below expects live `COLREGS_AVOID_IX_BEN = 10`
- head-on edge/above expect transition to `COLREGS_AVOID_IX_BEN = 50`
- mirrored head-on cases reuse the same expected index behavior with the
  contact mirrored to the opposite side of the centerline
- overtaking upper-bound family expects:
  - `43` below the cutoff
  - `22` at and above the cutoff
- overtaking lower-bound mirror family expects:
  - `50` below and near the cutoff
  - `47` above the cutoff
- overtaken entry family expects:
  - `cpa` below the cutoff
  - `standon_ot:port` or `standon_ot:starboard` at and above the cutoff
- give-way bow-distance family expects:
  - `20` below and at the current edge
  - `22` above the cutoff
  - the rotated mirror family reuses the same expected split:
  - `20` below and at the current edge
  - `22` above the cutoff
- give-way turn-gap family expects:
  - `20` below and at the current edge
  - `22` above the cutoff
  - the rotated mirror family reuses the same split
- stand-on representative additions expect:
  - `standon_band315_unsure_pass` -> `38`
  - `standon_band315_unsure_bow_pass` -> `36`
  - `standon_band315_bow_pass` -> `32`
  - `standon_band270_stern_pass` -> `30`
  - `standon_band350_unsurebow_pass` -> `36`
  - `standon_band350_unsurebow_alt_pass` -> `36`
  - `standon_band350_bow_pass` -> `32`
  - `standon_southwest_unsurebow_pass` -> `36`
  - `standon_southwest_unsure_pass` -> `38`
  - `standon_southwest_stern_pass` -> `30`
- outer range gate expects:
  - `outer_dist_below_pass` -> `30`
  - `outer_dist_edge_pass` -> `30`
  - `outer_dist_above_pass` -> `cpa`
- in-extremis additions expect:
  - `standon_inextremis_range_below_pass` -> `30`
  - `standon_inextremis_range_edge_pass` -> `31`
  - `standon_inextremis_range_above_pass` -> `31`
  - `standon_inextremis_cpa_below_pass` -> `30`
  - `standon_inextremis_cpa_edge_pass` -> `31`
  - `standon_inextremis_cpa_above_pass` -> `31`
  - `standon_ot_inextremis_range_below_pass` -> `standon_ot:port`
  - `standon_ot_inextremis_range_edge_pass` -> `standon_ot:inextremis`
  - `standon_ot_inextremis_range_above_pass` -> `standon_ot:inextremis`
  - `standon_ot_inextremis_cpa_below_pass` -> `standon_ot:port`
  - `standon_ot_inextremis_cpa_edge_pass` -> `standon_ot:inextremis`
  - `standon_ot_inextremis_cpa_above_pass` -> `standon_ot:inextremis`

## Planned Assertion Style

- expected `COLREGS_AVOID_MODE_<CONTACT>`
- expected `COLREGS_AVOID_IX_<CONTACT>`
- stable no-collision outcome
- deterministic resolution by timeout, not `actual=missing`
- optional anti-thrashing checks when a case is close enough to a threshold
  that oscillation is a real regression risk

## Coverage Priorities

Recommended next implementation order:
1. keep the supported geometry-first gate stable under both serial and wave-mode runs
2. only promote a deferred activation/release or CPA-utility family if it can be kept geometry-first and stock-parameter honest
3. otherwise move effort to `H03`, where the next useful question is maneuver quality after correct classification

This keeps `H02` focused on honest threshold coverage instead of expanding into
parameter overlays or fragile exploratory pockets.

## Expected Suite Size

If implemented fully, `H02` is likely the largest COLREGS unit harness in this
repo.

Reason:
- `H01` only needs one stable representative per canonical bin
- `H03` can stay relatively compact because it reuses canonical cases and checks
  outcome quality
- `H02` expands combinatorially because each source threshold tends to want:
  - below
  - edge
  - above
  - often a mirror
  - sometimes a parameterized or turn-gap variant

Typical runs:

```bash
./zlaunch.sh 10
./zlaunch.sh --jobs=2 10
./zlaunch.sh --case=head_on_thresh_edge_pass 10
```
