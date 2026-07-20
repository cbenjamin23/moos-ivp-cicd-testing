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
  passes before the six-second deadline when `PROC_WATCH_ALL_OK=true` and
  `PROC_WATCH_SUMMARY` is nonempty.
- `processwatch_custom_present_post_pass`
  Watches `pHostInfo:HOSTINFO_PRESENT` to exercise the custom presence-post
  syntax; passes before the deadline when both booleans are true and the full
  summary contains `pHostInfo(1/0)`.
- `processwatch_multi_present_pass`
  Watches `pHostInfo`, `pMissionEval`, and `uLoadWatch` together to verify
  per-process accounting; passes before the deadline when all-watch health is
  true and the full summary contains all three exact `(1/0)` entries.
- `processwatch_post_mapping_pass`
  Maps the short summary from `PROC_WATCH_SUMMARY` to `UTIL_PROC_SUMMARY`;
  passes before the deadline when the destination is published, the source is
  never published, and the ordinary full summary contains all three expected
  `(1/0)` entries.
- `processwatch_full_summary_mapping_pass`
  Maps the full summary from `PROC_WATCH_FULL_SUMMARY` to
  `UTIL_PROC_FULL_SUMMARY`; passes before the deadline when the source is
  never published and the mapped payload contains the exact `(1/0)` entry for
  each of the three watched processes.
- `processwatch_event_mapping_present_pass`
  Watches `pHostInfo` and maps `PROC_WATCH_EVENT` to `UTIL_PROC_EVENT`;
  passes before the deadline when a mapped event is published, the source
  event is never published, and the full summary contains `pHostInfo(1/0)`.
- `processwatch_missing_awol_fail`
  Watches nonexistent `pGhost:GHOST_PRESENT` to exercise configured-process
  loss; the harness passes with `expected=watch_awol` before the 125-second
  fallback when both booleans are false and the full summary contains
  `pGhost(1/1)`.

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

The evaluators lead on the relevant ProcessWatch health transition. Their
timers are missing-state deadlines, not the normal grading trigger.
