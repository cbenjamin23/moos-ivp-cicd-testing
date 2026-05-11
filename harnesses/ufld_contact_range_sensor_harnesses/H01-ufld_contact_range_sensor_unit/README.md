# H01-uFldContactRangeSensor Unit Harness

This harness tests `uFldContactRangeSensor` as a deterministic field-contact
range reporter. Cases script contact reports and `CRS_RANGE_REQUEST` mail, then
check range report variables, pulse publication, report mode, ping timing, echo
type filtering, and sensor-arc gating from the mission `.alog`.

## Current Matrix

- `baseline_short_report_pass` Verifies a near contact produces a short `CRS_RANGE_REPORT` and visual pulse.
- `report_vars_short_no_long_pass` Verifies `report_vars=short` does not also post the per-requester long variable.
- `report_vars_long_pass` Verifies `report_vars=long` posts only the per-requester long report variable.
- `report_vars_both_pass` Verifies `report_vars=both` posts both short and long report forms.
- `ground_truth_uniform_zero_pass` Verifies zero-noise uniform reporting posts matching reported and ground-truth ranges.
- `ground_truth_gaussian_unsupported_no_gt_pass` Verifies unsupported Gaussian `rn_algorithm` configuration does not activate ground-truth output.
- `ground_truth_long_only_pass` Verifies long-only ground-truth mode posts only per-requester long variables.
- `ground_truth_no_algorithm_absent_pass` Verifies `ground_truth=true` alone does not post ground-truth mail without a range-noise algorithm.
- `allow_echo_type_block_pass` Verifies `allow_echo_types` suppresses reports for a disallowed contact type.
- `allow_echo_type_accept_pass` Verifies `allow_echo_types` still permits a matching contact type.
- `allow_echo_type_multi_accept_ship_pass` Verifies a multi-type echo allow-list can admit a ship contact.
- `sensor_arc_forward_accept_pass` Verifies a forward contact inside the configured arc is reported.
- `sensor_arc_wrap_accept_pass` Verifies a wraparound sensor arc can report the same forward contact.
- `sensor_arc_multi_segment_aft_accept_pass` Verifies a multi-segment sensor arc can report an aft contact.
- `sensor_arc_aft_block_pass` Verifies a contact behind the requester is blocked by a forward-only arc.
- `sensor_arc_full_accept_pass` Verifies a full 360-degree arc reports a forward contact.
- `ping_wait_blocks_second_pass` Verifies the default ping wait suppresses an immediate second request.
- `named_ping_wait_blocks_second_pass` Verifies a named ping-wait override suppresses an immediate second request.
- `ping_wait_unlimited_allows_second_pass` Verifies a zero ping wait permits repeated requests.
- `named_ping_wait_unlimited_allows_second_pass` Verifies a named unlimited ping-wait override permits repeated requests.
- `ping_wait_nolimit_alias_allows_second_pass` Verifies the `nolimit` ping-wait alias permits repeated requests.
- `display_pulses_false_pass` Verifies range reporting continues while `VIEW_RANGE_PULSE` output is disabled.
- `unknown_request_no_report_pass` Verifies an unknown requester produces no range report.
- `missing_name_request_no_report_pass` Verifies range requests without a `name` field do not produce reports.
- `ground_truth_false_no_gt_pass` Verifies noisy-report mode can suppress ground-truth publications.
- `crs_node_report_local_pass` Verifies contacts learned through `NODE_REPORT_LOCAL` can satisfy range requests.
- `unlimited_far_range_pass` Verifies unlimited push distance allows a far contact report.
- `push_distance_alias_far_pass` Verifies the `push_distance` alias behaves like `push_dist` for far reports.
- `named_pull_unlimited_far_pass` Verifies a named target pull-distance override can allow a far contact report.
- `pull_distance_alias_far_pass` Verifies the `pull_distance` alias behaves like `pull_dist` for far reports.
- `default_short_blocks_far_pass` Verifies default push/pull distance limits block a far contact.
- `short_total_range_far_blocks_pass` Verifies a finite push-plus-pull range blocks a distant contact.
- `named_push_short_blocks_far_pass` Verifies named push/pull distance limits can block a far contact.
- `reciprocal_request_report_pass` Verifies a non-alpha requester can produce a range report back to alpha.

Typical runs:

```bash
./zlaunch.sh --jobs=4
./zlaunch.sh --case=baseline_short_report_pass --keep_workdirs
```
