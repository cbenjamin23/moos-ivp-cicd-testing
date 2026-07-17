# H01-ufld_beacon_range_sensor_unit

Live `uFldBeaconRangeSensor` harness for beacon range-report publication.
Each case runs in an isolated mission copy, and `pMissionEval` owns the
substantive grade from the sensor's existing reports, pulses, markers, and
publication counts.

```sh
./zlaunch.sh --jobs=4 --port_base=4700 10
./zlaunch.sh --case=request_short_report_pass --port_base=4700 10
```

## Cases

- `request_short_report_pass` Verifies a vehicle range request produces a short `BRS_RANGE_REPORT` with the expected beacon range.
- `request_long_report_pass` Verifies `report_vars=long` publishes the vehicle-specific long report and suppresses the short report.
- `request_both_report_pass` Verifies `report_vars=both` publishes both short and long report channels.
- `brs_ground_truth_uniform_zero_pass` Verifies uniform zero-noise reporting also publishes ground-truth range mail.
- `brs_ground_truth_false_no_gt_pass` Verifies ground-truth mail is suppressed when disabled even with a noise algorithm configured.
- `far_request_blocked_pass` Verifies a request outside combined push/pull range produces no range report.
- `node_push_unlimited_far_pass` Verifies unlimited node push/pull settings permit a far beacon reply.
- `brs_ping_wait_blocks_second_pass` Verifies ping-wait throttling blocks an immediate second request.
- `brs_named_ping_wait_blocks_only_alpha_pass` Verifies node-specific ping wait throttles one vehicle without blocking another vehicle.
- `ping_payment_request_blocks_retry_pass` Verifies `ping_payments=upon_request` charges a failed request and blocks an immediate retry.
- `ping_payment_response_allows_retry_pass` Verifies `ping_payments=upon_response` leaves a failed request uncharged so a later in-range retry can succeed.
- `ping_payment_accept_blocks_retry_pass` Verifies `ping_payments=upon_accept` charges an accepted request even when no beacon reply is returned.
- `brs_node_report_local_pass` Verifies `NODE_REPORT_LOCAL` updates the vehicle ledger for later beacon range requests.
- `beacon_style_defaults_pass` Verifies default beacon style parameters flow into startup marker visuals.
- `beacon_reply_push_blocks_pass` Verifies a request can reach the beacon while the beacon reply is blocked by beacon push distance.
- `request_path_pull_blocks_pass` Verifies a request pulse is visible but no reply is posted when the request cannot reach the beacon under combined range limits.
- `unknown_request_no_report_pass` Verifies a request from an unknown vehicle name is rejected before pulses or reports are posted.
- `missing_name_request_no_report_pass` Verifies a malformed request without `name` is rejected before pulses or reports are posted.
- `beacon_visual_marker_pass` Verifies configured beacons publish startup `VIEW_MARKER` visuals.
- `unsolicited_frequency_pass` Verifies beacon frequency broadcasts can publish an unsolicited range report.
