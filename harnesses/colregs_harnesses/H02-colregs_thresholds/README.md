# H02-colregs_thresholds

TLDR:
- threshold-edge harness on the shared `colregs_unit` stem
- proves that small geometry or parameter changes flip classification where the source says they should
- intended to absorb the unstable edge-space that does not belong in `H01`

This harness uses the shared `colregs_unit` stem mission to probe the exact
geometry and parameter boundaries that decide COLREGS classification in
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

## How To Read Case Names

Most H02 case names encode both the source threshold being probed and the side
of the threshold the geometry is expected to land on.

- `*_below_pass`: just below the source cutoff; should stay on the pre-flip side
- `*_edge_pass`: on the currently calibrated stock edge
- `*_above_pass`: just above the cutoff; should land on the post-flip side
- `*_mirror_pass`: same idea as the base case, but rotated or mirrored to prove
  the opposite-side geometry is behaving symmetrically
- `band315`, `band270`, `band350`: representative stand-on cases pinned to the
  source bearing bands in `BHV_AvdColregsV22`
- `southwest`: a custom stern-adjacent contact track added after calibration
  showed the older stern probes were a poor fit for the stock source

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
  - behavior parameter
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
- `standon_band315_unsure_pass`
- `standon_band315_unsurebow_pass`
- `standon_band315_bow_pass`
- `standon_band270_unsure_pass`
- `standon_band270_unsurebow_pass`
- `standon_band270_bow_pass`
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
- `standon_band315_unsure_pass`: Band #2 (`315 < rel_bng <= 350`) case pinned
  in `standon:unsure`
- `standon_band315_unsurebow_pass`: Band #2 case pinned in
  `standon:unsure_bow`
- `standon_band315_bow_pass`: Band #2 case pinned in `standon:bow`
- `standon_band270_unsure_pass`: Band #3 (`270 <= rel_bng <= 315`) case pinned
  in `standon:unsure`
- `standon_band270_unsurebow_pass`: Band #3 case pinned in
  `standon:unsure_bow`
- `standon_band270_bow_pass`: Band #3 case pinned in `standon:bow`
- `standon_band350_unsurebow_pass`: Band #1 (`rel_bng > 350`) case pinned in
  `standon:unsure_bow`
- `standon_band350_unsurebow_alt_pass`: alternate Band #1 geometry that lands
  in the same branch and helps keep this narrow slice from being under-sampled
- `standon_band350_bow_pass`: Band #1 case pinned in `standon:bow`

Still-planned later expansions:
- `standon_bowdist_minutil_below_pass`
- `standon_bowdist_minutil_edge_pass`
- `standon_bowdist_minutil_above_pass`
- `standon_bowdist_halfutil_below_pass`
- `standon_bowdist_halfutil_edge_pass`
- `standon_bowdist_halfutil_above_pass`
- `standon_bowdist_maxutil_below_pass`
- `standon_bowdist_maxutil_edge_pass`
- `standon_bowdist_maxutil_above_pass`
- `standon_bearing_350_below_pass`
- `standon_bearing_350_edge_pass`
- `standon_bearing_350_above_pass`
- `standon_bearing_315_below_pass`
- `standon_bearing_315_edge_pass`
- `standon_bearing_315_above_pass`
- `standon_bearing_270_below_pass`
- `standon_bearing_270_edge_pass`
- `standon_bearing_270_above_pass`

Expected behavior:
- first representative progression family:
  - `standon_unsurebow_below_pass` -> `standon:unsure` (`38`)
  - `standon_unsurebow_edge_pass` -> `standon:unsure_bow` (`36`)
  - `standon_unsurebow_above_pass` -> `standon:bow` (`32`)
- broader later-family coverage still depends on band and crossing distance:
  - `standon:unsure`
  - `standon:unsure_bow`
  - `standon:bow`

Notes:
- this family is likely larger than the give-way bow family because the source
  has several bearing bands with different thresholds
- some planned cases may get culled once the cleanest representative band set
  is identified
- the first implemented cut uses existing stable stem geometries and treats the
  family as an early-phase progression rather than an end-state classification

