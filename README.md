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
- `missions/cutrange_behavior_missions/cutrange_behavior_motion` - moving cut-range behavior stem
- `harnesses/cutrange_behavior_harnesses/H01-cutrange_behavior_motion` - cut-range behavior motion matrix
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
- `missions/shadow_behavior_missions/shadow_behavior_motion` - moving shadow behavior stem
- `harnesses/shadow_behavior_harnesses/H01-shadow_behavior_motion` - shadow behavior motion matrix
- `missions/fixedturn_behavior_missions/fixedturn_behavior_motion` - moving fixed-turn behavior stem
- `harnesses/fixedturn_behavior_harnesses/H01-fixedturn_behavior_motion` - fixed-turn behavior motion matrix
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
- [missions/cutrange_behavior_missions/cutrange_behavior_motion/README.md](./missions/cutrange_behavior_missions/cutrange_behavior_motion/README.md) for the cut-range behavior stem
- [harnesses/cutrange_behavior_harnesses/H01-cutrange_behavior_motion/README.md](./harnesses/cutrange_behavior_harnesses/H01-cutrange_behavior_motion/README.md) for the cut-range behavior matrix
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
- [missions/shadow_behavior_missions/shadow_behavior_motion/README.md](./missions/shadow_behavior_missions/shadow_behavior_motion/README.md) for the shadow behavior stem
- [harnesses/shadow_behavior_harnesses/H01-shadow_behavior_motion/README.md](./harnesses/shadow_behavior_harnesses/H01-shadow_behavior_motion/README.md) for the shadow behavior matrix
- [missions/fixedturn_behavior_missions/fixedturn_behavior_motion/README.md](./missions/fixedturn_behavior_missions/fixedturn_behavior_motion/README.md) for the fixed-turn behavior stem
- [harnesses/fixedturn_behavior_harnesses/H01-fixedturn_behavior_motion/README.md](./harnesses/fixedturn_behavior_harnesses/H01-fixedturn_behavior_motion/README.md) for the fixed-turn behavior matrix
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
  one ownship, a short transit corridor, and controlled spoofed contacts to
  prove contact-manager alerts can drive spawned collision avoidance. It now
  covers static and runtime alert creation, runtime alert disable/re-enable,
  helm-held alerts, alert filtering, stale-contact retirement/reappearance, and
  harsh no-avoid failure modes.
- The full `H02-cmgr_motion` matrix currently has `15` cases and the latest
  full grouped run passed at warp `10` with `--jobs=4 --port_base=15000`.
- `obmgr_unit` is the obstacle-manager companion suite. It focuses on
  runtime alert requests, given-obstacle acceptance and rejection, point-cluster
  hull generation, lasso and placeholder hull branches, obstacle resolution,
  config-time given-obstacle loading and aliasing, malformed input rejection,
  default general-alert naming, custom point variables, point-cluster trimming,
  view-polygon suppression, vsource payload preservation, new-obstacle macro
  expansion, and enable/disable/expunge message paths.
- The full `H01-obmgr_unit` matrix currently has `30` cases and the latest
  full grouped run passed at warp `10` with `--jobs=4 --port_base=15000`.
- `obmgr_motion` is the moving integration companion to the app-level
  `obmgr_unit` suite. It keeps one vehicle, one short transit corridor, and a
  patchable obstacle topology so obstacle-avoidance outcomes can be graded
  deterministically across configured polygons, runtime given obstacles,
  point-cluster hulls, lasso hulls, and alert-disabled failure modes.
- The full `H02-obmgr_motion` matrix currently has `10` cases and the latest
  full grouped run passed at warp `10` with `--jobs=4 --port_base=15000`.
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
- The full `H01-collision_behavior_motion` matrix currently has `21` cases
  covering baseline and head-on resolution, alert suppression, per-contact
  output posting, behavior/contact-type filters, alternate `pwt_grade`
  branches, refinery and no-extrapolate branches, too-small relevance failure,
  and invalid distance/grade/decay/depth settings. The latest full wave run
  passed at warp `10` with `--jobs=4` and `--port_base=17000`.
- `obstacle_behavior_motion` is the downstream moving correctness layer for
  `BHV_AvoidObstacleV24`. It keeps obstacle-manager input deterministic, but
  grades on behavior engagement, completion, and clean transit.
