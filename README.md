# moos-ivp-cicd-testing

MOOS-IvP CI/CD workspace for mission-level tests.

## What Lives Here

- `missions/cmgr_tests` - contact-manager stem mission and its local README
- `harnesses/H01-cmgr_tests` - patch-driven 33-case matrix over the stem mission
- `missions/obmgr_tests` - obstacle-manager stem mission and its local README
- `harnesses/H02-obmgr_tests` - patch-driven 20-case obstacle-manager matrix over the stem mission
- `missions/first_draft` - obstacle-avoidance baseline mission
- `src/` - legacy template code still built by the repository

## Build

```bash
./build.sh
```

## Where To Look Next

- [missions/cmgr_tests/README.md](./missions/cmgr_tests/README.md) for the stem mission details
- [harnesses/H01-cmgr_tests/README.md](./harnesses/H01-cmgr_tests/README.md) for the test matrix
- [missions/obmgr_tests/README.md](./missions/obmgr_tests/README.md) for the obstacle-manager stem
- [harnesses/H02-obmgr_tests/README.md](./harnesses/H02-obmgr_tests/README.md) for the obstacle test matrix
- [missions/first_draft/README.md](./missions/first_draft/README.md) for the baseline mission
- [PARALLELIZATION_IDEA.md](./PARALLELIZATION_IDEA.md) for future harness scaling notes

## Notes

- The mission content and harnesses are the active focus of this repo.
- `cmgr_tests` now covers core detect/no-detect cases, CPA-driven alerting,
  runtime alert add/disable requests, report requests, alert filtering,
  report-only branches like `CONTACT_RANGES` and `CONTACT_CLOSEST_RELBNG`,
  early-warning flags, multi-contact selection, and both age-based and
  reject-range retirement.
- The full `H01-cmgr_tests` matrix currently has `33` cases and the latest
  full run passed at warp `10`.
- `obmgr_tests` is the obstacle-manager companion suite. It focuses on
  runtime alert requests, given-obstacle acceptance and rejection, point-cluster
  hull generation, lasso and placeholder hull branches, obstacle resolution,
  default general-alert naming, custom point variables, point-cluster trimming,
  and enable/disable/expunge message paths.
- The full `H02-obmgr_tests` matrix currently has `20` cases and the latest
  full run passed at warp `10`.
- The `src/` tree is kept only for legacy template compatibility and can be trimmed further if you no longer need those builds.
