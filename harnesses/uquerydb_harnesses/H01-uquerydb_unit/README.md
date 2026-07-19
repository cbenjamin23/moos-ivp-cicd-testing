# H01-uquerydb_unit

Live `uQueryDB` harness for validating query conditions and `.checkvars`
output formats against a deterministic MOOSDB publisher mission.

The harness copies the mission stem for every case, assigns isolated port
blocks, runs one bounded `uQueryDB` invocation, and validates the command return
code, mission readiness grade, and expected `.checkvars` tokens. Cases use a
Bash 5.1 rolling scheduler when `--jobs` is greater than one.

This is a documented CLI-result exception to ordinary mission-only grading.
`pMissionEval` owns the deterministic publisher-readiness grade. The harness
then treats the configured `uQueryDB` return code and `.checkvars` output as the
subject contract, including cases where return code 1 is the expected result.
No shell check replaces the mission readiness verdict.

```sh
./zlaunch.sh --jobs=2 --port_base=15600 10
./zlaunch.sh --case=cli_numeric_pass --port_base=15600 10
./zlaunch.sh --keep_workdirs --jobs=2 --port_base=22000 10
```

The harness defaults to `--log=minimal`, which omits `pLogger` because the
mission result, command status, and `.checkvars` output are the grading inputs.
Use `--log=full` for the complete matrix, or combine it with `--case=<name>`
for one fully logged case.

## Cases

