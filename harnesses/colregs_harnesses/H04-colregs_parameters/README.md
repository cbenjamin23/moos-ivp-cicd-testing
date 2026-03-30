# H04-colregs_parameters

TLDR:
- parameter-regression harness on the shared `colregs_unit` stem
- varies selected `BHV_AvdColregsV22` knobs through `nspatch` overlays
- current implemented groups: `min_util_cpa_dist`, `headon_only`, and `use_refinery`

Parameter-sweep harness built on the shared `colregs_unit` stem mission and
using `nspatch` overlays on `BHV_AvdColregsV22`.

Primary intent:
- make parameter changes safe to reason about
- prove that important tuning knobs still produce expected, non-pathological
  behavior
- capture the accepted baseline before future refactors

## How This Harness Works

- keep geometry fixed for a small mini-suite
- vary one COLREGS parameter through a behavior-side `nspatch` overlay
- use a shoreside `nspatch` overlay to assert the expected classification or
  safe outcome

So this harness is where parameter changes are tested, not where geometry
changes are explored.

Current parameter groups:
- `min_util_cpa_dist`
- `headon_only`
- `use_refinery`

Current case structure:
- keep geometry fixed for each parameter-focused mini-suite
- vary one parameter group at a time in small, explicit steps
- compare mode selection and safety outcome first
- expand to CPA/time envelope checks later

Current implemented case list:
- `min_util_cpa_low_pass`
- `min_util_cpa_default_pass`
- `min_util_cpa_high_pass`
- `headon_only_true_pass`
- `headon_only_false_pass`
- `refinery_on_pass`
- `refinery_off_pass`

Current assertions:
- no collision
- no timeout
- expected `COLREGS_AVOID_IX_BEN`
- nonzero `UCD_CLOSEST_RANGE_EVER`

Visual/manual-run instrumentation:
- keep the viewer geometry consistent across parameter sweeps
- preserve track/leg overlays so manual comparison stays intuitive

Implemented expectations:
- `min_util_cpa_*`: head-on geometry should remain `headon` (`colregs_ix=10`)
- `headon_only_false_pass`: starboard crossing should remain give-way (`colregs_ix=20`)
- `headon_only_true_pass`: same crossing should demote to CPA (`colregs_ix=50`)
- `refinery_on/off_pass`: starboard crossing should remain give-way (`colregs_ix=20`)
