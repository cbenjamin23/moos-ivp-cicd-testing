# H04-colregs_parameters

TLDR:
- parameter-regression harness on the shared `colregs_unit` stem
- varies selected `BHV_AvdColregsV22` knobs through `nspatch` overlays
- current implemented groups: `min_util_cpa_dist`, `headon_only`,
  `giveway_bow_dist`, `max_util_cpa_dist`, `velocity_filter`, `eval_tol`,
  `pwt_outer_dist`, `pwt_inner_dist`, `pwt_grade`, `pts_port_turns_ok`,
  `turn_radius`, `check_plateaus`, `completed_dist`, and `use_refinery`
- current supported gate: 32 cases
- intended long-term pattern: keep geometry fixed and change one behavior knob
  at a time

Parameter-sweep harness built on the shared `colregs_unit` stem mission and
using `nspatch` overlays on `BHV_AvdColregsV22`.

Primary intent:
- make parameter changes safe to reason about
- prove that important tuning knobs still produce expected, non-pathological
  behavior
- capture the accepted baseline before future refactors

Primary H04 philosophy:
- keep one trusted underlying geometry fixed
- change one COLREGS behavior parameter
- prove that the behavior either:
  - stays stable when it should, or
  - changes in one specific expected way

So H04 is not where geometry edge cases are explored. It is where parameter
regressions are detected.

## How This Harness Works

- keep geometry fixed for a small mini-suite
- vary one COLREGS parameter through a behavior-side `nspatch` overlay
- use a shoreside `nspatch` overlay to assert the expected classification or
  safe outcome

Typical H04 case pattern:
- same geometry
- parameter at default or baseline setting
- parameter at alternate setting
- explicit expected difference, or explicit expected no-change result

Current parameter groups:
- `min_util_cpa_dist`
- `headon_only`
- `giveway_bow_dist`
- `max_util_cpa_dist`
- `velocity_filter`
- `eval_tol`
- `pwt_outer_dist`
- `pwt_inner_dist`
- `pwt_grade`
- `pts_port_turns_ok`
- `turn_radius`
- `check_plateaus`
- `completed_dist`
- `use_refinery`

Current case structure:
- keep geometry fixed for each parameter-focused mini-suite
- vary one parameter group at a time in small, explicit steps
- keep mission-owned grading where the source exposes a clean signal
- allow execution-style or timeout-snapshot families when that is the real
  parameter effect

## Running

```bash
./zlaunch.sh 10
./zlaunch.sh --jobs=2 --port_base=24500 10
./zlaunch.sh --case=min_util_cpa_default_pass 10
./zlaunch.sh --just_make --jobs=2 --port_base=24500 10
```

Wave mode uses isolated temp mission copies and compact per-case port blocks:
`case_base = port_base + case_idx*PORT_STRIDE`, with pShare ports starting at
`case_base + 10`.

## Case Guide

- `min_util_cpa_low_pass`
  Runs fixed stand-on geometry with low `min_util_cpa_dist`; expected
  classification should remain on the low/default CPA-distance side.
- `min_util_cpa_default_pass`
  Runs the same fixed stand-on geometry with default `min_util_cpa_dist`;
  expected classification should match the baseline.
- `min_util_cpa_high_pass`
  Runs the same fixed stand-on geometry with high `min_util_cpa_dist`;
  expected classification should shift to the higher CPA-distance branch.
- `headon_only_false_pass`
  Runs crossing geometry with `headon_only=false`; the non-head-on encounter
  should still be admitted as give-way.
- `headon_only_true_pass`
  Runs the same crossing geometry with `headon_only=true`; the non-head-on
  encounter should be demoted to CPA.
- `giveway_bow_dist_default_pass`
  Runs fixed give-way bow-distance geometry with the default threshold and
  expects the stock give-way bow classification.
- `giveway_bow_dist_high_pass`
  Runs the same geometry with high `giveway_bow_dist`; expected classification
  should reflect the widened bow-distance threshold.
