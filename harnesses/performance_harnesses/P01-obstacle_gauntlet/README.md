# P01 Obstacle Gauntlet Harness

Performance harness for the first deterministic gauntlet mission family.

## Current Case

- `baseline_field_pass`
- `dense_field_pass`
- `endurance_random_pass`

The harness currently expects the gauntlet mission to:

- in fixed-field cases, complete one full loop
- in the endurance case, complete 10 full loops before the fail-safe timeout
- incur zero collisions
- avoid timing out
- stay within the checked-in performance baseline bands
- avoid disallowed planner/runtime warning text in the vehicle `.alog`

It preserves the mission-owned report columns and wraps them in the usual harness summary line:

- `case`
- `expected`
- `actual`
- `status`
- `perf_status`
- `perf_notes`
- `warning_count`

## Typical Runs

```bash
./zlaunch.sh 10
./zlaunch.sh --jobs=2 10
./zlaunch.sh --case=baseline_field_pass 10
./zlaunch.sh --just_make 10
```

## Notes

- All cases run the same loop-course mission and select obstacle source through `--mmod`.
- The harness now uses `nspatch` for the endurance case so the longer 10-cycle semantics remain harness-owned instead of being only an internal mission branch.
- Two cases are deterministic comparison runs, and one case is a longer randomized endurance run.
- The harness supports wave-batch execution with `--jobs=<n>` using isolated temp mission copies and per-wave port blocks.
- The endurance case automatically uses a larger launcher `--max_time` budget than the fixed-field cases.
- Range checks live in [performance_baselines.tsv](/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/performance_harnesses/P01-obstacle_gauntlet/performance_baselines.tsv).
- See [BASELINES.md](/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/performance_harnesses/P01-obstacle_gauntlet/BASELINES.md) for what is measured and why the endurance metrics are treated as evaluation snapshots.
- See [NSPATCH.md](/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/performance_harnesses/P01-obstacle_gauntlet/NSPATCH.md) for the current split between stem-owned scenario defaults and harness-owned semantic overrides.
