# GitHub Actions Setup

This repo's CI workflow lives in `.github/workflows/build_extend.yml`.

## When It Runs

- manual `workflow_dispatch` only

This workflow is intentionally manual-only during the current development phase.

The workflow also uses concurrency cancellation:

- group: workflow name + git ref
- effect: a newer manual run on the same branch cancels the older in-progress run

## High-Level Job Layout

### 1. `build`

Builds the toolchain once on `ubuntu-latest`:

- checks out this repo
- clones `moos-ivp`
- restores a cache for built outputs
- builds `moos-ivp`
- builds this repo
- uploads a shared artifact named `workspace-build`

The uploaded artifact is a curated runtime/build subset, not the whole workspace. It currently includes:

- `moos-ivp/build/MOOS/MOOSCore/lib`
- `moos-ivp/build/MOOS/MOOSGeodesy/lib`
- `moos-ivp/bin`
- `moos-ivp/scripts`
- `moos-ivp/lib`
- `build`
- `bin`
- `lib`

This subset is intentionally conservative for the current active harnesses, but it should not be treated as a guaranteed future-proof bundle for all later correctness or performance expansion. When new harness families or benchmark jobs are added, artifact sufficiency should be revalidated and the uploaded paths expanded deliberately if those jobs need additional built apps, libraries, or runtime assets.

## 2. `just-make`

Runs a matrix of active harness generation checks on separate GitHub-hosted runners.

Each matrix job:

- downloads `workspace-build`
- restores executable bits on downloaded binaries/scripts
- sets `PATH` and `LD_LIBRARY_PATH`
- runs one harness with `./zlaunch.sh --just_make 10`

Current active harness list:

- `harnesses/cmgr_harnesses/H01-cmgr_unit`
- `harnesses/cmgr_harnesses/H02-cmgr_motion`
- `harnesses/obmgr_harnesses/H01-obmgr_unit`
- `harnesses/obmgr_harnesses/H02-obmgr_motion`
- `harnesses/colregs_harnesses/H01-colregs_classification`
- `harnesses/colregs_harnesses/H03-colregs_execution`
- `harnesses/collision_behavior_harnesses/H01-collision_behavior_motion`
- `harnesses/obstacle_behavior_harnesses/H01-obstacle_behavior_motion`

`--just_make` checks targ-file generation and launcher wiring. It does not prove mission correctness.

## 3. `collision-correctness`

Runs one live correctness harness after `build`:

- harness: `harnesses/collision_behavior_harnesses/H01-collision_behavior_motion`
- command: `./zlaunch.sh 10`

This job is the current runtime correctness gate.

## 4. `colregs-classification-correctness`

Runs one live classification harness after `build`:

- harness: `harnesses/colregs_harnesses/H01-colregs_classification`
- command: `./zlaunch.sh 10`

This job is the current canonical COLREGS classification gate.

## 5. `colregs-correctness`

Runs one live correctness harness after `build`:

- harness: `harnesses/colregs_harnesses/H03-colregs_execution`
- command: `./zlaunch.sh 10`

This job is the current COLREGS runtime correctness gate.

## 6. `p01-obstacle-performance`

Runs the first live performance harness after `build`, but only on:

- manual `workflow_dispatch` with `dispatch_mode=full`

Current command:

- harness: `harnesses/performance_harnesses/P01-obstacle_gauntlet`
- command: `PERF_PROFILE=ci ./zlaunch.sh`

This job is intended to catch:

- performance regressions outside the checked-in baseline bands
- runtime planner failures surfaced through the harness warning scan
- wave-batch regressions in the parallelized performance harness path

It uploads `results.txt` as `p01-obstacle-performance-results` and uses the same summary parser as the correctness job.

## 7. `p03-colregs-performance`

Runs the COLREGS traffic-ring performance harness after `build`, but only on:

- manual `workflow_dispatch` with `dispatch_mode=full`

Current command:

- harness: `harnesses/performance_harnesses/P03-colregs_traffic_ring`
- command: `PERF_PROFILE=ci ./zlaunch.sh`

Current `P03` CI profile behavior:

- `PERF_PROFILE=ci` defaults `zlaunch.sh` to warp `5`
- local/default runs still default to warp `10`

## Harness Result Handling

The correctness job does more than rely on shell exit status:

1. It runs the harness with `continue-on-error` so later steps still execute.
2. It always uploads `results.txt` as an artifact:
   `collision-correctness-results`,
   `colregs-classification-correctness-results`, or
   `colregs-correctness-results`.
3. It parses `results.txt` with `scripts/ci_harness_results_summary.py`.
4. It writes a per-case markdown table into the GitHub Actions step summary.
5. It fails the job if:
   - the harness command itself failed
   - `results.txt` is missing or empty
   - the parsed case count is wrong
   - any case line is missing required fields
   - any `actual` value does not match `expected`
   - any `case_result` is not `success`

For the current collision harness, the parser expects 6 case lines.
For the current COLREGS classification harness, the parser expects 22 case lines.
For the current COLREGS execution harness, the parser expects 23 case lines.
For the current `P01` performance harness, the parser expects 3 case lines.
For the current `P03` performance harness, the parser expects 3 case lines.

## Manual Dispatch Inputs

`build_extend.yml` now supports targeted manual dispatch on branches without
introducing a second workflow file.

Current `workflow_dispatch` inputs:

- `dispatch_mode`: `full`, `family_run`, `correctness_subset`, or `just_make_subset`
- `family`: family key used by `family_run`
- `targets`: comma-separated keys used by `correctness_subset` and `just_make_subset`
- `time_warp`: warp passed to manual runs for harnesses that do not use a harness-specific CI profile

Current supported subset target keys:

- `cmgr_h01`
- `cmgr_h02`
- `obmgr_h01`
- `obmgr_h02`
- `colregs_h01`
- `colregs_h02`
- `colregs_h03`
- `colregs_h04`
- `collision_h01`
- `obstacle_behavior_h01`
- `opregion_h01`
- `p01_obstacle`
- `p02_colregs`
- `p03_colregs`

Current supported family keys:

- `cmgr`
- `obmgr`
- `colregs`
- `collision_behavior`
- `obstacle_behavior`
- `opregion`
- `performance`

How manual dispatch behaves:

- `dispatch_mode=full`: existing full workflow behavior, including the active
  correctness and performance jobs
- `dispatch_mode=family_run`: build once, then run the selected live harness
  family only
  - the matrix box is labeled by family, e.g. `colregs-family`
  - `family=colregs` currently expands to `H01`, `H02`, `H03`, `H04`, `P02`,
    and `P03`
  - `family=performance` currently expands to `P01`, `P02`, and `P03`
  - `P01` and `P03` use `PERF_PROFILE=ci` in manual family runs instead of the
    manual `time_warp` input
- `dispatch_mode=correctness_subset`: build once, then run only the selected
  live harness targets
  - `P01` and `P03` also use `PERF_PROFILE=ci` here if selected directly
- `dispatch_mode=just_make_subset`: build once, then run only the selected
  `--just_make` harnesses

## Cache vs Artifact

- Cache: speeds up later workflow runs by reusing previous build outputs when keys match.
- Artifact: shares the current run's built outputs from `build` to downstream jobs so they do not rebuild.

## Notes

- These jobs run on GitHub-hosted Ubuntu runners.
- Because this repo is private, hosted-runner usage counts against the account's included GitHub Actions minutes.
- If you want to validate a feature branch without opening a PR, use the manual `workflow_dispatch` run in GitHub Actions.
