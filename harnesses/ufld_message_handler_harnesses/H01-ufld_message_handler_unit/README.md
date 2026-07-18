# H01-uFldMessageHandler Unit Harness

This harness tests `uFldMessageHandler` as an app-level message-routing
contract. Cases post controlled `NODE_MESSAGE` strings and check forwarded
payloads, rejection behavior, summary counters, and configured flags. Each
case runs in an isolated copy of the shared uField app stem, and
`pMissionEval` owns the substantive verdict from live MOOS variables.

## Current Matrix

- `dest_all_string_pass` Verifies a broad `dest_node=all` string message is forwarded when strict addressing is disabled.
- `dest_all_uppercase_pass` Verifies uppercase `dest_node=ALL` is normalized and forwarded.
- `dest_specific_self_pass` Verifies a message addressed to the local community is forwarded.
- `dest_mismatch_reject_pass` Verifies a message addressed to another node is counted as rejected and not forwarded.
- `strict_all_reject_pass` Verifies strict addressing rejects broad `dest_node=all` mail.
- `strict_group_reject_pass` Verifies strict addressing rejects group-only mail.
- `strict_specific_accept_pass` Verifies strict addressing still accepts mail addressed to the local community.
- `numeric_payload_pass` Verifies numeric `double_val` payloads are forwarded as numeric MOOS mail.
- `double_zero_payload_pass` Verifies a zero-valued numeric payload is forwarded instead of being treated as unset.
- `mixed_payload_invalid_pass` Verifies messages carrying both string and double payload fields are counted invalid and not forwarded.
- `quoted_string_unquoted_pass` Verifies quoted string payloads are forwarded in the parser-normalized form.
- `src_app_forward_pass` Verifies messages with `src_app` metadata still forward the requested payload.
- `invalid_payload_reject_pass` Verifies structurally invalid node messages are counted invalid and not forwarded.
- `missing_destination_invalid_pass` Verifies a message without `dest_node` or `dest_group` is counted invalid and not forwarded.
- `invalid_no_bad_flag_pass` Verifies invalid messages do not trigger `bad_msg_flag` postings reserved for valid rejected mail.
- `msg_flag_macro_pass` Verifies `msg_flag` macro expansion after accepted mail.
- `msg_flag_numeric_pass` Verifies numeric `msg_flag` postings after accepted mail.
- `bad_msg_flag_macro_pass` Verifies `bad_msg_flag` macro expansion after rejected mail.
- `bad_msg_flag_numeric_pass` Verifies numeric `bad_msg_flag` postings after rejected mail.
- `multiple_bad_summary_pass` Verifies repeated valid rejected messages advance bad counters and summary totals.
- `aux_info_node_app_forward_pass` Verifies `aux_info=node+app` configuration still forwards source-app messages correctly.
- `dest_group_accept_pass` Verifies group-addressed mail without a node destination is accepted in non-strict mode.
- `dest_node_overrides_group_reject_pass` Verifies an explicit mismatched `dest_node` rejects mail even when `dest_group` is present.
- `dest_all_mixedcase_reject_pass` Verifies mixed-case `dest_node=All` is not normalized like exact `ALL`.
- `multiple_good_summary_pass` Verifies summary counters across two accepted messages.
- `mixed_valid_rejected_summary_pass` Verifies summary counters distinguish one accepted and one rejected valid message.

Typical runs:

```bash
./zlaunch.sh --jobs=4
./zlaunch.sh --case=strict_all_reject_pass --keep_workdirs
```
