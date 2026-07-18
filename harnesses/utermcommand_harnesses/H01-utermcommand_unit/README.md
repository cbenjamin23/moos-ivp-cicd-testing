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

- `numeric_command_pass` Verifies a numeric command posts a numeric MOOS variable.
- `quoted_string_command_pass` Verifies a quoted string command posts a string payload.
- `unique_partial_command_pass` Verifies a unique partial key can select a configured command.
- `duplicate_key_first_wins_pass` Verifies duplicate command keys preserve the first configured mapping.
- `multiple_commands_pass` Verifies two configured commands can be posted from deterministic terminal sessions.
- `arrow_syntax_command_pass` Verifies `CMD = key-->var-->value` syntax is normalized and posts the configured string.
- `unquoted_string_command_pass` Verifies unquoted nonnumeric payloads are posted as string mail.
- `negative_numeric_command_pass` Verifies negative numeric payloads are posted as numeric mail.
- `config_key_case_normalized_pass` Verifies configured command keys are normalized to lowercase for matching.
- `uppercase_input_absent_pass` Verifies uppercase terminal input does not accidentally match a lowercase command key.
- `exact_over_ambiguous_pass` Verifies an exact key wins even when it is also a prefix of another command.
- `ambiguous_partial_absent_pass` Verifies an ambiguous partial key does not post either matching command.
- `delete_edit_command_pass` Verifies delete/backspace editing can correct a typed key before posting.
- `invalid_command_config_ignored_pass` Verifies malformed command configuration is ignored without blocking later valid commands.
- `unknown_command_absent_pass` Verifies an unknown key does not post a configured variable.
