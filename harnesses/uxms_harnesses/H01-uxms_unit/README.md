# H01-uxms_unit

Captured-output harness for
[`missions/uxms_missions/uxms_unit`](../../../missions/uxms_missions/uxms_unit).
It launches a controlled MOOSDB publisher and runs bounded `uXMS` terminal
captures to verify scoping, column display, history, truncation, source filters,
and mission-file configuration behavior.

This is a documented CLI-output grading exception. `pMissionEval` owns the
publisher-mission readiness grade and confirms the existing `XMS_ALPHA` and
`XMS_NUM` facts. Because `uXMS` is a terminal viewer and does not publish a
display-verdict variable, the harness supplements that mission grade with the
case's exact required and forbidden terminal tokens. No helper application or
additional grading variable is used.

The launcher requires Bash 5.1 or newer for rolling `--jobs` scheduling. Every
serial and rolling case runs in its own copy under `.harness_runs`, receives a
distinct 30-port block, uses the repository's canonical scoped teardown helper,
and contributes exactly one result row in declared-case order. A nonzero
subject, launch, setup, result-validation, or cleanup status is reported as a
runner failure rather than being mistaken for a pMissionEval verdict.

## Current Matrix

- `scoped_var_pass`: Passes `XMS_ALPHA` as the only scoped variable, testing explicit command-line scope; passes when the capture contains `XMS_ALPHA` and its posted value `alpha`.
- `scoped_excludes_noise_pass`: Scopes only `XMS_ALPHA` while the publisher also posts `XMS_NOISE`, testing exclusion of unrequested mail; passes when the capture contains `XMS_ALPHA=alpha` evidence and no `XMS_NOISE` token.
- `all_mode_pass`: Runs with `--all`, testing the long option for database-wide scope; passes when the capture contains both the MOOSDB variable `DB_UPTIME` and scripted `XMS_ALPHA`.
- `all_shortcut_pass`: Runs with `-a`, testing the short alias for database-wide scope; passes when the capture contains both `DB_UPTIME` and `XMS_ALPHA`.
- `show_source_pass`: Runs `--show=source` for `XMS_ALPHA`, testing publication-source display; passes when the capture contains `XMS_ALPHA` and its publisher `uTimerScript`.
- `show_time_only_pass`: Runs `--show=time` for `XMS_ALPHA`, testing the time-column option; passes when the capture contains `XMS_ALPHA` and the substring `(T`.
- `show_community_only_pass`: Runs `--show=community` for `XMS_ALPHA`, testing community display; passes when the capture contains `XMS_ALPHA` and `shoreside`.
- `show_time_community_pass`: Runs `--show=time,community` for `XMS_ALPHA`, testing the combined column request; passes when the capture contains `XMS_ALPHA` and `shoreside`, but the time column is not independently checked.
- `show_source_community_pass`: Runs `--show=source,community` for `XMS_ALPHA`, testing both named columns together; passes when the capture contains `XMS_ALPHA`, `uTimerScript`, and `shoreside`.
- `history_var_pass`: Posts `first`, `second`, and `third` to `XMS_HIST` while scoping it with `--history`, testing history-mode capture; passes when the output contains `XMS_HIST` and the final value `third`.
- `trunc_long_payload_pass`: Scopes a 62-character `XMS_LONG` value with `--trunc=25`, testing the long truncation option; passes when the capture contains `abcdefghijklmnopqrstuvwxyz` but not the complete payload.
- `clean_cli_scope_pass`: Loads a mission file that configures `XMS_ALPHA` and `XMS_SECONDARY`, then runs `--clean XMS_NUM`, testing replacement of file scope by CLI scope; passes when the capture contains `XMS_NUM=42` evidence and no `XMS_ALPHA` token.
- `config_var_pass`: Loads the mission file without CLI variable names, testing its configured `var=XMS_ALPHA` and `var=XMS_SECONDARY` scope; passes when both variable names appear in the capture.
- `filter_prefix_pass`: Combines `--all` with `--filter=XMS_`, testing prefix filtering of database-wide scope; passes when `XMS_ALPHA` and `XMS_NUM` appear and `DB_UPTIME` does not.
- `src_timer_pass`: Uses `--src=uTimerScript` without explicit variable names, testing source-derived scope; passes when the capture includes the publisher's `XMS_ALPHA` and `XMS_NUM` variables.
- `novirgins_pass`: Scopes posted `XMS_ALPHA` and never-posted `XMS_MISSING` with `--novirgins`, testing suppression of virgin variables; passes when `XMS_ALPHA=alpha` evidence appears and neither `XMS_MISSING` nor `n/a` appears.
- `novirgins_shortcut_pass`: Repeats the posted and never-posted scope with `-g`, testing the short alias for virgin suppression; passes with the same `XMS_ALPHA` presence and `XMS_MISSING`/`n/a` absence checks.
- `shortcut_source_pass`: Runs `-s` for `XMS_ALPHA`, testing the short alias for source display; passes when the capture contains `XMS_ALPHA` and `uTimerScript`.
- `shortcut_source_time_pass`: Runs `-st` for `XMS_ALPHA`, testing the combined source-and-time shortcut; passes when the capture contains `XMS_ALPHA` and `uTimerScript`, but the time column is not independently checked.
- `filter_specific_pass`: Combines `--all` with the narrower `--filter=XMS_A`, testing selective prefix filtering; passes when `XMS_ALPHA=alpha` evidence appears and neither `XMS_NUM` nor `DB_UPTIME` appears.
- `filter_no_match_absent_pass`: Combines `--all` with `--filter=NO_SUCH_PREFIX`, testing a filter with no matching variables; passes when a `SCOPING` report is produced without `XMS_ALPHA` or `DB_UPTIME`.
- `colorany_pass`: Applies `--colorany=XMS_ALPHA` to the scoped variable, testing that the option is accepted without losing its row; passes when the capture contains `XMS_ALPHA` and `alpha`, without checking terminal color codes.
- `events_mode_pass`: Runs `XMS_ALPHA` with `--mode=events` while the publisher posts `alpha` once, exercising event-mode output; passes when the capture contains `XMS_ALPHA` and `alpha`.
- `paused_shortcut_pass`: Runs `XMS_ALPHA` with `-p`, exercising the short alias for paused mode; passes when startup capture contains `XMS_ALPHA` and `alpha`, without checking that later refreshes remain paused.
- `shortcut_trunc_pass`: Scopes the 62-character `XMS_LONG` value with `-t`, testing the `--trunc=25` shortcut; passes when the capture contains `abcdefghijklmnopqrstuvwxyz` but not the complete payload.
- `alll_mode_pass`: Runs with `--alll`, which requests even the variables normally excluded by `--all`; passes when the capture contains `DB_UPTIME` and `XMS_ALPHA`, the same evidence used for ordinary `--all`.
- `server_underscore_args_pass`: Connects with `--server_host=localhost` and `--server_port=<case-port>`, testing the underscore server aliases; passes when the capture contains `XMS_ALPHA` and `alpha` from that database.
- `moos_alias_args_pass`: Connects with `--mooshost=localhost` and `--moosport=<case-port>`, testing the compact MOOS aliases; passes when the capture contains `XMS_ALPHA` and `alpha` from that database.
- `moos_underscore_args_pass`: Connects with `--moos_host=localhost` and `--moos_port=<case-port>`, testing the underscore MOOS aliases; passes when the capture contains `XMS_ALPHA` and `alpha` from that database.
- `alias_proc_name_pass`: Runs with `--alias=uXMS_ci` while scoping `XMS_ALPHA`, exercising custom process-name parsing; passes when the capture contains `XMS_ALPHA` and `alpha`, without checking the registered process name.
- `colormap_pair_pass`: Applies `--colormap=XMS_ALPHA,red` to the scoped variable, testing that a variable/color pair is accepted without losing its row; passes when the capture contains `XMS_ALPHA` and `alpha`, without checking terminal color codes.
- `clean_shortcut_scope_pass`: Loads the mission-file scope, then runs `-c XMS_NUM`, testing the short alias for ignoring configured variables; passes when the capture contains `XMS_NUM=42` evidence and no `XMS_ALPHA` token.

