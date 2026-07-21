# Harness Result Semantics Migration

## Goal

Make harness output directly presentable by making each mission result row express
the test verdict itself. A negative scenario should produce `grade=pass` when the
expected negative condition is observed.

The target harness output is:

```text
case=<case_name>  grade=<pass|fail>  eval=<true|false>  <case evidence fields...>
```

In the normal path, the harness should prepend `case=<case_name>` to the mission
result row and otherwise preserve the mission evidence fields. The harness should
only synthesize a row when the runner itself cannot obtain a mission result, for
example:

```text
case=<case_name>  grade=fail  reason=launch_error  launch_rc=<rc>
case=<case_name>  grade=fail  reason=missing_result
```

This removes the previous inverted pattern:

```text
case=<case_name>  <legacy harness success wrapper>  <expected/actual inversion>  grade=fail ...
```

The desired public contract is simpler: `grade=pass` means the case passed.

Mission criteria failures do not automatically get a synthetic `reason` field.
When `pMissionEval` grades `fail`, the row should still include the evidence
columns that explain which criterion was not satisfied, such as
`helm_malconfig=false`, `collisions=0`, `wpt_done=true`, `arrived=false`, or
`detected=true`. The `reason` field is reserved for harness-owned runner
failures such as launch, preparation, or missing-result failures unless a
mission explicitly chooses to report a compact mission-owned reason.

## Result Line Contract

Required fields:

- `case`: harness-owned case identifier.
- `grade`: final case verdict. `pass` means the case passed; any other value or
  missing row means the case failed.

Strongly recommended fields:

- `eval`: whether the mission reached its evaluation point.
- An explicit evidence field for the condition that justified the verdict, such
  as `helm_malconfig=true`, `process_loss=true`, `arrived=false`,
  `collisions=1`, `bhv_error=true`, or `warning=true`.
- `evidence`: optional compact label for the scenario-specific signal when a
  short reader-facing name is useful. Prefer this over `expected=<scenario>` for
  new or newly migrated rows.

Optional provenance fields:

- `form`: pMissionEval mission form or family label.
- `mmod`: mission mode injected by the launch path. Useful in raw mission
  `results.txt`, but redundant with harness-owned `case` in exported output.
- `mhash`: mission hash or short hash when available.

If both `case` and `mmod` are present, they should usually match. A mismatch is
useful evidence of launcher wiring trouble unless explicitly documented.

## Mission Versus Harness Responsibility

Mission-owned responsibilities:

- Decide scenario correctness with `pMissionEval`.
- Emit `grade=pass` for expected negative scenarios when the expected negative
  evidence is observed.
- Emit case-specific evidence fields that explain the grade.
- Keep metrics as rigorous as before; do not replace a strict check with a vague
  absence of success.

Harness-owned responsibilities:

- Launch each case, isolate ports/workdirs, and collect the mission result row.
- Emit the mission row directly with `case=<case_name>` prepended.
- Synthesize only runner-failure rows, such as `reason=launch_error` or
  `reason=missing_result`.
- Exit nonzero if any emitted case row lacks `grade=pass`.

Summary scripts may format or count rows, but should not reinterpret
expected/actual semantics.

During migration, summary tooling may derive a compatibility `result` from
historical rows with an explicit legacy harness-result field. Public summaries
should still lead with the case result and mission grade, not the old
expected/actual inversion.

For harnesses whose subject is mission utility machinery itself, preserve the
outer harness verdict as `grade=<pass|fail>` and rename the utility-under-test
verdict to `subject_grade=<pass|fail>`. If the case is checking a non-success
utility outcome, such as pMissionEval producing a fail row or uMayFinish timing
out, report that as `expected_subject=<fail|timeout>` rather than reviving the
generic harness `expected`/`actual` pair.

## Example

Expected bad configuration rejection:

```text
case=bad_speed_rejected_pass  grade=pass  eval=true  evidence=helm_malconfig  helm_malconfig=true  wpt_done=false
```

Regression where bad configuration is accepted:

```text
case=bad_speed_rejected_pass  grade=fail  eval=true  evidence=helm_malconfig  helm_malconfig=false  wpt_done=true
```

pMissionEval fail behavior tested as the subject:

