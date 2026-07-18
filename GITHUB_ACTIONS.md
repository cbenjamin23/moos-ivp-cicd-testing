# GitHub Actions

This repository has one GitHub Actions workflow:

- [Publish GitHub Pages](./.github/workflows/pages.yml) publishes `docs/` to
  the project website.

MOOS-IvP development testing is intentionally local. This lets CTests,
harnesses, and build-matrix checks use the same checkout, including edits that
have not been committed or pushed.

## Local Validation

Build the selected MOOS-IvP checkout and this test repository, then run the
relevant CTest family and harness as described in the
[Quick Start](./docs/quick-start.html).

Run the complete build matrix with:

```bash
./build-matrix.sh
```

This performs the normal native build on the host first, then copies the same
local source into clean Linux containers for the selected distribution and
compiler targets. Run `./build-matrix.sh --help` for focused targets, the
compatibility set, the headless profile, or native-only testing.

Build logs are written under the ignored `build_matrix_results/` directory.
