# H01-uFldMessageHandler Unit Harness

This is an intentionally non-visual unit harness. `--gui` is unsupported
because the stem has no `pMarineViewer` configuration.

This harness tests `uFldMessageHandler` as an app-level message-routing
contract. Cases post controlled `NODE_MESSAGE` strings and check forwarded
payloads, rejection behavior, summary counters, and configured flags. Each
case runs in an isolated copy of the shared uField app stem, and
`pMissionEval` owns the substantive verdict from live MOOS variables.

## Current Matrix

- `dest_all_string_pass` Posts `dest_node=all` with `string_val=all_ok` under `strict_addressing=false`, exercising the lowercase broadcast token; passes when `HANDLED_RESULT=all_ok` and the summary is exactly `total=1,valid=1,rejected=0`.
- `dest_all_uppercase_pass` Posts `dest_node=ALL` with `all_upper_ok`, exercising uppercase broadcast normalization; passes on that exact forwarded value and summary `total=1,valid=1,rejected=0`.
- `dest_specific_self_pass` Addresses `string_val=self_ok` directly to `shoreside`, exercising local-node delivery; passes when `HANDLED_RESULT=self_ok`.
- `dest_mismatch_reject_pass` Addresses `wrong_node` to `bravo`, exercising valid-mail rejection for a different node; passes when no `HANDLED_RESULT` is posted and the summary is exactly `total=1,valid=1,rejected=1`.
- `strict_all_reject_pass` Enables strict addressing and posts `dest_node=all`, exercising rejection of broad node mail; passes when no result is forwarded and the summary is exactly `total=1,valid=1,rejected=1`.
- `strict_group_reject_pass` Enables strict addressing and posts only `dest_group=red`, exercising rejection of group-only mail; passes when no result is forwarded and the summary is exactly `total=1,valid=1,rejected=1`.
- `strict_specific_accept_pass` Enables strict addressing and targets `shoreside` with `strict_accept`, exercising the permitted exact-address path; passes on that exact value and summary `total=1,valid=1,rejected=0`.
- `numeric_payload_pass` Posts `double_val=42.5` to `HANDLED_NUMERIC`, exercising numeric MOOS publication; passes when the resulting numeric value equals `42.5`.
- `double_zero_payload_pass` Posts `double_val=0`, exercising preservation of zero rather than treating it as unset; passes when `HANDLED_NUMERIC=0`.
- `mixed_payload_invalid_pass` Includes both `string_val=string_wins` and `double_val=99`, exercising ambiguous-payload rejection; passes when neither target variable is posted and the summary is exactly `total=1,valid=0,rejected=0`.
- `quoted_string_unquoted_pass` Posts `string_val="quoted ok"`, exercising parser normalization of a quoted string containing a space; passes when `HANDLED_RESULT` equals the exact normalized value `"quotedok"`.
- `src_app_forward_pass` Adds `src_app=pHelmIvP` to an otherwise ordinary self-addressed message, exercising source-app metadata parsing; passes when `HANDLED_RESULT=src_app_ok` and the summary is exactly `total=1,valid=1,rejected=0`.
- `invalid_payload_reject_pass` Omits `var_name` while supplying `string_val=missing_var`, exercising structural invalidation; passes when no result is forwarded and the summary is exactly `total=1,valid=0,rejected=0`.
- `missing_destination_invalid_pass` Supplies a variable and payload but neither `dest_node` nor `dest_group`, exercising destination validation; passes when no result is forwarded and the summary is exactly `total=1,valid=0,rejected=0`.
- `invalid_no_bad_flag_pass` Configures `bad_msg_flag=UMH_BAD=bad_$[BAD_CTR]_of_$[CTR]` and posts the missing-`var_name` message, exercising the distinction between invalid and valid-rejected mail; passes when neither result nor bad flag is posted and the summary is exactly `total=1,valid=0,rejected=0`.
- `msg_flag_macro_pass` Configures `msg_flag=UMH_GOOD=good_$[GOOD_CTR]_of_$[CTR]` and accepts one message; passes when the payload is `flagged` and the expanded flag is exactly `good_1_of_1`.
- `msg_flag_numeric_pass` Configures numeric `msg_flag=UMH_GOOD_NUM=7` and accepts one message; passes when the payload is `flagged_num` and `UMH_GOOD_NUM=7`.
- `bad_msg_flag_macro_pass` Configures `bad_msg_flag=UMH_BAD=bad_$[BAD_CTR]_of_$[CTR]` and rejects one message addressed to Bravo; passes when no payload is forwarded and the expanded flag is exactly `bad_1_of_1`.
- `bad_msg_flag_numeric_pass` Configures numeric `bad_msg_flag=UMH_BAD_NUM=11` and rejects one message addressed to Bravo; passes when no payload is forwarded and `UMH_BAD_NUM=11`.
- `multiple_bad_summary_pass` Posts two valid messages to nonlocal nodes with the macro bad flag enabled, exercising cumulative rejection counters; passes when neither payload is forwarded, `UMH_BAD=bad_2_of_2`, and the summary is exactly `total=2,valid=2,rejected=2`.
- `aux_info_node_app_forward_pass` Sets `aux_info=node+app` and posts a `src_app=pHelmIvP` message, exercising node-and-app auxiliary source annotation; passes when `HANDLED_RESULT=aux_ok` and the summary is exactly `total=1,valid=1,rejected=0`.
- `dest_group_accept_pass` Posts `dest_group=red` without `dest_node` under non-strict addressing, exercising group-only acceptance; passes when `HANDLED_RESULT=group_ok`.
- `dest_node_overrides_group_reject_pass` Posts `dest_node=bravo,dest_group=red`, exercising node precedence over an otherwise acceptable group; passes when no result is forwarded and the summary is exactly `total=1,valid=1,rejected=1`.
- `dest_all_mixedcase_reject_pass` Posts mixed-case `dest_node=All`, exercising the case-sensitive distinction from `all|ALL`; passes when no result is forwarded and the summary is exactly `total=1,valid=1,rejected=1`.
- `multiple_good_summary_pass` Posts two accepted self-addressed messages, exercising cumulative good counters; passes when the final payload is `second_ok` and the summary is exactly `total=2,valid=2,rejected=0`.
- `mixed_valid_rejected_summary_pass` Posts one accepted self-addressed message and one valid message to `charlie`, exercising mixed counter accounting; passes when `first_ok` is forwarded, the rejected variable is absent, and the summary is exactly `total=2,valid=2,rejected=1`.

Typical runs:

```bash
./zlaunch.sh --jobs=4
./zlaunch.sh --case=strict_all_reject_pass --keep_workdirs
```

Logging is minimal by default and runs without `pLogger`. Use `--log=full` for
the complete matrix, or combine it with `--case=NAME` for one fully logged
diagnostic case.
