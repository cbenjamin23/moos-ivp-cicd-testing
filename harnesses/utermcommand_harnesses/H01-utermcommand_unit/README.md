# H01-utermcommand_unit

Live `uTermCommand` harness for deterministic terminal-command posting. Each
case runs in an isolated mission copy. `pMissionEval` grades MOOS mail, and the
harness independently requires exactly one terminal process per case. The
launcher supports Bash 5.1 rolling scheduling and root-scoped teardown.

```sh
./zlaunch.sh --jobs=4 --port_base=16200 10
./zlaunch.sh --case=numeric_command_pass --port_base=16200 --keep_workdirs 10
```

The harness defaults to `--log=minimal`, which omits `pLogger` because terminal
stimulus, the process-count artifact, and mission results are the grading
inputs. Use `--log=full` for the complete matrix, or combine it with
`--case=<name>` for one fully logged case.

## Cases

- `numeric_command_pass`: Configures unquoted `num -> TERM_NUM=42`, enters `num`, and uses a `pEchoVar` string-only probe to distinguish a MOOS double from the numeric-looking string `"42"`; passes when `TERM_NUM=42` and no string-evidence mail appears.
- `quoted_string_command_pass`: Configures quoted payload `str -> TERM_STR="alpha007"` and enters `str`, testing quote removal without numeric coercion; passes when `TERM_STR=alpha007`.
- `unique_partial_command_pass`: Configures only key `panic` and enters unique prefix `pan`, testing partial-key completion; passes when `TERM_PARTIAL=true`.
- `duplicate_key_first_wins_pass`: Configures `dup` twice with values `first` then `second`, testing duplicate-key precedence; passes when `TERM_DUP=first` and never equals `second`.
- `multiple_commands_pass`: Configures `aa -> TERM_A=11` and `bb -> TERM_B=bravo`, then enters both keys through one running `uTermCommand` process; passes when both exact values are posted and the terminal artifact contains exactly one process-launch banner.
- `arrow_syntax_command_pass`: Configures `arrow-->TERM_ARROW-->ok`, testing normalization of arrow-delimited command syntax; passes when entering `arrow` produces `TERM_ARROW=ok`.
- `unquoted_string_command_pass`: Configures unquoted nonnumeric `raw -> TERM_RAW=bravo`; passes when entering `raw` produces string value `bravo`.
- `negative_numeric_command_pass`: Configures unquoted `neg -> TERM_NEG=-3.5`, enters `neg`, and uses a `pEchoVar` string-only probe to distinguish a negative MOOS double from the numeric-looking string `"-3.5"`; passes when `TERM_NEG=-3.5` and no string-evidence mail appears.
- `config_key_case_normalized_pass`: Configures mixed-case key `MiXeD` and enters lowercase `mixed`, testing lowercase normalization of configured keys; passes when `TERM_MIXED=ok`.
- `uppercase_input_absent_pass`: Configures lowercase key `lower` but enters uppercase `LOWER`, testing case-sensitive terminal matching; passes when readiness is reached without `TERM_UPPER=true`.
- `exact_over_ambiguous_pass`: Configures keys `pan` and `panic`, then enters exact `pan`, testing exact-match precedence over prefix ambiguity; passes when `TERM_EXACT=yes` and `TERM_LONG=no` remains absent.
- `ambiguous_partial_absent_pass`: Configures `pan` and `pat`, then enters shared prefix `pa`, testing ambiguous-prefix rejection; passes when neither target variable becomes true.
- `delete_edit_command_pass`: Types `raww`, sends DEL, and submits corrected key `raw`, testing terminal line editing; passes when `TERM_DELETE=edited`.
- `invalid_command_config_ignored_pass`: Places malformed `CMD=novar` before valid `good -> TERM_GOOD=ok`, testing per-entry rejection without aborting later configuration; passes when entering `good` posts `ok`.
- `unknown_command_absent_pass`: Configures key `good` but enters `bogus`, testing unknown-key rejection; passes when readiness is reached without `TERM_BAD=true`.
