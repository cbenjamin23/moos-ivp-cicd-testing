# H01-utermcommand_unit

Live `uTermCommand` harness for deterministic terminal-command posting. Each
case runs in an isolated mission copy, and `pMissionEval` owns the final grade.
The launcher supports Bash 5.1 rolling scheduling and root-scoped teardown.

```sh
./zlaunch.sh --jobs=4 --port_base=16200 10
./zlaunch.sh --case=numeric_command_pass --port_base=16200 --keep_workdirs 10
```

The harness defaults to `--log=minimal`, which omits `pLogger` because terminal
stimulus and mission results are the grading inputs. Use `--log=full` for the
complete matrix, or combine it with `--case=<name>` for one fully logged case.

## Cases

- `numeric_command_pass`: Configures `num -> TERM_NUM=42` and enters `num`, testing numeric command-value parsing; passes when pMissionEval observes `TERM_NUM=42`.
- `quoted_string_command_pass`: Configures quoted payload `str -> TERM_STR="alpha007"` and enters `str`, testing quote removal without numeric coercion; passes when `TERM_STR=alpha007`.
- `unique_partial_command_pass`: Configures only key `panic` and enters unique prefix `pan`, testing partial-key completion; passes when `TERM_PARTIAL=true`.
- `duplicate_key_first_wins_pass`: Configures `dup` twice with values `first` then `second`, testing duplicate-key precedence; passes when `TERM_DUP=first` and never equals `second`.
- `multiple_commands_pass`: Configures `aa -> TERM_A=11` and `bb -> TERM_B=bravo`, then enters each in a separate terminal session; passes when both exact values are present.
- `arrow_syntax_command_pass`: Configures `arrow-->TERM_ARROW-->ok`, testing normalization of arrow-delimited command syntax; passes when entering `arrow` produces `TERM_ARROW=ok`.
- `unquoted_string_command_pass`: Configures unquoted nonnumeric `raw -> TERM_RAW=bravo`; passes when entering `raw` produces string value `bravo`.
- `negative_numeric_command_pass`: Configures `neg -> TERM_NEG=-3.5`, testing negative numeric parsing; passes when pMissionEval observes `TERM_NEG=-3.5`.
- `config_key_case_normalized_pass`: Configures mixed-case key `MiXeD` and enters lowercase `mixed`, testing lowercase normalization of configured keys; passes when `TERM_MIXED=ok`.
- `uppercase_input_absent_pass`: Configures lowercase key `lower` but enters uppercase `LOWER`, testing case-sensitive terminal matching; passes when readiness is reached without `TERM_UPPER=true`.
- `exact_over_ambiguous_pass`: Configures keys `pan` and `panic`, then enters exact `pan`, testing exact-match precedence over prefix ambiguity; passes when `TERM_EXACT=yes` and `TERM_LONG=no` remains absent.
- `ambiguous_partial_absent_pass`: Configures `pan` and `pat`, then enters shared prefix `pa`, testing ambiguous-prefix rejection; passes when neither target variable becomes true.
- `delete_edit_command_pass`: Types `raww`, sends DEL, and submits corrected key `raw`, testing terminal line editing; passes when `TERM_DELETE=edited`.
- `invalid_command_config_ignored_pass`: Places malformed `CMD=novar` before valid `good -> TERM_GOOD=ok`, testing per-entry rejection without aborting later configuration; passes when entering `good` posts `ok`.
- `unknown_command_absent_pass`: Configures key `good` but enters `bogus`, testing unknown-key rejection; passes when readiness is reached without `TERM_BAD=true`.
