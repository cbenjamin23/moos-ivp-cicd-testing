# H01-pmissioneval_unit

This is an intentionally non-visual unit harness. `--gui` is unsupported
because the stem has no `pMarineViewer` configuration.

Patch-driven unit harness for `pMissionEval`.

The harness uses isolated copies of the stem mission at
`missions/mission_utility_missions/mission_utility_unit`. Each case applies an
explicit overlay, runs through the standard `xlaunch` lifecycle, and then
pokes the case mail. The subject grade comes from `pMissionEval` through its
single `results.txt` row plus selected publication evidence for flag and macro
contracts.

Logging defaults to the shared grading allowlist. Use `--log=full` to restore
the original wildcard diagnostic logger for every selected case.

Exported harness rows use the mission-utility edge shape:
`grade=<pass|fail>` is the harness case verdict, while the pMissionEval row is
reported as `subject_grade=<pass|fail>`. Negative pMissionEval cases therefore
pass the harness when `expected_subject=fail` and the subject row actually
grades fail.

## Current Matrix

- `eval_baseline_pass` Posts `TEST_EVAL_READY=true` and `PASS_INPUT=true`, testing the basic one-lead, one-pass-condition evaluator; passes when `pMissionEval` writes exactly one result row with `subject_grade=pass` and the mission launcher exits successfully.
- `eval_false_condition_fail` Posts `TEST_EVAL_READY=true` with `PASS_INPUT=false`, testing a false pass condition after the lead condition opens evaluation; the harness passes when `pMissionEval` writes `subject_grade=fail` with `pass_input=false`.
- `eval_builtin_finish_pass` Removes the explicit `MISSION_EVALUATED` result flag while satisfying the normal lead and pass conditions, testing pMissionEval's built-in completion post; passes when the subject grades pass, the `.alog` contains `MISSION_EVALUATED`, no `EVAL_RESULT_FLAG` line contains `result:`, and the mission launcher exits successfully.
- `eval_numeric_multi_pass` Requires both `TEST_EVAL_READY=true` and `CASE_PHASE=ready`, plus `PASS_INPUT=true` and `NUM_SCORE>40`, testing multiple lead/pass conditions and numeric macro expansion; passes with `subject_grade=pass`, `phase=ready`, `score=42.5`, and `EVAL_SCORE_FLAG=score:42.5` in the `.alog`.
- `eval_sequence_two_stage_pass` Defines consecutive stages requiring `STAGE1_READY/STAGE1_PASS` and then `STAGE2_READY/STAGE2_PASS`, testing ordered `LogicAspect` evaluation; passes when both ready posts appear and the result row records `subject_grade=pass`, `stage1=true`, and `stage2=true`.
- `eval_sequence_first_stage_fail` Posts `STAGE1_READY=true` with `STAGE1_PASS=false` and never posts stage-two mail, testing failure at the first `LogicAspect`; the harness passes when the result records `subject_grade=fail` and `stage1=false`, `EVAL_FAIL_FLAG=stage1_failed` appears, and no `STAGE2_READY` mail appears.
- `eval_sequence_second_stage_fail` Passes stage one, then posts `STAGE2_READY=true` with `STAGE2_PASS=false`, testing failure after advancing to the second `LogicAspect`; the harness passes when the result records `subject_grade=fail`, `stage1=true`, and `stage2=false`, and the `.alog` contains both `STAGE2_READY=true` and `EVAL_FAIL_FLAG=stage2_failed`.
- `eval_numeric_literal_flag_pass` Configures `result_flag=EVAL_LITERAL_NUM=17.25`, testing numeric-literal publication through the non-string flag path; passes when the subject grades pass with `case_value=literalnum` and the `.alog` contains `EVAL_LITERAL_NUM=17.25`.
- `eval_lead_only_pass` Configures only `lead_condition=TEST_EVAL_READY=true`, testing an evaluator with no pass or fail conditions; passes when the ready post produces `subject_grade=pass`, `eval=true`, and `case_value=leadonly`.
- `eval_numeric_partial_fail` Satisfies both lead conditions and `PASS_INPUT=true` but posts `NUM_SCORE=39` against `NUM_SCORE>40`, testing partial failure in a multi-condition evaluator; the harness passes when the result records `subject_grade=fail`, `phase=ready`, and `score=39`, and the `.alog` contains `EVAL_FAIL_FLAG=partial_numeric_fail`.
- `eval_fail_only_condition_fail` Configures no pass conditions and posts `FORCE_FAIL=true` after the lead condition, testing a fail-only evaluator; the harness passes when the result records `subject_grade=fail`, `eval=true`, and `force_fail=true`, and the `.alog` contains `EVAL_FAIL_FLAG=fail_only`.
- `eval_fail_condition_fail` Posts `PASS_INPUT=true` and `FORCE_FAIL=true`, testing that an explicit fail condition overrides a satisfied pass condition; the harness passes when the result records `subject_grade=fail` and `force_fail=true`, and the `.alog` contains `EVAL_FAIL_FLAG`.
- `eval_flags_and_mail_pass` Posts `CASE_VALUE=alpha` and `CASE_MAIL=baseline`, testing macro expansion in configured `result_flag`, `pass_flag`, and `mailflag` publications; passes when the subject grades pass and the `.alog` records exactly `EVAL_RESULT_FLAG=result:alpha`, `EVAL_PASS_FLAG=pass:alpha`, and `EVAL_MAIL_SEEN=true`.
- `eval_csp_report_pass` Sets `report_line_format=csp` and satisfies the baseline conditions, testing comma-and-space report formatting; passes when the subject grades pass and the result row contains the ordered comma-separated fields `form`, `mmod`, `grade=pass`, and `eval=true`.
- `eval_mhash_macros_pass` Places `$[MHASH_SHORT]`, `$[MHASH_LSHORT]`, `$[MHASH_UTC]`, `$[MHASH]`, and `$[MISSION_HASH]` in report columns, testing mission-hash macro expansion; passes when the row matches the expected short-word, timestamp, and full-hash formats for every macro.
- `eval_clock_macros_pass` Places `$[MONTH]`, `$[DAY]`, `$[HOUR]`, `$[MIN]`, `$[SEC]`, `$[DATE]`, and `$[TIME]` in report columns, testing clock macro expansion; passes when every field has its expected numeric format and no raw `$[` token remains.
- `eval_no_hash_report_pass` Removes `pMissionHash` from ANTLER and writes a report with no hash columns, testing evaluation without mission-hash mail; passes when the subject grades pass with `eval=true` and `case_value=nohash`, and no `MISSION_HASH` publication appears in the `.alog`.

## Running

```bash
./zlaunch.sh --case=eval_baseline_pass --port_base=9000 10
./zlaunch.sh --jobs=3 --port_base=9000 10
./zlaunch.sh --just_make --jobs=3 --port_base=9000 10
```

Serial and rolling runs use the same isolated-case path. Grouped runs use
30-port case blocks from `--port_base`; the default starts at `9000`.

## Coverage Validation

The July 20, 2026 artifact-oracle pass completed all 17 cases with both
minimal and full logging at four concurrent jobs. Independently changing the
result flag to `result:wrong`, the pass flag to `pass:wrong`, or the mail flag
to `false` left `subject_grade=pass` and `case_value=alpha` unchanged but made
`eval_flags_and_mail_pass` fail with `reason=evidence_mismatch` in each run.