- The full `H01-obstacle_behavior_motion` matrix currently has `21` cases
  covering auto-requested obstacle avoidance, range and CPA flag paths,
  off-lane non-engagement, refinery/no-refinery objective construction,
  multi-obstacle completion, launch-time `polygon`/`poly` configuration,
  no-threshold range flags, disabled avoidance, and invalid polygon,
  distance/CPA, flag, boolean, TTC, and unsupported `pwt_grade` settings. The
  latest full wave run passed at warp `10` with `--jobs=4` and
  `--port_base=18000`.
- `cutrange_behavior_motion` is the moving correctness layer for
  `BHV_CutRange`. It uses two simulated vehicles so the chaser behavior can be
  graded against target geometry, closest-range telemetry, pursuit/giveup flags,
  warning/error handling, and runtime updates.
- The full `H01-cutrange_behavior_motion` matrix currently has `39` cases
  covering startup, static/crossing/head-on/angled/dogleg/S-turn target
  geometry, slow/fast/stationary targets, time-on-leg and patience settings,
  relevance/giveup boundaries, runtime PWT/patience/giveup updates,
  no-extrapolate and missing-contact paths, and invalid distance/giveup/
  patience/time/decay/legacy utility settings. The latest full wave run passed
  at warp `10` with `--jobs=4` and `--port_base=25000`.
- `opregion_motion` is the moving safety-envelope layer for `BHV_OpRegionV24`.
  It keeps one ownship and one short transit corridor, then grades on
  behavior-owned save, halt, and time-limit flags.
- The `H01-opregion_safety` matrix has `25` cases covering inside-region
  transit, save recovery, generated save buffers, halt breach, entry gating,
  entry/exit debounce timing, reset behavior, max-time failure, max-depth and
  min-altitude runtime breaches, invalid scalar/boolean configuration
  malconfig failures, dynamic-region expansion/recovery, and dynamic-region
  halt failure. The latest full wave run passed at warp `10` with `--jobs=4`
  and `--port_base=31000`.
- `waypoint_behavior_motion` is the moving correctness layer for
  `BHV_Waypoint`. It keeps one ownship and grades on behavior-owned completion,
  waypoint-hit, cycle, end-flag, and update handling signals.
- The `H01-waypoint_behavior_motion` matrix has `43` cases covering single and
  multi-point transit, waypoint/end flags, repeat/cycle handling, runtime
  `points`, `newpt`, and `xpoints` updates, capture radius, absolute capture
  line, slip radius, route ordering, nonzero current index, lead-to-start, lead
  conditions, end-speed tapering, alternate speed, reset-on-idle,
  `wptflag_on_start`, custom status/index/distance variables, polygon input,
  alternate `ipf_type` branches, greedy tour ordering, cycle-index and
  efficiency outputs, slow-speed non-arrival, no-point timeout, malformed
  update failure, bad `xpoints` rejection, and invalid speed/capture-line/
  capture-radius/slip-radius/order/repeat/lead-condition/lead-damper/end-speed/
  patience/efficiency configuration malconfig failures. The latest full wave
  run passed at warp `10` with `--jobs=4` and `--port_base=15000`.
- `loiter_behavior_motion` is the moving correctness layer for `BHV_Loiter`.
  It keeps one ownship near a compact loiter polygon and grades on behavior
  outputs such as index, mode, acquisition state, polygon distance, ETA,
  bearing accumulation, and clean behavior state.
- The `H01-loiter_behavior_motion` matrix has `34` cases covering radial and
  explicit polygon geometry, clockwise variants, start-inside and far-acquire
  paths, early acquisition, `center_activate`, capture/slip radii, static and
  runtime alternate speed, center assignment/update paths, polygon radius
  updates, slingshot bearing accumulation, suffixed outputs, ETA/report output,
  ZAIC speed, slow-speed acquisition, malformed/empty polygon failures, bad
  update failure, update recovery, spiral factor, patience variants, and invalid
  clockwise/use-alt-speed/patience/capture-radius configuration failures. The
  latest full wave run passed at warp `10` with `--jobs=4` and
  `--port_base=15000`.
- `stationkeep_behavior_motion` is the moving correctness layer for
  `BHV_StationKeep`. It keeps one ownship near a compact station point and
  grades on behavior-owned distance, mode, settings, warning, and error
  outputs.
