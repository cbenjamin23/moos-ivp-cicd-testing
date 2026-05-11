# H01 pLogger Unit Harness

This harness runs one small MOOS community and verifies `pLogger` artifacts
after the mission exits. `pMissionEval` owns the live MOOS grade, while the
harness checks the generated `.alog` because log capture is the behavior under
test.

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