```text
case=eval_false_condition_fail  grade=pass  subject=pMissionEval  expected_subject=fail  subject_rc=0  form=mission_utility_unit  mmod=eval_false_condition_fail  subject_grade=fail  eval=true  pass_input=false
```

uMayFinish timeout behavior tested as the subject:

```text
case=mayfinish_timeout_fail  grade=pass  subject=uMayFinish  expected_subject=timeout  subject_rc=1
```

Runner failure before mission result:

```text
case=bad_speed_rejected_pass  grade=fail  reason=missing_result
```

## Migration Principles

- Preserve or improve evidence. Negative cases should pass only when the expected
  failure signal is explicitly present.
- Prefer mission-owned grades over harness-owned interpretation.
- Keep the exported shape small. Avoid replacing `expected/actual` with a larger
  public enum unless a concrete consumer needs it.
- Avoid new `expected=<scenario>` labels. Use concrete evidence fields, plus
  `evidence=<label>` only when a compact scenario label helps readability.
- Treat `_fail` suffix renames as presentation improvements, not the first
  blocker. The first behavioral target is `grade=pass` for expected negative
  scenarios.
- Exempt harnesses whose subject is failure machinery itself until they are
  reviewed separately.

## Likely Exemptions And Special Cases

- `harnesses/mission_utility_harnesses/H01-pmissioneval_unit`: retains real
  `subject_grade=fail` mission rows because it tests pMissionEval fail behavior
  directly; the harness case still reports `grade=pass` when that subject result
  is expected.
- `harnesses/mission_utility_harnesses/H03-umayfinish_unit`: direct timeout and
  nonzero-return behavior is exported as `expected_subject=timeout` with the
  observed `subject_rc`.
- `harnesses/uquerydb_harnesses/H01-uquerydb_unit`: nonzero CLI return-code
  cases are not pMissionEval inversion and should be handled as CLI semantics.

## Harness Inventory

The expected-negative behavior/config and route-resilience harnesses have been
migrated through `harnesses/ufield_comms_harnesses/H03-ufield_route_resilience`.
The pNodeReporter blackout edge has also been converted so the mission owns the
expected-negative evidence. Its harness wrapper now uses the direct `grade=pass`
shape and keeps `token_check=pass` only as supplemental structured-payload
evidence. The mission-utility family now uses the edge-subject shape described
above.

All 67 harness launchers have been normalized away from the legacy harness
success wrapper. Non-empty harness `results.txt` files now use `case=<name>` and
place `grade=...` immediately after the case field. Empty sample-output files
remain empty until those harnesses are run.

Earlier migrated harnesses may still use `expected=<scenario>` as a compact
evidence label. Treat that as legacy-compatible wording, not the preferred
shape for new migrations. A future cleanup can rename those labels to
`evidence=<scenario>` harness by harness with focused validation, but this
migration does not require a broad churn pass to preserve rigor.

## Implementation Plan

### Pilot Status

`harnesses/testfailure_behavior_harnesses/H01-testfailure_behavior_unit` has
been migrated as the first pilot. The two expected process-loss cases now
produce `grade=pass` when `HELM_PRESENT=false`, `PROC_WATCH_ALL_OK=false`, and
`ULW_BREACH_COUNT=0`; their result rows include `expected=process_loss`.

Validated paths:

- Serial full matrix: `./zlaunch.sh --port_base=18220 10`
- Grouped full matrix: `./zlaunch.sh --jobs=4 --port_base=18660 10`
- Focused validation confirmed the expected case count and new result shape.

`harnesses/obmgr_harnesses/H02-obmgr_motion` has been migrated as the second
pilot. The expected collision stress cases now produce `grade=pass` when the
mission observes arrival, the expected encounter count, and exactly one
collision; their result rows include `expected=collision`.

Validated paths:

- Serial full matrix: `./zlaunch.sh --port_base=18940 10`
- Grouped full matrix: `./zlaunch.sh --jobs=4 --port_base=19260 10`
- Focused validation confirmed the expected case count and new result shape.

