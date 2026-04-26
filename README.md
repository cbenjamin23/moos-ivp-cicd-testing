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
- `missions/colregs_missions/colregs_unit` - shared two-vessel COLREGS correctness stem
- `harnesses/colregs_harnesses/H01-colregs_classification` - canonical COLREGS classification matrix
- `harnesses/colregs_harnesses/H02-colregs_thresholds` - COLREGS threshold-boundary matrix
- `harnesses/colregs_harnesses/H03-colregs_execution` - COLREGS execution-quality matrix
- `harnesses/colregs_harnesses/H04-colregs_parameters` - COLREGS parameter-regression matrix
- `missions/collision_behavior_missions/collision_behavior_motion` - moving avoid-collision behavior stem
- `harnesses/collision_behavior_harnesses/H01-collision_behavior_motion` - moving avoid-collision behavior matrix
- `missions/obstacle_behavior_missions/obstacle_behavior_motion` - moving avoid-obstacle behavior stem
- `harnesses/obstacle_behavior_harnesses/H01-obstacle_behavior_motion` - moving avoid-obstacle behavior matrix
- `missions/opregion_missions/opregion_motion` - moving OpRegionV24 safety-envelope stem
- `harnesses/opregion_harnesses/H01-opregion_safety` - OpRegionV24 safety-envelope matrix
- `missions/waypoint_behavior_missions/waypoint_behavior_motion` - moving waypoint behavior stem
- `harnesses/waypoint_behavior_harnesses/H01-waypoint_behavior_motion` - waypoint behavior motion matrix
- `missions/loiter_behavior_missions/loiter_behavior_motion` - moving loiter behavior stem
- `harnesses/loiter_behavior_harnesses/H01-loiter_behavior_motion` - loiter behavior motion matrix
- `missions/stationkeep_behavior_missions/stationkeep_behavior_motion` - moving station-keep behavior stem
- `harnesses/stationkeep_behavior_harnesses/H01-stationkeep_behavior_motion` - station-keep behavior motion matrix
- `missions/trail_behavior_missions/trail_behavior_motion` - moving trail behavior stem
- `harnesses/trail_behavior_harnesses/H01-trail_behavior_motion` - trail behavior motion matrix
- `missions/convoy_behavior_missions/convoy_behavior_motion` - moving convoy behavior stem
- `harnesses/convoy_behavior_harnesses/H01-convoy_behavior_motion` - convoy behavior motion matrix
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
- [missions/colregs_missions/colregs_unit/README.md](./missions/colregs_missions/colregs_unit/README.md) for the shared COLREGS unit stem
- [harnesses/colregs_harnesses/H01-colregs_classification/README.md](./harnesses/colregs_harnesses/H01-colregs_classification/README.md) for the canonical COLREGS classification matrix
- [harnesses/colregs_harnesses/H02-colregs_thresholds/README.md](./harnesses/colregs_harnesses/H02-colregs_thresholds/README.md) for the COLREGS threshold-boundary matrix
- [harnesses/colregs_harnesses/H03-colregs_execution/README.md](./harnesses/colregs_harnesses/H03-colregs_execution/README.md) for the COLREGS execution-quality matrix
- [harnesses/colregs_harnesses/H04-colregs_parameters/README.md](./harnesses/colregs_harnesses/H04-colregs_parameters/README.md) for the COLREGS parameter-regression matrix
- [missions/collision_behavior_missions/collision_behavior_motion/README.md](./missions/collision_behavior_missions/collision_behavior_motion/README.md) for the avoid-collision behavior stem
- [harnesses/collision_behavior_harnesses/H01-collision_behavior_motion/README.md](./harnesses/collision_behavior_harnesses/H01-collision_behavior_motion/README.md) for the avoid-collision behavior matrix
- [missions/obstacle_behavior_missions/obstacle_behavior_motion/README.md](./missions/obstacle_behavior_missions/obstacle_behavior_motion/README.md) for the avoid-obstacle behavior stem
- [harnesses/obstacle_behavior_harnesses/H01-obstacle_behavior_motion/README.md](./harnesses/obstacle_behavior_harnesses/H01-obstacle_behavior_motion/README.md) for the avoid-obstacle behavior matrix
- [missions/opregion_missions/opregion_motion/README.md](./missions/opregion_missions/opregion_motion/README.md) for the OpRegionV24 safety-envelope stem
- [harnesses/opregion_harnesses/H01-opregion_safety/README.md](./harnesses/opregion_harnesses/H01-opregion_safety/README.md) for the OpRegionV24 safety-envelope matrix
- [missions/waypoint_behavior_missions/waypoint_behavior_motion/README.md](./missions/waypoint_behavior_missions/waypoint_behavior_motion/README.md) for the waypoint behavior stem
- [harnesses/waypoint_behavior_harnesses/H01-waypoint_behavior_motion/README.md](./harnesses/waypoint_behavior_harnesses/H01-waypoint_behavior_motion/README.md) for the waypoint behavior matrix
- [missions/loiter_behavior_missions/loiter_behavior_motion/README.md](./missions/loiter_behavior_missions/loiter_behavior_motion/README.md) for the loiter behavior stem
- [harnesses/loiter_behavior_harnesses/H01-loiter_behavior_motion/README.md](./harnesses/loiter_behavior_harnesses/H01-loiter_behavior_motion/README.md) for the loiter behavior matrix
- [missions/stationkeep_behavior_missions/stationkeep_behavior_motion/README.md](./missions/stationkeep_behavior_missions/stationkeep_behavior_motion/README.md) for the station-keep behavior stem
- [harnesses/stationkeep_behavior_harnesses/H01-stationkeep_behavior_motion/README.md](./harnesses/stationkeep_behavior_harnesses/H01-stationkeep_behavior_motion/README.md) for the station-keep behavior matrix
- [missions/trail_behavior_missions/trail_behavior_motion/README.md](./missions/trail_behavior_missions/trail_behavior_motion/README.md) for the trail behavior stem
- [harnesses/trail_behavior_harnesses/H01-trail_behavior_motion/README.md](./harnesses/trail_behavior_harnesses/H01-trail_behavior_motion/README.md) for the trail behavior matrix
- [missions/convoy_behavior_missions/convoy_behavior_motion/README.md](./missions/convoy_behavior_missions/convoy_behavior_motion/README.md) for the convoy behavior stem
- [harnesses/convoy_behavior_harnesses/H01-convoy_behavior_motion/README.md](./harnesses/convoy_behavior_harnesses/H01-convoy_behavior_motion/README.md) for the convoy behavior matrix
- [missions/first_draft/README.md](./missions/first_draft/README.md) for the baseline mission
- [time_benchmarking/PARALLELIZATION_STATUS.md](./time_benchmarking/PARALLELIZATION_STATUS.md) for the current harness parallelization model
- [time_benchmarking/parallel_jobs_2026-03-20/README.md](./time_benchmarking/parallel_jobs_2026-03-20/README.md) for the current timing sweep and recommended `--jobs` values
- [scripts/benchmark_parallel.sh](./scripts/benchmark_parallel.sh) for repeatable harness timing sweeps

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
- `colregs_unit` is the shared two-vessel correctness stem for the COLREGS
  family. It underpins the canonical classification, threshold, execution, and
  parameter-regression harnesses for `BHV_AvdColregsV22`.
