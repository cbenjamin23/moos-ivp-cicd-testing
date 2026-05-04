# moos-ivp-cicd-testing

CI/CD test workspace for MOOS-IvP. This repo contains two complementary test
layers:

- mission harnesses that launch MOOS-IvP apps and behaviors in controlled
  scenarios
- C++ tests that exercise library-level tools without launching missions

Use this repo to validate a MOOS-IvP checkout, reproduce app/behavior
regressions, and grow repeatable coverage around important upstream tools.

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

## Test Layers

### C++ Tests

Use `tests/cpp` for pure C++ contracts: geometry, math, parsers, format
utilities, IvP tools, contact records, log utilities, and app helper classes.

These tests are registered with CTest labels, so focused runs are cheap:

```bash
ctest --test-dir build -L XYArc --output-on-failure
ctest --test-dir build -L pMarineViewer --output-on-failure
```

Prefer GoogleTest discovery for C++ classes and internal logic because each
`TEST()` becomes its own CTest/JUnit case in CI. For command-line tool behavior,
use one narrow CTest case per fixture command rather than one broad runner, and
make the command print the exact fixture failure. That preserves useful GitHub
Actions summaries while still allowing black-box checks where they are the right
test shape.

Start here: [tests/cpp/README.md](./tests/cpp/README.md).

### Mission Harnesses

Use mission harnesses when behavior depends on live MOOS process interaction:
MOOSDB mail, helm behavior lifecycle, app postings, vehicle motion, or
multi-process timing.

Each harness normally contains:

- a `zlaunch.sh` runner
- one or more patch files, usually `.xmoos` or `.xbhv`
- a `README.md` describing the matrix
- a generated `results.txt` from the most recent local run

Common harness options:

```bash
./zlaunch.sh --help
./zlaunch.sh --case=<case_name> 10
./zlaunch.sh --jobs=4 --port_base=15000 10
./zlaunch.sh --just_make 10
```

## Coverage Areas

The active harness families cover:

| Family | Focus |
| --- | --- |
| `cmgr` | `pContactMgrV20` app-level contact alerting and contact-driven motion. |
| `obmgr` | `uFldObstacleMgr` app-level obstacle ingestion, alerts, and motion outcomes. |
| `pid` | `pMarinePIDV22` controller configuration, mail handling, and closed-loop motion. |
| `colregs` | `BHV_AvdColregsV22` classification, thresholds, execution, and parameter regressions. |
| behavior families | Moving correctness layers for waypoint, loiter, station-keep, trail, convoy, shadow, fixed-turn, cut-range, avoid-collision, avoid-obstacle, and OpRegion. |
| `performance` | Longer deterministic scenario tests for obstacle and COLREGS performance behavior. |

List configured CI harness targets:

```bash
python3 scripts/list_harness_targets.py list
```

Validate harness target metadata:

```bash
python3 scripts/list_harness_targets.py validate
python3 scripts/check_repo_invariants.py
```

## CI

The main workflow is
[Build And Check Active Harnesses](./.github/workflows/build_extend.yml). It
can run:

- the full active harness set
- one harness family
- several harness families
- specific harness keys from `config/harness_targets.json`
- all C++ tests or one CTest label through `cpp_test_filter`

The workflow builds MOOS-IvP, builds this repo, runs invariant checks, runs the
selected C++ tests, and executes selected harnesses.

## Where To Start

- New to the C++ tests: [tests/cpp/README.md](./tests/cpp/README.md)
- New to harnesses: open a harness README, such as
  [H01-waypoint_behavior_motion](./harnesses/waypoint_behavior_harnesses/H01-waypoint_behavior_motion/README.md)
- Need the configured CI target list:
  [config/harness_targets.json](./config/harness_targets.json)
- Need timing guidance:
  [time_benchmarking/PARALLELIZATION_STATUS.md](./time_benchmarking/PARALLELIZATION_STATUS.md)
- Need script details: [scripts/README](./scripts/README)

Keep detailed case matrices, latest run notes, and family-specific behavior
contracts in the harness READMEs. Keep this top-level README focused on what
the repo is, how to run it, and where to go next.