`harnesses/waypoint_behavior_harnesses/H01-waypoint_behavior_motion` has been
migrated as the third pilot. Expected timeout/update-rejection cases now produce
`grade=pass` when waypoint completion remains absent at the timer verdict, with
`expected=no_waypoint_completion`. Expected bad launch-time behavior
configuration cases now produce `grade=pass` when the helm reports
`HELM_MALCONFIG=true`, with `expected=helm_malconfig`.

Validated paths:

- Focused no-completion case:
  `./zlaunch.sh --case=no_points_timeout_fail --port_base=19600 10`
- Focused helm-malconfig case:
  `./zlaunch.sh --case=bad_speed_fail --port_base=19720 10`
- Focused xpoints rejection edge case:
  `./zlaunch.sh --case=bad_xpoints_size_fail --port_base=19780 10`
- Grouped full matrix: `./zlaunch.sh --jobs=4 --port_base=20000 10`
- Serial full matrix: `./zlaunch.sh --port_base=21400 10`

`harnesses/processwatch_harnesses/H01-processwatch_unit` has also been migrated
as the first small post-pilot harness. Its expected-AWOL case now produces
`grade=pass` when `uProcessWatch` reports `PROC_WATCH_ALL_OK=false`,
`GHOST_PRESENT=false`, and an AWOL summary for the missing process, with
`expected=watch_awol`.

Validated paths:

- Focused expected-AWOL case:
  `./zlaunch.sh --case=processwatch_missing_awol_fail --port_base=22800 --max_time=40 10`
- Grouped full matrix:
  `./zlaunch.sh --jobs=3 --port_base=22900 --max_time=40 10`
- Serial full matrix:
  `./zlaunch.sh --port_base=23150 --max_time=40 10`

`harnesses/pid_harnesses/H02-pid_motion` has been migrated as the first
closed-loop motion harness beyond the pilot set. Its expected-negative cases now
produce `grade=pass` when explicit degraded-motion evidence is observed:
low rudder authority, low thrust authority, low elevator authority, disabled
depth control, or held manual override.

Validated paths:

- Focused expected-negative cases:
  `low_rudder_authority_fail`, `low_thrust_authority_fail`,
  `low_elevator_authority_fail`, `depth_control_disabled_fail`,
  `manual_override_fail`
- Grouped full matrix:
  `./zlaunch.sh --jobs=3 --port_base=23800 10`
- Serial full matrix:
  `./zlaunch.sh --port_base=24300 10`

`harnesses/cmgr_harnesses/H02-cmgr_motion` has been migrated as the first
contact-manager motion harness. Its expected-negative cases now produce
`grade=pass` when contact-manager and collision-detection evidence confirms the
intended degraded scenario: late alert with no detection, encounter without an
avoid-spawned path, runtime-disabled alert, or fast-intruder encounter.

Validated paths:

- Focused expected-negative cases:
  `tight_alert_fail`, `avoid_disabled_fail`, `runtime_alert_disable_fail`,
  `fast_intruder_fail`
- Grouped full matrix:
  `./zlaunch.sh --jobs=4 --port_base=25700 10`
- Serial full matrix:
  `./zlaunch.sh --port_base=26250 10`

`harnesses/pnodereporter_harnesses/H01-pnodereporter_unit` has been normalized
for this migration. The blackout case now passes only when the current
implementation's posting gap remains below the sustained blackout threshold,
with `evidence=blackout_gap_below_threshold`. The wrapper exports the mission
row directly with `case=<name>` prepended and retains `token_check=pass` for the
structured `NODE_REPORT` payload checks.

`harnesses/pid_harnesses/H01-pid_unit` has been migrated as the app-level PID
companion to `H02-pid_motion`. Its two current-bug cases now produce
`grade=pass` only when the documented buggy evidence is observed:
simulation mode leaves zero actuator output with control still true, and runtime
thrust-cap mail leaves thrust uncapped near `100`.

Validated paths:

- Focused current-bug cases:
  `simulation_mode_fail`, `thrust_cap_runtime_fail`
- Grouped full matrix:
  `./zlaunch.sh --jobs=4 --port_base=27620 10`
- Serial full matrix:
  `./zlaunch.sh --port_base=28320 10`

`harnesses/legrun_behavior_harnesses/H01-legrun_behavior_motion` has been
migrated. Its expected bad-configuration cases now produce `grade=pass` when
the mission reaches the malconfiguration timeout without completing the second
turn and without a behavior runtime error, with `expected=malconfig_timeout`.

