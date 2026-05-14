# GitHub Actions

This repo has two active workflows:

- [Build And Check Active Harnesses](./.github/workflows/build_extend.yml)
  builds a selected MOOS-IvP ref, builds this repo against it, then runs the
  selected CTest families and/or runtime harnesses.
- [Publish GitHub Pages](./.github/workflows/pages.yml) publishes `docs/` to
  the project website.

## Build And Check Active Harnesses

The workflow is manual-only through `workflow_dispatch`.

Main inputs:

- `moos_ivp_ref`: MOOS-IvP branch, tag, or commit SHA to clone and test.
- `time_warp`: time warp passed to runtime harnesses that do not use a
  harness-specific CI profile.
- `dispatch_mode`: harness selection mode:
  - `none`
  - `full`
  - `family_run`
  - `batch_family_run`
  - `specific_harnesses`
- `family`: one harness family for `family_run`.
- `families`: comma-separated harness families for `batch_family_run`.
- `targets`: comma-separated harness target keys for `specific_harnesses`.
- `cpp_test_mode`: CTest selection mode:
  - `none`
  - `all`
  - `family_run`
  - `batch_family_run`
- `cpp_test_family`: one CTest family for `family_run`.
- `cpp_test_families`: comma-separated CTest families for `batch_family_run`.

At least one harness workload or CTest workload must be selected.

## Job Flow

`prepare` validates workflow/script metadata, checks repo invariants, and builds
the selected harness and CTest matrices.

`build` clones `https://github.com/moos-ivp/moos-ivp.git`, resolves
`moos_ivp_ref`, builds MOOS-IvP with:

```bash
./build-moos.sh --minrobot --release
./build-ivp.sh --nogui
```

It then configures this repo with:

```bash
cmake -DMOOSIVP_SOURCE_TREE_BASE="${GITHUB_WORKSPACE}/moos-ivp" ..
```

The build job uploads a short-lived `workspace-build` artifact containing the
built MOOS-IvP runtime, scripts, libraries, and this repo's `build/`, `bin/`,
and `lib/` outputs.

`cpp-unit-tests` downloads that artifact, puts the built tools on `PATH`, sets
the library paths, and runs the selected CTest families through
`scripts/ci_cpp_test_targets.py`.

`runtime-harnesses` downloads the same artifact, sets the same runtime
environment, then runs each selected harness from its configured directory.
Performance harnesses with a configured CI profile run through `PERF_PROFILE`;
other harnesses receive the manual `time_warp` input.

## Common Manual Runs

Run all C++ tests against MOOS-IvP `main`, with no runtime harnesses:

```text
dispatch_mode = none
cpp_test_mode = all
moos_ivp_ref = main
```

Run one CTest family against a MOOS-IvP branch:

```text
dispatch_mode = none
cpp_test_mode = family_run
cpp_test_family = geometry
moos_ivp_ref = my-moos-ivp-branch
```

Run one harness family:

```text
dispatch_mode = family_run
family = waypoint_behavior
cpp_test_mode = none
moos_ivp_ref = main
```

Run specific harness targets:

```text
dispatch_mode = specific_harnesses
targets = colregs_h01,colregs_h03
cpp_test_mode = none
moos_ivp_ref = main
```

List valid local harness keys with:

```bash
python3 scripts/list_harness_targets.py list
```

## Local Equivalent

For the common local case, check out the branch in the `moos-ivp` tree this
repo already uses, rebuild MOOS-IvP, rebuild this repo, and run the normal
commands:

```bash
cd /path/to/moos-ivp
git checkout my-moos-ivp-branch
./build-moos.sh --minrobot --release
./build-ivp.sh --nogui

cd /Users/charlesbenjamin/moos-ivp-cicd-testing
./build.sh

ctest --test-dir build -L geometry --output-on-failure
```

When testing a different local checkout, configure this repo with
`MOOSIVP_SOURCE_TREE_BASE=/path/to/moos-ivp` and put that checkout's `bin/`,
`scripts/`, and libraries first in the runtime environment before running
harnesses.