- The full `H01-colregs_classification` matrix currently has `22` stable
  canonical cases and is the first implemented COLREGS correctness suite.
- `H03-colregs_execution` is the downstream execution-quality layer for the
  same family. It currently has `23` default cases and most recently cleared a
  `5/5` repeatability signoff on April 11, 2026.
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
- `opregion_motion` is the moving safety-envelope layer for `BHV_OpRegionV24`.
  It keeps one ownship and one short transit corridor, then grades on
  behavior-owned save, halt, and time-limit flags.
- The `H01-opregion_safety` matrix has `15` cases covering inside-region
  transit, save recovery, generated save buffers, halt breach, entry gating,
  entry/exit debounce timing, reset behavior, max-time failure, dynamic-region
  expansion/recovery, and dynamic-region halt failure. The latest full run
  passed at warp `10` after tightening most cases to event-driven evaluation
  gates, and the full matrix also passes in wave mode with `--jobs=4`.
- `waypoint_behavior_motion` is the moving correctness layer for
  `BHV_Waypoint`. It keeps one ownship and grades on behavior-owned completion,
  waypoint-hit, cycle, end-flag, and update handling signals.
- The `H01-waypoint_behavior_motion` matrix has `27` cases covering single and
  multi-point transit, waypoint/end flags, repeat/cycle handling, runtime
  `points`, `newpt`, and `xpoints` updates, capture radius, absolute capture
  line, slip radius, route ordering, nonzero current index, lead-to-start, lead
  conditions, end-speed tapering, alternate speed, reset-on-idle,
  `wptflag_on_start`, custom status/index/distance variables, polygon input,
  slow-speed non-arrival, no-point timeout, malformed update failure, and bad
  `xpoints` rejection. The latest full run passed at warp `10`, and the full
  matrix also passes in wave mode with `--jobs=4`.