- The `H01-stationkeep_behavior_motion` matrix has `34` cases covering default
  station arrival, `station_pt` aliasing, start-inside and center-activate
  paths, swing behavior, wide/tight/inner-greater-than-outer radius settings,
  transit and outer-speed variants including aliases and runtime updates,
  hibernation seek/settle/off paths including runtime hibernation-radius
  updates, runtime point/radius/speed updates, malformed-update recovery,
  visual hints, and missing or malformed point/radius/speed/swing-time
  failures, including invalid center-activate and extra-speed aliases. The
  latest full wave run passed at warp `10` with `--jobs=4` and
  `--port_base=15000`.
- `trail_behavior_motion` is the moving correctness layer for `BHV_Trail`. It
  keeps one chaser and one target vehicle, with the chaser graded on trail
  distance, relevance, pursuit state, region branch, warning, and error
  outputs.
- The `H01-trail_behavior_motion` matrix has `42` cases covering default
  trailing, absolute and relative trail angles, short/long trail ranges,
  tight/wide radius settings, inside/outside `nm_radius` branches,
  active/inactive `pwt_outer_dist`, static and runtime trail-range changes,
  lower-bound range clamp behavior, runtime trail-angle and relevance updates,
  runtime modifier updates, malformed runtime update recovery, idle distance
  posting and suppression, `no_extrapolate`, automatic and suppressed contact
  alert requests, warning-only missing contact handling, fatal missing contact
  handling, and malformed angle/range/radius/pwt/decay/time settings. The
  latest full wave run passed at warp `10` with `--jobs=4` and
  `--port_base=15000`.
- `convoy_behavior_motion` is the moving correctness layer for `BHV_Convoy`.
  It keeps one chaser and one target vehicle, then grades on the convoy
  breadcrumb queue, mark range, contact speed averages, physical chaser/target
  speed, warning, and error outputs.
- The `H01-convoy_behavior_motion` matrix has `48` cases covering default
  convoy following, fine/coarse mark spacing, short/long mark queues,
  tight/wide radius settings, cruise-speed and speed-cap branches, range-safety
  auto-adjust and disabled-safety branches, explicit tailgating/lagging/estop
  speed branches, range aliases, zero `nm_radius`, visual breadcrumb posting,
  angled/cross-track/opposite-heading/close-offset entry geometry, slow/fast
  follower regimes, runtime mark-spacing/max-range/cruise-speed updates,
  runtime estop zero-speed behavior, malformed update recovery, `no_extrapolate`,
  warning-only missing contact handling, fatal missing contact handling, and
  malformed mark/radius/speed/range settings. The latest full wave run passed at
  warp `10` with `--jobs=4` and `--port_base=15000`.
- `shadow_behavior_motion` is the moving correctness layer for `BHV_Shadow`.
  It keeps one chaser and one target vehicle, then grades on Shadow contact
  telemetry, relevance, chaser/target speed and heading, warning, and error
  outputs.
- The `H01-shadow_behavior_motion` matrix has `35` cases covering startup
  contact acquisition, cardinal and wraparound target headings, target turns,
  slow/fast/stationary contact speed regimes, relevance on/edge/off branches,
  runtime relevance updates, malformed-update recovery, missing-contact
  recovery, heading/speed width aliases, `no_extrapolate`, inherited contact
  flag macro posting, warning-only and fatal missing-contact paths, missing
  contact configuration, and invalid pwt/width/decay/boolean/time/contact-flag
  settings. The latest full wave run passed at warp `10` with `--jobs=2` and
  `--port_base=15000`.
- `fixedturn_behavior_motion` is the moving correctness layer for
  `BHV_FixedTurn`. It uses a short waypoint leg to establish stem speed, then
  grades turn completion, final heading, timeout completion, scheduled-turn
  flags, and invalid configuration branches.
- The `H01-fixedturn_behavior_motion` matrix has `24` cases covering starboard
  and port 90-degree turns, long starboard and port turns, automatic and
  explicit speed branches, turn delay, timeout completion, scheduled turn
  sequencing, scheduled-turn fallback/keyed-update/clear/runtime-recovery
  paths, runtime static updates, scheduled-turn alias and clear-add parser
  branches, zero-turn completion, and invalid `fix_turn`, `stale_nav_thresh`,
  `turn_dir`, `speed`, `turn_spec`, and `schedule_repeat` failures. The latest
  full wave run passed at warp `10` with `--jobs=4` and `--port_base=15000`.
- The `src/` tree is kept only for legacy template compatibility and can be trimmed further if you no longer need those builds.
