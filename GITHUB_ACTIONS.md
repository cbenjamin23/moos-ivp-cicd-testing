# GitHub Actions

This repo has three active workflows:

- [Build And Check Active Harnesses](./.github/workflows/build_extend.yml)
  builds a selected MOOS-IvP ref, builds this repo against it, then runs the
  selected CTest families and/or runtime harnesses.
- [Build MOOS-IvP Portability Matrix](./.github/workflows/moos_ivp_build_matrix.yml)
  builds a selected MOOS-IvP ref across standalone OS/compiler targets without
  running CTests or runtime harnesses.
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

## Build MOOS-IvP Portability Matrix

This workflow is manual-only and intentionally independent from the harness and
CTest workflow. It answers whether a requested MOOS-IvP ref compiles on the
configured OS/compiler targets.

Main inputs:

- `moos_ivp_repo`: MOOS-IvP git repository URL to clone. Defaults to
  `https://github.com/moos-ivp/moos-ivp.git`; set this to a fork URL to test a
  branch from a developer fork.
- `moos_ivp_ref`: MOOS-IvP branch, tag, or commit SHA to clone and build.
- `build_profile`: `full` builds upstream's general/full MOOS-IvP target,
  including GUI apps; `headless` builds the smaller nogui/minrobot profile used
  by CI harness work.
- `target_set`: target family from `config/moos_ivp_build_targets.json`, such
  as `modern`, `current`, `compat`, `gcc`, `clang`, `ubuntu`, `debian`,
  `rocky`, `all`, or `specific_targets`.
- `targets`: comma-separated target keys used only with
  `target_set = specific_targets`.

The workflow clones `moos_ivp_repo` inside each target container and checks out
`moos_ivp_ref`. For the default `full` profile it runs:

```bash
./build.sh -mx
```

For the optional `headless` profile it runs:

```bash
./build-moos.sh --minrobot --release
./build-ivp.sh --nogui
```

It uploads the build log for each target. It does not build this extension repo,
and does not run CTests or missions.

Example fork-branch run:

```text
moos_ivp_repo = https://github.com/myname/moos-ivp.git
moos_ivp_ref = my-feature-branch
build_profile = full
target_set = modern
```

Targets declare which build profiles they support. Family selections skip targets
that do not support the requested profile; explicit `specific_targets` requests
fail clearly when a requested target does not support the selected profile. Rocky
9 is retained for `headless` compatibility coverage, but not for `full`, because
the standard Rocky 9 CRB/EPEL package set does not provide FLTK Fluid.

The current configured target generations are:

- `current`: `ubuntu_2604_gcc`, `ubuntu_2604_clang`, `debian_13_gcc`,
  `rocky_10_gcc`
- `compat`: `ubuntu_2404_gcc`, `ubuntu_2404_clang`, `debian_12_gcc`,
  `rocky_9_gcc`

List configured build targets locally with:

```bash
python3 scripts/moos_ivp_build_matrix.py list
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
