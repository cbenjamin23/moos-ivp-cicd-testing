# COLREGS Correctness Plan

This document outlines the intended correctness-harness structure for the
COLREGS family built around `BHV_AvdColregsV22`.

## Mission Split

Planned correctness split:
- `H01-colregs_classification`: classification-centric canonical two-vessel encounters
- `H02-colregs_thresholds`: geometry and parameter boundary checks
- `H03-colregs_execution`: realized maneuver quality checks
- `H04-colregs_parameters`: behavior-parameter regression suite

All four of the planned suites above should use the shared
`missions/colregs_missions/colregs_unit` stem mission.

Why:
- one canonical two-vessel stem keeps geometry comparable across suites
- `nspatch` overlays can vary encounter geometry, assertions, and behavior
  parameters without forking the mission
- the shared stem keeps manual-run behavior and viewer cues consistent

The current `colregs_motion` stem is still useful, but it should be treated as
the broader integration seed, not the base for `H01-H04`. It now maps to the
separate `H05-colregs_motion` slot for the broader multi-vessel suite.

## Core Principles

- `nspatch` should remain the main harness variation mechanism.
- The unit stem should stay narrow, stable, and heavily instrumented.
- Manual runs must be visually intuitive:
  - draw the contact leg or route line
  - draw enough geometry to make port/starboard pass side readable
  - expose CPA / closest-range cues where possible
- Mission-side evaluation should remain simple and robust.
- Harness-side evaluation should own most case-specific assertions.

## Planned Assertions By Suite

### H01 Classification

Primary questions:
- Which COLREGS mode did the behavior choose?
- Was the contact classified correctly?
- Did the simple encounter remain collision-free?

### H02 Thresholds

Primary questions:
- Do mode transitions occur at the expected edges?
- Are there unstable or surprising flips just around the thresholds?

### H03 Execution

Primary questions:
- Did ownship resolve on the expected side?
- What was the realized CPA?
- How long did resolution take?

### H04 Parameters

Primary questions:
- Do important tuning knobs still behave sensibly?
- Can we detect unsafe or qualitatively wrong regressions after parameter or
  source changes?

## Planned Metrics

Across the family, the intended measured outputs include:
- collision count
- near-miss count
- closest range / realized CPA
- time-to-resolution
- chosen COLREGS mode and index
- pass side relative to the contact track
- completion / all-clear state

## Visual Instrumentation Requirement

The viewer should make the encounter understandable without digging into raw
logs. Planned visual cues include:
- route or leg lines for ownship and contact
- geometry markers that make the encounter angle obvious
- optional CPA or closest-range markers where practical
- stable viewer framing across related cases for easier manual comparison
