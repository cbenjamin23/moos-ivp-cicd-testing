# COLREGS Harness TODO

This file tracks legitimate follow-on work that is still open in the core
COLREGS harness family.

The current signed-off core is:
- `H01-colregs_classification`
- `H02-colregs_thresholds`
- `H03-colregs_execution`
- `H04-colregs_parameters`

## Highest-Value Open H02 Families

- `standon:unsure_stern` supported threshold family
  Reason: `unsure_stern` (`34`) is source-real in `checkModeStandOn()`, but the
  current turn-driven and band350 exploratory geometries still do not produce a
  CI-honest repeatable family.

- stand-on turn-rate-qualified threshold family
  Reason: parts of the stand-on classifier depend on contact turn-rate, and the
  current backed-off Band #2 probes show that this matters in source, but they
  still do not yield one honest same-geometry supported family.

- `315 -> 270` stand-on band-boundary family
  Reason: the other two cross-band seams now have supported families, but this
  remaining seam is still dynamically blurrier and has not yet admitted a
  trustworthy `below / edge / above` triplet.

## Secondary H04 Follow-On

- stronger `use_refinery` path-quality assertion
  Reason: H04 already has a real `use_refinery` family, but it still mainly
  proves mode and diagnostics stability rather than a stronger path-quality
  difference on a trusted CPA geometry.

## Notes

- The open H02 work is not under-searched. These are the genuinely hard dynamic
  corners left after the stable threshold and parameter families were admitted.
- If future work resumes on the H02 dynamic seams, the likely leverage is a
  better evaluator contract for transition traces, not just more geometry
  tweaking.
