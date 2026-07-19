# H01-loadwatch_unit

Logging is minimal by default and launches no `pLogger`. Use `--log=full` for
the whole matrix or with `--case=NAME` for one diagnostic case.

Patch-driven harness for
[`missions/loadwatch_missions/loadwatch_unit`](../../../missions/loadwatch_missions/loadwatch_unit).
It isolates `uLoadWatch` threshold handling with scripted `DB_UPTIME` mail.

## Current Matrix

- `loadwatch_no_breach_pass`
  Sets the generic gap threshold to 100 seconds and posts
  `UTILITY_ITER_GAP=1.5`; passes when both hard- and near-breach counts remain
  zero.
- `loadwatch_near_breach_pass`
  Sets `gapthresh=3.0` and `near_breach_thresh=0.5`, making the derived near
  boundary 2.0, then posts a 2.6-second gap; passes when a near breach and
  positive near count appear while hard-breach count remains zero.
- `loadwatch_near_boundary_no_breach_pass`
  Posts a 2.0-second gap exactly at the derived near boundary for threshold 3.0
  and factor 0.5; passes when both near- and hard-breach counts remain zero,
  demonstrating the comparison is strict.
- `loadwatch_breach_pass`
  Posts a 3.5-second gap above the 3.0-second generic threshold with
  `breach_trigger=0`; passes when `ULW_BREACH=true` and both hard- and
  near-breach counts are positive.
- `loadwatch_trigger_suppresses_first_pass`
  Sets `breach_trigger=1` and posts one 3.5-second threshold crossing; passes
  when near-breach count is positive but hard-breach count remains zero.
- `loadwatch_trigger_second_breaches_pass`
  Sets `breach_trigger=1` and posts 3.5- and 3.7-second crossings; passes when
  `ULW_BREACH=true`, hard-breach count is positive, and near-breach count is at
  least two.
- `loadwatch_lowercase_any_no_breach_pass`
  Configures case-sensitive `app=any` while posting `UTILITY_ITER_GAP=3.5`;
  passes when neither breach counter increments, demonstrating lowercase
  `any` does not provide the `ANY` fallback.
- `loadwatch_specific_app_breach_pass`
  Sets a 3.0-second threshold for `UTIMERSCRIPT` and posts
  `UTILITY_ITER_GAP=3.5`; passes when `ULW_BREACH=true` and both breach counts
  are positive.
- `loadwatch_specific_app_ignores_other_pass`
  Configures a threshold only for `PHOSTINFO` and posts
  `OTHER_ITER_GAP=3.5`; passes when both breach counts remain zero.
- `loadwatch_hard_boundary_no_breach_pass`
  Posts a 3.0-second gap exactly at the hard threshold; passes when hard-breach
  count remains zero while near-breach count is positive, demonstrating the
  hard comparison is strict.
- `loadwatch_low_threshold_ignored_pass`
  Configures `gapthresh=1.0` and posts a 3.5-second gap; passes when both breach
  counts remain zero, exercising the code path that ignores thresholds at or
  below 1.0.
- `loadwatch_near_thresh_clamped_high_pass`
  Configures `near_breach_thresh=2.0`, which is clamped to 1.0, and posts a
  2.5-second gap below the resulting 3.0-second near boundary; passes when both
  breach counts remain zero.

## Run

```sh
./zlaunch.sh --jobs=3 --port_base=11200 --max_time=40 10
```

`--jobs` uses rolling scheduling. Every selected case runs from its own mission
copy and port block; add `--keep_workdirs` to retain those copies for target,
sidecar, result, and log inspection.

Run one inspectable case:

```sh
./zlaunch.sh --case=loadwatch_breach_pass --port_base=11200 --max_time=40 10
```
