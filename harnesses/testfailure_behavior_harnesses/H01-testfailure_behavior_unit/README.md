# H01-testfailure_behavior_unit

Patch-driven instrumentation harness for
`missions/testfailure_behavior_missions/testfailure_behavior_unit`.

This harness focuses on `BHV_TestFailure` as the behavior under test. The stem
mission runs a single `pHelmIvP` community and evaluates failure behavior through
mission-owned health signals: `uProcessWatch` process presence for
crash/default-crash cases, `PHELMIVP_ITER_GAP` for burn and hang completion
stalls, `TEST_FAILURE_DONE` endflags for immediate-completion and burn/hang
cases, and `pMissionEval` result rows in `results.txt`. Expected crash cases
now grade `pass` when process loss is observed, so the harness can report the
mission result row directly.

The launcher requires Bash 5.1 or newer, gives every selected case an
isolated mission copy and 20-port block, and refills rolling `--jobs` slots as
cases finish. It does not reinterpret expected failures: the two legacy case
tokens ending in `_fail` are retained for compatibility, but their
pMissionEval contracts produce `grade=pass expected=process_loss` when the
requested crash is observed.

## Current Matrix

- `baseline_armed_no_trigger_pass` Baseline case. The behavior is armed but has a long duration, so the mission must evaluate before any failure is triggered.
- `burn_gap_detected_pass` Busy-wait case. `failure_type=burn,2.0` must create a measurable helm iteration gap while leaving `pHelmIvP` alive.
- `hang_alias_gap_detected_pass` Alias busy-wait case. `failure_type=hang,2.0` must behave like burn and create a measurable helm iteration gap.
- `burn_default_time_gap_pass` Default-duration burn case. `failure_type=burn` must use the default burn time and create a measurable helm iteration gap.
- `burn_malformed_time_default_gap_pass` Malformed-duration burn case. `failure_type=burn,not_a_number` must preserve the default burn time and create a measurable helm iteration gap.
- `burn_negative_time_complete_pass` Negative-duration burn case. `failure_type=burn,-1` must complete without process loss or load breach.
- `burn_zero_complete_pass` Immediate-completion case. `failure_type=burn,0` must complete without process loss or load breach.
- `crash_on_complete_fail` Crash case. `failure_type=crash` must make `pHelmIvP` disappear and produce a passing mission row with `expected=process_loss`.
- `unknown_type_default_crash_fail` Default-crash case. An unsupported `failure_type` leaves the behavior's default crash mode in force and must pass by observed process loss.

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=burn_gap_detected_pass 10
./zlaunch.sh --jobs=4 --port_base=9000 10
./zlaunch.sh --just_make --jobs=4 --port_base=9000 10
```

Grouped runs use 20-port case blocks from `--port_base`; this unit mission has
one MOOSDB and no pShare route, but the wider block keeps the harness compatible
with the repo's grouped validation conventions.