- `max_util_cpa_default_pass`
  Runs fixed stand-on geometry with default `max_util_cpa_dist`; expected
  classification should match the baseline.
- `max_util_cpa_high_pass`
  Runs the same geometry with high `max_util_cpa_dist`; expected
  classification should move to the higher CPA envelope branch.
- `velocity_filter_off_pass`
  Runs fixed stand-on geometry with velocity filtering off; expected
  classification should match the unfiltered baseline.
- `velocity_filter_on_pass`
  Runs the same geometry with velocity filtering on; expected classification
  should show the filter's effect on CPA evaluation.
- `eval_tol_default_pass`
  Runs fixed overtaking geometry with default `eval_tol`; expected execution
  quality should remain in the baseline CPA band.
- `eval_tol_short_pass`
  Runs the same overtaking geometry with short `eval_tol`; expected execution
  quality should widen as configured.
- `eval_tol_long_pass`
  Runs the same overtaking geometry with long `eval_tol`; expected execution
  quality should remain stable.
- `pwt_outer_dist_default_pass`
  Runs fixed range-gate geometry with default `pwt_outer_dist`; expected
  relevance should match the stock gate.
- `pwt_outer_dist_high_pass`
  Runs the same range-gate geometry with high `pwt_outer_dist`; expected
  relevance should admit COLREGS behavior earlier.
- `pwt_inner_dist_low_pass`
  Runs fixed stand-on geometry with low `pwt_inner_dist`; expected posted
  priority weight should follow the low inner-distance band.
- `pwt_inner_dist_default_pass`
  Runs the same geometry with default `pwt_inner_dist`; expected posted
  priority weight should match the baseline curve.
- `pwt_inner_dist_high_pass`
  Runs the same geometry with high `pwt_inner_dist`; expected posted priority
  weight should move toward the saturated band.
- `pwt_grade_linear_pass`
  Runs fixed relevance geometry with `pwt_grade=linear` and checks the linear
  priority-weight curve.
- `pwt_grade_quasi_pass`
  Runs fixed relevance geometry with `pwt_grade=quasi` and checks the quasi
  priority-weight curve.
- `pwt_grade_quadratic_pass`
  Runs fixed relevance geometry with `pwt_grade=quadratic` and checks the
  quadratic priority-weight curve.
- `pts_port_turns_ok_true_pass`
  Runs fixed give-way geometry with `pts_port_turns_ok=true` and checks the
  early side relationship.
- `pts_port_turns_ok_false_pass`
  Runs the same give-way geometry with `pts_port_turns_ok=false` and checks
  that the early side relationship flips as expected.
- `turn_radius_default_pass`
  Runs fixed give-way turn-gap geometry with default `turn_radius`; expected
  classification should match the stock branch.
- `turn_radius_high_pass`
  Runs the same geometry with high `turn_radius`; expected classification
  should flip to the high-radius branch.
- `check_plateaus_false_pass`
  Runs CPA fallback geometry with `check_plateaus=false`; refinery diagnostics
  should remain at their initialized values.
- `check_plateaus_true_pass`
  Runs the same geometry with `check_plateaus=true`; refinery diagnostics
  should post the expected plateau/basin signature.
- `completed_dist_default_pass`
  Runs CPA fallback geometry with default `completed_dist`; the encounter
  should already be complete at the shared checkpoint.
- `completed_dist_high_pass`
  Runs the same geometry with high `completed_dist`; the encounter should
  still be active at the shared checkpoint.
- `refinery_on_pass`
  Runs CPA fallback geometry with refinery enabled; classification should
  remain CPA while diagnostics are posted.
- `refinery_off_pass`
  Runs the same geometry with refinery disabled; classification should remain
  CPA while refinery diagnostics stay initialized.

Current assertion styles:
- classification-time checks
  - expected `COLREGS_AVOID_IX_BEN`
  - no collision
  - no timeout
- relevance-shaped checks
  - expected `COLREGS_AVOID_PWT_BEN`
  - fixed mode / fixed range gate
- execution-shaped checks
  - expected side/CPA envelope or stable end-state shape
