# H01-processwatch_unit

Logging is minimal by default and launches no `pLogger`. Use `--log=full` for
the whole matrix or with `--case=NAME` for one diagnostic case.

Patch-driven harness for
[`missions/processwatch_missions/processwatch_unit`](../../../missions/processwatch_missions/processwatch_unit).
It isolates `uProcessWatch` process-presence reporting while using small peer
apps as watched subjects.

## Current Matrix

- `processwatch_all_present_pass`
  Enables `watch_all=true` while excluding the command-line utility patterns;
  passes when `PROC_WATCH_ALL_OK=true` and `PROC_WATCH_SUMMARY` is nonempty,
  without comparing its reported `All Present` text.
- `processwatch_custom_present_post_pass`
  Watches `pHostInfo:HOSTINFO_PRESENT`; passes when both
  `PROC_WATCH_ALL_OK` and the custom `HOSTINFO_PRESENT` post are true.
- `processwatch_multi_present_pass`
  Watches `pHostInfo`, `pMissionEval`, and `uLoadWatch`; passes when
  `PROC_WATCH_ALL_OK=true` and both summary variables are nonempty, without
  comparing the per-process counts.
- `processwatch_post_mapping_pass`
  Maps `PROC_WATCH_SUMMARY` to `UTIL_PROC_SUMMARY` while watching the three
  peer apps; passes when all-watch health is true and the mapped summary and
  ordinary full summary are nonempty.
- `processwatch_full_summary_mapping_pass`
  Maps `PROC_WATCH_FULL_SUMMARY` to `UTIL_PROC_FULL_SUMMARY`; passes when
  all-watch health is true and the ordinary short summary and mapped full
  summary are nonempty.
- `processwatch_event_mapping_present_pass`
  Watches `pHostInfo` and maps `PROC_WATCH_EVENT` to `UTIL_PROC_EVENT`;
  passes when all-watch health is true and both the mapped event and ordinary
  full summary are nonempty.
- `processwatch_missing_awol_fail`
  Watches nonexistent `pGhost:GHOST_PRESENT` for 125 seconds; the harness
  passes with `expected=watch_awol` when `PROC_WATCH_ALL_OK=false` and
  `GHOST_PRESENT=false`, while the reported AWOL summary text is not graded.

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
