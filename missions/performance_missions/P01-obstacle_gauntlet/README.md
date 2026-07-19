# P01 Obstacle Gauntlet

Deterministic obstacle-course mission for early performance-style evaluation.

## Intent

This mission is not a narrow correctness micro-test. It is a gauntlet mission family:

- one ownship
- one fixed `02-obavoid`-style loop course
- self-grading on safe completion

The goal is to measure whether the autonomy stack can handle both repeatable fixed-field runs and a longer endurance run cleanly and in a reasonable amount of time.

## Current Grade

The stem mission currently supports two evaluation modes:

- fixed-field modes: complete one loop before timeout
- endurance mode: complete 10 loops before a fail-safe timeout

Manual pass conditions can depend on the selected scenario:

- `baseline_field` / `dense_field`
  - `WPT_CYCLES >= 1`
  - `OB_TOTAL_COLLISIONS = 0`
  - `OB_TOTAL_ENCOUNTERS >= 1`
  - `MISSION_TIMEOUT = false`
- `endurance_random`
  - `WPT_CYCLES >= 10`
  - `OB_TOTAL_COLLISIONS = 0`
  - `MISSION_TIMEOUT = false`

Reported metrics:

- wall-clock time from `pMissionEval`
- completed loop count
- collision count
- near-miss count
- encounter count
- short mission hash

The companion harness treats those metrics as evaluation-snapshot values and
compares them against explicit range baselines. That matters most for the
randomized endurance case, where the launcher may let the mission run briefly
after `MISSION_EVALUATED=true` before teardown finishes.

## Typical Runs

```bash
./launch.sh 10
./zlaunch.sh --scenario=baseline_field 10
./zlaunch.sh --log=full --scenario=baseline_field 10
./zlaunch.sh --just_make 10
```

Direct launches default to `--log=minimal`, with only vehicle `APP_LOG`
evidence retained for performance warning scans. `--log=full` restores the
original shoreside and vehicle wildcard logging.

## Notes

- The course layout and viewer framing are derived from `missions-auto/02-obavoid`.
- The fixed field layouts now live in [obstacles/](obstacles/README.md), and `init_field.sh` stages the selected one into the mission root as `obstacles.txt`.
- `endurance_random` generates a fresh random `obstacles.txt` at launch and enables obstacle reset behavior in `uFldObstacleSim`.
- Mission `zlaunch.sh` automatically raises its default `--max_time` for `endurance_random` so `uMayFinish` does not cut the run off at the short fixed-case budget.
- The current named mission scenarios are `baseline_field`, `dense_field`, and `endurance_random`.
- The companion harness maps its cases to these mission scenarios and applies
  explicit evaluator patches with `nspatch`.
- This is the first performance-style example in the repo and is intended to grow into additional fixed-field variants later.
