# H01-uFldContactRangeSensor Unit Harness

This harness tests `uFldContactRangeSensor` as a deterministic field-contact
range reporter. Cases script contact reports and `CRS_RANGE_REQUEST` mail;
mission-owned `pMissionEval` conditions then check range report variables,
pulse publication, report mode, ping timing, echo type filtering, and
sensor-arc gating.

## Current Matrix

- `baseline_short_report_pass` Places Alpha at `(0,0)` and Bravo at `(50,0)`, then requests `name=alpha`; passes when `CRS_RANGE_REPORT` identifies Alpha, Bravo, and range `50`, and `VIEW_RANGE_PULSE` identifies Bravo at `(50,0)` with radius `40`, fill `0.9`, and chartreuse colors.
- `report_vars_short_no_long_pass` Sets `report_vars=short` for the 50-meter request; passes when the exact short report prefix is published and no `CRS_RANGE_REPORT_ALPHA` is published.
- `report_vars_long_pass` Sets `report_vars=long`; passes when no short report is published and `CRS_RANGE_REPORT_ALPHA` identifies Bravo at range `50`.
- `report_vars_both_pass` Sets `report_vars=both`; passes when both the requester-tagged short report and Alpha-specific long report identify Bravo at range `50`.
- `ground_truth_uniform_zero_pass` Sets `rn_algorithm=uniform,pct=0`, `ground_truth=true`, and `report_vars=both`; passes when normal and `CRS_RANGE_REPORT_GT` short reports both identify the same 50-meter Alpha-to-Bravo range.
- `ground_truth_gaussian_unsupported_no_gt_pass` Sets unsupported `rn_algorithm=gaussian`, sigma `0`, and `ground_truth=true`; passes when the normal short report identifies range `50` and no short `CRS_RANGE_REPORT_GT` is published.
- `ground_truth_long_only_pass` Sets `report_vars=long`, zero-noise uniform reporting, and ground truth; passes when neither short form is published and both `CRS_RANGE_REPORT_ALPHA` and `CRS_RANGE_REPORT_GT_ALPHA` identify Bravo at range `50`.
- `ground_truth_no_algorithm_absent_pass` Sets `ground_truth=true` without `rn_algorithm`, exercising the algorithm gate; passes when the normal short report identifies range `50` and no short `CRS_RANGE_REPORT_GT` is published.
- `allow_echo_type_block_pass` Allows only `kayak` echoes while Bravo reports `TYPE=SHIP`; passes when neither short nor long range report is published.
- `allow_echo_type_accept_pass` Allows `kayak` echoes while both contacts report `TYPE=KAYAK`; passes on the baseline exact short report and pulse evidence.
- `allow_echo_type_multi_accept_ship_pass` Allows `kayak,ship` and makes Bravo a ship, exercising multi-type matching; passes on the exact 50-meter short report and Bravo pulse.
- `sensor_arc_forward_accept_pass` Sets `sensor_arc=45:135`; with Alpha heading north and Bravo due east at relative bearing `90`, it passes on the exact short report and pulse.
- `sensor_arc_wrap_accept_pass` Sets wraparound `sensor_arc=315:135`; passes when the same relative-bearing-90 contact produces the exact short report and pulse.
- `sensor_arc_multi_segment_aft_accept_pass` Sets `sensor_arc=45:135,225:315` and places Bravo 50 meters west at relative bearing `270`; passes when the short report identifies Alpha, Bravo, and range `50`.
- `sensor_arc_aft_block_pass` Sets forward-only `sensor_arc=45:135` and places Bravo 50 meters west at relative bearing `270`; passes when neither short nor long report is published.
- `sensor_arc_full_accept_pass` Sets `sensor_arc=360`, exercising the full-circle token; passes on the baseline exact short report and pulse.
- `ping_wait_blocks_second_pass` Sets `ping_wait=default=30` and sends Alpha requests at 1.5 and 2.5 seconds; passes when exactly one short report is counted.
- `named_ping_wait_blocks_second_pass` Sets default wait `0` plus `ping_wait=alpha=30` and sends the same two requests; passes when exactly one report is counted.
- `ping_wait_unlimited_allows_second_pass` Uses the common `ping_wait=default=0` and sends two requests one second apart; passes when at least two reports are counted.
- `named_ping_wait_unlimited_allows_second_pass` Sets default wait `30` plus `ping_wait=alpha=unlimited`; passes when the two one-second-separated requests produce at least two reports.
- `ping_wait_nolimit_alias_allows_second_pass` Sets default wait `30` plus `ping_wait=alpha=nolimit`, exercising the unlimited alias; passes when at least two reports are counted.
- `display_pulses_false_pass` Sets `display_pulses=false`; passes when the exact 50-meter short report is published and no `VIEW_RANGE_PULSE` is observed.
- `unknown_request_no_report_pass` Requests `name=ghost` without a known ghost report; passes when neither short nor long range output is published.
- `missing_name_request_no_report_pass` Posts known Alpha and Bravo reports but requests `vname=alpha` instead of the required `name` field; passes when neither report form is published.
- `ground_truth_false_no_gt_pass` Sets zero-noise uniform reporting, `report_vars=both`, and `ground_truth=false`; passes when both normal report forms identify range `50` and no short ground-truth report is published.
- `crs_node_report_local_pass` Supplies both 50-meter contact positions through `NODE_REPORT_LOCAL`, exercising the local-report subscription; passes on the exact short report and Bravo pulse.
- `unlimited_far_range_pass` Places Bravo 500 meters away with `push_dist=default=unlimited` and `pull_dist=default=10`; passes when the short report identifies range `500`.
- `push_distance_alias_far_pass` Uses alias `push_distance=default=unlimited` with `pull_distance=default=10` for the same 500-meter geometry; passes when the short report identifies range `500`.
- `named_pull_unlimited_far_pass` Sets `push_dist=alpha=10` and `pull_dist=bravo=unlimited`, exercising a target-specific pull allowance; passes when the 500-meter short report is published.
- `pull_distance_alias_far_pass` Uses aliases `push_distance=alpha=10` and `pull_distance=bravo=unlimited`; passes when the 500-meter short report is published.
- `default_short_blocks_far_pass` Sets default push and pull distances to `10` with Bravo 500 meters away; passes when neither report form is published.
- `short_total_range_far_blocks_pass` Sets default push and pull distances to `25` with Bravo 5,000 meters away; passes when neither report form is published.
- `named_push_short_blocks_far_pass` Sets `push_dist=alpha=10` and `pull_dist=bravo=10` with Bravo 500 meters away; passes when neither report form is published.
- `reciprocal_request_report_pass` Uses the same 50-meter geometry but requests `name=bravo`, exercising the reverse requester/target roles; passes when the short report identifies `vname=bravo`, `target=alpha`, and range `50`.

Typical runs:

```bash
./zlaunch.sh --jobs=4
./zlaunch.sh --case=baseline_short_report_pass --keep_workdirs
```

Logging is minimal by default and runs without `pLogger`. Use `--log=full` for
the complete matrix, or combine it with `--case=NAME` for one fully logged
diagnostic case.