Status:
- first representative `unsure -> unsure_bow -> bow` family implemented
- additional stand-on band representative cases are now reasonable to carry in
  `H02` even when they partially overlap the first progression family, because
  they pin specific source-bearing slices for future work

### 4. Stand-On `unsure` vs `stern` and `unsure_stern`

Purpose:
- verify the stern-side stand-on branches that are still uncovered in `H01`

Current implemented supported cases:
- `standon_band315_stern_pass`
- `standon_band270_stern_pass`
- `standon_southwest_unsurebow_pass`
- `standon_southwest_unsure_pass`
- `standon_southwest_stern_pass`
- `standon_stern_direct_pass`

Exploratory cases kept off the supported gate:
- turn-driven stem probes:
  - `crossing_port_standon_turn_unsure_stern_below_pass`
  - `crossing_port_standon_turn_unsure_stern_edge_pass`
  - `crossing_port_standon_turn_unsure_stern_above_pass`
- `standon_band350_unsure_stern_pass`

What each supported case means:
- `standon_band315_stern_pass`: Band #2 stern-side reference case showing that
  the classifier does cleanly resolve to `standon:stern` once the contact is
  on the stern side of the bow-stern line in that band
- `standon_band270_stern_pass`: Band #3 stern-side reference case with the same
  role for the next source bearing slice
- `standon_southwest_unsurebow_pass`: custom southwest-track geometry that
  approaches the stern-side transition but still lands in `standon:unsure_bow`
- `standon_southwest_unsure_pass`: small southwest-track shift that lands in
  `standon:unsure`
- `standon_southwest_stern_pass`: another small southwest-track shift that
  lands in `standon:stern`
- `standon_stern_direct_pass`: simple direct stern reference case kept as an
  easy-to-understand anchor for `standon:stern`

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
- a direct `standon:stern` probe is still worth carrying because it stays an
  easy-to-understand reference while the threshold family is still being
  calibrated

Status:
- a new supported southwest-track stern-adjacent family now covers the honest
  reachable progression under the stock source:
  - `standon_southwest_unsurebow_pass` -> `standon:unsure_bow` (`36`)
  - `standon_southwest_unsure_pass` -> `standon:unsure` (`38`)
  - `standon_southwest_stern_pass` -> `standon:stern` (`30`)
- the new turn-driven stern probes are the first ones that can hit `34`, but
  they are still calibration-only because repeatability is not yet honest
  enough for the supported H02 gate
- the old straight-line stern probes have been de-scoped from the active H02
  harness surface for this checkpoint so the harness only advertises cases that
  pass
- the current best stern-side probe remains the dedicated diagonal stem mode
  `crossing_port_standon_unsure_stern_diag_probe`, but it is still manual-only
  because it is still useful source-study material alongside the newer
  turn-driven `34` search
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
- verify the terminal branch where ownship has crossed to the contact's port
  side

Planned cases:
- `standon_neither_below_pass`
- `standon_neither_edge_pass`
- `standon_neither_above_pass`

Expected behavior:
- below threshold: prior stand-on submode
- edge/above threshold: `standon:neither`

Notes:
- likely requires careful trajectory design rather than only a static offset

### 6. In-Extremis Entry

Purpose:
- verify entry into `standon:inextremis` and `standon_ot:inextremis` when range
  and current CPA collapse together

Planned cases:
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
- below threshold: non-inextremis stand-on mode
- edge/above threshold: `standon:inextremis` or `standon_ot:inextremis`

Notes:
- this family is probably more useful than trying to force `inextremis` into
  `H01`

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
- `standon_band315_stern_pass`
- `standon_band270_unsure_pass`
- `standon_band270_unsurebow_pass`
- `standon_band270_bow_pass`
- `standon_band270_stern_pass`
- `standon_band350_unsurebow_pass`
- `standon_band350_unsurebow_alt_pass`
- `standon_band350_bow_pass`
- `standon_southwest_unsurebow_pass`
- `standon_southwest_unsure_pass`
- `standon_southwest_stern_pass`
- `standon_stern_direct_pass`
- `giveway_bowdist_below_pass`
- `giveway_bowdist_edge_pass`
- `giveway_bowdist_above_pass`

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
- the overtaken port/starboard geometries both sit near the same observed CPA,
  so the family is parameterized with `max_util_cpa_dist` overlays
