# moos-ivp-cicd-testing

CI/CD test workspace for MOOS-IvP. This repo contains two complementary test
layers:

- mission harnesses that launch MOOS-IvP apps and behaviors in controlled
  scenarios
- C++ tests that exercise library-level tools without launching missions

Use this repo to validate a MOOS-IvP checkout, reproduce app/behavior
regressions, and grow repeatable coverage around important upstream tools.

Website: [MOOS-IvP Harnesses](https://cbenjamin23.github.io/moos-ivp-cicd-testing/)

Developer guide: [Quick Start](https://cbenjamin23.github.io/moos-ivp-cicd-testing/quick-start.html)

## Quick Start

Check out the MOOS-IvP branch containing your edits, then build MOOS-IvP and
this test repository:

```bash
cd /path/to/moos-ivp
git switch my-branch
./build.sh

cd /path/to/moos-ivp-cicd-testing
./build.sh
```

From the test repository root, run the focused pEchoVar CTest family:

```bash
cd tests/cpp
./ctests.sh --pechovar
```

Run `./ctests.sh` without a family option to test everything, or use `--help`
to list all family options. The first focused run may configure and build the
selected CTest executables before testing; later runs rebuild incrementally.

From the test repository root, run the full pEchoVar mission harness:

```bash
cd harnesses/pechovar_harnesses/H01-pechovar_unit
./zlaunch.sh 10
```

CTest passes with `100% tests passed`. The harness passes with `failures=0`;
open its `results.txt` for per-case details.

Do not run two harnesses at the same time unless their `--port_base` ranges are
isolated.

## Optional Build Matrix

To run the normal build on this computer and compile the same local MOOS-IvP
checkout in the current Ubuntu, Debian, and Rocky Linux environments, run:

```bash
./build-matrix.sh
```

The Linux builds are Docker compile checks, not additional CTest or harness
runs. Use `./build-matrix.sh --help` for native-only testing, focused Linux
targets, compatibility targets, and the smaller headless build profile.

## Using a Different MOOS-IvP Checkout

If you prefer to test a different local `moos-ivp` checkout, configure this
repo with `cmake -S . -B build -DMOOSIVP_SOURCE_TREE_BASE=/path/to/moos-ivp`
and make sure that checkout's `bin/`, `scripts/`, and libraries are first in
your shell's `PATH` and library path before running harnesses.

## Repo Map

| Path | Purpose |
| --- | --- |
| `missions/` | Reusable MOOS mission stems. These define the base app/behavior scenario. |
| `harnesses/` | Patch-driven test matrices around mission stems. These own expected outcomes. |
| `tests/cpp/` | Fast GoogleTest/CTest library-level tests. These do not launch missions. |
| `scripts/` | Local and CI helpers, harness metadata checks, repeatability sweeps, timing tools. |
| `config/harness_targets.json` | Manual-dispatch harness target metadata used by CI helper scripts. |
| `docs/` | Generated/static project documentation and harness pages. |
| `time_benchmarking/` | Local timing and parallelization notes. |
| `src/` | Legacy/example MOOS app code still built by the repository. |
