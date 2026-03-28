# GitHub Actions Setup

This repo's CI workflow lives in `.github/workflows/build_extend.yml`.

## When It Runs

- `push` to `main`
- `pull_request` targeting `main`
- manual `workflow_dispatch`

Branch pushes outside `main` do not run the workflow automatically. That keeps hosted-runner usage lower during active development.

The workflow also uses concurrency cancellation:

- group: workflow name + git ref
- effect: a newer run on the same branch/PR cancels the older in-progress run

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
- `harnesses/collision_behavior_harnesses/H01-collision_behavior_motion`
- `harnesses/obstacle_behavior_harnesses/H01-obstacle_behavior_motion`

`--just_make` checks targ-file generation and launcher wiring. It does not prove mission correctness.

## 3. `collision-correctness`

Runs one live correctness harness after `build`:

- harness: `harnesses/collision_behavior_harnesses/H01-collision_behavior_motion`
- command: `./zlaunch.sh 10`

This job is the current runtime correctness gate.

## 4. `p01-obstacle-performance`

Runs the first live performance harness after `build`, but only on:

- `pull_request` targeting `main`
- manual `workflow_dispatch`

It does not run on ordinary `push` to `main`.

Current command:

- harness: `harnesses/performance_harnesses/P01-obstacle_gauntlet`
- command: `./zlaunch.sh 10`

This job is intended to catch:

- performance regressions outside the checked-in baseline bands
- runtime planner failures surfaced through the harness warning scan

It uploads `results.txt` as `p01-obstacle-performance-results` and uses the same summary parser as the correctness job.

## Harness Result Handling

The correctness job does more than rely on shell exit status:

1. It runs the harness with `continue-on-error` so later steps still execute.
2. It always uploads `results.txt` as an artifact: `collision-correctness-results`.
3. It parses `results.txt` with `scripts/ci_harness_results_summary.py`.
4. It writes a per-case markdown table into the GitHub Actions step summary.
5. It fails the job if:
   - the harness command itself failed
   - `results.txt` is missing or empty
   - the parsed case count is wrong
   - any case line is missing required fields
   - any `actual` value does not match `expected`
   - any `status` is not `ok`

For the current collision harness, the parser expects 6 case lines.
For the current `P01` performance harness, the parser expects 3 case lines.

## Cache vs Artifact

- Cache: speeds up later workflow runs by reusing previous build outputs when keys match.
- Artifact: shares the current run's built outputs from `build` to downstream jobs so they do not rebuild.

## Notes

- These jobs run on GitHub-hosted Ubuntu runners.
- Because this repo is private, hosted-runner usage counts against the account's included GitHub Actions minutes.
- If you want to validate a feature branch without opening a PR, use the manual `workflow_dispatch` run in GitHub Actions.
