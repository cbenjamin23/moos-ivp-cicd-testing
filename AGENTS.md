# AGENTS.md

This repo is a MOOS-IvP CI/CD workspace. Keep the rules below in mind when
editing missions, harnesses, and launcher scripts.

## Main Rules

- Prefer the nearest existing mission or harness exemplar before inventing a
  new pattern.
- When running harnesses, prefer batch or grouped validation over ad hoc
  one-off runs when that is faster and still representative.
- Do not run two scripts at the same time if their MOOSDB or pShare port
  ranges could overlap.
- Harness launchers intended for grouped local or CI validation should expose
  caller-controlled `--jobs` and `--port_base` options so runs can be isolated
  safely.
