# H01-uFldPathCheck Unit Harness

This harness tests `uFldPathCheck` as a single-community app-level MOOS mail
contract. Each case scripts `NODE_REPORT` or `NODE_REPORT_LOCAL` mail, and
`pMissionEval` grades the resulting `UPC_ODOMETRY_REPORT` and
`UPC_SPEED_REPORT` evidence.

## Current Matrix

- `odometry_baseline_pass` Verifies a three-report track produces five meters of total/trip odometry plus average speed.
- `two_reports_speed_only_pass` Verifies two reports are enough for speed output but not enough for odometry under current app logic.
- `trip_reset_pass` Verifies `UPC_TRIP_RESET` clears trip distance while preserving accumulated total distance.
- `node_report_local_pass` Verifies `NODE_REPORT_LOCAL` is accepted the same way as field `NODE_REPORT` mail.
- `mixed_report_sources_pass` Verifies mixed `NODE_REPORT` and `NODE_REPORT_LOCAL` mail is tracked as one vehicle path.
- `multi_node_independent_pass` Verifies independent odometry accounting for two named contacts.
- `variable_speed_average_pass` Verifies average speed across uneven legs and continued odometry accumulation.
- `history_length_three_average_pass` Verifies a three-report history window averages only the retained recent legs.
- `invalid_history_length_default_pass` Verifies an invalid too-short history configuration falls back to the default averaging behavior.
- `reverse_leg_accumulates_pass` Verifies a path that reverses direction still accumulates traveled distance.
- `history_length_two_speed_pass` Verifies the configured short history window drives the latest average-speed calculation.
- `invalid_report_rejected_pass` Verifies malformed node reports do not generate odometry or speed output.
- `stationary_zero_distance_pass` Verifies repeated reports at the same position produce a zero-distance odometry report.
- `diagonal_chain_distance_pass` Verifies chained diagonal legs accumulate distance across multiple reports.
- `trip_reset_unknown_ignored_pass` Verifies a reset for an unknown vehicle does not corrupt later known-node odometry.
- `trip_reset_twice_pass` Verifies repeated trip resets preserve total distance while resetting trip distance again.
- `bad_then_good_recovery_pass` Verifies a malformed report does not prevent later valid reports from producing output.

Typical runs:

```bash
./zlaunch.sh --jobs=4
./zlaunch.sh --case=trip_reset_pass --keep_workdirs
```