## Run

```sh
./zlaunch.sh --jobs=4 --port_base=15200 --max_time=30 10
```

Run one inspectable case:

```sh
./zlaunch.sh --case=history_var_pass --port_base=15200 --max_time=30 10
```

Use `--keep_workdirs` to retain generated targets, `uxms.out`, mission results,
and alogs for inspection. `--gui` is accepted only with one explicit case.

The harness defaults to `--log=minimal`, which omits `pLogger` because terminal
capture and mission results are the grading inputs. Use `--log=full` for the
complete matrix, or combine it with `--case=<name>` for one fully logged case.

## Migration Validation

The legacy two-slot batch-wave matrix passed 96/96 case runs in 106.33,
106.33, and 105.85 wall seconds. Its one-slot matrix passed 32/32 in 200.21
seconds. The migrated rolling matrix passed 96/96 in 126.65, 127.46, and
125.63 seconds; the migrated one-slot matrix passed 32/32 in 249.00 seconds.

The retained migrated run contains 32 isolated missions, terminal captures,
alogs, one-row pMissionEval reports, and distinct MOOSDB ports from 54000
through 54930. Validation also covered all-case target generation, standalone
stem execution, output-check rejection, invalid arguments, port bounds, lock
contention, exact result ordering, Bash 3.2 re-execution/rejection, and scoped
post-run cleanup.
