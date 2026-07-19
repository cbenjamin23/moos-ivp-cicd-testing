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

The launcher also accepts `--log=minimal|full`, with `minimal` as the default.
For this subject harness the two modes intentionally preserve the same
case-defined pLogger configurations and artifacts: changing or narrowing those
configurations would change the behavior being tested. The option provides the
repository-wide interface without imposing an ordinary logging allowlist.

## Cases

- `plogger_explicit_log_pass` Configures `Log=PLOGGER_ALPHA @ 0 nosync` and posts `PLOGGER_ALPHA=alpha`, testing explicit asynchronous logging; passes when the live value is `alpha` and an `.alog` contains that variable-value pair.
- `plogger_wildcard_omit_pass` Enables wildcard logging for `*_ME`, omits `OMIT_ME`, and posts `KEEP_ME=keep` plus `OMIT_ME=omit`, testing the omit pattern; passes when both posts reach the mission evaluator, the `.alog` contains `KEEP_ME=keep`, and it does not contain `OMIT_ME=omit`.
- `plogger_dynamic_log_request_pass` Sends `PLOGGER_CMD="LOG_REQUEST=DYN_LOG @ 0 nosync"` before posting `DYN_LOG=dynamic`, testing runtime addition of an asynchronous log variable; passes when the live value is `dynamic` and an `.alog` contains that variable-value pair.
- `plogger_double_log_pass` Explicitly logs the numeric post `PLOGGER_NUM=42.5`, exercising double-valued asynchronous logging; passes when the live value is between 42 and 43 and an `.alog` contains a `PLOGGER_NUM` entry.
- `plogger_sync_log_pass` Enables a 0.5-second synchronous log and configures `SYNC_ALPHA` as a synchronous column, testing static `.slog` column creation; passes when `SYNC_ALPHA=sync` reaches the evaluator and an `.slog` header contains `SYNC_ALPHA`.
- `plogger_wildcard_xlog_pass` Enables wildcard logging for `XLOG_*`, omits `XLOG_OMIT`, and enables `WildcardExclusionLog`, testing exclusion-file routing; passes when both live posts arrive, the `.alog` contains `XLOG_KEEP=keep` but not `XLOG_OMIT=omit`, and an `.xlog` contains the omitted pair.
- `plogger_filetimestamp_false_pass` Sets `FileTimeStamp=false`, `File=LOG_plogger_filetimestamp_false_pass`, and posts `FIXED_NAME=fixed`, testing deterministic log naming; passes when that exact directory and `.alog` filename exist and the file contains the posted pair.
- `plogger_datatype_precision_pass` Sets `MarkDataType=true` and `DoublePrecision=3`, then posts `DTYPE_NUM=3.14159265` and `DTYPE_STR=typed`, testing type markers and numeric rounding; passes when the live values match and an `.alog` contains `D:3.142` and `S:typed` entries.
- `plogger_dynamic_sync_column_pass` Enables a 0.5-second synchronous log, sends `PLOGGER_CMD="LOG_REQUEST=DYN_SYNC @ 0"`, and posts `DYN_SYNC=dynsync`, testing runtime addition of a synchronous column; passes when the live value is `dynsync` and an `.slog` header contains `DYN_SYNC`.
- `plogger_copy_file_request_pass` Sends `PLOGGER_CMD="COPY_FILE_REQUEST=logger_copy_source.txt"`, testing runtime file copying into the log directory; passes when the live completion flag is true and the generated `logger_copy_source._txt` contains the source text `pLogger copy-file source artifact`.

Typical commands:

```sh
./zlaunch.sh --jobs=2 --port_base=12000 --max_time=20 10
./zlaunch.sh --jobs=4 --log=full --port_base=12000 --max_time=20 10
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