Validated paths:

- Focused expected-negative cases:
  `bad_leg_config_fail`, `bad_speed_count_fail`
- Grouped full matrix:
  `./zlaunch.sh --jobs=4 --port_base=30120 10`
- Serial full matrix:
  `./zlaunch.sh --port_base=31200 10`

`harnesses/timer_behavior_harnesses/H01-timer_behavior_motion` has been
migrated. Its expected-negative cases now produce `grade=pass` when the expected
timer-output evidence is observed, such as idle-only output, intentionally
absent stock status counters, or a silent remapped running counter.

Validated paths:

- Focused expected-negative cases with assigned port band:
  `--port_base=29700`
- Grouped full matrix:
  `./zlaunch.sh --jobs=4 --port_base=29700 --port_stride=15 10`

`harnesses/fixedturn_behavior_harnesses/H01-fixedturn_behavior_motion` has been
migrated. Its expected invalid-configuration cases now produce `grade=pass` when
the behavior does not complete, the timeout is reached, the vehicle remains
stationary near the starting heading, and no behavior runtime error is posted,
with `expected=invalid_config_rejected`.

Validated paths:

- Focused expected-negative cases with assigned port band:
  `--port_base=29400`
- Grouped full matrix:
  `./zlaunch.sh --jobs=4 --port_base=29400 --port_stride=12 10`

`harnesses/loiter_behavior_harnesses/H01-loiter_behavior_motion` has been
migrated. Most expected bad launch-time behavior configurations now pass on
`HELM_MALCONFIG=true` with `expected=helm_malconfig`; the bad runtime update
case passes on explicit warning-and-stability evidence with
`expected=bad_update_rejected`.

Validated paths:

- Focused expected-negative cases:
  `empty_polygon_fail`, `bad_polygon_fail`, `bad_update_fail`,
  `bad_clockwise_fail`, `bad_use_alt_speed_fail`, `bad_patience_fail`,
  `bad_capture_radius_fail`
- Serial full matrix through per-case runs:
  `--port_base=29100`
- Grouped full matrix after review cleanup:
  `./zlaunch.sh --jobs=4 --port_base=33000 --port_stride=12 10`

`harnesses/depth_behavior_harnesses/H05-min_altitude_motion` has been migrated.
Its expected-negative cases now produce `grade=pass` on explicit evidence:
zero-altitude rejection with a behavior error, bad behavior configuration with
`HELM_MALCONFIG=true`, or absent nav/depth mail with neutral elevator output.

Validated paths:

- Focused expected-negative cases:
  `min_altitude_zero_altitude_fail`, `min_altitude_bad_config_fail`,
  `min_altitude_negative_min_fail`, `min_altitude_bad_critical_bool_fail`,
  `min_altitude_missing_nav_fail`,
  `min_altitude_noncritical_missing_altitude_fail`
- Grouped full matrix:
  `./zlaunch.sh --jobs=3 --port_base=35400 --port_stride=12 --max_time=80 10`

`harnesses/periodic_speed_behavior_harnesses/H01-periodic_speed_behavior_motion`
has been migrated. Expected bad periodic-speed parameter cases now produce
`grade=pass` when `HELM_MALCONFIG=true` is observed. During validation, the
`busy_to_lazy_pass` case was also tightened to accept `PS_PENDING_ACTIVE >= 3`,
matching the documented lazy-state evidence at the evaluation tick.

Validated paths:

- Focused expected-negative case:
  `./zlaunch.sh --case=bad_period_lazy_fail --port_base=34000 --port_stride=12 10`
- Grouped full matrix:
  `./zlaunch.sh --jobs=4 --port_base=34000 --port_stride=12 10`

`harnesses/zigzag_behavior_harnesses/H01-zigzag_behavior_motion` has been
migrated. Expected bad zigzag configuration cases now produce `grade=pass` when
the helm reports malconfiguration with `expected=helm_malconfig`.

Validated paths:

- Focused expected-negative sweep over all 10 bad-config cases:
  `--port_base=34800 --port_stride=12 --max_time=40`
- Grouped full matrix:
  `./zlaunch.sh --jobs=4 --port_base=34800 --port_stride=12 10`

