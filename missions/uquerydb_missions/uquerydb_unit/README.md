# uquerydb_unit

Headless single-community evaluation stem for exercising `uQueryDB` as a
command-line app-under-test. `pMissionEval` grades the deterministic publisher
state after `TEST_EVAL_READY=true`; the harness posts that existing readiness
flag only after its bounded `uQueryDB` subject finishes. A standalone
`./zlaunch.sh` run posts the same flag itself.

The CLI return code and optional `.checkvars` output remain explicit
harness-level subject evidence because they are not MOOS publications. The
mission-owned result remains the authoritative publisher-readiness grade.

The case matrix covers CLI and mission-file pass/fail conditions,
missing-variable timeouts, config `halt_max_time`, compound AND/OR conditions,
unique app names, string and numeric comparisons, and supported check-variable
output formats including multi-variable and timeout-path outputs. It also
checks fail-only query shapes.

Typical harness run:

```sh
../../../harnesses/uquerydb_harnesses/H01-uquerydb_unit/zlaunch.sh --jobs=4 --port_base=15600 10
```

Run the stem independently:

```sh
./zlaunch.sh --max_time=30 --shore_mport=15600 --shore_pshare=15615 10
```

Run one inspectable case:

```sh
../../../harnesses/uquerydb_harnesses/H01-uquerydb_unit/zlaunch.sh --case=cli_numeric_pass --port_base=15600 10
```

Logging defaults to `minimal`, with no active `pLogger`; mission results,
command status, and `.checkvars` output supply the grading evidence. Pass
`--log=full` to either mission launcher to restore the previous logger
configuration.