- `loiter_behavior_motion` is the moving correctness layer for `BHV_Loiter`.
  It keeps one ownship near a compact loiter polygon and grades on behavior
  outputs such as index, mode, acquisition state, polygon distance, ETA,
  bearing accumulation, and clean behavior state.
- The `H01-loiter_behavior_motion` matrix has `30` cases covering radial and
  explicit polygon geometry, clockwise variants, start-inside and far-acquire
  paths, early acquisition, `center_activate`, capture/slip radii, static and
  runtime alternate speed, center assignment/update paths, polygon radius
  updates, slingshot bearing accumulation, suffixed outputs, ETA/report output,
  ZAIC speed, slow-speed acquisition, malformed/empty polygon failures, bad
  update failure, update recovery, spiral factor, and patience variants. The
  latest full wave run passed at warp `10` with `--jobs=4`.
- `stationkeep_behavior_motion` is the moving correctness layer for
  `BHV_StationKeep`. It keeps one ownship near a compact station point and
  grades on behavior-owned distance, mode, settings, warning, and error
  outputs.
- The `H01-stationkeep_behavior_motion` matrix has `32` cases covering default
  station arrival, `station_pt` aliasing, start-inside and center-activate
  paths, swing behavior, wide/tight/inner-greater-than-outer radius settings,
  transit and outer-speed variants including aliases and runtime updates,
  hibernation seek/settle/off paths including runtime hibernation-radius
  updates, runtime point/radius/speed updates, malformed-update recovery,
  visual hints, and missing or malformed point/radius/speed/swing-time
  failures. The latest full wave run passed at warp `10` with `--jobs=4`.
- `trail_behavior_motion` is the moving correctness layer for `BHV_Trail`. It
  keeps one chaser and one target vehicle, with the chaser graded on trail
  distance, relevance, pursuit state, region branch, warning, and error
  outputs.
- The `H01-trail_behavior_motion` matrix has `35` cases covering default
  trailing, absolute and relative trail angles, short/long trail ranges,
  tight/wide radius settings, inside/outside `nm_radius` branches,
  active/inactive `pwt_outer_dist`, static and runtime trail-range changes,
  runtime trail-angle and relevance updates, runtime modifier updates,
  malformed runtime update recovery, idle distance posting, `no_extrapolate`,
  `no_alert_request`, warning-only missing contact handling, fatal missing
  contact handling, and malformed angle/range/radius/pwt/decay/time settings.
  The latest full wave run passed at warp `10` with `--jobs=8`.
- `convoy_behavior_motion` is the moving correctness layer for `BHV_Convoy`.
  It keeps one chaser and one target vehicle, then grades on the convoy
  breadcrumb queue, mark range, contact speed averages, physical chaser/target
  speed, warning, and error outputs.
- The `H01-convoy_behavior_motion` matrix has `41` cases covering default
  convoy following, fine/coarse mark spacing, short/long mark queues,
  tight/wide radius settings, cruise-speed and speed-cap branches, range-safety
  auto-adjust and disabled-safety branches, explicit tailgating/lagging/estop
  speed branches, range aliases, zero `nm_radius`, visual breadcrumb posting,
  angled/cross-track/opposite-heading/close-offset entry geometry, slow/fast
  follower regimes, runtime mark-spacing/max-range/cruise-speed updates,
  malformed update recovery, `no_extrapolate`, warning-only missing contact
  handling, fatal missing contact handling, and malformed mark/radius/speed/range
  settings. The latest full wave run passed at warp `10` with `--jobs=8`.
- The `src/` tree is kept only for legacy template compatibility and can be trimmed further if you no longer need those builds.