- timeout-snapshot checks
  - intentional `MISSION_TIMEOUT=true` at a shared checkpoint
  - expected posted mode/state at that checkpoint

Visual/manual-run instrumentation:
- keep the viewer geometry consistent across parameter sweeps
- preserve track/leg overlays so manual comparison stays intuitive

Implemented expectations:
- `min_util_cpa_low_pass` / `min_util_cpa_default_pass`: fixed
  `crossing_port_standon_band350_bow_pass` geometry should remain
  `standon:bow` (`colregs_ix=32`)
- `min_util_cpa_high_pass`: the same geometry with a raised
  `min_util_cpa_dist` should demote to `standon:unsure_bow` (`colregs_ix=36`)
- `headon_only_false_pass`: starboard crossing should remain give-way (`colregs_ix=20`)
- `headon_only_true_pass`: same crossing should demote to CPA (`colregs_ix=50`)
- `giveway_bow_dist_default_pass`: fixed `giveway_bowdist_above_pass` geometry
  should remain `giveway:bow` (`colregs_ix=22`) at the stock threshold
- `giveway_bow_dist_high_pass`: the same geometry with
  `giveway_bow_dist=12` now demotes to `cpa` (`colregs_ix=50`) under the
  stock stem settings
- `max_util_cpa_default_pass`: fixed `crossing_port_standon_band315_bow_pass`
  geometry should remain `standon:bow` (`colregs_ix=32`) at the stock
  `max_util_cpa_dist`
- `max_util_cpa_high_pass`: the same geometry with `max_util_cpa_dist=24`
  should demote to `standon:unsure_bow` (`colregs_ix=36`)
- `velocity_filter_off_pass`: fixed
  `crossing_port_standon_band350_bow_pass` geometry should remain
  `standon:bow` (`colregs_ix=32`) with no velocity filter applied
- `velocity_filter_on_pass`: the same geometry with
  `velocity_filter=min_spd=1.0,max_spd=2.0,pct=40` should demote to
  `standon:unsure_bow` (`colregs_ix=36`) by shrinking the effective CPA
  thresholds during evaluation
- `eval_tol_default_pass`: fixed `overtaking_starboard_range_far_pass`
  geometry should finish with the stable overtaking-port execution shape and a
  realized CPA in the default band around `20.7-21.8`
- `eval_tol_short_pass`: the same geometry with `eval_tol=30` should keep the
  same overtaking classification but widen the realized CPA into the higher
  `23.5-24.8` band
- `eval_tol_long_pass`: the same geometry with `eval_tol=240` should keep the
  same execution shape and stay in the default CPA band
- `pwt_outer_dist_default_pass`: fixed
  `crossing_port_standon_southwest_outer_above_pass` geometry should remain
  `cpa` (`colregs_ix=50`) at the stock `pwt_outer_dist=40`
- `pwt_outer_dist_high_pass`: the same geometry with `pwt_outer_dist=45`
  should admit `standon:stern` (`colregs_ix=30`)
- `pwt_inner_dist_low_pass` / `pwt_inner_dist_default_pass` /
  `pwt_inner_dist_high_pass`: fixed
  `crossing_port_standon_southwest_outer_above_pass` geometry should already
  be in `standon:stern` (`colregs_ix=30`) by `AVDCOL_RANGE_BEN <= 30`, while
  the posted `COLREGS_AVOID_PWT_BEN` should separate into low / default /
  saturated bands
- `pwt_grade_linear_pass` / `pwt_grade_quasi_pass` /
  `pwt_grade_quadratic_pass`: the same fixed geometry and fixed range gate
  should stay in `standon:stern` (`colregs_ix=30`) while the posted
  `COLREGS_AVOID_PWT_BEN` follows the linear / quasi / quadratic relevance
  curve ordering
- `pts_port_turns_ok_true_pass` / `pts_port_turns_ok_false_pass`: fixed
  `giveway_turngap_edge_pass` geometry should be checked at an early
  active-give-way gate, `AVDCOL_RANGE_BEN <= 20` while still in
  `giveway:stern` (`colregs_ix=20`); the stock `true` setting should still
  have the obstacle ship on port (`cn_port=1`), while `false` should keep the
  same give-way mode but flip the early side state to starboard (`cn_port=0`)