`harnesses/memoryturnlimit_behavior_harnesses/H01-memoryturnlimit_behavior_motion`
has been migrated. Bad launch-time limiter values now pass on
`HELM_MALCONFIG=true`; missing required limiter parameters now pass on explicit
behavior-warning evidence without a behavior error.

Validated paths:

- Focused expected-negative sweep over all five negative cases:
  `--port_base=34400 --port_stride=12`
- Grouped full matrix:
  `./zlaunch.sh --jobs=4 --port_base=34400 --port_stride=12 10`

### Subagent Experiment

The subagent pass was useful for throughput but still needs reviewer ownership.
Timer and fixedturn followed the migration pattern cleanly and added compact
`--port_stride` support for assigned port bands. Loiter produced a semantically
stronger patch by changing the expected-negative criteria to explicit helm
malconfiguration or warning/stability evidence, but needed an oversight cleanup
to expose and validate the compact `--port_stride` option. The second wave
confirmed the same pattern: periodic-speed, zigzag, and memoryturnlimit moved
quickly, but oversight still caught cleanup work in memoryturnlimit's leftover
`EXPECTED` bookkeeping and flagged periodic-speed's adjacent threshold change
for explicit review. The practical model is: delegate one harness per worker
with disjoint write ownership, require each worker to report exact validation
commands and port ranges, then independently review diffs and rerun cheap
summaries plus at least one representative grouped validation for any patch that
changes launcher mechanics.

### Phase 1: Contract And Harness Helper Pattern

Define a small shell helper pattern for harnesses:

```sh
emit_case_row() {
    case_name="$1"
    result_line="$2"
    if [ "$result_line" = "" ]; then
        echo "case=$case_name  grade=fail  reason=missing_result"
    else
        echo "case=$case_name  $result_line"
    fi
}
```

Each harness can adapt this locally at first. A shared helper can be extracted
later only if repetition becomes a maintenance problem.

### Phase 2: Pilot Harnesses

For each pilot harness:

1. Change expected-negative mission patches so `pMissionEval` grades pass when
   the negative evidence is observed.
2. Keep or add clear evidence fields in the mission result row.
3. Change the harness aggregation line to `case=<name> <mission result row>`.
4. Change harness success logic to require `grade=pass`.
5. Update README case descriptions.
6. Run each pilot harness serially and grouped with a non-overlapping
   `--port_base`.

### Phase 3: Summary And Docs Compatibility

During the migration, summary tooling treated rows with `grade=pass` as
successful and rows without `grade=pass` as failed. The workflow-only summary
and repeatability helpers were later removed when their workflow was retired.

### Phase 4: Bulk Harness Migration

Migrate harnesses by family, not all at once. For each family:

1. Convert mission criteria for expected-negative cases.
2. Convert harness result emission.
3. Update README cases.
4. Regenerate committed `results.txt` only if the repo intends to keep sample
   outputs checked in for that harness.
5. Run grouped validation for the changed harness.

### Phase 5: Generated Docs And Context

After source READMEs and catalog text are updated:

1. Update `docs/tools/build_pages.py` strings that mention expected-fail cases.
2. Regenerate generated harness pages if that is part of the current docs
   workflow.
3. Regenerate the traversal map:

```sh
python3 scripts/generate_context_graph.py
python3 scripts/generate_context_graph.py --check
```

### Phase 6: CI Invariants

Run static and representative dynamic validation:

```sh
python3 scripts/list_harness_targets.py validate
python3 scripts/check_repo_invariants.py
python3 scripts/generate_context_graph.py --check
```

For changed harnesses:

```sh
./zlaunch.sh --jobs=4 --port_base=<unique_base> 10
test "$(grep -c 'grade=pass' results.txt)" -gt 0
```

Do not run two harness batches at the same time if their MOOSDB or pShare port
blocks could overlap.

## Open Decisions

- Whether to rename `_fail` cases immediately or after grade semantics are
  migrated.
- Whether to remove optional `form` and `mmod` from harness-exported lines or
  simply tolerate them when mission rows already include them.
- Whether to introduce a shared harness result helper after the pilot phase.
- Whether committed `results.txt` files should remain checked in or be treated
  as generated local artifacts.
