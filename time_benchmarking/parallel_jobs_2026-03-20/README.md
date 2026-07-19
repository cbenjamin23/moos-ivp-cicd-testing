# Parallel Jobs Benchmark

Date: March 20, 2026

Machine notes:

- local benchmark machine
- logical CPU count observed during this session: `10`
- warp used for all runs: `10`

These results measure the current wave-based harness mode:

- isolated temp mission copy per live case
- unique port block per live case
- one `ktm` barrier between waves

## Collision Behavior Motion

Harness:

- [`harnesses/collision_behavior_harnesses/H01-collision_behavior_motion`](../../harnesses/collision_behavior_harnesses/H01-collision_behavior_motion)

Results:

- `jobs=1`: `63.63s`
- `jobs=2`: `44.38s`
- `jobs=3`: `32.54s`
- `jobs=4`: `32.16s`
- `jobs=5`: `33.56s`
- `jobs=6`: `21.33s`

Recommendation:

- use `jobs=6` on this machine for this harness
- this harness has only `6` cases, so `jobs=6` collapses the full suite into one wave

## Contact Manager Unit

Harness:

- [`harnesses/cmgr_harnesses/H01-cmgr_unit`](../../harnesses/cmgr_harnesses/H01-cmgr_unit)

Results:

- `jobs=1`: `237.40s`
- `jobs=2`: `170.19s`
- `jobs=3`: `120.90s`
- `jobs=4`: `103.49s`
- `jobs=5`: `85.68s`
- `jobs=6`: `80.11s`
- `jobs=7`: `73.31s`
- `jobs=8`: `71.24s`
- `jobs=9`: `64.92s`
- `jobs=10`: `62.11s`
- `jobs=11`: `60.66s`
- `jobs=12`: `62.19s`
- `jobs=16`: `58.04s`
- `jobs=20`: `48.62s`
- `jobs=33`: `50.65s` but unstable (`2` case mismatches)

Recommendation:

- best stable value tested: `jobs=20`
- `jobs=33` is faster than many lower values, but it is not stable enough to
  use as the default on this machine

## Obstacle Manager Unit

Harness:

- [`harnesses/obmgr_harnesses/H01-obmgr_unit`](../../harnesses/obmgr_harnesses/H01-obmgr_unit)

Results:

- `jobs=1`: `136.77s`
- `jobs=2`: `94.69s`
- `jobs=3`: `79.02s`
- `jobs=4`: `57.86s`
- `jobs=5`: `50.81s`
- `jobs=6`: `51.57s`
- `jobs=7`: `41.03s`
- `jobs=8`: `47.00s`
- `jobs=9`: `44.35s`
- `jobs=10`: `33.48s`
- `jobs=11`: `38.08s` but unstable (`1` case mismatch)
- `jobs=12`: `32.90s`
- `jobs=16`: `31.99s`
- `jobs=20`: `25.35s` but unstable (`1` case mismatch)

Recommendation:

- best stable value tested: `jobs=16`
- there are local regressions at some intermediate job counts because wave
  count and launch contention both matter
- the absolute fastest point tested, `jobs=20`, was not stable enough to use
  as the default on this machine

## Interpreting Jobs Above 10

Testing above `10` was worthwhile for the larger harnesses.

Why:

- wave-based performance is not limited only by CPU count
- a higher `jobs` value can reduce the number of waves even if it oversubscribes the CPU
- for example, a `20`-case suite can still benefit from moving from `jobs=10` to `jobs=20` because that changes `2` waves into `1`

Why it may stop helping:

- this machine has only `10` logical CPUs
- above that point, per-wave launch overhead, process contention, and I/O pressure rise
- once a harness is already in one wave, larger `jobs` values cannot improve wall clock further

Observed takeaway:

- `H01-cmgr_unit` benefits from oversubscription up to at least `jobs=20`
  because reducing the number of waves matters more than matching CPU count
- `H01-obmgr_unit` also benefits above `10`, but not all the way to `20`
  stably; `jobs=16` was the best stable point tested

## Saved Artifacts

This directory now keeps the benchmark summary only.

The original raw `.log` and `.time` files were removed because they were mostly
launcher noise and the important timing conclusions are already captured here.

## Post-Tuning Checkpoint

After the initial wave-mode rollout, the harness hot path was tuned further by:

- removing the wave-barrier `sleep 1`
- removing the stem `launch.sh` vehicle-to-shoreside `sleep 0.5`
- temporarily reducing the local upstream `xlaunch.sh` post-kill sleep from
  `2.0s` to `0.25s`

These are not full resweeps. They are representative reruns on the same
machine with the same warp, using the previously chosen stable `jobs` values.

Observed checkpoints:

- [`harnesses/collision_behavior_harnesses/H01-collision_behavior_motion`](../../harnesses/collision_behavior_harnesses/H01-collision_behavior_motion)
  - `jobs=6`: about `18.72s`