- `turn_radius_default_pass`: fixed `giveway_turngap_edge_pass` geometry
  should remain `giveway:stern` (`colregs_ix=20`) at the stock
  `turn_radius=0`
- `turn_radius_high_pass`: the same geometry with `turn_radius=8` should
  flip to `giveway:bow` (`colregs_ix=22`)
- `check_plateaus_false_pass` / `check_plateaus_true_pass`: fixed
  `head_on_cpa_fallback_pass` geometry should remain `cpa` (`colregs_ix=50`)
  with `use_refinery=true`; with `check_plateaus=false` the bridged refinery
  diagnostics should stay at their initialized values, and with
  `check_plateaus=true` the same geometry should post the stable diagnostic
  signature `plateau_ok=false`, `basin_ok=true`, `plateau_logic=aft`
- `completed_dist_default_pass` / `completed_dist_high_pass`: fixed
  `head_on_cpa_fallback_pass` geometry is graded at a shared late timeout
  checkpoint instead of an arrival checkpoint; the stock
  `completed_dist=45` case should already be `complete`, while the same
  geometry with `completed_dist=80` should still be `none`
- `refinery_on_pass` / `refinery_off_pass`: fixed `head_on_cpa_fallback_pass`
  geometry should remain `cpa` (`colregs_ix=50`) with and without refinery;
  with `check_plateaus=true`, the `use_refinery=true` case should also post
  the stable refinery diagnostics `plateau_ok=false`, `basin_ok=true`, and
  `plateau_logic=aft`, while the `use_refinery=false` case should leave those
  bridged diagnostics at their initialized values

Notes on the strengthened `min_util_cpa_dist` group:
- H04 now uses one fixed stand-on geometry:
  `crossing_port_standon_band350_bow_pass`
- this is stronger than the earlier head-on version because the source logic
  for the stand-on bow/unsure-bow split directly depends on `m_min_util_cpa_dist`
- under the stock stem settings, low/default stay in `standon:bow`, while the
  higher setting pushes the same geometry into `standon:unsure_bow`

Notes on the implemented `giveway_bow_dist` group:
- H04 intentionally uses one fixed geometry from the H02 bow-distance family:
  `giveway_bowdist_above_pass`
- a lower-than-stock `giveway_bow_dist` case is not meaningful in the current
  stem because the source clamps `giveway_bow_dist` so it can never be below
  `min_util_cpa_dist`
- since the stock stem already sets both to `10`, a nominal “low” case would
  silently collapse back to the default threshold

Notes on the strengthened `use_refinery` group:
- H04 no longer uses a give-way geometry for `use_refinery`
- the current fixed geometry is `head_on_cpa_fallback_pass`, which naturally
  lands in `cpa`
- this is a better fit because `use_refinery` only affects the CPA IPF path in
  the current source
- H04 now enables `check_plateaus=true` in both refinery cases and bridges the
  posted refinery diagnostics to shoreside
- that makes the family more than a pure stability check:
  - `use_refinery=true` must still stay in `cpa`
  - and it must post the stable same-geometry refinery signature observed on
    this stem: `PLATEAU_CHECK_OK_ben=false`, `BASIN_CHECK_OK_ben=true`, and
    `PLATEAU_LOGIC_CASE_ben=aft`
  - `use_refinery=false` must also stay in `cpa`, but it should leave those
    bridged diagnostics at their initialized `false/false/none` values because
    the refinery path is not active
- room for improvement: this is now a real refinery-specific assertion, but it
  still checks a source-visible diagnostic signature rather than a direct path
  quality improvement

Notes on the implemented `check_plateaus` group:
- H04 uses the same trusted CPA geometry as the `use_refinery` family:
  `head_on_cpa_fallback_pass`
- this family isolates `check_plateaus` itself by holding `use_refinery=true`
  constant
