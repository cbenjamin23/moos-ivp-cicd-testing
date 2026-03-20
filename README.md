# moos-ivp-cicd-testing

MOOS-IvP CI/CD workspace for mission-level tests.

## What Lives Here

- `missions/cmgr_missions/cmgr_unit` - contact-manager unit-style stem mission
- `harnesses/cmgr_harnesses/H01-cmgr_unit` - patch-driven contact-manager unit matrix
- `missions/cmgr_missions/cmgr_motion` - moving contact-avoidance integration stem
- `harnesses/cmgr_harnesses/H02-cmgr_motion` - moving contact integration matrix
- `missions/obmgr_missions/obmgr_unit` - obstacle-manager unit-style stem mission
- `harnesses/obmgr_harnesses/H01-obmgr_unit` - patch-driven obstacle-manager unit matrix
- `missions/obmgr_missions/obmgr_motion` - moving obstacle-avoidance integration stem
- `harnesses/obmgr_harnesses/H02-obmgr_motion` - moving obstacle integration matrix
- `missions/collision_behavior_missions/collision_behavior_motion` - moving avoid-collision behavior stem
- `harnesses/collision_behavior_harnesses/H01-collision_behavior_motion` - moving avoid-collision behavior matrix
- `missions/obstacle_behavior_missions/obstacle_behavior_motion` - moving avoid-obstacle behavior stem
- `harnesses/obstacle_behavior_harnesses/H01-obstacle_behavior_motion` - moving avoid-obstacle behavior matrix
- `missions/first_draft` - older obstacle-avoidance baseline mission kept for reference
- `src/` - legacy template code still built by the repository

## Build

```bash
./build.sh
```

## Where To Look Next

- [missions/cmgr_missions/cmgr_unit/README.md](./missions/cmgr_missions/cmgr_unit/README.md) for the contact-manager unit stem
- [harnesses/cmgr_harnesses/H01-cmgr_unit/README.md](./harnesses/cmgr_harnesses/H01-cmgr_unit/README.md) for the contact-manager unit matrix
- [missions/cmgr_missions/cmgr_motion/README.md](./missions/cmgr_missions/cmgr_motion/README.md) for the moving contact stem
- [harnesses/cmgr_harnesses/H02-cmgr_motion/README.md](./harnesses/cmgr_harnesses/H02-cmgr_motion/README.md) for the moving contact matrix
- [missions/obmgr_missions/obmgr_unit/README.md](./missions/obmgr_missions/obmgr_unit/README.md) for the obstacle-manager unit stem
- [harnesses/obmgr_harnesses/H01-obmgr_unit/README.md](./harnesses/obmgr_harnesses/H01-obmgr_unit/README.md) for the obstacle-manager unit matrix
- [missions/obmgr_missions/obmgr_motion/README.md](./missions/obmgr_missions/obmgr_motion/README.md) for the moving obstacle-avoidance stem
- [harnesses/obmgr_harnesses/H02-obmgr_motion/README.md](./harnesses/obmgr_harnesses/H02-obmgr_motion/README.md) for the moving obstacle integration matrix
- [missions/collision_behavior_missions/collision_behavior_motion/README.md](./missions/collision_behavior_missions/collision_behavior_motion/README.md) for the avoid-collision behavior stem
- [harnesses/collision_behavior_harnesses/H01-collision_behavior_motion/README.md](./harnesses/collision_behavior_harnesses/H01-collision_behavior_motion/README.md) for the avoid-collision behavior matrix
- [missions/obstacle_behavior_missions/obstacle_behavior_motion/README.md](./missions/obstacle_behavior_missions/obstacle_behavior_motion/README.md) for the avoid-obstacle behavior stem
- [harnesses/obstacle_behavior_harnesses/H01-obstacle_behavior_motion/README.md](./harnesses/obstacle_behavior_harnesses/H01-obstacle_behavior_motion/README.md) for the avoid-obstacle behavior matrix
- [missions/first_draft/README.md](./missions/first_draft/README.md) for the baseline mission
- [PARALLELIZATION_IDEA.md](./PARALLELIZATION_IDEA.md) for future harness scaling notes

## Notes

- The mission content and harnesses are the active focus of this repo.
- `cmgr_unit` now covers core detect/no-detect cases, CPA-driven alerting,
  runtime alert add/disable requests, report requests, alert filtering,
  report-only branches like `CONTACT_RANGES` and `CONTACT_CLOSEST_RELBNG`,
  early-warning flags, multi-contact selection, and both age-based and
  reject-range retirement.
- The full `H01-cmgr_unit` matrix currently has `33` cases and the latest
  full run passed at warp `10`.
- `cmgr_motion` is the moving integration companion to `cmgr_unit`. It uses
  one ownship, a short transit corridor, one spoofed contact at a time, and
  mission-level grading based on safe arrival plus encounter outcome rather
  than on contact-manager outputs alone.
- The full `H02-cmgr_motion` matrix currently has `9` cases and the latest
  full run passed at warp `10` in about `97` seconds wall clock.
- `obmgr_unit` is the obstacle-manager companion suite. It focuses on
  runtime alert requests, given-obstacle acceptance and rejection, point-cluster
  hull generation, lasso and placeholder hull branches, obstacle resolution,
  default general-alert naming, custom point variables, point-cluster trimming,
  and enable/disable/expunge message paths.
- The full `H01-obmgr_unit` matrix currently has `20` cases and the latest
  full run passed at warp `10`.
- `obmgr_motion` is the moving integration companion to the app-level
  `obmgr_unit` suite. It keeps one vehicle, one short transit corridor, and a
  fixed obstacle topology so obstacle-avoidance outcomes can be patched and
  graded deterministically.
- The full `H02-obmgr_motion` matrix currently has `6` cases and the latest
  full run passed at warp `10`.
- `collision_behavior_motion` is the downstream moving correctness layer for
  `BHV_AvoidCollision`. It keeps the synthetic-contact setup deterministic, but
  grades on behavior-owned lifecycle and mission outcome rather than on raw
  contact-manager outputs.
- The full `H01-collision_behavior_motion` matrix currently has `6` cases and
  the latest full run passed at warp `10` in about `65` seconds wall clock.
- `obstacle_behavior_motion` is the downstream moving correctness layer for
  `BHV_AvoidObstacleV24`. It keeps obstacle-manager input deterministic, but
  grades on behavior engagement, completion, and clean transit.
- The full `H01-obstacle_behavior_motion` matrix currently has `7` cases and
  the latest full run passed at warp `10` in about `93` seconds wall clock.
- The `src/` tree is kept only for legacy template compatibility and can be trimmed further if you no longer need those builds.
