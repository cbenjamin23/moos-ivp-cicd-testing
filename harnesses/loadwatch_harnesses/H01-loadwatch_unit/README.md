# H01-loadwatch_unit

Patch-driven harness for
[`missions/loadwatch_missions/loadwatch_unit`](../../../missions/loadwatch_missions/loadwatch_unit).
It isolates `uLoadWatch` threshold handling with scripted `DB_UPTIME` mail.

## Current Matrix

- `loadwatch_no_breach_pass` Baseline high-threshold case; expects zero near-breach and hard-breach counts.
- `loadwatch_near_breach_pass` Posts a gap inside the near-breach band and expects a near-breach without a hard breach.
- `loadwatch_near_boundary_no_breach_pass` Posts exactly at the near threshold boundary and expects no near-breach or hard-breach count.
- `loadwatch_breach_pass` Posts a gap above the hard threshold and expects both hard-breach and near-breach evidence.
- `loadwatch_trigger_suppresses_first_pass` Enables breach-trigger holdoff and expects the first threshold crossing not to become a hard breach.
- `loadwatch_trigger_second_breaches_pass` Enables breach-trigger holdoff and expects a later crossing to become a hard breach.
- `loadwatch_lowercase_any_no_breach_pass` Uses lowercase `any` to document case-sensitive threshold matching; expects no breach.
- `loadwatch_specific_app_breach_pass` Watches a named app and expects that app's gap to produce a hard breach.
- `loadwatch_specific_app_ignores_other_pass` Watches one named app while another app crosses the gap; expects no breach.
- `loadwatch_hard_boundary_no_breach_pass` Posts exactly at the hard threshold boundary and expects no hard breach.
- `loadwatch_low_threshold_ignored_pass` Configures a below-minimum threshold and expects the low threshold to be ignored.
- `loadwatch_near_thresh_clamped_high_pass` Configures an over-high near threshold and expects clamped behavior with no near breach.

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
