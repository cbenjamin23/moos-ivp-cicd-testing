# H01-pnodereporter_unit

Patch-driven harness for
[`missions/pnodereporter_missions/pnodereporter_unit`](../../../missions/pnodereporter_missions/pnodereporter_unit).
It isolates `pNodeReporter` report construction in one MOOS community with
scripted navigation, helm, platform, and status mail.

Harness result rows prepend `case=<name>` to the mission result row and retain
the supplemental `token_check` field for structured report payload checks.
This is a documented exception to sole mission-owned grading because the exact
comma-delimited/JSON report representation is the subject of these tests and
contains dynamic fields that `pMissionEval` cannot match directly.

## Current Matrix

- `nav_report_baseline_pass` Baseline CSP node report; expects scripted NAV fields, platform color/type/length, helm mode, and first-report publication.
- `platform_metadata_pass` Overrides platform metadata and expects group, color, type, length, beam, and custom platform aspects to ride in the report.
- `helm_nohelm_mode_pass` Withholds helm state and expects the generated report mode to mark the helm as absent.
- `helm_standby_switch_pass` Sends standby helm state and expects the standby state to become the visible report mode.
- `helm_stale_threshold_pass` Lowers the no-helm threshold and expects a once-valid helm state to age into `NOHELM`.
- `color_change_blocked_pass` Posts a runtime color change without enabling it and expects the configured color to remain.
- `color_change_allowed_pass` Enables runtime color changes and expects `NODE_COLOR_CHANGE` to update the report color.
- `platform_report_pass` Configures platform-report inputs and expects aliased telemetry to publish through `PLATFORM_REPORT_LOCAL`.
- `platform_report_gap_pass` Adds a platform-report post gap and expects the later telemetry value to be held until the gap expires.
- `rider_payload_pass` Adds rider fields and expects additional mail values to ride along in the node report.
- `custom_output_var_pass` Routes node reports to a custom output variable and expects the default report variable to stay unused.
- `json_only_report_pass` Enables JSON-only reporting and expects the node report variable to carry JSON instead of CSP.
- `json_dual_report_pass` Enables dual CSP/JSON output and expects both report variables to publish.
- `alt_nav_report_pass` Enables alternate navigation reporting and expects a second `_GT` node report with the alternate group and NAV_GT values.
- `alt_nav_named_absolute_pass` Uses an absolute alternate-node name and expects depth and altitude from the alternate nav stream.
- `coord_policy_global_pass` Enables global-coordinate report policy and expects LAT/LON while suppressing X/Y fields.
- `heading_error_pass` Adds a heading error and expects the report heading to shift without changing the input NAV mail.
- `crossfill_local_to_global_pass` Uses local X/Y as authoritative and expects LAT/LON to be filled from the mission origin.
- `crossfill_global_to_local_pass` Uses global LAT/LON as authoritative and expects X/Y to be filled from the mission origin.
- `crossfill_global_terse_pass` Uses global-terse cross-fill and expects global coordinates to remain while local X/Y is synthesized from them.
- `node_group_update_pass` Posts `NODE_GROUP_UPDATE` at runtime and expects the node report group to change.
- `platform_color_mail_pass` Posts `PLATFORM_COLOR` at runtime and expects the report color to update.
- `reverse_load_warning_pass` Posts reverse-thrust mode and a load warning and expects both fields to ride in the report.
- `blackout_interval_reset_fail` Enables a two-second blackout interval and expects mission-owned evidence that the current posting gap remains below the sustained blackout threshold.
- `mhash_odometer_pass` Moves the node after mission-hash startup and expects `PNR_MHASH` odometry evidence.
- `report_cog_pass` Enables course-over-ground reporting and expects the report to include a computed COG field.
- `terse_reports_pass` Enables terse reports and expects depth, lat/lon, and yaw details to be omitted while core position remains.
- `vsource_aux_allstop_pass` Configures a vehicle source and posts aux/all-stop mail, expecting all three report fields.
- `pause_resume_pass` Starts paused, resumes at runtime, and expects the first visible report to carry the post-resume navigation update.
- `crossfill_fill_empty_pass` Uses `fill-empty` with only global coordinates and expects local X/Y to be synthesized.
- `crossfill_use_latest_local_pass` Uses `use-latest` and expects later local X/Y mail to override older global coordinates.
- `extrap_gap_metric_pass` Enables extrapolation checks and expects the extrapolation position/heading gap metrics to publish.
- `extrap_prune_pass` Sets permissive extrapolation thresholds and expects a predictable update to be pruned from node-report output.

## Run

```sh
./zlaunch.sh --jobs=2 --port_base=22000 --max_time=35 10
```

Run one inspectable case:

```sh
./zlaunch.sh --case=platform_report_pass --port_base=22000 --max_time=35 10
```

## Launcher Contract

Every path, including single-case and `--jobs=1`, runs from an isolated copy of
the stem. The launcher requires Bash 5.1 or newer and uses rolling refill for
`--jobs`. Each case receives a stride-30 block with its MOOSDB at `+0` and a
reserved shoreside pShare slot at `+15`; this mission does not launch pShare.

The copied stem's `pMissionEval` must write exactly one row with exactly one
`grade=pass|fail`. When that grade is `pass`, the harness checks the case's
explicit structured-payload fragments. Ordinary mission failures and payload
failures do not stop the remaining selected cases. Results are aggregated once
in declared case order. Preparation, launch, malformed-result, and teardown
failures use runner-owned failure rows.

The alternate-report cases repeat their navigation inputs after startup before
evaluation. `reverse_load_warning_pass` repeats its ephemeral warning before
pausing. For those three cases, the supplement accepts either the final mission
snapshot or a matching `NODE_REPORT_LOCAL` publication in the case `.alog`.
The expected fields are unchanged; this removes snapshot timing races without
weakening the case contract.

`--keep_workdirs` retains the invocation under `.harness_runs` for target,
sidecar, result, and `.alog` inspection. See [`NSPATCH.md`](NSPATCH.md).

## Migration Validation

The migrated harness passed three complete rolling matrices (33/33 each) in
101, 102, and 98 seconds, for a 100.33-second mean. It also passed the complete
isolated serial matrix in 193 seconds with canonical cleanup defaults. The
stabilized legacy runner averaged 108.48 seconds rolling and took 168.44
seconds serial. A supported one-second cleanup-grace diagnostic reduced the
migrated serial time to 178 seconds; canonical defaults remain unchanged.

Focused repetition and retained-log inspection verified the three
history-sensitive report cases. A real warp-1 timeout also produced the
required single `missing_result` failure row and left no process or listener.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case. The three
history-sensitive cases use their mission result payload as the primary grade;
minimal mode retains only `NODE_REPORT_LOCAL` for their historical fallback.