- with `check_plateaus=false`, the geometry still stays in `cpa`, but the
  bridged refinery diagnostics remain at their initialized values
- with `check_plateaus=true`, the same geometry still stays in `cpa` and
  posts the stable diagnostic signature `plateau_ok=false`,
  `basin_ok=true`, `plateau_logic=aft`
- this is the cleanest supported refinery-diagnostics family in the current
  source because the behavior visibly changes what it posts without changing
  the underlying CPA-mode outcome

Notes on the implemented `completed_dist` group:
- `completed_dist` changes when the behavior self-completes via the
  `m_contact_range >= (m_completed_dist * 1.1)` path
- unlike `pwt_outer_dist`, a low `completed_dist` setting is not a pure
  completion-only tweak in the current source because setting it below
  `pwt_outer_dist` also clamps both relevance distances downward
- the admitted H04 family therefore uses only the clean default/high
  direction on the trusted CPA geometry `head_on_cpa_fallback_pass`
- a fixed late-range gate did not work cleanly because once the stock
  behavior completes, it stops posting fresh `AVDCOL_RANGE_BEN` updates
- the supported family instead uses a shared timeout checkpoint at mission
  time `50`:
  - stock `completed_dist=45` has already self-completed and posts
    `mode=complete:none`
  - raised `completed_dist=80` has not yet self-completed and remains
    `mode=none:none`
- this makes `completed_dist` an execution/completion-timing family rather
  than a mode-entry family, which matches the actual source semantics

Notes on the implemented `max_util_cpa_dist` group:
- H04 uses one fixed stand-on geometry:
  `crossing_port_standon_band315_bow_pass`
- this is the companion to the `min_util_cpa_dist` family, but it targets the
  `max_util_cpa_dist` side of the stand-on bow vs unsure-bow logic
- under the stock setting the geometry stays `standon:bow`; raising
  `max_util_cpa_dist` to `24` pushes the same geometry into
  `standon:unsure_bow`

Notes on the implemented `velocity_filter` group:
- H04 uses one fixed stand-on geometry:
  `crossing_port_standon_band350_bow_pass`
- the admitted filter spec, `min_spd=1.0,max_spd=2.0,pct=40`, is strong enough
  to shrink the effective `min_util_cpa_dist` / `max_util_cpa_dist` pair for
  this 1.6 m/s ownship / 1.0 m/s contact setup
- that filtered shrink is enough to demote the same geometry from
  `standon:bow` to `standon:unsure_bow`

Notes on the implemented `eval_tol` group:
- H04 uses one fixed execution geometry:
  `overtaking_starboard_range_far_pass`
- this is the first H04 mini-suite that intentionally uses a completion-style
  contract instead of an early classification-time contract
- the same geometry keeps the overtaking classification in all three settings,
  but `eval_tol=30` widens the realized CPA materially while `eval_tol=240`
  stays aligned with the default band
- this makes `eval_tol` a good example of an execution-shaping knob rather
  than a mode-selection knob
- in isolated probes, the short case stayed cleanly in `overtaking:port`, but
  the default geometry occasionally fell back to late `cpa` at arrival without
  changing the realized overtaking-port side/CPA outcome
- the supported H04 gate therefore keys the default/long cases on the stable
  execution envelope and side metrics rather than an overfit final-mode check

Notes on the implemented `pwt_outer_dist` group:
- H04 uses one fixed southwest stand-on geometry:
  `crossing_port_standon_southwest_outer_above_pass`
- this is the same family H02 uses to verify the stock outer-distance release
  cutoff geometrically
- in H04, the geometry is held fixed and the behavior knob moves instead:
  stock `pwt_outer_dist=40` leaves the case in `cpa`, while raising
  `pwt_outer_dist` to `45` admits `standon:stern`

Notes on the implemented `pwt_inner_dist` group:
- `pwt_inner_dist` only changes relevance saturation in `getRelevance()`;
  it does not change mode-entry gates directly
- because of that, the admitted H04 family is intentionally not a mode-split
  family