- [`harnesses/cmgr_harnesses/H01-cmgr_unit`](../../harnesses/cmgr_harnesses/H01-cmgr_unit)
  - `jobs=20`: about `38.64s`
- [`harnesses/cmgr_harnesses/H02-cmgr_motion`](../../harnesses/cmgr_harnesses/H02-cmgr_motion)
  - `jobs=4`: about `38.50s`
- [`harnesses/obmgr_harnesses/H02-obmgr_motion`](../../harnesses/obmgr_harnesses/H02-obmgr_motion)
  - `jobs=4`: about `23.40s`

Interpretation:

- the wave-barrier sleep was pure overhead on the tested harnesses
- the per-stem vehicle/shore launch gap was also unnecessary on the tested
  suites
- the shared `xlaunch.sh` wait could not be removed entirely, but `0.25s`
  stayed stable where `0s` did not

The new helper script for future sweeps is:

- [`scripts/benchmark_parallel.sh`](../../scripts/benchmark_parallel.sh)

Typical usage:

```bash
./scripts/benchmark_parallel.sh \
  --harness=/path/to/moos-ivp-cicd-testing/harnesses/cmgr_harnesses/H01-cmgr_unit \
  --jobs=1,2,4,8,16,20
```

Note:

- the `xlaunch.sh` reduction was only a local experiment in the separate
  `moos-ivp` checkout at `/path/to/moos-ivp/scripts/xlaunch.sh`
- that foreign-repo change has been reverted
- the timings above remain useful as evidence for what a smaller shared
  launcher wait could buy if that upstream script is intentionally changed

## Sleep Changes Applied Here

Repo-local changes that were actually kept:

- wave-harness `sleep 1` after `ktm`: removed from the active wave-enabled
  harnesses in this repo
- stem `launch.sh` vehicle-to-shoreside `sleep 0.5`: removed from the active
  harnessed stems in this repo

These were applied to the current automated mission families, not blindly to
every mission in the workspace.

They were kept in files such as:

- [`missions/cmgr_missions/cmgr_unit/launch.sh`](../../missions/cmgr_missions/cmgr_unit/launch.sh)
- [`missions/cmgr_missions/cmgr_motion/launch.sh`](../../missions/cmgr_missions/cmgr_motion/launch.sh)
- [`missions/obmgr_missions/obmgr_unit/launch.sh`](../../missions/obmgr_missions/obmgr_unit/launch.sh)
- [`missions/obmgr_missions/obmgr_motion/launch.sh`](../../missions/obmgr_missions/obmgr_motion/launch.sh)
- [`missions/collision_behavior_missions/collision_behavior_motion/launch.sh`](../../missions/collision_behavior_missions/collision_behavior_motion/launch.sh)
- [`harnesses/cmgr_harnesses/H01-cmgr_unit/zlaunch.sh`](../../harnesses/cmgr_harnesses/H01-cmgr_unit/zlaunch.sh)
- [`harnesses/cmgr_harnesses/H02-cmgr_motion/zlaunch.sh`](../../harnesses/cmgr_harnesses/H02-cmgr_motion/zlaunch.sh)
- [`harnesses/obmgr_harnesses/H01-obmgr_unit/zlaunch.sh`](../../harnesses/obmgr_harnesses/H01-obmgr_unit/zlaunch.sh)
- [`harnesses/obmgr_harnesses/H02-obmgr_motion/zlaunch.sh`](../../harnesses/obmgr_harnesses/H02-obmgr_motion/zlaunch.sh)
- [`harnesses/collision_behavior_harnesses/H01-collision_behavior_motion/zlaunch.sh`](../../harnesses/collision_behavior_harnesses/H01-collision_behavior_motion/zlaunch.sh)

Recommended scope going forward:

- keep applying those two repo-local reductions to harness hot-path stems and
  wave-enabled harnesses after validation
- do not assume the same is safe for unrelated or legacy missions without
  testing

## Shared `xlaunch.sh` Wait

The separate shared launcher lives at:

- [`moos-ivp/scripts/xlaunch.sh`](https://github.com/moos-ivp/moos-ivp/blob/main/scripts/xlaunch.sh)

The relevant sequence is:

1. `uMayFinish` waits for mission completion.
2. `pkill -INT -P $$` asks the launched mission processes to stop.
3. `sleep 2` gives them time to finish shutdown and flush files.
4. `xlaunch.sh` then repairs a trailing newline in `results.txt` if needed and
   runs optional post-processing.

That sleep exists because `pMissionEval` and other mission processes may still
be writing `results.txt` at shutdown time. In the local experiment:

- reducing it from `2.0s` to `0.25s` stayed stable on the tested harnesses
- removing it entirely caused `actual=missing` cases on a motion harness

So the best tested interpretation is:

- the shared launcher still needs a small bounded wait
- the current upstream `2.0s` value is conservative
- any change there should be managed in the separate `moos-ivp` repo, not here
