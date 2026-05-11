# uquerydb_unit

Headless single-community stem for exercising `uQueryDB` as a command-line
app-under-test. The harness launches a deterministic publisher mission, runs
bounded `uQueryDB` queries against the live MOOSDB, and validates return codes
plus optional `.checkvars` output.

The case matrix covers CLI and mission-file pass/fail conditions,
missing-variable timeouts, config `halt_max_time`, compound AND/OR conditions,
unique app names, string and numeric comparisons, and supported check-variable
output formats including multi-variable and timeout-path outputs. It also
checks fail-only query shapes.

Typical harness run:

```sh
../../../harnesses/uquerydb_harnesses/H01-uquerydb_unit/zlaunch.sh --jobs=4 --port_base=15600 10
```

Run one inspectable case:

```sh
../../../harnesses/uquerydb_harnesses/H01-uquerydb_unit/zlaunch.sh --case=cli_numeric_pass --port_base=15600 10
```
