# H01-testfailure_behavior_unit

Patch-driven instrumentation harness for
`missions/testfailure_behavior_missions/testfailure_behavior_unit`.

This harness focuses on `BHV_TestFailure` as the behavior under test. The stem
mission runs a single `pHelmIvP` community and evaluates failure behavior through
mission-owned health signals: `uProcessWatch` process presence for
crash/default-crash cases, a high-rate `pGapLatch` evaluator that turns a
transient `PHELMIVP_ITER_GAP` breach into durable `TEST_GAP_SEEN` evidence for
burn and hang completion stalls, `TEST_FAILURE_DONE` endflags for behavior
lifecycle completion, and `pMissionEval` result rows in `results.txt`. Expected
crash cases
now grade `pass` when process loss is observed, so the harness can report the
mission result row directly.

The launcher requires Bash 5.1 or newer, gives every selected case an
isolated mission copy and 20-port block, and refills rolling `--jobs` slots as
cases finish. It does not reinterpret expected failures: the two legacy case
tokens ending in `_fail` are retained for compatibility, but their
pMissionEval contracts produce `grade=pass expected=process_loss` when the
requested crash is observed.

## Current Matrix

- `baseline_armed_no_trigger_pass`
  Arms `failure_type=burn,0` behind `duration=50` and evaluates at eight
  seconds, before completion; passes while `pHelmIvP` remains healthy, load
  breach count is zero, and `TEST_FAILURE_DONE` remains false.
- `burn_gap_detected_pass`
  Completes immediately with `failure_type=burn,2.0`, busy-waiting inside the
  helm for two seconds; passes when a `PHELMIVP_ITER_GAP>1.5` is latched,
  `TEST_FAILURE_DONE=true`, and all watched processes remain present.
- `hang_alias_gap_detected_pass`
  Completes immediately with `failure_type=hang,2.0`, exercising `hang` as the
  burn alias; passes when a greater-than-1.5-second helm iteration gap is
  latched, the end flag fires, and all watched processes remain present.
- `burn_default_time_gap_pass`
  Uses `failure_type=burn` with no time field, exercising the three-second
  constructor default; passes when a greater-than-1.5-second helm iteration gap
  is latched, the end flag fires, and all watched processes remain present.
- `burn_malformed_time_default_gap_pass`
  Uses `failure_type=burn,not_a_number`, which leaves the three-second default
  unchanged; passes when a greater-than-1.5-second helm iteration gap is
  latched, the end flag fires, and all watched processes remain present.
- `burn_negative_time_complete_pass`
  Uses `failure_type=burn,-1`, causing the completion loop's elapsed-time test
  to succeed immediately; passes when `TEST_FAILURE_DONE=true`, all watched
  processes remain present, and load breach count stays zero.
- `burn_zero_complete_pass`
  Uses `failure_type=burn,0`, exercising the zero-duration completion loop;
  passes when `TEST_FAILURE_DONE=true`, all watched processes remain present,
  and load breach count stays zero.
- `crash_on_complete_fail`
  Completes immediately with `failure_type=crash`, triggering the behavior's
  assertion; the harness passes with `expected=process_loss` when `pHelmIvP`
  disappears, process-watch health becomes false, and load breach count is zero.
- `unknown_type_default_crash_fail`
  Sets `failure_type=unsupported_mode`, which is accepted without changing the
  constructor's default crash mode; the harness passes with
  `expected=process_loss` when `pHelmIvP` disappears, process-watch health
  becomes false, and load breach count is zero.

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=burn_gap_detected_pass 10
./zlaunch.sh --log=full --case=burn_gap_detected_pass 10
./zlaunch.sh --jobs=4 --port_base=9000 10
./zlaunch.sh --just_make --jobs=4 --port_base=9000 10
```

Grouped runs use 20-port case blocks from `--port_base`; this unit mission has
one MOOSDB and no pShare route, but the wider block keeps the harness compatible
with the repo's grouped validation conventions.

The harness defaults to minimal logging, which launches no `pLogger`. Use
`--log=full` for the whole matrix or combine it with `--case=NAME` to restore
the stem's original logger for one diagnostic case.
