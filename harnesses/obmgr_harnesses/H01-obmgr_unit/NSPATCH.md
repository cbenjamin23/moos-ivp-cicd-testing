# NSPatch Notes For H01-obmgr_unit

This harness uses `nspatch` to turn one obstacle-manager source stem into 30
short `pObstacleMgr` cases. Every case first receives its own stem copy, so
neither serial nor rolling runs patch the source mission in place.

## Stem And Generated Sidecars

The source templates are:

- `missions/obmgr_missions/obmgr_unit/meta_vehicle.moos`
- `missions/obmgr_missions/obmgr_unit/meta_shoreside.moos`

Within each isolated case copy, `nspatch` may generate:

- `meta_vehicle.moosx`
- `meta_shoreside.moosx`

This matrix currently has no behavior overlays, so it does not generate a
`meta_vehicle.bhvx`. The copied launchers use `nsplug -x`, which folds any
generated sidecars into that case's `targ_*` files.

## What Gets Patched

Typical patch targets are:

- `ProcessConfig = pObstacleMgr`
  For alert ranges, general-alert configuration, lasso mode, ignore range,
  point aging, duration limits, or enable/disable/expunge variables.
- `ProcessConfig = uTimerScript`
  For obstacle injection timing and the obstacle or point messages sent into
  `pObstacleMgr`.
- `ProcessConfig = pMissionEval`
  For case-specific accepted/rejected, alerted/not-alerted, resolution,
  distance-band, payload, or filter-message grading.
- `ProcessConfig = uFldNodeBroker`
  Only when a case needs an additional bridged variable on shoreside.

Vehicle-side patches usually change what obstacle manager sees. Shoreside
patches usually change the evaluation checkpoint, mission-owned conditions,
and the columns written to `results.txt`.

## Per-Case Flow

For each selected case, the harness:

1. Copies the source stem beneath a harness-owned run root and runs the copied
   `clean.sh`.
2. Looks up the case's explicit shoreside and vehicle patch paths in
   `zlaunch.sh`.
3. Applies those patches to sidecars inside the copy.
4. Calls the copy's `zlaunch.sh` with the case token and its distinct ports.
5. Requires exactly one valid mission-owned `pMissionEval` result row.
6. Stops processes scoped to that case root and later publishes rows in
   selected-case order.

The source stem is read-only during this flow. `--keep_workdirs` preserves the
copies, sidecars, generated targets, result rows, and logs for inspection.

## Structured Payload Conditions

Four cases compare structured strings containing their own `=` assignments:

- `given_general_alert_pass`
- `general_alert_default_name_pass`
- `point_vsource_alert_pass`
- `disable_vsource_pass`

Embedding those complete literals directly in a condition is ambiguous to the
condition parser. Each shoreside patch therefore uses the existing
`uTimerScript` to publish the expected string as `OBM_EXPECTED_PAYLOAD`, then
uses that variable as the runtime right-hand side of a `pMissionEval`
condition. The same variable name is reused in all four cases. No new app and
no shell-side token check are involved.

The two general-alert comparisons preserve the same complete payloads checked
by the legacy launcher. The point-vsource and vsource-disable cases now use
complete whole-field equality, which is deliberately stricter than their
legacy substring checks. This is the mission-owned form of the existing
payload cases, not a redesign of the wider coverage strategy.

Seven expected-absence patches use:

```text
fail_condition = OBSTACLE_ALERT != ""
```

Those cases are `no_alert_request_absent_pass`,
`bad_alert_request_absent_pass`, `given_far_absent_pass`,
`given_max_duration_reject_absent_pass`,
`invalid_given_nonconvex_absent_pass`,
`invalid_point_missing_key_absent_pass`, and
`points_ignore_range_absent_pass`. This expresses the absence rule directly in
`pMissionEval` without placing a structured alert literal in the condition.

## Cleanup And Concurrency

Each case copy has its own port block and sidecars, so cases cannot overwrite
one another's generated mission. The canonical scoped teardown helper only
stops MOOS processes rooted in the case copy. The harness also holds one safety
lock for the invocation; do not start another invocation of this same harness
until the first has finished and released that lock.

## Adding A New Case

1. Decide whether the case changes obstacle input, app configuration,
   evaluation, or a combination of them.
2. Add the required `*.xmoos` patches in this harness directory. Prefer full
   `ProcessConfig` replacement when editing repeated keys such as `event`,
   `pass_condition`, `fail_condition`, or `report_column`.
3. Add the exact case token to the `CASES=(...)` list and its explicit patch
   paths to `get_case_config()` in `zlaunch.sh`.
4. Make the copied stem's `pMissionEval` produce `grade=pass` when the intended
   behavior is observed, including expected-negative behavior.
5. Add the exact token and intent to the `Current Matrix` in `README.md`.
6. Validate the case alone, then in serial and rolling execution with fresh
   port ranges.