- H04 uses the fixed southwest stand-on geometry
  `crossing_port_standon_southwest_outer_above_pass`
- each case waits until the geometry is already in `standon:stern` at a fixed
  range gate, `AVDCOL_RANGE_BEN <= 30`, and then checks the posted
  `COLREGS_AVOID_PWT_BEN`
- on that same geometry, lowering `pwt_inner_dist` to `10` reduces the
  avoidance weight into the low band, stock `20` lands in the middle band,
  and raising it to `30` saturates the weight at `150`

Notes on the implemented `pwt_grade` group:
- `pwt_grade` also changes only the relevance curve in `getRelevance()`
- H04 uses the same fixed geometry as the `pwt_inner_dist` family, but an
  earlier fixed range gate, `AVDCOL_RANGE_BEN <= 35`, so the three curve
  shapes stay well separated under repeated runs
- at that gate, the same `standon:stern` case produces the expected weight ordering:
  `quadratic < quasi < linear`
- this is a better H04 fit than the earlier attempted mode-split approach,
  because it tests the exact source-visible effect of the knob

Notes on the implemented `turn_radius` group:
- `turn_radius` feeds directly into the `turnGap()` check inside
  `checkModeGiveWay()`
- that means it is a clean give-way submode knob, not a generic path-shaping
  parameter
- H04 uses the fixed threshold geometry `giveway_turngap_edge_pass`
- under the stock setting the geometry stays `giveway:stern`
- raising `turn_radius` to `8` makes the stern option less feasible on the
  same geometry and flips the result to `giveway:bow`

Notes on the implemented `pts_port_turns_ok` group:
- `pts_port_turns_ok` only affects give-way maneuver generation in the current
  source; it does not create a clean mode-entry split on the trusted stem
- because of that, the admitted H04 family is intentionally not a
  completion-style family
- H04 uses the fixed geometry `giveway_turngap_edge_pass`, but grades it at
  an early active-encounter checkpoint:
  `AVDCOL_RANGE_BEN <= 20` while `COLREGS_AVOID_IX_BEN = 20`
- that early gate is the stable point where the knob changes the realized
  side-of-pass state without forcing the family into the unsafe late
  completion dynamics seen in the original turn-gap probes
- at that gate, the stock `pts_port_turns_ok=true` case keeps the obstacle
  ship on port (`cn_port=1`), while `pts_port_turns_ok=false` keeps the same
  `giveway:stern` mode but flips the early side state to starboard
  (`cn_port=0`)
- room for improvement: this family still infers maneuver choice from early
  relative-side state, not from a direct helm-direction signal; a stronger
  future version would expose a clean mission-visible turn-direction metric

## Parameter Inventory

This section lists the behavior-specific `BHV_AvdColregsV22` parameters exposed
directly through `setParam()`. It is the right H04 inventory to reason about.
Generic inherited `IvPContactBehavior` parameters are not listed here.

### High Priority H04 Parameters

- `min_util_cpa_dist`
  - minimum acceptable CPA distance used throughout the COLREGS utility logic
  - default in V22 constructor: `10`
  - already implemented in H04
- `max_util_cpa_dist`
  - all-clear / outer utility CPA distance paired with `min_util_cpa_dist`
  - default in V22 constructor: `75`
  - high-value regression knob because it changes the clearance envelope
- `giveway_bow_dist`
  - explicit bow-crossing acceptance threshold for give-way logic
  - default in V22 constructor: disabled (`-1`)
  - high-value because it directly affects crossing-starboard behavior
- `headon_only`
  - if enabled, non-head-on COLREGS modes are suppressed and some cases demote
    to CPA
  - default in V22 constructor: `false`
  - already implemented in H04
- `use_refinery`
  - enables refinery-based IPF refinement for the generated function
  - default in V22 constructor: `false`
  - stem default in `colregs_unit`: `true`
  - already implemented in H04
- `pts_port_turns_ok`
  - controls whether point-turn style port turns are allowed
  - default in V22 constructor: `true`
  - already implemented in H04 as an early active-give-way checkpoint family
