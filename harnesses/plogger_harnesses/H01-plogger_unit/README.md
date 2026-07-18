# H01 pLogger Unit Harness

This harness runs one small MOOS community and verifies `pLogger` artifacts
after the mission exits. `pMissionEval` owns the live MOOS grade, while the
harness checks the generated `.alog`, `.slog`, `.xlog`, or copied file because
those filesystem artifacts are the behavior under test and cannot be graded
through live MOOS mail after shutdown. A passing aggregate row therefore
requires both a valid mission-owned `grade=pass` row and the expected artifact.
An artifact mismatch is normalized as `grade=fail reason=artifact_check_failed`
while preserving the mission result as provenance.

Every selected case runs in its own mission copy and port block. The launcher
supports Bash 5.1 rolling `--jobs` scheduling, deterministic result ordering,
`--keep_workdirs`, explicit `--case`, `--max_time`, and `--port_base` controls,
and root-scoped teardown. Serial and rolling runs use the same isolated path.

## Cases

- `plogger_explicit_log_pass` Logs an explicitly configured variable and expects the `.alog` to contain the posted value.
- `plogger_wildcard_omit_pass` Enables wildcard logging with an omit pattern and expects the kept variable in the `.alog` while the omitted variable is absent.
- `plogger_dynamic_log_request_pass` Sends a `PLOGGER_CMD` log request at runtime and expects the dynamically requested variable to appear in the `.alog`.
- `plogger_double_log_pass` Logs a numeric MOOS value and expects both mission-owned numeric bounds and `.alog` variable evidence.
- `plogger_sync_log_pass` Enables synchronous logging and expects the `.slog` header to contain the configured synchronous column.
- `plogger_wildcard_xlog_pass` Enables wildcard exclusion logging and expects accepted mail in `.alog` while omitted mail is written to `.xlog`.
- `plogger_filetimestamp_false_pass` Disables file timestamping and expects the fixed log directory and fixed `.alog` name to contain the posted value.
- `plogger_datatype_precision_pass` Enables datatype marking and reduced double precision, then expects typed string and rounded numeric entries in `.alog`.
- `plogger_dynamic_sync_column_pass` Sends a runtime log request with synchronous logging enabled and expects the dynamic column name in `.slog`.
- `plogger_copy_file_request_pass` Sends a runtime copy-file request and expects pLogger to copy the requested file into the log directory.

Typical commands:

```sh
./zlaunch.sh --jobs=2 --port_base=12000 --max_time=20 10
./zlaunch.sh --case=plogger_wildcard_omit_pass --port_base=12000 --max_time=20 10
```

## Migration Validation

The July 2026 migration preserved all ten case configurations and all existing
mission conditions. Three legacy two-job runs passed 30/30 rows in 29.65,
28.61, and 29.23 seconds; legacy serial passed 10/10 in 50.56 seconds. Migrated
two-job validation passed 40/40 rows in 29.39, 31, 31, and 29.73 seconds;
migrated isolated serial passed 10/10 in 56 seconds.

Validation also covered both skill static checks, Bash syntax, ShellCheck,
standalone target generation and live execution, all-case generation, distinct
ports, exact one-row aggregation, failure-path probes, retained logger
artifacts, and post-run listener checks. A focused negative probe confirmed
that removing the expected copied file now produces `artifact_check_failed`;
the final full matrix produced ten `.alog` files, two `.slog` files, one
`.xlog`, one copied file, and ten passing aggregate rows.
