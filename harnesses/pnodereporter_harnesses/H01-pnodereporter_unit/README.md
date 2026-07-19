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

- `nav_report_baseline_pass`: Posts the complete baseline nav, platform, helm, all-stop, altitude, yaw, transparency, and trajectory inputs, testing normal CSP report assembly and the first-report variable; passes when both report variables publish and the shell finds `NAME=reporter`, `X=10`, `Y=-5`, `SPD=2.5`, `HDG=123`, `DEP=7`, `TYPE=kayak`, `COLOR=red`, `LENGTH=4.2`, `BEAM=1.4`, and `MODE=MODE@ACTIVE:TRANSIT`.
- `platform_metadata_pass`: Configures type `uuv`, color `blue`, length `9.5`, beam `2.1`, group `survey`, and platform aspects `PAYLOAD=ctd` and `ROLE=lead`, testing static metadata insertion; passes when every exact field is present in `NODE_REPORT_LOCAL`.
- `helm_nohelm_mode_pass`: Omits all helm-state mail while supplying valid navigation, testing the never-seen helm mode; passes when a node report publishes with `NAME=reporter` and `MODE=NOHELM-EVER`.
- `helm_standby_switch_pass`: Posts `IVPHELM_STATE=DRIVE+` with summary mode `MODE@ACTIVE:STANDBY_TRACK`, testing helm-summary mode extraction during standby; passes when the node report contains `MODE=MODE@ACTIVE:STANDBY_TRACK`.
- `helm_stale_threshold_pass`: Sets `nohelm_threshold=1`, posts `IVPHELM_STATE=DRIVE` once at 0.5 seconds, and evaluates at four seconds, testing stale helm-state labeling; passes when the node report mode begins `NOHELM-`.
- `color_change_blocked_pass`: Starts with configured color `red` and posts `NODE_COLOR_CHANGE=yellow` without enabling runtime color changes, testing the default rejection path; passes when the report contains `COLOR=red` and not `COLOR=yellow`.
- `color_change_allowed_pass`: Sets `allow_color_change=true` and posts `NODE_COLOR_CHANGE=yellow`, testing the permitted runtime color path; passes when the report contains `COLOR=yellow`.
- `platform_report_pass`: Maps `BATTERY_VOLTAGE` to `batt` and `GPS_SATS` to `sats` with zero gaps, then posts `12.7` and `9`, testing `PLATFORM_REPORT_LOCAL` field aliasing; passes when a platform report publishes with `platform=reporter`, `batt=12.7`, and `sats=9`.
- `platform_report_gap_pass`: Maps battery voltage to `batt` with `gap=2`, then posts `12.7` at one second and `12.8` at two seconds, testing that pNodeReporter holds the changed value until the configured publication gap expires; passes when the log contains `batt=12.7` followed 1.8–2.4 seconds later by `batt=12.8`.
- `rider_payload_pass`: Defines always-on riders for battery voltage with one-decimal precision and payload state, then posts `12.74` and `armed`, testing rider formatting and insertion; passes when the node report contains `batt=12.7` and `payload=armed`.
- `custom_output_var_pass`: Sets `node_report_output=MY_NODE_REPORT`, testing custom current and first-report variable names; passes when both `MY_NODE_REPORT` variables publish with `NAME=reporter` and the final result does not contain a CSP payload from `NODE_REPORT_LOCAL`.
- `json_only_report_pass`: Sets `json_report=true`, testing JSON replacement on the normal node-report variable; passes when `NODE_REPORT_LOCAL` begins a JSON object containing `"NAME":"reporter"` and `"TYPE":"kayak"`, and no CSP `NAME=reporter` form is present.
- `json_dual_report_pass`: Sets `json_report=NODE_REPORT_JSON`, testing simultaneous CSP and JSON output; passes when `NODE_REPORT_LOCAL` contains CSP `NAME=reporter` and `NODE_REPORT_JSON` contains JSON `"NAME":"reporter"`.
- `alt_nav_report_pass`: Configures alternate prefix `NAV_GT`, suffix name `_GT`, and group `truth`, then repeatedly posts `(20,-15)`, speed `3.1`, and heading `210`, testing alternate-report construction; passes when a current result or retained report contains `NAME=reporter_GT`, `GROUP=truth`, and every exact alternate nav value.
- `alt_nav_named_absolute_pass`: Configures alternate prefix `NAV_TRUTH`, absolute name `truth_node`, and group `truth_abs`, then posts position `(22,-12)`, depth `4`, and altitude `40`, testing absolute alternate naming plus vertical fields; passes when a current or retained report contains all those exact fields.
- `coord_policy_global_pass`: Sets `coord_policy_global=true` while supplying both local `(10,-5)` and global `(43.82535,-70.33045)` navigation, testing that global-only reports retain the supplied latitude/longitude and omit local coordinates entirely; passes when those global values are within `0.000001` degrees and neither an `X` nor `Y` component exists.
- `heading_error_pass`: Configures `hdg_error=5` while the input remains `NAV_HEADING=123`, testing report-only heading bias; passes when the report contains `HDG=128` and not `HDG=123`.
- `crossfill_local_to_global_pass`: Sets `cross_fill_policy=local` and supplies only local position `(12,-8)`, testing conversion from the configured datum to global coordinates; passes when the report retains `(12,-8)` and generates latitude `43.82522971` and longitude `-70.33024916` within `0.000001` degrees.
- `crossfill_global_to_local_pass`: Sets `cross_fill_policy=global` and supplies only latitude `43.825360` and longitude `-70.330460`, testing conversion from that global pair into the local datum; passes when the global inputs remain unchanged and the generated position is within `0.01` meters of `(-4.72,6.74)`.
- `crossfill_global_terse_pass`: Sets `cross_fill_policy=global-terse` with latitude `43.825360`, longitude `-70.330460`, and conflicting local position `(99,99)`, testing that the global pair overrides supplied local coordinates; passes when the report contains the global values and derived local position `(-4.72,6.74)` rather than `(99,99)`.
- `node_group_update_pass`: Posts `NODE_GROUP_UPDATE=dynamic` after startup, testing runtime group replacement; passes when the node report contains `GROUP=dynamic`.
- `platform_color_mail_pass`: Posts `PLATFORM_COLOR=orange`, testing the platform-color mail path distinct from `NODE_COLOR_CHANGE`; passes when the node report contains `COLOR=orange`.
- `reverse_load_warning_pass`: Repeats `THRUST_MODE_REVERSE=true` and `LOAD_WARNING="app=pHelmIvP,maxgap=5"` before pausing, testing report riders for reverse mode and the normalized load warning; passes when a current or retained node report contains `THRUST_MODE_REVERSE=true` and `LOAD_WARNING=pHelmIvP:5`.
- `blackout_interval_reset_fail`: Configures `blackout_interval=2.0`, preserving a known bug in which the interval resets to zero after a post instead of sustaining the blackout; the harness passes only while a report is seen and `PNR_POST_GAP<1.8`.
- `mhash_odometer_pass`: Moves from `(0,0)` through `(8,0)` to `(20,0)` after mission-hash startup, testing that pNodeReporter accumulates the furthest displacement in `PNR_MHASH`; passes when the publication contains a hash and exact `ext=20`.
- `report_cog_pass`: Enables `report_cog` and moves from `(0,0)` to `(8,0)`, testing course-over-ground derived from consecutive positions rather than the supplied heading field; passes when the report history contains the moved position with `COG=90`.
- `terse_reports_pass`: Enables `terse_reports` with the full baseline input set, testing omission of non-terse nav details; passes when core name, x/y, speed, heading, type, and color fields remain while every `DEP=`, `LAT=`, `LON=`, and `YAW=` field is absent.
- `vsource_aux_allstop_pass`: Configures `vsource=ais`, posts `AUX_MODE=survey`, and posts helm all-stop reason `manual`, testing those three optional report fields; passes when the report contains `VSOURCE=ais`, `MODE_AUX=survey`, and `ALLSTOP=manual`.
- `pause_resume_pass`: Starts pNodeReporter paused, posts `(1,1)`, updates the stored nav state to `(30,-10)` while still paused, and then posts `PNR_PAUSE=false`, testing both suppression and release of accumulated state; passes when no `(1,1)` report exists and the first report after resume contains `(30,-10)`.
- `crossfill_fill_empty_pass`: Sets `cross_fill_policy=fill-empty` and supplies only latitude `43.825370` and longitude `-70.330470`, testing that only the missing local pair is generated; passes when the global inputs remain unchanged and the generated position is within `0.01` meters of `(-5.50,7.87)`.
- `crossfill_use_latest_local_pass`: Sets `cross_fill_policy=use-latest`, posts global coordinates first, then local `(25,5)` one second later, testing that the newer local source replaces and regenerates the older global source; passes when the local pair is exact and the regenerated latitude/longitude are within `0.000001` degrees of `(43.82534865,-70.33009010)`.
- `extrap_gap_metric_pass`: Enables extrapolation with zero thresholds, establishes eastbound motion at one meter per second, then jumps `NAV_X` from `0` to `3`, testing the discrepancy between the predicted and newly observed positions; passes when history records a position gap from `2.4` to `2.9` meters and a heading gap from `0` to `0.01` degrees.
- `extrap_prune_pass`: Enables extrapolation with ten-unit thresholds, establishes eastbound motion at `(0,0)`, then posts `NAV_X=12` after the extrapolator has capped its prediction at `5`, testing that the seven-meter discrepancy is measured but the below-threshold report is pruned; passes when the final gap is `6.5–7.5`, heading gap is below `0.1`, and no report contains `X=12`.

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

Focused repetition and retained-log inspection verified the platform-gap,
COG, pause/resume, and extrapolation-history contracts. A real warp-1 timeout
also produced the required single `missing_result` failure row and left no
process or listener.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for one fully logged diagnostic case. Minimal
mode retains the node report, platform report, mission-hash odometry, and
extrapolation metrics needed by the structured-payload and history checks.
