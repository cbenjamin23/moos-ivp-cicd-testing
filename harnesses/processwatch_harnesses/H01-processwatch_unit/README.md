# H01-processwatch_unit

Logging is minimal by default and launches no `pLogger`. Use `--log=full` for
the whole matrix or with `--case=NAME` for one diagnostic case.

Patch-driven harness for
[`missions/processwatch_missions/processwatch_unit`](../../../missions/processwatch_missions/processwatch_unit).
It isolates `uProcessWatch` process-presence reporting while using small peer
apps as watched subjects.

## Current Matrix

- `processwatch_all_present_pass` Watches the default peer set and expects `PROC_WATCH_ALL_OK=true` with an `All Present` summary.
- `processwatch_custom_present_post_pass` Maps present status to a custom post variable and expects the watched app to be reported present.
- `processwatch_multi_present_pass` Watches multiple peer apps and expects a complete full-summary payload.
- `processwatch_post_mapping_pass` Maps the all-ok summary post and expects the mapped summary plus full process counts.
- `processwatch_full_summary_mapping_pass` Maps the full-summary post and expects the mapped payload to carry the watched process counts.
- `processwatch_event_mapping_present_pass` Maps event output and expects a present event for the watched app.
- `processwatch_missing_awol_fail` Watches a nonexistent app long enough to cross the AWOL delay; this case passes when `PROC_WATCH_ALL_OK=false`, `GHOST_PRESENT=false`, and the AWOL summary names the missing app.

## Run

```sh
./zlaunch.sh --jobs=3 --port_base=11600 --max_time=180 10
```

Run one inspectable case:

```sh
./zlaunch.sh --case=processwatch_missing_awol_fail --port_base=11600 --max_time=180 10
```

Results are emitted as `case=<name>` followed by the mission-owned row. The
expected-AWOL case reports `grade=pass` when the absent-process evidence is
observed, with `expected=watch_awol`.

Every selected case runs from its own mission copy and port block. `--jobs=N`
uses rolling scheduling, so a new case starts as soon as any active case ends.
Use `--keep_workdirs` to retain the per-case missions, logs, and result rows for
inspection.

Latest validation:

- July 15, 2026
- focused expected-AWOL case: three consecutive passes with
  `--max_time=180 10`
- grouped matrix: three `7/7` rolling passes with `--jobs=3 --max_time=180 10`
- serial matrix: `7/7` cases passed with `--jobs=1 --max_time=180 10`
- retained inspection confirmed one target, intended sidecar, log, and
  mission-owned result row per case, with no remaining mission processes
