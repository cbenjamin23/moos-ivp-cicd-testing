# H01-ufld_beacon_range_sensor_unit

This is an intentionally non-visual unit harness. `--gui` is unsupported
because the stem has no `pMarineViewer` configuration.

Live `uFldBeaconRangeSensor` harness for beacon range-report publication.
Each case runs in an isolated mission copy, and `pMissionEval` owns the
substantive grade from the sensor's existing reports, pulses, markers, and
publication counts.

```sh
./zlaunch.sh --jobs=4 --port_base=4700 10
./zlaunch.sh --case=request_short_report_pass --port_base=4700 10
```

## Cases

- `request_short_report_pass` Places Alpha at `(0,0)` and beacon `beacon` at `(30,40)`, then requests a range with `report_vars=short`; passes when `BRS_RANGE_REPORT` identifies Alpha and the beacon at range `50` and the reply pulse identifies the beacon at `(30,40)`.
- `request_long_report_pass` Requests the same 50-meter measurement with `report_vars=long`; passes when `BRS_RANGE_REPORT_ALPHA` identifies `beacon` at range `50` and no short `BRS_RANGE_REPORT` is published.
- `request_both_report_pass` Requests the same 50-meter measurement with `report_vars=both`; passes when both `BRS_RANGE_REPORT` and `BRS_RANGE_REPORT_ALPHA` identify `beacon` at range `50`.
- `brs_ground_truth_uniform_zero_pass` Enables `ground_truth=true` with zero-percent uniform noise and requests the 50-meter beacon range; passes when the ordinary short report and `BRS_RANGE_REPORT_GT` both identify the beacon at range `50`.
- `brs_ground_truth_false_no_gt_pass` Configures zero-percent uniform noise with `ground_truth=false`; passes when the ordinary short report identifies the beacon at range `50` and no short `BRS_RANGE_REPORT_GT` is published.
- `far_request_blocked_pass` Places Alpha at `(500,0)`, beyond the configured 50-meter node and beacon transfer limits; passes when neither short nor Alpha-specific range report is published.
- `node_push_unlimited_far_pass` Places Alpha 500 meters from `farbeacon`, whose own push and pull limits are only 10 meters, while setting both node transfer limits to `unlimited`; passes when the short report identifies `farbeacon` at range `500`.
- `brs_ping_wait_blocks_second_pass` Sets the default `ping_wait` to 30 seconds and sends Alpha requests 0.8 seconds apart; passes when exactly one report is published and it identifies `beacon` at range `50`.
- `brs_named_ping_wait_blocks_only_alpha_pass` Sets `ping_wait=alpha=30` over a zero-second default, sends two Alpha requests and then one Bravo request; passes when the first Alpha report is observed, the second Alpha request is suppressed, and Bravo produces the second and final 50-meter report.
- `ping_payment_request_blocks_retry_pass` Uses `ping_payments=upon_request`: Alpha requests while 470 meters from the beacon, moves into range, and retries one second later; passes when the failed first request consumes the 30-second wait and no report is published for the retry.
- `ping_payment_response_allows_retry_pass` Uses `ping_payments=upon_response` with the same failed far request and in-range retry; passes when the failed request does not consume the wait and the retry reports `beacon` at range `50`.
- `ping_payment_accept_blocks_retry_pass` Uses `ping_payments=upon_accept` with a request path that reaches the beacon but a 10-meter beacon reply limit: Alpha requests from 50 meters, moves within 5 meters, and retries; passes when only the first request pulse is published and no range report is returned, showing the accepted first request consumed the wait despite its blocked reply.
- `brs_node_report_local_pass` Supplies Alpha's position through `NODE_REPORT_LOCAL` rather than `NODE_REPORT`, then requests the beacon range; passes when the short report identifies `beacon` at range `50`.
- `beacon_style_defaults_pass` Sets the default beacon color to green, shape to square, and width to `7`; passes when the startup `VIEW_MARKER` at `(30,40)` contains exactly those style fields.
- `beacon_reply_push_blocks_pass` Gives the request path 100-meter reach but the beacon reply only 10-meter reach for a 50-meter request; passes when the Alpha request pulse is published and no range report is returned.
- `request_path_pull_blocks_pass` Gives the beacon reply 100-meter reach but the request path only 10-meter reach for a 50-meter request; passes when the Alpha request pulse is published and no range report is returned.
- `unknown_request_no_report_pass` Requests a range for unregistered vehicle `ghost`; passes when neither a request pulse nor a range report is published.
- `missing_name_request_no_report_pass` Posts `vehicle=alpha` instead of the required `name=alpha` request field; passes when neither a request pulse nor a range report is published.
- `beacon_visual_marker_pass` Configures a beacon at `(30,40)` and makes no range request; passes when a `VIEW_MARKER` payload beginning with those coordinates is published at startup.
- `unsolicited_frequency_pass` Configures the beacon with `freq=0.6` and makes no range request; passes when Alpha receives an unsolicited short report identifying `beacon` at range `50`.

Logging is minimal by default and runs without `pLogger`. Use `--log=full` for
the complete matrix, or combine it with `--case=NAME` for one fully logged
diagnostic case.
