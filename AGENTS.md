# AGENTS.md

This repo is a MOOS-IvP CI/CD workspace. Keep the rules below in mind when
editing missions, harnesses, and launcher scripts.

## Main Rules

- Prefer the nearest existing mission or harness exemplar before inventing a
  new pattern.
- Before broad repo exploration, consult `docs/context/dependency_tree.md` when
  present; regenerate it with `python3 scripts/generate_context_graph.py` after
  changing harness target metadata, harness launchers, mission stems, docs
  catalog wiring, or C++ test family metadata.
- When running harnesses, prefer batch or grouped validation over ad hoc
  one-off runs when that is faster and still representative.
- Do not run two scripts at the same time if their MOOSDB or pShare port
  ranges could overlap.