- `cli_numeric_pass`: Queries `QUERY_NUM = 42` from the command line, testing numeric equality against the published value; passes when `uQueryDB` exits `0` within the three-second wait.
- `cli_string_pass`: Queries `QUERY_STR = ready`, testing string equality against the published value; passes when `uQueryDB` exits `0` within three seconds.
- `cli_boolean_pass`: Queries `QUERY_BOOL = true`, testing equality for the published boolean-looking string; passes when `uQueryDB` exits `0` within three seconds.
- `cli_negative_numeric_pass`: Queries `QUERY_NEG = -7.5`, testing parsing and equality for a negative decimal; passes when `uQueryDB` exits `0` within three seconds.
- `multiple_pass_conditions_pass`: Supplies separate `QUERY_NUM = 42` and `QUERY_STR = ready` pass conditions, testing the all-conditions contract; passes when `uQueryDB` exits `0` after both published values satisfy their conditions.
- `fail_condition_false_pass`: Requires `QUERY_NUM = 42` while declaring `QUERY_FAIL = true` as a fail condition, testing that a false fail condition does not veto a satisfied pass condition; passes when `uQueryDB` exits `0` with `QUERY_FAIL=false`.
- `fail_condition_true_fail`: Requires `QUERY_NUM = 42` while declaring the published `QUERY_FAIL = false` as a fail condition, testing fail-condition precedence over a satisfied pass condition; the harness passes when `uQueryDB` exits `1`.
- `missing_var_timeout_fail`: Waits one second for never-published `QUERY_MISSING = ready`, testing the unsatisfied-variable timeout path; the harness passes when `uQueryDB` exits `1`.
- `late_var_wait_pass`: Waits up to three seconds for `QUERY_LATE = arrived`, which is posted later than the baseline variables, testing that the query remains active for delayed mail; passes when `uQueryDB` exits `0`.
- `mission_file_config_pass`: Runs the stem mission file's `pass_condition=QUERY_NUM = 42`, `check_var=QUERY_NUM`, and ESV format without CLI query arguments, testing `ProcessConfig` loading; passes when `uQueryDB` exits `0` and `.checkvars` contains `QUERY_NUM=42`.
- `checkvar_esv_pass`: Requests `QUERY_NUM` with `--check_var` and `--esv`, testing equals-separated output; passes when the numeric query exits `0` and `.checkvars` contains `QUERY_NUM=42`.
- `checkvar_csv_pass`: Requests `QUERY_STR` with `--check_var` and `--csv`, testing comma-separated output; passes when the string query exits `0` and `.checkvars` contains `QUERY_STR,ready`.
- `checkvar_wsv_pass`: Requests `QUERY_OTHER` with `--check_var` and `--wsv`, testing whitespace-separated output; passes when `QUERY_OTHER = bravo` exits `0` and `.checkvars` contains `QUERY_OTHER bravo`.
- `checkvar_value_only_pass`: Generates a config with `check_var_format=vo` for `QUERY_BOOL`, testing value-only output; passes when the query exits `0`, `.checkvars` contains `true`, and no ESV, CSV, or WSV variable prefix appears.
- `unique_name_pass`: Adds `--unique` to `QUERY_NUM = 42`, exercising randomized client-name startup; passes when the query exits `0`, without independently checking the registered name.
- `compound_or_pass`: Queries `(QUERY_STR = not_ready) or (QUERY_OTHER = bravo)`, testing that a true right-hand branch satisfies an OR expression when the left branch is false; passes when `uQueryDB` exits `0`.
- `pass_condition_alias_pass`: Supplies `QUERY_STR = ready` through the CLI `--pass_condition` alias, testing equivalence with `--condition`; passes when `uQueryDB` exits `0`.
- `compound_and_pass`: Queries `(QUERY_NUM = 42) and (QUERY_STR = ready)`, testing a compound AND expression with both terms true; passes when `uQueryDB` exits `0`.
- `compound_or_fail`: Queries `(QUERY_STR = not_ready) or (QUERY_OTHER = not_bravo)`, testing an OR expression with both branches false; the harness passes when `uQueryDB` exits `1` after the one-second wait.
- `config_halt_max_time_timeout_fail`: Generates `halt_max_time=1` with the never-satisfied `QUERY_MISSING = ready` pass condition, testing the configuration alias for the timeout; the harness passes when `uQueryDB` exits `1`.
- `config_fail_condition_false_pass`: Generates a config requiring `QUERY_NUM = 42` and failing on `QUERY_FAIL = true`, testing a false config-file fail condition; passes when `uQueryDB` exits `0` with `QUERY_FAIL=false`.
- `config_fail_condition_true_fail`: Generates a config requiring `QUERY_NUM = 42` and failing on the published `QUERY_FAIL = false`, testing a true config-file fail condition; the harness passes when `uQueryDB` exits `1`.
- `checkvar_multi_csv_pass`: Requests `QUERY_NUM` and `QUERY_STR` in CSV format, testing multiple `.checkvars` rows from one successful query; passes when the query exits `0` and the file contains both `QUERY_NUM,42` and `QUERY_STR,ready`.
- `checkvar_timeout_esv_pass`: Waits one second on missing `QUERY_MISSING` while requesting published `QUERY_NUM` in ESV format, testing check-variable output on query failure; the harness passes when `uQueryDB` exits `1` and `.checkvars` still contains `QUERY_NUM=42`.
- `case_config_checkvar_csv_pass`: Generates a config requiring `QUERY_OTHER = bravo` and writing that variable as CSV, testing config-file check-variable and format settings together; passes when the query exits `0` and `.checkvars` contains `QUERY_OTHER,bravo`.
- `fail_only_false_pass`: Supplies only the fail condition `QUERY_FAIL = true`, testing a query with no pass conditions when the published value is false; passes when `uQueryDB` exits `0`.
- `fail_only_true_fail`: Supplies only the fail condition `QUERY_FAIL = false`, testing a query with no pass conditions when the published value satisfies the fail condition; the harness passes when `uQueryDB` exits `1`.
- `numeric_greater_than_pass`: Queries `QUERY_NUM > 41` against the published value `42`, testing a true numeric greater-than comparison; passes when `uQueryDB` exits `0`.
- `numeric_less_than_fail`: Queries `QUERY_NUM < 41` against the published value `42`, testing a false numeric less-than comparison; the harness passes when `uQueryDB` exits `1` after the one-second wait.
- `config_condition_alias_pass`: Generates `condition=QUERY_BOOL = true`, testing the config-file alias for `pass_condition`; passes when `uQueryDB` exits `0`.