- the H02 checkpoint now treats both the port and mirror families as supported
  gate cases
- the behavior overlays for this family use `.xbhv` patch inputs so `nspatch`
  generates a live `meta_vehicle.bhvx` sidecar before launch
- current calibrated split:
  - `overtaken_thresh_below_pass` and `overtaken_thresh_below_mirror_pass`
    cleanly remain in `cpa`
  - edge and above cases cleanly enter `standon_ot:port` or
    `standon_ot:starboard`

### 9. Activation and Release Distance Gates

Purpose:
- verify the range gates that turn classification authority on and off

Planned parameter-driven families:
- `outer_dist_below_pass`
- `outer_dist_edge_pass`
- `outer_dist_above_pass`
- `inner_dist_below_pass`
- `inner_dist_edge_pass`
- `inner_dist_above_pass`
- `completed_dist_below_pass`
- `completed_dist_edge_pass`
- `completed_dist_above_pass`

Expected behavior:
- outside `pwt_outer_dist`: mode should not activate
- inside `pwt_outer_dist`: expected mode should activate
- beyond `completed_dist`: contact should release cleanly

Notes:
- these may be implemented with `.xbhv` overlays rather than pure geometry
- there is some overlap with `H04`, but `H02` should still own the decision
  threshold semantics for the stock baseline parameters

### 10. CPA Utility Boundaries

Purpose:
- verify transitions caused by `min_util_cpa_dist` and `max_util_cpa_dist`

Planned cases:
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
- some of these may end up as small families attached to specific canonical
  encounters rather than one generic family

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
- Stand-on representative `unsure -> unsure_bow -> bow` progression:
  `standon_unsurebow_below_pass`, `standon_unsurebow_edge_pass`,
  `standon_unsurebow_above_pass`
- Stand-on per-band representatives:
  `standon_band315_unsure_pass`, `standon_band315_unsurebow_pass`,
  `standon_band315_bow_pass`, `standon_band315_stern_pass`,
  `standon_band270_unsure_pass`, `standon_band270_unsurebow_pass`,
  `standon_band270_bow_pass`, `standon_band270_stern_pass`,
  `standon_band350_unsurebow_pass`, `standon_band350_unsurebow_alt_pass`,
  `standon_band350_bow_pass`
- Stand-on stern-adjacent southwest family:
  `standon_southwest_unsurebow_pass`, `standon_southwest_unsure_pass`,
  `standon_southwest_stern_pass`
- Direct stern reference:
  `standon_stern_direct_pass`

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
- stand-on representative additions expect:
  - `standon_band315_unsure_pass` -> `38`
  - `standon_band315_unsurebow_pass` -> `36`
  - `standon_band315_bow_pass` -> `32`
  - `standon_band315_stern_pass` -> `30`
  - `standon_band270_unsure_pass` -> `38`
  - `standon_band270_unsurebow_pass` -> `36`
  - `standon_band270_bow_pass` -> `32`
  - `standon_band270_stern_pass` -> `30`
  - `standon_band350_unsurebow_pass` -> `36`
  - `standon_band350_unsurebow_alt_pass` -> `36`
  - `standon_band350_bow_pass` -> `32`
  - `standon_southwest_unsurebow_pass` -> `36`
  - `standon_southwest_unsure_pass` -> `38`
  - `standon_southwest_stern_pass` -> `30`
  - `standon_stern_direct_pass` -> `30`

## Planned Assertion Style

- expected `COLREGS_AVOID_MODE_<CONTACT>`
- expected `COLREGS_AVOID_IX_<CONTACT>`
- stable no-collision outcome
- deterministic resolution by timeout, not `actual=missing`
- optional anti-thrashing checks when a case is close enough to a threshold
  that oscillation is a real regression risk

## Coverage Priorities

Recommended next implementation order:
1. either stabilize the turn-driven `standon:unsure_stern` probes or keep them
   explicitly manual-only
2. add one in-extremis family
3. add one range-gate family
4. add one CPA-utility family

This keeps the suite moving from already-scaffolded work toward the largest
currently uncovered semantic areas.

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
