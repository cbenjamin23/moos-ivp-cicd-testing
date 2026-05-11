# H01-uquerydb_unit

Live `uQueryDB` harness for validating query conditions and `.checkvars`
output formats against a deterministic MOOSDB publisher mission.

The harness copies the mission stem per case, assigns isolated port ranges, runs
one bounded `uQueryDB` invocation, and grades the command return code, mission
readiness grade, and expected `.checkvars` tokens.

```sh
./zlaunch.sh --jobs=4 --port_base=15600 10
./zlaunch.sh --case=cli_numeric_pass --port_base=15600 10
```

## Cases

- `cli_numeric_pass`: CLI numeric equality condition.
- `cli_string_pass`: CLI string equality condition.
- `cli_boolean_pass`: CLI boolean-looking equality condition.
- `cli_negative_numeric_pass`: CLI negative numeric equality condition.
- `multiple_pass_conditions_pass`: Multiple CLI pass conditions must all become true.
- `fail_condition_false_pass`: False CLI fail condition does not block success.
- `fail_condition_true_fail`: True CLI fail condition returns non-zero as expected.
- `missing_var_timeout_fail`: Missing variable times out and returns non-zero.
- `late_var_wait_pass`: Delayed variable succeeds within the wait window.
- `mission_file_config_pass`: Mission-file `ProcessConfig` pass condition and check variable.
- `checkvar_esv_pass`: ESV `.checkvars` output.
- `checkvar_csv_pass`: CSV `.checkvars` output.
- `checkvar_wsv_pass`: WSV `.checkvars` output.
- `checkvar_value_only_pass`: Config-driven value-only `.checkvars` output.
- `unique_name_pass`: CLI `--unique` app naming still queries successfully.
- `compound_or_pass`: Compound OR expression succeeds through the true branch.
- `pass_condition_alias_pass`: CLI `--pass_condition` alias behaves like `--condition`.
- `compound_and_pass`: Compound AND expression requires both terms.
- `compound_or_fail`: Compound OR expression returns non-zero when both terms remain false.
- `config_halt_max_time_timeout_fail`: Config `halt_max_time` bounds a missing-variable timeout.
- `config_fail_condition_false_pass`: Generated config with false fail condition succeeds.
- `config_fail_condition_true_fail`: Generated config with true fail condition returns non-zero.
- `checkvar_multi_csv_pass`: CSV `.checkvars` output includes multiple requested variables.
- `checkvar_timeout_esv_pass`: Timeout path still writes requested ESV check variables.
- `case_config_checkvar_csv_pass`: Generated config writes CSV `.checkvars` output.
- `fail_only_false_pass`: Fail-only query succeeds when the fail condition stays false.
- `fail_only_true_fail`: Fail-only query returns non-zero when the fail condition is true.
- `numeric_greater_than_pass`: Numeric greater-than comparison succeeds.
- `numeric_less_than_fail`: Numeric less-than comparison returns non-zero when false.
- `config_condition_alias_pass`: Generated config `condition` alias behaves like `pass_condition`.
