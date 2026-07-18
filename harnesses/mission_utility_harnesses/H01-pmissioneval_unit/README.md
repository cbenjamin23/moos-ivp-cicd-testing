# H01-pmissioneval_unit

Patch-driven unit harness for `pMissionEval`.

The harness uses isolated copies of the stem mission at
`missions/mission_utility_missions/mission_utility_unit`. Each case applies an
explicit overlay, runs through the standard `xlaunch` lifecycle, and then
pokes the case mail. The subject grade comes from `pMissionEval` through its
single `results.txt` row plus selected publication evidence for flag and macro
contracts.

Exported harness rows use the mission-utility edge shape:
`grade=<pass|fail>` is the harness case verdict, while the pMissionEval row is
reported as `subject_grade=<pass|fail>`. Negative pMissionEval cases therefore
pass the harness when `expected_subject=fail` and the subject row actually
grades fail.

## Current Matrix

- `eval_baseline_pass` Baseline `pMissionEval` case. Scripted mail satisfies the lead and pass conditions, writes a pass row, expands ordinary macros, and terminates through the default mission-evaluated flag.
- `eval_false_condition_fail` Negative `pMissionEval` case. The lead condition fires while `PASS_INPUT=false`, so the mission row must grade fail.
- `eval_builtin_finish_pass` Built-in finish publication case. `pMissionEval` omits an explicit `MISSION_EVALUATED` result flag, so the default `uMayFinish` exit proves the app's built-in finish publication path.
- `eval_numeric_multi_pass` Multi-condition and numeric macro case. Two lead/pass conditions must be satisfied, and a numeric MOOS value must expand into both a report row and a result flag.
- `eval_sequence_two_stage_pass` Two-stage sequence case. A second lead condition starts a new `LogicAspect`, and both stages must pass before the mission is evaluated.
- `eval_sequence_first_stage_fail` Sequence short-circuit case. The first stage fails and the mission must grade fail without waiting for second-stage mail.
- `eval_sequence_second_stage_fail` Later-stage sequence failure case. The first stage passes, the second stage fails, and the mission must grade fail after reaching the later `LogicAspect`.
- `eval_numeric_literal_flag_pass` Literal numeric flag case. A numeric `result_flag` must publish through the non-string flag path while the mission grades pass.
- `eval_lead_only_pass` Lead-only evaluator case. A valid evaluator with no pass or fail conditions must grade pass once the lead condition is met.
- `eval_numeric_partial_fail` Multi-condition partial failure case. The lead conditions are satisfied but one numeric pass condition is below threshold, so the mission must grade fail.
- `eval_fail_only_condition_fail` Fail-only evaluator case. A valid evaluator with no pass conditions must still grade fail when a fail condition is met.
- `eval_fail_condition_fail` Explicit fail-condition case. A true `FORCE_FAIL` variable must override an otherwise passing input set and produce a fail row.
- `eval_flags_and_mail_pass` Flag publication case. The baseline pass must also emit result/pass flags and react to configured mailflag input in the alog.
- `eval_csp_report_pass` Report-format case. `pMissionEval` must honor `report_line_format=csp` by writing comma-separated report columns.
- `eval_mhash_macros_pass` Mission-hash macro case. `pMissionEval` must expand short, long-short, UTC, full hash, and mission-hash aliases from `MISSION_HASH` mail.
- `eval_clock_macros_pass` Clock macro case. `pMissionEval` must expand month, day, hour, minute, second, date, and time macros without leaving raw macro tokens in the report row.
- `eval_no_hash_report_pass` No-hash evaluator case. The stem omits `pMissionHash`, and `pMissionEval` must still evaluate and write a report row.

## Running

```bash
./zlaunch.sh --case=eval_baseline_pass --port_base=9000 10
./zlaunch.sh --jobs=3 --port_base=9000 10
./zlaunch.sh --just_make --jobs=3 --port_base=9000 10
```

Serial and rolling runs use the same isolated-case path. Grouped runs use
30-port case blocks from `--port_base`; the default starts at `9000`.
