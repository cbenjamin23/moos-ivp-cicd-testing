# moos-ivp-cicd-testing

CI/CD test workspace for MOOS-IvP. This repo contains two complementary test
layers:

- mission harnesses that launch MOOS-IvP apps and behaviors in controlled
  scenarios
- C++ tests that exercise library-level tools without launching missions

Use this repo to validate a MOOS-IvP checkout, reproduce app/behavior
regressions, and grow repeatable coverage around important upstream tools.

Website: [MOOS-IvP Harnesses](https://cbenjamin23.github.io/moos-ivp-cicd-testing/)

## Quick Start

Build the repo:

```bash
./build.sh
```

Run all C++ tests:

```bash
ctest --test-dir build --output-on-failure
```

Run one C++ label:

```bash
ctest --test-dir build -L geometry --output-on-failure
```

Run one mission harness case:

```bash
cd harnesses/waypoint_behavior_harnesses/H01-waypoint_behavior_motion
./zlaunch.sh --case=single_point_arrival_pass 10
```

Run a harness matrix locally:

```bash
./zlaunch.sh --jobs=4 --port_base=15000 10
```

Do not run two harnesses at the same time unless their `--port_base` ranges are
isolated.

## Local Testing Against MOOS-IvP Edits

This is the simplest workflow. Check out the branch in the local `moos-ivp`
tree this repo already uses, rebuild MOOS-IvP if the branch changes compiled
C++ code, then run the normal tests.

```bash
cd /path/to/moos-ivp
git checkout my-branch
./build-moos.sh --minrobot --release
./build-ivp.sh --nogui

cd /Users/charlesbenjamin/moos-ivp-cicd-testing
./build.sh

ctest --test-dir build -L geometry --output-on-failure

cd harnesses/waypoint_behavior_harnesses/H01-waypoint_behavior_motion
./zlaunch.sh --case=single_point_arrival_pass 10
```

If the branch only changes docs, scripts, or mission files, a MOOS-IvP rebuild
may not matter. If it changes apps, behaviors, or libraries, rebuild before
trusting the result.

If you prefer to test a different local `moos-ivp` checkout, configure this
repo with `cmake -S . -B build -DMOOSIVP_SOURCE_TREE_BASE=/path/to/moos-ivp`
and make sure that checkout's `bin/`, `scripts/`, and libraries are first in
your shell's `PATH` and library path before running harnesses.

For hosted validation, use the manual workflow input `moos_ivp_ref` with a
MOOS-IvP branch, tag, or commit SHA. The workflow clones that ref, builds it,
then runs the selected CTest families and harnesses.

## Repo Map

| Path | Purpose |
| --- | --- |
| `missions/` | Reusable MOOS mission stems. These define the base app/behavior scenario. |
| `harnesses/` | Patch-driven test matrices around mission stems. These own expected outcomes. |
| `tests/cpp/` | Fast GoogleTest/CTest library-level tests. These do not launch missions. |
| `scripts/` | CI helpers, harness metadata checks, repeatability sweeps, timing tools. |
| `config/harness_targets.json` | Manual-dispatch harness target metadata used by CI helper scripts. |
| `docs/` | Generated/static project documentation and harness pages. |
| `time_benchmarking/` | Local timing and parallelization notes. |
| `src/` | Legacy/example MOOS app code still built by the repository. |
