# H01-uFldPathCheck Unit Harness

This harness tests `uFldPathCheck` as a single-community app-level MOOS mail
contract. Each case scripts `NODE_REPORT` or `NODE_REPORT_LOCAL` mail, and
`pMissionEval` grades the resulting `UPC_ODOMETRY_REPORT` and
`UPC_SPEED_REPORT` evidence.

## Current Matrix

- `odometry_baseline_pass` Posts Alpha at `(0,0)`, `(3,4)`, and `(6,8)` one second apart, exercising baseline path and speed accounting; passes when the exact outputs are `vname=alpha,total_dist=5.0,trip_dist=5.0` and `vname=alpha,avg_spd=5.00`.
- `two_reports_speed_only_pass` Posts only `(0,0)` and `(3,4)`, exercising the app's two-sample startup state; passes when speed is exactly `vname=alpha,avg_spd=5.00` and no `UPC_ODOMETRY_REPORT` is published.
- `trip_reset_pass` Posts three five-meter samples, resets Alpha's trip at three seconds, then posts one more five-meter sample; passes when odometry is exactly `vname=alpha,total_dist=10.0,trip_dist=5.0`.
- `node_report_local_pass` Sends the baseline three-report path entirely through `NODE_REPORT_LOCAL`, exercising the local-report subscription; passes on the exact five-meter odometry and `5.00` average-speed payloads.
- `mixed_report_sources_pass` Sends Alpha's first and third samples through `NODE_REPORT` and the middle sample through `NODE_REPORT_LOCAL`, exercising one shared history across both inputs; passes on the exact five-meter odometry and `5.00` speed payloads.
- `multi_node_independent_pass` Interleaves Alpha's five-meter diagonal path with Bravo's 12-meter vertical path, exercising per-name state separation; passes after first latching Alpha's exact `5.0/5.0` odometry and then receiving Bravo's exact `12.0/12.0` odometry.
- `variable_speed_average_pass` Posts successive five-, ten-, and ten-meter legs with reported speeds `5,10,10`, exercising uneven-leg averaging; passes when odometry is exactly `20.0/20.0` and average speed is exactly `8.33`.
- `history_length_three_average_pass` Sets `history_length=3` and posts positions `0,5,15,30`, exercising retention of the three newest reports; passes when odometry is exactly `25.0/25.0` and average speed is exactly `12.50`.
- `invalid_history_length_default_pass` Sets invalid `history_length=1` and posts the same four-sample speed sequence, exercising fallback to the default history length; passes when odometry is exactly `25.0/25.0` and average speed is exactly `10.00`.
- `reverse_leg_accumulates_pass` Posts Alpha at x `0`, then `5`, then back to `0`, exercising distance accumulation across a heading reversal; passes when odometry is exactly `5.0/5.0` and average speed is exactly `5.00`.
- `history_length_two_speed_pass` Sets `history_length=2` and posts `(0,0)`, `(3,4)`, and `(3,14)`, exercising latest-leg averaging; passes when odometry is exactly `10.0/10.0` and average speed is exactly `10.00`.
- `invalid_report_rejected_pass` Posts `NAME=alpha,X=bad,Y=0`, exercising malformed-coordinate rejection; passes at six seconds only if neither odometry nor speed output has been published.
- `stationary_zero_distance_pass` Posts three Alpha reports at `(0,0)` with zero speed, exercising stationary accounting; passes when odometry is exactly `0.0/0.0` and average speed is exactly `0.00`.
- `diagonal_chain_distance_pass` Posts `(0,0)`, `(3,4)`, `(6,8)`, and `(9,12)`, exercising chained Pythagorean legs; passes when odometry is exactly `vname=alpha,total_dist=10.0,trip_dist=10.0`.
- `trip_reset_unknown_ignored_pass` Posts `UPC_TRIP_RESET=ghost` before Alpha's baseline path, exercising an unknown-name reset; passes when Alpha still produces the exact five-meter odometry and `5.00` speed payloads.
- `trip_reset_twice_pass` Resets Alpha after each of two successive five-meter legs and then posts a third, exercising repeated trip resets; passes when total distance is exactly `15.0` and the current trip is exactly `5.0`.
- `bad_then_good_recovery_pass` Posts one malformed Alpha report before the valid baseline sequence, exercising parser recovery without state contamination; passes on the exact five-meter odometry and `5.00` speed payloads.

Typical runs:

```bash
./zlaunch.sh --jobs=4
./zlaunch.sh --case=trip_reset_pass --keep_workdirs
```

Logging is minimal by default and runs without `pLogger`. Use `--log=full` for
the complete matrix, or combine it with `--case=NAME` for one fully logged
diagnostic case.