- `eval_tol`
  - evaluation time horizon / tolerance used in the behavior logic
  - default in V22 constructor: `120`
  - worth testing because it can change whether encounters are treated as
    actionable under the same geometry
- `velocity_filter`
  - filters contact-motion interpretation before COLREGS logic uses it
  - no single scalar constructor default; parsed as a filter spec
  - high-value because it can change perceived encounter validity and mode
    entry/release

### Medium Priority H04 Parameters

- `pwt_inner_dist`
  - inner relevance distance for behavior priority weight activation
  - default in V22 constructor: `90`
- `pwt_outer_dist`
  - outer relevance distance for behavior priority weight activation
  - default in V22 constructor: `100`
- `completed_dist`
  - distance used for completion semantics, also constrained against the
    relevance distances
  - default in V22 constructor: `100`
  - now implemented in H04 as a shared-timeout completion split
- `pwt_grade`
  - relevance-weight falloff style: `linear`, `quadratic`, or `quasi`
  - default in V22 constructor: `linear`
- `check_plateaus`
  - refinery diagnostic/sanity option
  - default in V22 constructor: `false`
  - now implemented in H04
- `check_validity`
  - refinery validity-check option
  - default in V22 constructor: `false`
  - currently not implemented in H04 because `m_check_validity` is parsed in
    `BHV_AvdColregsV22::setParam()` but not used elsewhere in the current
    source
- `pcheck_thresh`
  - refinery plateau/validity threshold
  - default in V22 constructor: `0.001`
  - currently not implemented in H04 because the current CPA path uses
    `AOF_CPA`, which does not report known min/max values; as a result,
    `OF_Reflector::checkPlateaus()` returns the same early-fail diagnostic
    regardless of threshold on the current supported geometry
- `contact_type_required`
  - restricts behavior application to a required contact type
  - no stem default
- `turn_radius`
  - turn-radius assumption used in turn-gap style reasoning
  - default in V22 constructor: `0`

These matter, but they are secondary to the direct COLREGS decision knobs above.
I would only expand into these after the high-priority set is in good shape.

### Low Priority Or Usually Out Of Scope

- `no_alert_request`
  - suppresses alert-request behavior
  - default in V22 constructor: `false`
- `post_status_info_on_idle`
  - diagnostic/status-posting behavior while idle
  - default in V22 constructor: `false`
- `verbose`
  - extra diagnostic logging
  - default in V22 constructor: `false`
- `can_disable`
  - generic enable/disable policy hook

These are valid parameters, but they are weak H04 material because they are
mostly diagnostic, integration, or behavior-lifecycle controls rather than
core COLREGS logic.

### Supported But Not Normal H04 Targets

- `avoid_mode`
  - forces or resets the behavior mode directly
- `avoid_submode`
  - forces or resets the behavior submode directly

These are useful for manual probing or debugging, but not for a parameter
regression harness whose purpose is to test natural behavior selection.

## Recommended H04 Roadmap

Current implemented groups:
- `min_util_cpa_dist`
- `headon_only`
- `giveway_bow_dist`
- `max_util_cpa_dist`
- `velocity_filter`
- `eval_tol`
- `pwt_outer_dist`
- `pwt_inner_dist`
- `pwt_grade`
- `pts_port_turns_ok`
- `turn_radius`
- `check_plateaus`
- `completed_dist`
- `use_refinery`

Recommended next groups:
1. a source fix upstream for `check_validity` / `pcheck_thresh`, then revisit
   those as real H04 families
2. decide whether any lower-value integration/filtering knobs are worth
   including at all without changing the app

Practical signoff note:
- for the current no-app-change scope, H04 is effectively complete
- the remaining gaps are either source-dead (`check_validity`), source-blocked
  on the CPA path (`pcheck_thresh`), or lower-value integration controls

Recommended test style per parameter:
- mode-selection knobs:
  - same geometry, expected classification change
- execution-shaping knobs:
  - same geometry, same classification, but bounded difference in safe outcome
- diagnostic/integration knobs:
  - usually do not belong in H04 unless they materially affect behavior
