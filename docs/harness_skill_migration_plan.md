# Harness Skill Migration Plan

## Goal

Migrate the repository's MOOS-IvP harness launchers to the current
`moos-ivp-harness-builder` standard while preserving mission behavior, case
intent, result evidence, and the documented failure-machinery exemptions.

The migration should improve isolation, rolling parallel execution, cleanup,
result completeness, and auditability without using the migration as a reason
to redesign working mission stems.

## Scope

The migration covers:

- all configured harness launchers under `harnesses/`
- shared teardown infrastructure used by harnesses
- launcher validation and repository invariant checks
- direct before-and-after harness runs
- migration statistics and harness-specific findings
- documentation and generated dependency metadata affected by launcher, stem,
  or harness target changes

The unrelated MOOS-IvP build-matrix work already present in the working tree is
out of scope and must remain untouched.

## Settled Design Decisions

### Bash 5.1 And Rolling Scheduling

Harnesses that expose real `--jobs` execution will require Bash 5.1 or newer
and use rolling scheduling with `wait -p <pidvar> -n`.

Launchers will use `#!/usr/bin/env bash`, check the Bash version near startup,
and may re-execute a known Homebrew or Linuxbrew Bash before failing with a
clear installation/configuration message. Supporting Apple `/bin/bash` 3.2 is
not a migration requirement for these launchers.

Serial-only harnesses should omit `--jobs`; they do not need rolling scheduler
machinery merely for visual uniformity.

### Isolated Copies For All Cases

Serial, single-case, and parallel runs will use the same isolated per-case
mission-copy path. Each case receives:

- its own case directory beneath a harness-owned run root
- its own mission copy
- its own explicit patch or fixture setup
- its own port block
- its own log and intermediate result row

The pilots will confirm the implementation and expose unexpected copy or
cleanup costs, but unified isolation is the target design rather than an
optional optimization. Performance findings will be recorded and investigated
without returning ordinary serial execution to the shared stem directory.

### Result Publication

Each selected case must produce exactly one normalized result row. Ordinary
mission rows are published with `case=<case_name>` prepended while preserving
the mission-owned `grade=` and evidence columns.

Harness-owned rows are limited to runner failures such as:

```text
case=<case_name> grade=fail reason=prepare_error
case=<case_name> grade=fail reason=launch_error launch_rc=<rc>
case=<case_name> grade=fail reason=missing_result
case=<case_name> grade=fail reason=missing_result_file
```

Results will be aggregated deterministically in selected-case order even when
cases complete in a different order. A nonempty selection that produces zero
rows, missing rows, or duplicate rows is a harness failure. Ordinary case
failures do not stop the remaining selected cases from reporting.

### Teardown

The project will adopt `scripts/moos_scoped_teardown.sh` and
`moos_scoped_teardown_stop_root` as the canonical cleanup interface.

`scripts/harness_teardown.sh` may remain temporarily as a compatibility wrapper
while launchers are migrated. It will be removed after all callers have moved
unless a verified external compatibility requirement is found.

No migration path may depend on global `ktm`, broad `pkill`, `killall`, or
machine-wide cleanup.

### Ports

Every case receives a distinct port block large enough for all communities and
listeners in that case. Generated targets, not launcher arguments alone, are
the proof that forwarding worked.

Existing harness defaults will not be normalized mechanically. A harness may
retain a documented default when repo automation or mission structure gives a
reason to do so. Validation and local repeated runs will use fresh explicit
port bases as needed, and two runs with potentially overlapping ranges will not
execute concurrently.

### Mission Stems

Harness launcher modernization does not by itself require a stem change. A stem
will change only when evidence shows a concrete contract failure, such as:

- shell-owned rather than `pMissionEval`-owned grading
- incorrect expected-negative semantics
- missing or unusable `results.txt` output
- broken forwarded-port propagation
- missing `nsplug -x` sidecar consumption

When a shared stem changes, every configured consumer of that stem must be
identified and included in validation.

Evaluator changes should prefer information the mission already publishes.
Use existing mission variables directly for plain scalar, presence, absence,
and count assertions. `LogicCondition` rejects relation metacharacters such as
`=` inside a literal even when quoted. For exact structured equality, publish
the expected payload as a mission variable and compare
`ACTUAL = $ (EXPECTED_VAR)`. Use a narrowly scoped stock adapter only when
field selection or retained multi-publication evidence cannot be expressed
from current raw values. Adapter outputs should normally remain plain scalars
or presence sentinels; when the complete record association matters, compare
the smallest useful composite to a runtime expected variable rather than
embedding its structured payload in the condition. Do not introduce a grading
application.

### Existing Exemptions

The result shapes already documented in
`docs/harness_result_semantics_migration.md` remain valid for harnesses whose
subject is failure machinery or CLI return behavior. These include:

- `pMissionEval` subject-grade failure cases
- `uMayFinish` timeout and nonzero-return cases
- documented `uQueryDB` CLI-return cases

The migration will preserve these exemptions and make validation aware of them.
It will not generalize their subject-versus-harness verdict model to ordinary
harnesses.

## Migration Workflow

### Phase 1: Inventory

Classify all configured harnesses by:

- family, case count, and mission stem
- shared-stem consumers
- serial and parallel interfaces
- legacy batch-wave or rolling scheduling
- workdir and patching behavior
- port stride and community layout
- teardown API
- result aggregation and missing-row behavior
- redundant legacy result logic such as `EXPECTED=pass`
- documented special result semantics
- likely runtime and timing sensitivity

The inventory should drive the rollout order and provide a checklist showing
that all configured harnesses were considered.

### Phase 2: Direct Legacy Baselines

Run harnesses directly through their existing `zlaunch.sh` before editing them.
Do not repair or extend `scripts/benchmark_parallel.sh` or
`scripts/repeatability_sweep.py` for this migration. Treat those scripts as
legacy and remove them in a reviewed migration batch after confirming that no
current workflow depends on them.

Baseline runs are selected harness by harness. Time warp, `--max_time`, jobs,
and port bases may vary when different combinations expose useful behavior.
For meaningful before-and-after comparisons, record the exact configuration
used and repeat a matching configuration after migration.

The normal starting point is at least three complete baseline runs for a
harness. Use additional runs for performance harnesses, suspected flaky cases,
large timing variance, or any unexplained failure. Useful configurations may
include:

- a single named case for contract inspection
- `--jobs=1` for serialized full-matrix behavior
- `--jobs=2` or higher for the current parallel behavior
- more than one time warp or `--max_time` when timing slack is relevant

Runs must be sequential when their port ranges might overlap. Preserve
workdirs and logs when a case fails or when generated targets need inspection.

### Phase 3: Shared Infrastructure And Launcher Pattern

Add the canonical teardown helper and establish one auditable launcher pattern:

- early Bash 5.1 detection/re-execution for `--jobs` launchers
- explicit argument validation
- explicit case selection and case-to-setup mapping
- one unique harness-owned run root per invocation
- protection against unsafe concurrent writes to top-level `results.txt`
- per-case mission copies, logs, result rows, and port blocks
- patch existence checks and clean sidecar setup
- rolling scheduling for `--jobs`
- direct forwarding of harness `--max_time` to the stem wrapper
- per-case scoped teardown and exit-trap cleanup
- deterministic aggregation after all selected cases report
- final nonzero exit when any ordinary row lacks `grade=pass`

Use a consistent launcher pattern rather than creating a large shared runner
framework during the initial migration. Shared runner code may be considered
later only if the completed pilots demonstrate a clear maintenance benefit.

### Phase 4: Pilots

Migrate these representative ordinary harnesses first:

1. `cmgr_h01`
   - remove legacy `EXPECTED=pass`, `actual`, and `mismatch` plumbing
   - exercise a typical app-level unit harness
2. `obmgr_h02`
   - exercise a moving/integration harness
   - validate multi-community ports and sidecars
3. `ufld_obstacle_sim_h01`
   - exercise a newer large-matrix implementation
   - replace generic temporary directories with harness-owned runs

Also run regression checks for the mission-utility and CLI exemptions so the
ordinary launcher pattern and new static checks do not invalidate their
documented contracts.

### Phase 5: Pilot Validation

For each ordinary pilot:

- run the harness static checker
- confirm the stem independently writes its mission-owned result
- run nominal and expected-negative cases where applicable
- probe an unknown case
- exercise preparation, launch, and missing-result failure paths where safe
- compare serialized and rolling results
- inspect preserved targets for distinct forwarded ports
- inspect intended `.moosx` and `.bhvx` sidecars
- confirm pending work starts as soon as an active case finishes
- confirm deterministic case counts and result ordering
- confirm no mission-owned processes or listeners remain
- test the Bash re-execution and old-Bash failure message

Compare the migrated pilot with its recorded baseline configurations. Pilot
completion is the gate for broad rollout.

### Phase 6: Sequential Harness Rollout

Family relationships may guide migration order, but runtime work remains
strictly one harness at a time. For each harness:

1. Capture and record the legacy baseline.
2. Apply the proven launcher pattern.
3. Remove redundant legacy result comparisons.
4. Preserve exact README case tokens and explicit setup mappings.
5. Avoid stem edits unless a contract failure is demonstrated.
6. Run static validation and direct representative cases.
7. Run complete serialized and rolling matrices as appropriate.
8. Compare correctness, timing, warnings, ports, and process hygiene.
9. Record findings and close the harness before proceeding to the next one.

Shared-stem consumers do not have to be migrated simultaneously when the stem
is unchanged. If the stem changes, all its consumers enter the validation set.

### Phase 7: Repository Validation And Cleanup

After sequential harness rollout:

- validate the harness target registry
- reconcile README tokens, launcher case lists, and setup mappings
- run the repository invariant checker
- group non-MOOS static checks when useful, but run live harness validation
  sequentially even when nominal port ranges do not overlap
- audit retained workdirs and generated target ports
- verify no broad cleanup commands or legacy teardown calls remain
- remove the obsolete benchmarking/repeatability scripts if dependency checks
  confirm they are unused
- remove the compatibility teardown wrapper after its final caller is gone
- regenerate `docs/context/dependency_tree.md` and JSON after qualifying
  launcher, stem, target metadata, or catalog changes
- review the final diff while excluding unrelated build-matrix work

## Statistics And Findings

Maintain a migration record with both aggregate and harness-specific entries.
For each measured run, record enough context to reproduce it:

- date and repository revision
- harness and selected cases
- legacy or migrated implementation
- machine/platform note
- time warp, `--max_time`, jobs, and port base
- scheduler type: legacy batch wave, serial, or rolling
- exit code
- selected, reported, passing, failing, missing, and duplicate row counts
- full-run wall time
- per-case wall or MOOS timing when the mission publishes it
- warnings and relevant evidence-field changes
- preserved-workdir or log location when investigation is needed
- leftover listener/process result

Summaries should report pass rate and useful runtime statistics such as mean,
median, minimum, maximum, and observed range. Compare scheduler throughput only
between clearly labeled configurations; timing is operational evidence, not
proof of mission correctness.

### Failures And Flakiness

A baseline failure or outlier is suspicious and should be investigated. Useful
follow-up includes repeating the individual case, repeating the complete
harness, preserving workdirs, checking generated ports, and distinguishing:

- a deterministic test defect
- a timing-sensitive or flaky case
- port or leftover-process contamination
- insufficient `--max_time` slack
- a launcher or measurement artifact

Record the evidence and provisional classification in the migration document.
A pre-existing failure or flaky test does not automatically block launcher
migration. It becomes a migration blocker only when the launcher cannot be
changed safely, the migrated result is materially worse without explanation,
or validation cannot distinguish a migration regression from the baseline.

Do not hide, reinterpret, or silently exclude an outlier merely to produce a
passing aggregate.

## Completion Criteria

The migration is complete when:

- every configured harness is classified and accounted for
- every `--jobs` harness uses the approved Bash 5.1 rolling scheduler
- serial, single-case, and parallel execution use isolated mission copies
- workdirs and cleanup are harness-owned and root-scoped
- every selected case produces exactly one deterministic result row
- ordinary harness verdicts come directly from mission-owned `grade=`
- only documented failure/CLI subjects use special outer-versus-subject verdicts
- before-and-after findings and performance statistics are recorded
- unexplained regressions and outliers are investigated and documented
- every consumer of a changed shared stem is validated
- obsolete migration-era helpers and unused legacy benchmark scripts are removed
- repository checks and sequential runtime validation pass
- dependency metadata is current
- unrelated build-matrix work remains untouched

## Execution Record

### Static Inventory: July 12, 2026

The configured target registry contains 67 harnesses, and direct case-list
parsing succeeded for all 67 current launchers.

Static launcher findings:

- all 67 expose real `--jobs` execution
- all 67 use legacy batch-wave scheduling; none use rolling `wait -p -n`
- 60 create their parallel work tree beneath the harness directory
- 7 use a generic system temporary root
- all 67 use the legacy scoped teardown helper/API
- none use the canonical `moos_scoped_teardown` API
- 21 contain an `EXPECTED=` variable and require case-by-case review for
  redundant legacy result comparison
- 3 have documented special result semantics: `pmissioneval_h01`,
  `umayfinish_h01`, and `uquerydb_h01`
- 5 mission stems are shared, accounting for 22 harness consumers

The generated dependency JSON was already out of date because of pre-existing,
out-of-scope working-tree changes. The inventory uses the configured target
registry for target paths, direct parsing for current case lists, and the
existing dependency map only for mission relationships. It does not regenerate
or overwrite the user's unrelated dependency-tree changes.

Inventory columns:

- `consumers`: configured harness count for the mission stem
- `scheduler`: current full-matrix scheduling method
- `workdir`: current parallel work-root placement
- `legacy expected`: launcher contains `EXPECTED=` and needs semantic review;
  this does not by itself prove incorrect expected-negative grading
- `special`: documented outer-versus-subject or CLI-result semantics

| target | cases | mission stem | consumers | scheduler | workdir | default port | teardown | legacy expected | special |
| --- | ---: | --- | ---: | --- | --- | ---: | --- | --- | --- |
| `cmgr_h01` | 33 | `missions/cmgr_missions/cmgr_unit` | 1 | wave | harness-temp | 9000 | legacy-scoped | yes | no |
| `cmgr_h02` | 15 | `missions/cmgr_missions/cmgr_motion` | 1 | wave | harness-temp | 9000 | legacy-scoped | no | no |
| `obmgr_h01` | 30 | `missions/obmgr_missions/obmgr_unit` | 1 | wave | harness-temp | 9000 | legacy-scoped | yes | no |
| `obmgr_h02` | 10 | `missions/obmgr_missions/obmgr_motion` | 1 | wave | harness-temp | 9100 | legacy-scoped | no | no |
| `pid_h01` | 34 | `missions/pid_missions/pid_unit` | 1 | wave | harness-temp | 12000 | legacy-scoped | no | no |
| `pid_h02` | 13 | `missions/pid_missions/pid_motion` | 1 | wave | harness-temp | 26000 | legacy-scoped | no | no |
| `usim_marine_h01` | 36 | `missions/usim_marine_missions/usim_marine_motion` | 1 | wave | harness-temp | 30000 | legacy-scoped | yes | no |
| `pnodereporter_h01` | 33 | `missions/pnodereporter_missions/pnodereporter_unit` | 1 | wave | harness-temp | 12400 | legacy-scoped | no | no |
| `upokedb_h01` | 28 | `missions/upokedb_missions/upokedb_unit` | 1 | wave | harness-temp | 15000 | legacy-scoped | yes | no |
| `uxms_h01` | 32 | `missions/uxms_missions/uxms_unit` | 1 | wave | harness-temp | 15200 | legacy-scoped | no | no |
| `uquerydb_h01` | 30 | `missions/uquerydb_missions/uquerydb_unit` | 1 | wave | harness-temp | 15600 | legacy-scoped | no | yes |
| `pechovar_h01` | 33 | `missions/pechovar_missions/pechovar_unit` | 1 | wave | system-temp | 17600 | legacy-scoped | no | no |
| `utimerscript_h01` | 33 | `missions/utimerscript_missions/utimerscript_unit` | 1 | wave | harness-temp | 7300 | legacy-scoped | yes | no |
| `pdeadmanpost_h01` | 20 | `missions/pdeadmanpost_missions/pdeadmanpost_unit` | 1 | wave | harness-temp | 15800 | legacy-scoped | no | no |
| `pspoofnode_h01` | 33 | `missions/pspoofnode_missions/pspoofnode_unit` | 1 | wave | harness-temp | 16000 | legacy-scoped | no | no |
| `utermcommand_h01` | 15 | `missions/utermcommand_missions/utermcommand_unit` | 1 | wave | harness-temp | 16200 | legacy-scoped | no | no |
| `psearchgrid_h01` | 23 | `missions/psearchgrid_missions/psearchgrid_unit` | 1 | wave | harness-temp | 16400 | legacy-scoped | no | no |
| `depth_constant_h01` | 19 | `missions/depth_behavior_missions/depth_behavior_motion` | 5 | wave | harness-temp | 34000 | legacy-scoped | no | no |
| `depth_goto_h02` | 18 | `missions/depth_behavior_missions/depth_behavior_motion` | 5 | wave | harness-temp | 34000 | legacy-scoped | no | no |
| `depth_periodic_surface_h03` | 24 | `missions/depth_behavior_missions/depth_behavior_motion` | 5 | wave | harness-temp | 34000 | legacy-scoped | no | no |
| `depth_max_h04` | 15 | `missions/depth_behavior_missions/depth_behavior_motion` | 5 | wave | harness-temp | 34000 | legacy-scoped | no | no |
| `depth_min_altitude_h05` | 13 | `missions/depth_behavior_missions/depth_behavior_motion` | 5 | wave | harness-temp | 34000 | legacy-scoped | no | no |
| `colregs_h01` | 22 | `missions/colregs_missions/colregs_unit` | 4 | wave | harness-temp | 9800 | legacy-scoped | yes | no |
| `colregs_h02` | 58 | `missions/colregs_missions/colregs_unit` | 4 | wave | harness-temp | 9900 | legacy-scoped | yes | no |
| `colregs_h03` | 23 | `missions/colregs_missions/colregs_unit` | 4 | wave | harness-temp | 10080 | legacy-scoped | yes | no |
| `colregs_h04` | 32 | `missions/colregs_missions/colregs_unit` | 4 | wave | harness-temp | 10280 | legacy-scoped | yes | no |
| `collision_h01` | 21 | `missions/collision_behavior_missions/collision_behavior_motion` | 1 | wave | harness-temp | 9300 | legacy-scoped | no | no |
| `convoy_h01` | 48 | `missions/convoy_behavior_missions/convoy_behavior_motion` | 1 | wave | harness-temp | 9700 | legacy-scoped | no | no |
| `cutrange_h01` | 39 | `missions/cutrange_behavior_missions/cutrange_behavior_motion` | 1 | wave | harness-temp | 11000 | legacy-scoped | no | no |
| `fixedturn_h01` | 24 | `missions/fixedturn_behavior_missions/fixedturn_behavior_motion` | 1 | wave | harness-temp | 15000 | legacy-scoped | no | no |
| `periodic_speed_h01` | 24 | `missions/periodic_speed_behavior_missions/periodic_speed_behavior_motion` | 1 | wave | harness-temp | 15000 | legacy-scoped | no | no |
| `zigzag_h01` | 35 | `missions/zigzag_behavior_missions/zigzag_behavior_motion` | 1 | wave | harness-temp | 15000 | legacy-scoped | no | no |
| `legrun_h01` | 33 | `missions/legrun_behavior_missions/legrun_behavior_motion` | 1 | wave | system-temp | 15400 | legacy-scoped | no | no |
| `memoryturnlimit_h01` | 21 | `missions/memoryturnlimit_behavior_missions/memoryturnlimit_behavior_motion` | 1 | wave | harness-temp | 15000 | legacy-scoped | no | no |
| `timer_h01` | 18 | `missions/timer_behavior_missions/timer_behavior_motion` | 1 | wave | system-temp | 15000 | legacy-scoped | no | no |
| `testfailure_h01` | 9 | `missions/testfailure_behavior_missions/testfailure_behavior_unit` | 1 | wave | harness-temp | 18000 | legacy-scoped | no | no |
| `hostinfo_h01` | 7 | `missions/hostinfo_missions/hostinfo_unit` | 1 | wave | harness-temp | 11000 | legacy-scoped | yes | no |
| `loadwatch_h01` | 12 | `missions/loadwatch_missions/loadwatch_unit` | 1 | wave | harness-temp | 11200 | legacy-scoped | yes | no |
| `processwatch_h01` | 7 | `missions/processwatch_missions/processwatch_unit` | 1 | wave | harness-temp | 11600 | legacy-scoped | no | no |
| `pshare_h01` | 13 | `missions/pshare_missions/pshare_unit` | 1 | wave | harness-temp | 11000 | legacy-scoped | yes | no |
| `pshare_h02` | 12 | `missions/pshare_missions/pshare_topology` | 1 | wave | harness-temp | 11000 | legacy-scoped | yes | no |
| `plogger_h01` | 10 | `missions/plogger_missions/plogger_unit` | 1 | wave | harness-temp | 12000 | legacy-scoped | yes | no |
| `pantler_h01` | 6 | `missions/pantler_missions/pantler_unit` | 1 | wave | harness-temp | 12200 | legacy-scoped | yes | no |
| `pmissioneval_h01` | 17 | `missions/mission_utility_missions/mission_utility_unit` | 3 | wave | harness-temp | 7100 | legacy-scoped | no | yes |
| `pmissionhash_h01` | 4 | `missions/mission_utility_missions/mission_utility_unit` | 3 | wave | harness-temp | 7300 | legacy-scoped | no | no |
| `umayfinish_h01` | 4 | `missions/mission_utility_missions/mission_utility_unit` | 3 | wave | harness-temp | 7400 | legacy-scoped | no | yes |
| `loiter_h01` | 34 | `missions/loiter_behavior_missions/loiter_behavior_motion` | 1 | wave | harness-temp | 9700 | legacy-scoped | no | no |
| `obstacle_behavior_h01` | 21 | `missions/obstacle_behavior_missions/obstacle_behavior_motion` | 1 | wave | harness-temp | 9300 | legacy-scoped | no | no |
| `opregion_h01` | 25 | `missions/opregion_missions/opregion_motion` | 1 | wave | harness-temp | 9300 | legacy-scoped | no | no |
| `p01_obstacle` | 3 | `missions/performance_missions/P01-obstacle_gauntlet` | 1 | wave | harness-temp | 9600 | legacy-scoped | yes | no |
| `shadow_h01` | 35 | `missions/shadow_behavior_missions/shadow_behavior_motion` | 1 | wave | harness-temp | 9000 | legacy-scoped | no | no |
| `stationkeep_h01` | 34 | `missions/stationkeep_behavior_missions/stationkeep_behavior_motion` | 1 | wave | harness-temp | 9700 | legacy-scoped | no | no |
| `trail_h01` | 42 | `missions/trail_behavior_missions/trail_behavior_motion` | 1 | wave | harness-temp | 9700 | legacy-scoped | no | no |
| `waypoint_h01` | 43 | `missions/waypoint_behavior_missions/waypoint_behavior_motion` | 1 | wave | harness-temp | 9700 | legacy-scoped | no | no |
| `p02_colregs` | 3 | `missions/performance_missions/P02-colregs_joust` | 1 | wave | harness-temp | 9600 | legacy-scoped | yes | no |
| `p03_colregs` | 4 | `missions/performance_missions/P03-colregs_traffic_ring` | 1 | wave | harness-temp | 9900 | legacy-scoped | yes | no |
| `ufld_pathcheck_h01` | 17 | `missions/ufield_app_missions/ufield_app_unit` | 7 | wave | harness-temp | 4300 | legacy-scoped | no | no |
| `ufld_message_handler_h01` | 26 | `missions/ufield_app_missions/ufield_app_unit` | 7 | wave | harness-temp | 4400 | legacy-scoped | no | no |
| `ufld_contact_range_sensor_h01` | 34 | `missions/ufield_app_missions/ufield_app_unit` | 7 | wave | harness-temp | 4500 | legacy-scoped | no | no |
| `ufld_beacon_range_sensor_h01` | 20 | `missions/ufield_app_missions/ufield_app_unit` | 7 | wave | harness-temp | 4700 | legacy-scoped | no | no |
| `ufld_collision_detect_h01` | 20 | `missions/ufield_app_missions/ufield_app_unit` | 7 | wave | harness-temp | 4800 | legacy-scoped | no | no |
| `ufld_collob_detect_h01` | 14 | `missions/ufield_app_missions/ufield_app_unit` | 7 | wave | harness-temp | 4900 | legacy-scoped | no | no |
| `ufld_scope_h01` | 8 | `missions/ufield_app_missions/ufield_app_unit` | 7 | wave | harness-temp | 5000 | legacy-scoped | no | no |
| `ufield_comms_h01` | 35 | `missions/ufield_comms_missions/ufield_comms_unit` | 3 | wave | system-temp | 4000 | legacy-scoped | yes | no |
| `ufield_comms_h02` | 12 | `missions/ufield_comms_missions/ufield_comms_unit` | 3 | wave | system-temp | 4000 | legacy-scoped | yes | no |
| `ufield_comms_h03` | 25 | `missions/ufield_comms_missions/ufield_comms_unit` | 3 | wave | system-temp | 4000 | legacy-scoped | no | no |
| `ufld_obstacle_sim_h01` | 49 | `missions/ufld_obstacle_sim_missions/ufld_obstacle_sim_unit` | 1 | wave | system-temp | 7600 | legacy-scoped | yes | no |

### Legacy Pilot Baseline: `cmgr_h01`

Environment and configuration:

- date: July 12-13, 2026 EDT
- machine: local Apple Silicon macOS host `becker`
- launcher: legacy batch-wave `zlaunch.sh`
- full-matrix configuration: time warp 10, default `--max_time=65`,
  `--jobs=2`, `--port_base=20000`
- assigned range: 20000-20971
- run 1 used `--keep_workdirs`; runs 2 and 3 used normal cleanup

Full-matrix results:

| run | exit | rows | passing | failing | wall seconds | scoped processes after | listeners after |
| ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| 1 | 1 | 33 | 32 | 1 | 149.64 | 0 | 0 |
| 2 | 0 | 33 | 33 | 0 | 144.25 | 0 | 0 |
| 3 | 0 | 33 | 33 | 0 | 144.70 | 0 | 0 |

Full-matrix wall time was 146.20 seconds mean, 144.70 median, 144.25
minimum, and 149.64 maximum. Across the three matrices, 98 of 99 published
case rows passed.

The sole failure was `detect_baseline_pass` in run 1:

```text
case=detect_baseline_pass grade=fail form=cmgr_tests_harness
mmod=detect_baseline_pass eval=true seen=false closest=intruder range=24
```

The preserved original shoreside `.alog` identifies a startup ordering race:

```text
38.84137 CONTACT_SEEN pContactMgrV20 true
40.21028 CONTACT_SEEN pHelmIvP:HELM_VAR_INIT false
45.33507 TEST_EVAL_READY uTimerScript true
```

Thus `pContactMgrV20` observed the contact, but the later helm initialization
overwrote the flag before `pMissionEval` evaluated it. The retained run was
inspected at the following temporary path before generated artifacts were
removed:

```text
harnesses/cmgr_harnesses/H01-cmgr_unit/.parallel_cmgr_unit_FoGhDx/
  000_detect_baseline_pass/XLOG_SHORESIDE_12_7_2026_____23_57_58/
  XLOG_SHORESIDE_12_7_2026_____23_57_58.alog
```

Five sequential direct repetitions of `detect_baseline_pass` all passed. Their
wall times were 7.39, 6.48, 6.49, 7.23, and 6.54 seconds: 6.83 mean, 6.54
median, 6.48 minimum, and 7.39 maximum. No scoped processes remained.

Classification: pre-existing parallel-start flake, recorded but not a launcher
migration blocker. The migrated launcher must not worsen its observed rate, and
the unified isolated path plus rolling pilot should be checked for the same
flag-ordering race.

### Legacy Pilot Baseline: `obmgr_h02`

Environment and configuration:

- date: July 13, 2026 EDT
- machine: local Apple Silicon macOS host `becker`
- launcher: legacy batch-wave `zlaunch.sh`
- full-matrix configuration: time warp 10, default `--max_time=70`,
  `--jobs=2`, `--port_base=24000`
- assigned range: 24000-24281
- run 1 used `--keep_workdirs`; runs 2 and 3 used normal cleanup

Full-matrix results:

| run | exit | rows | passing | failing | wall seconds | scoped processes after | listeners after |
| ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| 1 | 0 | 10 | 10 | 0 | 58.24 | 0 | 0 |
| 2 | 0 | 10 | 10 | 0 | 57.09 | 0 | 0 |
| 3 | 0 | 10 | 10 | 0 | 57.88 | 0 | 0 |

All 30 published case rows passed. Full-matrix wall time was 57.74 seconds
mean, 57.88 median, 57.09 minimum, and 58.24 maximum. Nominal cases retained
zero-collision evidence; all four expected-collision cases retained
mission-owned `grade=pass`, `expected=collision`, and `collisions=1` evidence.

The preserved first run contained 20 generated `targ_*.moos` files: one vehicle
and one shoreside target for each case. The targets prove distinct 30-port case
blocks, with MOOSDB offsets 0/1 and pShare offsets 10/11. Nineteen intended
`.moosx` sidecars were present and no `.bhvx` sidecars were used. That generated
tree was inspected at the following temporary path before cleanup:

```text
harnesses/obmgr_harnesses/H02-obmgr_motion/
  .parallel_obmgr_motion_etEwZH/
```

Classification: stable legacy baseline with no observed correctness, port,
teardown, or listener anomaly.

### Legacy Pilot Baseline: `ufld_obstacle_sim_h01`

Environment and configuration:

- date: July 13, 2026 EDT
- machine: local Apple Silicon macOS host `becker`
- launcher: legacy batch-wave `zlaunch.sh`
- full-matrix configuration: time warp 10, default `--max_time=30`,
  `--jobs=2`, `--port_base=28000`
- assigned MOOSDB range: 28000-29440
- run 1 used `--keep_workdirs`; runs 2 and 3 used normal cleanup

Full-matrix results:

| run | exit | rows | mission grade pass | launcher extra-check failures | wall seconds | scoped processes after | listeners after |
| ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| 1 | 1 | 49 | 49 | 1 | 141.11 | 0 | 0 |
| 2 | 0 | 49 | 49 | 0 | 138.08 | 0 | 0 |
| 3 | 0 | 49 | 49 | 0 | 139.80 | 0 | 0 |

All 147 mission result rows reported `grade=pass`. Full-matrix wall time was
139.66 seconds mean, 139.80 median, 138.08 minimum, and 141.11 maximum.
The timing sample remains a valid legacy baseline. Detailed behavior
correctness, however, came from the launcher's supplemental `.alog` checks;
the readiness-style mission grades alone did not prove the 49 detailed case
contracts.

Run 1 exited nonzero because the supplemental shell check for
`fixed_field_publish_pass` saw unexpanded obstacle macros in an otherwise
passing mission row:

```text
case=fixed_field_publish_pass grade=pass form=ufld_obstacle_sim_unit
mmod=fixed_field_publish_pass eval=true known=$[KNOWN_OBSTACLE]
given=$[GIVEN_OBSTACLE] tracked=$[TRACKED_FEATURE_ALPHA]
```

The preserved original `.alog` proves the app behavior and the timing of the
reporting race:

```text
5.26327 TEST_EVAL_READY true
5.35956 KNOWN_OBSTACLE pts={10,-10:14,-10:14,-6:10,-6},label=ufos_a
5.35957 GIVEN_OBSTACLE pts={10,-10:14,-10:14,-6:10,-6},label=ufos_a
5.41994 MISSION_EVALUATED true
```

Five sequential direct repetitions of `fixed_field_publish_pass` all passed
the supplemental check. Their wall times were 5.80, 5.69, 5.48, 5.70, and
5.70 seconds: 5.67 mean, 5.70 median, 5.48 minimum, and 5.80 maximum.

The preserved run contained 49 generated shoreside targets with 49 unique
MOOSDB ports from 28000 through 29440 in 30-port blocks. It contains 48
intended `.moosx` sidecars, no `.bhvx` sidecars, and no pShare routes. Evidence
was inspected at the following temporary path before cleanup:

```text
/var/folders/6n/9mlmm9f17zsgqbkmc1c306z80000gn/T/
  ufld_obstacle_sim_harness.Gvo1u7/
```

Classification: pre-existing parallel-start/report-expansion flake. The
mission-owned grade and original app evidence agree that the behavior passed,
but the legacy launcher can exit nonzero without publishing a failing row. The
migration should move necessary verdict evidence into the mission result
contract so the row and process exit cannot disagree.

### Combined Legacy Pilot Summary

Nine complete legacy matrices published 276 case rows in 1030.79 wall-clock
seconds. Of those rows, 275 reported `grade=pass`; seven of nine launcher
invocations exited zero. The two nonzero invocations were both first-run
parallel startup/reporting anomalies and both affected cases passed 5/5 direct
repetitions.

| target | full runs | successful launcher runs | passing rows / rows | mean wall seconds | focused repeat result |
| --- | ---: | ---: | ---: | ---: | --- |
| `cmgr_h01` | 3 | 2 | 98 / 99 | 146.20 | `detect_baseline_pass` 5/5 pass |
| `obmgr_h02` | 3 | 3 | 30 / 30 | 57.74 | no outlier |
| `ufld_obstacle_sim_h01` | 3 | 2 | 147 / 147 | 139.66 | `fixed_field_publish_pass` 5/5 pass |

Every post-run scoped-process and reserved-port listener check was clean. These
baselines provide the initial correctness, timing, isolation, and flake-rate
comparison for the migrated pilot launchers.

### Migrated Pilot: `cmgr_h01`

Implementation began only after all three legacy pilot baselines were complete.
No `obmgr_h02` or `ufld_obstacle_sim_h01` launcher was edited or run during
this stage.

The canonical `scripts/moos_scoped_teardown.sh` was imported from the installed
harness skill asset. The initially installed asset required one local
compatibility correction for an optional variable under `set -u`. Skill 1.4.2
subsequently incorporated that correction and explicit discovery-failure
propagation; the repository helper is now the refreshed asset verbatim. The
legacy compatibility helper remains for unmigrated harnesses.

The pre-edit stem probes demonstrated a concrete reason to change this
single-consumer stem:

- the eval-mission checker failed because `zlaunch.sh` neither detected a
  missing `grade=` nor named the canonical teardown API
- a `--just_make` probe with explicit MOOSDB and pShare ports exited 1 because
  the stem `zlaunch.sh` rejected `--shore_mport`
- `launch.sh` already supported all four port arguments, so the repair was to
  forward the existing contract rather than redesign the mission

The migrated stem now passes the eval-mission static checker, forwards all four
ports through its `xlaunch.sh` call, verifies that `pMissionEval` wrote a grade,
and performs canonical root-scoped teardown.

Several inconsistencies or unsafe edges were found in the skill asset/example
and corrected in the pilot pattern rather than copied:

1. The example appends rows in completion order even though the surrounding
   contract requires deterministic selected-case order. The pilot records each
   case row separately and aggregates only after scheduling is complete.
2. The example advertises and forwards `--just_make`, but then treats its
   intentionally empty mission `results.txt` as a missing-result failure. The
   pilot emits one explicit `grade=pass mode=just_make` preparation row when
   target generation succeeds. This row is not used as a mission-behavior
   verdict.
3. Shared `xlaunch.sh` can return zero in just-make mode after a lower launch or
   `nsplug` failure. Both the stem and harness now require the expected
   nonempty shoreside, vehicle, and behavior targets before passing
   preparation mode; the stem also clears stale targets first.
4. The example uses one fixed run root, which means a later invocation can
   delete an earlier `--keep_workdirs` artifact. The pilot creates one unique
   invocation directory beneath `.harness_runs/` and removes only its own root.
5. Teardown errors are suppressed in the example. The pilot turns a per-case
   teardown failure into a runner-failure row and retains both the run root and
   safety lock if final teardown cannot prove the root clear.

Initial migrated probes on July 13, 2026:

| probe | exit | rows | wall seconds | result |
| --- | ---: | ---: | ---: | --- |
| static harness checker | 0 | n/a | n/a | pass |
| static eval-stem checker | 0 | n/a | n/a | pass |
| Bash 3.2 with re-exec disabled | 2 | n/a | n/a | clear Bash 5.1 requirement |
| Bash 3.2 normal path | 0 | n/a | n/a | re-executed Homebrew Bash 5.3.9 |
| unknown case | 2 | 0 | n/a | diagnostic, no stale lock |
| `count_two_pass --just_make` | 0 | 1 | 1.68-2.06 | expected targets and preparation row passed |
| broken copied just-make target | 1 | 0 | n/a | stale/partial targets could not false-pass |
| copied `count_two_pass` stem run | 0 | 1 | 5.98 | mission-owned grade passed |
| `detect_baseline_pass` | 0 | 1 | 8.47 | nominal grade passed |
| `detect_strict_absent_pass` | 0 | 1 | 8.12 | intended non-detection grade passed |
| delayed case, warp 1, `--max_time=1` | 1 | 1 | 8.33 | `reason=missing_result`, clean teardown |

The just-make target inspection proved MOOSDB ports 31000/31001 and pShare
ports 31015/31016 in the generated files. Source stem hashes were identical
before and after preparation, and patch sidecars existed only in the isolated
copy. Both live single-case probes had zero scoped process and listener
leftovers.

Full-matrix validation:

| configuration | run | exit | rows | pass | fail | wall seconds | order | cleanup |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| isolated serial, `--jobs=1 --port_base=32000` | 1 | 0 | 33 | 33 | 0 | 257.51 | exact | clean |
| rolling, matching legacy settings | 1 | 0 | 33 | 33 | 0 | 132.84 | exact | clean |
| rolling, matching legacy settings | 2 | 0 | 33 | 33 | 0 | 136.17 | exact | clean |
| rolling, matching legacy settings | 3 | 0 | 33 | 33 | 0 | 135.12 | exact | clean |
| intentional timeout, warp 1, `--max_time=1` | 1 | 1 | 33 | 0 | 33 | 133.10 | exact | clean |
| final successful checkout state, matching settings | 1 | 0 | 33 | 33 | 0 | 132.42 | exact | clean |

The three matched rolling matrices published 99/99 passing rows and all three
launchers exited zero. Their wall time was 134.71 seconds mean, 135.12 median,
132.84 minimum, and 136.17 maximum. Compared with the matching legacy mean of
146.20 seconds, rolling migration reduced mean wall time by 11.49 seconds, or
about 7.9 percent. The migrated `detect_baseline_pass` case passed all three
rolling matrices; the observed legacy full-matrix rate had been 2/3.

The intentional timeout matrix proved continued execution after ordinary case
failure: all 33 workers exited nonzero, all 33 selected cases still published
one `reason=missing_result` row, and the launcher returned nonzero only after
ordered aggregation.

The final 132.42-second matrix restored the checked-out harness `results.txt`
to a complete 33/33 passing state after the intentional failure probe. It is
reported separately and is not included in the three-run legacy comparison.

The dependency metadata gate also exposed two generator assumptions that no
longer held for the modern launcher. The case-count parser recognized array
syntax but only expanded the legacy `ALL_CASES` name; it now prefers
`ALL_CASES` when present and otherwise accepts the modern `CASES` array. The
graph traversal also counted copied launchers and thousands of generated
mission files beneath preserved run roots. Generated harness-copy and MOOS log
trees are now excluded from repository source counts. Eleven focused unit
tests pass, direct parsing reports 33 cases, the graph again reports 67 source
harness launchers, and `generate_context_graph.py --check` is clean.

Before cleanup, the preserved first rolling run contained 33 case directories, 33 intermediate
rows, 66 MOOS targets, 33 behavior targets, 59 intended `.moosx` sidecars, and
no `.bhvx` sidecars. Generated targets contain 66 unique MOOSDB ports from
20000 through 20961 and 66 unique pShare ports from 20015 through 20976, with
the exact 30-port case-block pattern. The inspected temporary paths were:

```text
harnesses/cmgr_harnesses/H01-cmgr_unit/.harness_runs/
  run_20260713T005059_57344/   # passing rolling run 1
  run_20260713T010052_10700/   # intentional 33-row timeout run
```

#### Skill 1.4.2 alignment: `cmgr_h01`

The refreshed-skill audit on July 14, 2026 left the 33-case matrix and all
mission evaluation conditions unchanged. It corrected two wrapper contracts:
Apple Bash 3.2 no longer expands empty GUI, forwarding, or result arrays under
`set -u`, and a nonzero stem exit now retains any valid mission row as renamed
`mission_*` provenance on the runner-owned `launch_error` row. Static harness
and eval checks, Bash syntax, ShellCheck, all 67 repository tests, dependency
metadata, and repository invariants passed after the changes.

Focused `detect_baseline_pass` repetitions passed 5/5, with wall times of 12,
8, 11, 9, and 9 seconds. The expected-absence
`detect_strict_absent_pass` case passed in 8.75 seconds with `seen=false`. An
isolated serial matrix passed 33/33 rows in exact order in 282.27 seconds.

Canonical-default rolling results were:

| configuration | runs | passing rows / rows | wall seconds | performance use |
| --- | ---: | ---: | --- | --- |
| skill 1.4.2 defaults, clean runs | 3 | 99 / 99 | 154, 140, 153.88 | mean 149.29 seconds |
| clamshell-sleep run | 1 | 33 / 33 | 570 | excluded; macOS slept for 429 seconds |
| supported one-second INT/TERM diagnostic overrides | 1 | 33 / 33 | 134.38 | causal timing probe only |

The diagnostic nearly equals the prior 134.71-second migrated mean. Original
`.alog` evidence from representative cases placed `MISSION_EVALUATED` 3.75 to
4.56 wall seconds before the per-case result was finalized, and the exact 1.4.2
asset waits up to three seconds after INT before escalating. The observed
14.58-second rolling-mean increase is therefore attributed to the refreshed
helper's deliberately longer cleanup grace rather than case logic, isolated
copying, or rolling refill. The canonical defaults remain unchanged.

The retained 1.4.2 rolling run again contained 33 case directories, 33 rows,
66 MOOS targets, 33 behavior targets, 59 intended `.moosx` sidecars, 132 unique
listener ports from 22000 through 22976, and no log warnings. Verbose timestamps
showed immediate refill. No scoped process, listener, safety lock, source
sidecar, or retained run tree remained after inspection.

### Migrated Pilot: `obmgr_h02`

Migration began only after `cmgr_h01` was fully closed. No `cmgr_h01` or
`ufld_obstacle_sim_h01` mission ran while this pilot was active.

The read-only contract audit confirmed ten cases, nineteen overlays, and one
direct consumer of the stem. It also found a legacy patch-error bug: a failed
shoreside `nspatch` could be masked by the status of the later vehicle-patch
branch. The migrated copy path verifies every patch file and gives each
`nspatch` its own explicit failure return. The historical mapping from
`two_sequential_fail` to `two-sequential-pass-{shoreside,vehicle}.xmoos`
remains explicit and unchanged.

The four collision-expected cases require no grading exemption. Their
`pMissionEval` patches already define the expected collision as a passing
mission outcome, so the harness now retains those mission-owned rows directly:
`grade=pass expected=collision` with the required collision and encounter
counts.

Pre-edit evidence showed why the single-consumer stem wrapper needed repair:

- the harness checker failed legacy teardown naming and warned about
  batch-barrier scheduling, missing zero-row protection, and old pShare
  offsets
- the eval-mission checker failed missing-grade detection and canonical
  teardown integration
- an isolated `--just_make` invocation rejected `--shore_mport` before
  producing any target, even though `launch.sh` already accepted and forwarded
  all four explicit ports

The migrated launcher uses the proven `cmgr_h01` runner architecture with the
exact `obmgr_h02` case contract. Every path uses a unique isolated case copy;
rolling scheduling uses Bash 5.1 `wait -p -n`; pShare ports use offsets 15/16
inside each 30-port block; results are strictly normalized and aggregated in
selected order; preparation and teardown failures become runner-owned failing
rows; and a unique invocation root plus safety lock protects retained
evidence. The copied stem receives `--mmod=<case>` as well as all four ports.
The stem validates `targ_shoreside.moos`, `targ_abe.moos`, and `targ_abe.bhv`
in preparation mode, requires a mission `grade=` during live runs, and uses
canonical root-scoped teardown.

Initial migrated probes on July 13, 2026:

| probe | exit | rows | wall seconds | result |
| --- | ---: | ---: | ---: | --- |
| static harness checker | 0 | n/a | n/a | pass |
| static eval-stem checker | 0 | n/a | n/a | pass |
| Bash 3.2 with re-exec disabled | 2 | n/a | n/a | clear Bash 5.1 requirement |
| Bash 3.2 normal path | 0 | n/a | n/a | re-executed Homebrew Bash 5.3.9 |
| unknown case and invalid jobs/ports | 2 | 0 | n/a | diagnostics, no stale lock |
| `baseline_center_pass --just_make` | 0 | 1 | 1.84 | all expected targets passed |
| broken copied just-make target | 1 | 0 | n/a | partial targets could not false-pass |
| copied `baseline_center_pass` stem run | 0 | 1 | 10.01 | mission-owned grade passed |
| `baseline_center_pass` | 0 | 1 | 11.07 | nominal grade passed |
| `wide_center_fail` | 0 | 1 | 10.88 | expected collision graded pass |
| delayed case, warp 1, `--max_time=1` | 1 | 1 | 7.30 | `reason=missing_result`, clean teardown |
| forced shoreside patch failure | 1 | 1 | about 1 | `reason=prepare_error`; not masked |

Preparation generated MOOSDB ports 24000/24001 and pShare ports 24015/24016.
All three source template hashes remained unchanged throughout validation.
The standalone copied stem wrote exactly one row with exactly one `grade=`
field, and every probe left the reserved range clear.

Full-matrix validation:

| configuration | run | exit | rows | pass | fail | wall seconds | order | cleanup |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| isolated serial, `--jobs=1 --port_base=24000` | 1 | 0 | 10 | 10 | 0 | 110.13 | exact | clean |
| rolling, matching legacy settings | 1 | 0 | 10 | 10 | 0 | 58.18 | exact | clean |
| rolling, matching legacy settings | 2 | 0 | 10 | 10 | 0 | 55.16 | exact | clean |
| rolling, matching legacy settings | 3 | 0 | 10 | 10 | 0 | 56.77 | exact | clean |
| intentional timeout, warp 1, `--max_time=1` | 1 | 1 | 10 | 0 | 10 | 41.23 | exact | clean |
| final successful checkout state, matching settings | 1 | 0 | 10 | 10 | 0 | 56.15 | exact | clean |

The three matched rolling matrices published 30/30 passing rows and all three
launchers exited zero. Wall time was 56.70 seconds mean, 56.77 median, 55.16
minimum, and 58.18 maximum. Compared with the matching legacy mean of 57.74
seconds, the migration reduced mean wall time by 1.04 seconds, or about 1.8
percent. The separate final matrix restored the checked-out `results.txt` to a
complete 10/10 passing state and is not included in that comparison.

The intentional timeout matrix proved continued execution after ordinary
failure: every worker exited nonzero, all ten selected cases still published
one ordered `reason=missing_result launch_rc=1` row, and the launcher returned
nonzero only after aggregation.

Before cleanup, the retained first rolling run contained ten case directories, ten intermediate
rows, twenty MOOS targets, ten behavior targets, all nineteen intended
`.moosx` overlays, and no `.bhvx` overlays. It contains twenty unique MOOSDB
ports from 24000 through 24271 and twenty unique pShare ports from 24015
through 24286 in exact 30-port blocks. Verbose events directly show rolling
refill after each completion. The inspected temporary paths were:

```text
harnesses/obmgr_harnesses/H02-obmgr_motion/.harness_runs/
  run_20260713T012620_9679/   # preparation and standalone-stem evidence
  run_20260713T013209_25611/  # passing rolling run 1
  run_20260713T013607_42780/  # intentional ten-row timeout run
```

The final repository gate passes both skill static checkers, Bash syntax,
ShellCheck, dependency metadata `--check`, the 43-test unit suite, repository
invariants, all 67 target-registry entries, and the ten-row result summarizer.
The dependency graph records ten parsed cases, no parse error, and the
canonical `scripts/moos_scoped_teardown.sh` helper.

#### Skill 1.4.2 alignment: `obmgr_h02`

The refreshed-skill audit on July 14, 2026 left the ten-case matrix, nineteen
overlays, expected-collision semantics, geometry, and application configuration
unchanged. It applied the same two wrapper repairs proven in `cmgr_h01`: Apple
Bash 3.2 now handles empty GUI, forwarding, and result arrays safely, and
nonzero stem exits retain valid mission evidence as renamed `mission_*`
provenance. The historical `two_sequential_fail` mapping to the
`two-sequential-pass-*` filenames remains explicit and unchanged.

The nominal `baseline_center_pass` case passed in 11.69 seconds. The
expected-collision `wide_center_fail` case passed in 10.83 seconds with
`grade=pass expected=collision collisions=1 encounters=1`. The isolated serial
matrix passed 10/10 in exact order in 112.37 seconds. Three canonical-default
rolling matrices passed 30/30 rows in 58, 58, and 57 seconds, for a 57.67-second
mean. That is 0.97 seconds above the earlier 56.70-second mean and remains
inside the earlier 55.16-to-58.18-second range, so no meaningful performance
regression is observed.

A retained 58.25-second rolling matrix contained ten case directories, ten
rows, twenty MOOS targets, ten behavior targets, all nineteen intended
`.moosx` sidecars, and forty unique listener ports from 28000 through 28286.
All four expected-collision cases retained mission-owned passing rows. Verbose
timestamps showed immediate rolling refill, original `.alog` files contained
no run warnings, and no scoped process, listener, safety lock, source sidecar,
or retained run tree remained after inspection. The standalone stem also
passed with explicit ports 29000/29001 and 29015/29016.

### Migrated Pilot: `ufld_obstacle_sim_h01`

Migration began only after `obmgr_h02` was closed, and no other harness was
launched while this pilot was active. The dependency audit confirmed that
`missions/ufld_obstacle_sim_missions/ufld_obstacle_sim_unit` has one harness
consumer. This was therefore a dedicated-stem migration; it did not require a
change to the seven-consumer `ufield_app_unit` shared stem or coordinated
validation of that separate family.

#### Legacy verdict audit

The 49-case matrix and README tokens matched, and the 48 nonbaseline cases all
had explicit historical shoreside overlays. The important legacy finding was
that `pMissionEval` did not own the detailed verdict for those 48 cases. Their
patches graded only `TEST_EVAL_READY=true`; the launcher then inspected `.alog`
files for publication counts, absence, labels, scalar values, and visual
payload fields. A mission row could therefore say `grade=pass` while the shell
exited nonzero. This was legacy implementation that had not yet been cleaned
up, not an intentional expected-negative exemption.

The missing-file, suppressed-publication, zero-rate, invalid-input, and reset-
blocked cases remain ordinary harness cases. Their expected negative subject
evidence is now expressed as a passing `pMissionEval` contract, normally by a
direct `fail_condition` on the subject's existing output variable. Only the
already documented failure-machinery and CLI harnesses need
outer-versus-subject verdict exemptions.

The baseline's fixed-case first-run failure was also classified more precisely:
all 49 mission rows said pass, but a supplemental shell check lost an expanded
macro and returned nonzero. Three full legacy matrices took 141.11, 138.08,
and 139.80 seconds, for a 139.66-second mean; all 147 mission rows passed and
two of three launchers exited zero. Five focused fixed-case repeats passed.

#### Mission-owned evidence implementation

The first implementation attempt added a generic evidence-monitor app. Review
showed that this was an unnecessary architectural expansion: the repository
did not need a new C++ application merely to migrate one harness. That
prototype, its build wiring, rules file, and tests were removed before the
final design. No `pHarnessEvidence` source, executable, generated target, or
runtime dependency remains.

The final design uses the smallest existing evidence surface that preserves
case intent:

- Plain scalar, presence, absence, and count checks use existing subject
  variables.
- Exact structured equality uses timer-published expected variables and the
  parser-safe `$ (EXPECTED_VAR)` right-variable form.
- One stock `pEchoVar` instance supplies narrowly filtered field/history
  sentinels only where raw current values are insufficient.
- Most `pEchoVar` output is checked for presence or plain components. The one
  exact composite check compares against a runtime expected variable; no
  `key=value` payload is embedded as a condition literal.
- Counters exist only where publication multiplicity is the behavior under
  test.

This leaves `pMissionEval` as the sole owner of every ordinary `grade=` row and
adds no repository application code. The evaluator and `pEchoVar` start before
`uFldObstacleSim`, and each replaced timer block waits for all three clients
before beginning its event clock.

The final semantic audit tightened the reset and point contracts without
adding a grading application or unnecessary expected-value variables:

- Four repost-after-reset cases no longer send a redundant `PMV_CONNECT` before
  reset. Their first `NODE_REPORT` now produces the sole pre-reset obstacle
  post, so `KNOWN_OBSTACLE_COUNT >= 2` necessarily proves a post-reset repost.
- The inactive-A adapter maps both `active` and `label` and compares the exact
  composite value to a runtime expected value. This prevents an unlabeled
  `active=false` record from satisfying the case. The case reuses its existing
  `UFOS_EXPECT_OBSTACLE` variable rather than adding another expected variable.
- Four redundant yellow/cyan point helpers were replaced by one strict beta
  label helper. The size cases grade the existing size evidence directly, and
  the multi-vehicle case requires distinct alpha and beta point and tracked-
  feature evidence. This is a net reduction of three adapter variables.
- The missing-file case is intentionally fail-only: absence of subject output
  is sufficient, so its vacuous `DB_UPTIME > 0` pass condition was removed.

#### Overlay and launcher migration

The historical case overlays remain the explicit source of timer, subject, and
evaluator configuration. Each of the 48 nonbaseline cases now uses one direct
`nspatch` operation from the copied `meta_shoreside.moos` to
`meta_shoreside.moosx`. The default fixed case uses the copied stem without a
sidecar. There is no intermediate mission file, evaluator patch, or shared-stem
change outside this dedicated uFld mission.

The modern harness launcher now provides:

- Bash 5.1 guard plus safe Homebrew/Linuxbrew re-execution
- a separate harness-owned copy and 30-port block for every case in serial and
  rolling runs
- rolling `wait -p -n` scheduling with immediate refill
- explicit 49-case patch mapping and a single invocation lock
- direct forwarding of `--max_time` to the stem wrapper
- canonical root-scoped teardown, including signal paths
- one intermediate result per case and deterministic selected-order aggregation
- strict missing, duplicate, malformed, invalid-grade, preparation, and
  teardown normalization
- continuation after ordinary failures and a hard zero-row/count mismatch gate
- runner-owned failure normalization when the stem wrapper exits nonzero, with
  every usable mission field retained as provenance under nonreserved names
- retained run root and invocation lock after infrastructure cleanup failure, so
  evidence cannot be silently deleted or overwritten before diagnosis

The stem wrapper accepts the generic port aliases and `--mmod`, validates its
target in preparation mode, requires a live `grade=`, and uses canonical
teardown. The advertised `--gui` option was removed because this headless stem
contains no viewer; accepting it had been misleading.

#### Validation evidence

The parser audit found 95 structured conditions across 42 cases whose quoted
right-hand literals contained `=`. `LogicCondition` scans relation characters
inside quotes, so those lines were rejected rather than escaped; six cases
were consequently grading readiness alone. The corrected overlays use plain
conditions, runtime right-variable comparisons, and narrow stock adapters.
Static tests now reject any recurrence of a literal structured RHS.

Current parser-corrected probes:

| probe | exit | rows | result |
| --- | ---: | ---: | --- |
| harness skill static checker | 0 | n/a | pass |
| eval-stem static checker | 0 | n/a | pass, expected unit-style `pAutoPoke` warning |
| Bash syntax and ShellCheck | 0 | n/a | pass |
| 22 focused uFld and teardown contract tests | 0 | n/a | pass |
| Bash 3.2, re-exec disabled | 2 | 0 | clear Bash 5.1 requirement |
| Bash 3.2, normal `--help` | 0 | 0 | re-executed Homebrew Bash 5.3.9 |
| unknown case, `--jobs=0`, and overflowing port range | 2 | 0 | clear diagnostics, no stale lock |
| 49-case retained `--just_make --jobs=1` | 0 | 49 | 49 s; 49 targets, 48 sidecars, unique ports/modifiers |
| nominal fixed case | 0 | 1 | exact obstacle values and two visuals passed |
| final direct-stem fixed case | 0 | 1 | authoritative row refreshed with exact `obstacle_view=label=ufos_a` |
| point-size numeric case, five repeats | 0 | 5 | every run passed in 5-6 s |
| corrected point-size sequence, five repeats | 0 | 5 | every two-stage run passed in 5-6 s |
| missing-file expected absence | 0 | 1 | no subject outputs; mission-owned pass |
| forbidden `PMV_CONNECT` injected in retained absence case | 0 | 1 | mission-owned `grade=fail`, proving the direct fail condition |
| reset ID replacement | 0 | 1 | exact inactive-A composite, clear, and final `label=ob_1` passed |
| exact three-point case, six focused repeats | 0 | 6 | every row reported `tracked_count=3` |
| two semantic mutation probes | 0 | 1 each | wrong inactive label and beta filter each produced mission-owned `grade=fail` |
| direct stem invalid-argument probes | 2 | 0 | zero warp/time and empty, zero, or out-of-range port rejected |
| complete repository test suite | 0 | 68 | pass |

Runner-only probes also covered empty, missing, duplicate, malformed, and
invalid-grade mission results; preparation failure; a pre-made invocation
lock; continued ordered aggregation after ordinary failures; signal cleanup;
and scheduler abort after a scoped-teardown infrastructure failure.

##### Historical pre-parser-repair migrated measurements

These runs exercised rolling scheduling, isolated copies, result ordering,
ports, and cleanup. Their structured `pMissionEval` assertions were not all
active, so their reported pass rows are not validation of the corrected case
contracts. The high port bases were safe on the recorded macOS host but are
host-specific and are not portable examples for Linux hosts whose ephemeral
range commonly starts at 32768.

| configuration | runs | rows reported pass | wall seconds | order | cleanup |
| --- | ---: | ---: | --- | --- | --- |
| isolated serial, `--jobs=1 --port_base=41000` | 1 | 49 / 49 | 275.69 | exact | clean |
| rolling, retained, `--jobs=2 --port_base=43000` | 1 | 49 / 49 | 143.43 | exact | clean |
| rolling, default cleanup, `--jobs=2 --port_base=45000` | 4 | 196 / 196 | 140.14, 142.90, 141.66, 140.37 | exact | clean |

The five pre-repair rolling runs published 245/245 readiness-style passing
rows. Wall time was 141.70 seconds mean, 141.66 median, 140.14 minimum, 143.43 maximum, and a
3.29-second observed range. The matching legacy mean was 139.66 seconds, so
the final mean is 2.04 seconds, or about 1.46 percent, higher. The legacy and
final ranges overlap, and the difference is smaller than either sample's
observed spread. It is launcher timing evidence only, not evidence for or
against the cost of the corrected grading contract.

##### Parser-corrected live matrices

The first corrected serial matrix exposed one timing race in
`point_size_sequence_pass`: uFld correctly published size 3 and then size 2,
but the first pMissionEval aspect checked the timer's transient `bigger` input
one iterate after it had advanced to `smaller`. The fix grades that stage from
the already observed size-3 output, while the second stage still requires the
final `smaller` input and observed size-2 output. No state variable was added.

| configuration | runs | passing rows / rows | wall seconds | order | cleanup |
| --- | ---: | ---: | --- | --- | --- |
| diagnostic serial, `--jobs=1 --port_base=30000` | 1 | 48 / 49 | 286 | exact | clean |
| definitive serial, `--jobs=1 --port_base=12000` | 1 | 49 / 49 | 282 | exact | clean |
| rolling, retained, `--jobs=2 --port_base=15000` | 1 | 49 / 49 | 149 | exact | clean |
| rolling, default cleanup, `--jobs=2`, bases 17000-23000 | 4 | 196 / 196 | 142, 145, 145, 160 | deterministic | clean |

The five corrected rolling runs published 245/245 passing rows. Wall time was
148.20 seconds mean, 145 median, 142 minimum, 160 maximum, and an 18-second
observed range. Relative to the 139.66-second legacy mean, the corrected mean
is 8.54 seconds (6.11 percent) higher. Relative to the pre-parser-repair
launcher mean, it is 6.50 seconds (4.59 percent) higher. The median difference
is smaller, and the 160-second run is a clear timing outlier with no correctness
or cleanup anomaly. Retained per-case timestamps show the difference spread
across many one-second shifts and several reset cases, not one copy/setup
bottleneck. Corrected missions also perform detailed evidence checks that the
pre-repair evaluator silently omitted, and the host load varied during the
sample. Record the end-to-end difference, but do not attribute it to isolated
copies, the watchdog, or any other single mechanism without a controlled
profile.

Before generated-artifact cleanup, the retained corrected serial and rolling matrices each contained 49 case
directories, 49 mission targets, 49 intermediate result rows, and 48 intended
`.moosx` sidecars. Generated MOOSDB ports match their distinct 30-port blocks;
no assigned ports retained listeners. Generated targets contain one evaluator,
one stock adapter, one subject, the required timer barrier, a unique mission
modifier, no unresolved nsplug macros, and only intentional pMissionEval
right-variable references. Runtime warning searches and scoped-process checks
were clear.

A historical rolling diagnostic at base 53000 produced 48 passing cases and
one MOOSDB startup failure because `apsd` held port 53660 as an outbound client.
It also demonstrated that `uMayFinish --max_time` cannot expire before it
connects to MOOSDB. An independent watchdog, ephemeral-range policy, and
process-group supervisor were implemented and validated temporarily, then
removed after review because they exceed the current harness-skill baseline.
The final design uses explicit low port bases for validation and accepts the
baseline limitation that a pre-MOOSDB startup failure may require operator
intervention. Broader pre-MOOSDB supervision belongs in a separate skill-design
decision, not this migration.

##### Final skill-baseline closure

After removing that extension, retained `--just_make --jobs=1` validation
produced 49 isolated mission copies, 49 targets, 48 intended sidecars, unique
port blocks, and no supervision metadata. Focused live runs for the nominal,
expected-absence, and reset-repost cases all passed. The final direct-stem row
also passed with the current `obstacle_view=label=ufos_a` evidence column.

The definitive simplified serial matrix passed all 49 cases in exact declared
order in 289 seconds. Four simplified rolling matrices also passed 196/196
rows with exact ordering and clean scoped teardown:

| configuration | runs | passing rows / rows | launcher wall seconds | performance use |
| --- | ---: | ---: | --- | --- |
| rolling, `--jobs=2 --port_base=15000` | 1 | 49 / 49 | 148 | clean timing sample |
| rolling, `--jobs=2`, later bases/repeat | 3 | 147 / 147 | 147, 143, 145 | correctness only; external ArduPilot/Briggs activity overlapped |

The one uncontaminated simplified rolling timing is two seconds below the
temporary supervised design's 150-second mean and lies inside every prior
corrected range. One sample is not enough to claim a performance improvement,
but there is no evidence that simplification slowed the harness. The later
rows remain valid correctness evidence because they used separate ports and
workdirs; their timings are excluded rather than averaged.

After the harness skill was refreshed to version 1.4.2, the repository teardown
helper was replaced with the new asset verbatim and its executable mode was
retained. A byte comparison against the installed asset passes. The refreshed
asset supplies the unset-extra-app default and propagates process-discovery
failure through its checked public path, so the earlier local compatibility
edits and stricter private lsof policy are no longer needed. The focused tests
now target that published contract rather than requiring local behavior absent
from the asset.

Post-refresh live validation kept one harness active at a time. The nominal
`fixed_field_publish_pass` case passed in 6.50 wall seconds, the expected-
absence `missing_obstacle_file_absent_pass` case passed in 6.76 wall seconds,
and a complete rolling `--jobs=2 --port_base=33000` matrix passed 49/49 rows in
146.80 wall seconds. The complete run lies inside the earlier 143-148 second
simplified range. Each run left no scoped process, safety lock, run root, or
assigned listener.

The final static gates comprise the two skill checkers, Bash syntax,
ShellCheck, 21 focused uFld/teardown tests, and all 67 repository tests. No uFld
run root, lock, assigned listener, generated log, or scoped uFld process
remained after validation. The generated dependency graph check and
documentation build also pass.

Temporary `.harness_runs` evidence is removed after these findings are
recorded so generated logs do not appear as hundreds of thousands of review
lines. The catalog parser continues to accept both legacy `ALL_CASES` and
modern `CASES` arrays while preserving declared order.

### Migrated Rollout: `cmgr_h02`

This rollout began only after all three pilots were closed. No other harness
was run while `cmgr_h02` baseline or migrated measurements were active.

The read-only audit confirmed one dedicated mission stem, fifteen launcher
cases, and twenty-four historical patch files. The matrix already used
`pMissionEval` correctly: expected encounter, disabled-alert, and late-alert
outcomes were expressed as mission-owned passing contracts. No evaluator
condition, patch file, mission geometry, or case-to-patch mapping changed.

The legacy launcher used isolated copies only for parallel runs, scheduled
parallel work in barrier-separated waves, called the legacy teardown wrapper,
and stopped a serial matrix after the first ordinary failure. Its direct stem
wrapper rejected the explicit ports and `--mmod` required by the current
harness contract. The migrated launcher now gives every selected case an
isolated copy and distinct 30-port block, uses Bash 5.1 rolling scheduling,
forwards all four ports plus `--mmod` and `--max_time`, performs canonical
root-scoped teardown, and aggregates exactly one normalized row per selected
case in declared order. It adds no watchdog, process-group supervisor, grading
application, or new mission variable.

Legacy measurements at time warp 10 and `--max_time=70` were:

| configuration | runs | passing rows / rows | wall seconds |
| --- | ---: | ---: | --- |
| serial, `--jobs=1 --port_base=30000` | 1 | 15 / 15 | 158.32 |
| batch-wave, `--jobs=2`, bases 31000-33000 | 3 | 45 / 45 | 93, 93, 95 |
| focused `stale_reappear_pass`, fresh bases | 5 | 5 / 5 | 12, 11, 11, 11, 11 |

The legacy batch-wave mean was 93.67 seconds. In the complete serial run,
`stale_reappear_pass` reached the outer `uMayFinish` ceiling but still wrote
its valid passing row during shutdown. All five focused repeats completed
normally, so this was recorded as a timing outlier rather than a grading or
launcher defect.

Migration probes covered both skill static checkers, Bash syntax, Bash 3.2
re-execution and disabled-re-execution behavior, unknown and invalid arguments,
port overflow, a retained fifteen-case preparation matrix, a standalone stem
run, nominal and expected-negative live cases, and missing, duplicate,
launch-error, preparation-error, and teardown-error normalization. The
preparation matrix contained fifteen isolated copies, thirty MOOS targets,
fifteen behavior targets, twenty-four intended `.moosx` sidecars, no `.bhvx`
sidecars, and unique MOOSDB and pShare ports. Source templates remained clean.

Complete migrated results were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| first isolated serial, diagnostic | 1 | 14 / 15 | 170 | clean |
| definitive isolated serial | 1 | 15 / 15 | 171 | clean |
| rolling, `--jobs=2`, bases 37000-39000 | 3 | 45 / 45 | 90, 91, 90 | clean |
| final checkout state after runner hardening, `--jobs=2 --port_base=27000` | 1 | 15 / 15 | 89.58 | clean |
| focused `fast_intruder_fail`, fresh bases | 5 | 5 / 5 | 12, 11, 11, 12, 11 | clean |

The first migrated serial matrix's only failure was
`fast_intruder_fail`, which observed two encounters against the unchanged
strict requirement of one. All five focused repeats observed exactly one
encounter, and the definitive serial plus all three rolling matrices passed.
Classification: non-reproducing timing or trajectory outlier. The assertion
was deliberately not weakened or otherwise redesigned during migration.

The migrated rolling mean was 90.33 seconds, 3.34 seconds (about 3.6 percent)
below the matched legacy mean. This is operational evidence that isolated
copies and rolling refill did not degrade throughput on this sample, not a
claim that launcher migration makes every harness intrinsically faster.
Verbose timestamps showed immediate refill, aggregation order remained exact,
and retained targets proved the assigned ports. No scoped process or listener
remained after any measured run.

A final read-only review identified three runner-schema edge cases before the
harness was closed. Empty `--case=` is now rejected instead of selecting the
full matrix; the standalone stem requires exactly one `grade=` field, not just
one result row; and a per-case teardown error is now treated as infrastructure
failure. The latter stops scheduler refill, writes explicit aborted rows for
all pending cases, and retains the run root and safety lock even if a later
cleanup probe succeeds. A transient second-probe failure was injected with
`--jobs=1 --just_make`: one case reported `teardown_error`, fourteen pending
cases reported `scheduler_aborted_after_teardown_error`, and the root and lock
were retained. No process-group or watchdog mechanism was added. The normal
post-fix rolling matrix then passed 15/15 in 89.58 seconds with clean teardown.

Every retained vehicle `.alog` contained the same finite-waypoint advisory:
`BHV_WARNING waypt_transit: No waypts given. Set val to empty if intentional`.
The behavior and mission configuration were unchanged, so the advisory is
recorded for later case-quality work and was neither suppressed nor added to
grading.

### Legacy Rollout Baseline: `obmgr_h01`

The target was clean before measurement and was not run concurrently with any
other harness. The dependency map records one dedicated consumer of
`missions/obmgr_missions/obmgr_unit`. The legacy matrix contains thirty cases
and fifty-nine `.xmoos` overlays; README tokens, launcher cases, setup mappings,
and all `pMissionEval mission_mod` values agree.

Legacy measurements used time warp 10 and `--max_time=70`:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| batch-wave, `--jobs=2`, bases 11000, 13000, 17000 | 3 | 90 / 90 | 123.24, 121.97, 121.55 | clean |
| shared-stem serial, `--jobs=1 --port_base=19000` | 1 | 30 / 30 | 182.36 | clean |

The three batch-wave runs had a 122.25-second mean, 121.97-second median,
121.55-second minimum, 123.24-second maximum, and 1.69-second range. The first
run retained evidence long enough to inspect thirty case directories, thirty
intermediate rows, sixty MOOS targets, thirty behavior targets, all fifty-nine
intended `.moosx` sidecars, no `.bhvx` sidecars, sixty original `.alog` files,
sixty unique MOOSDB ports, and sixty unique pShare listener ports. All thirty
rows passed, the four shell-checked payloads were present exactly as expected,
no behavior warning was logged, and no assigned listener or scoped process
remained. The retained tree was removed after inspection.

Static inspection found two grading-contract issues that must not be hidden by
the launcher migration:

- All thirty `EXPECTED` values are `pass`, so the expected/actual shell layer
  is redundant. Four payload-format cases additionally inspect result text in
  the shell; a missing token can make the launcher fail while leaving a
  published `grade=pass` row. These checks need mission-owned equivalents, not
  silent removal or a new exemption.
- Seven expected-absence patches embed structured alert values containing
  additional `=` relations directly in `LogicCondition`. The parser rejects
  those right-hand literals, so the alert-specific fail branch is ineffective
  even though other case conditions made every baseline row pass. Their
  intended absence contract can be represented directly as
  `fail_condition = OBSTACLE_ALERT != ""` without a new variable or app.

The legacy harness static checker therefore fails canonical teardown and warns
about batch barriers, redundant grade comparison, zero-row handling, and old
pShare offsets. The stem checker fails missing-grade detection and canonical
teardown. A no-launch argument probe also confirms that the stem rejects
`--mmod` and all explicit harness ports even though its underlying launch chain
already forwards those values and consumes sidecars with `nsplug -x`.

### Migrated Rollout: `obmgr_h01`

The migrated launcher preserves the thirty case tokens, their declared order,
all fifty-nine historical overlay mappings, and the dedicated `obmgr_unit`
stem. Every selected case now runs from its own isolated mission copy, including
serial runs. Each copy receives a stride-30 port block with MOOSDB ports at
`+0/+1` and pShare ports at `+15/+16`. Bash 5.1 rolling scheduling replaces the
legacy barrier-separated batches, and the launcher forwards `--max_time`,
`--mmod`, and all four ports through the repaired stem wrapper. The shell no
longer compares a redundant all-pass `EXPECTED` table or searches logs for the
verdict. It aggregates exactly one mission-owned row per selected case in
declared order and reserves its own failures for preparation, launch, result
schema, scheduler, and teardown errors.

The grading repair did not add an application. Seven no-alert cases now express
their intended contract with the parser-safe
`fail_condition = OBSTACLE_ALERT != ""`. Four former shell payload checks use
the existing `uTimerScript` to publish one reused internal right-hand value,
`OBM_EXPECTED_PAYLOAD`, which `pMissionEval` compares against the observed
field. A right-hand variable is necessary because `LogicCondition` rejects a
literal containing embedded `=` relations. The two general-alert comparisons
are materially equivalent to their former full-payload tokens. The point
vsource and vsource-disable comparisons deliberately require the complete
current payload, whereas the legacy shell required only a substring. This is
the simplest mission-owned representation available without a new parser app,
but it is recorded as a stricter migration form so that future serialization
changes are not mistaken for an unexplained regression.

Migration preparation and live validation covered both skill static checkers,
Bash syntax, ShellCheck, Bash 3.2 re-execution and rejection, invalid CLI and
port-boundary handling, all fifty-nine overlays, a retained thirty-case
generation matrix, a standalone stem launch, all seven repaired absence cases,
all four moved payload cases, an isolated serial matrix, and repeated rolling
matrices. The retained generation matrix contained thirty case directories,
sixty MOOS targets, thirty behavior targets, all fifty-nine intended `.moosx`
sidecars, no `.bhvx` sidecars, and sixty unique MOOSDB plus sixty unique pShare
ports. The first retained live rolling matrix additionally contained sixty
`.alog` files. Every case had one evaluator row with one `grade=pass`; no
behavior, configuration, parser, launcher, or scheduler warning was found.
Every later case started within zero or one second of a prior completion, and
no scoped process, listener, run lock, or generated source sidecar remained
after cleanup.

Complete live measurements at time warp 10 and `--max_time=70` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy shared-stem serial, `--jobs=1` | 1 | 30 / 30 | 182.36 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 30 / 30 | 218.18 | clean |
| legacy batch-wave, `--jobs=2` | 3 | 90 / 90 | 123.24, 121.97, 121.55 | clean |
| migrated rolling, canonical defaults, `--jobs=2` | 5 | 150 / 150 | 115.57, 108.39, 142.95, 123.22, 116.98 | clean |
| migrated rolling, supported one-second INT/TERM overrides | 1 | 30 / 30 | 113.88 | clean |

The legacy two-job mean was 122.25 seconds. The migrated canonical mean was
121.42 seconds, 0.83 seconds (about 0.7 percent) lower, with a
116.98-second median. Its 142.95-second run is retained in the statistics as a
timing outlier rather than discarded; it still passed all thirty cases and
cleaned up normally. A subsequent retained 123.22-second run showed case
durations of six through twelve seconds, with three through five seconds
typically occurring after the evaluator had already written its result.

Controlled cleanup probes explain part of that latency. Three representative
default cases averaged 7.35 seconds and three cases with the supported
one-second INT/TERM ceilings averaged 6.96 seconds. A verbose pair showed that
the default allowed all processes to finish during the interrupt grace period,
while the shorter ceiling escalated four remaining processes to `TERM` and
saved about one second in that sample. The canonical three-second defaults are
unchanged: they have a measurable but bounded graceful-shutdown cost here, and
the intended rolling throughput is statistically level with the legacy
baseline. The larger serial increase is recorded rather than optimized away;
serial now follows the same isolated-copy path as parallel execution, which is
the selected consistency contract.

Synthetic no-MOOS probes verified missing, duplicate, invalid-grade, and
launch-error result normalization with mission provenance retained. A forced
overlay preparation failure yielded thirty ordered rows: twenty-nine passes and
one `prepare_error`. A forced teardown-discovery failure with
`--jobs=1 --just_make` yielded one `teardown_error`, marked the remaining
twenty-nine cases `scheduler_aborted_after_teardown_error`, retained the run
root and safety lock, and started no additional case. The retained synthetic
tree was then checked and removed with the real canonical helper. A direct
stem probe also rejected a single result row containing two `grade=` fields.

A final read-only review reconciled every case, overlay, mission modifier, and
port formula against the legacy launcher and the corrected `cmgr_h02`
exemplar. It found no blocking implementation defect. Pre-existing case-quality
questions remain deliberately out of scope: enable/disable/expunge primarily
prove filter mail, expunge does not independently prove map removal, the point
trim case uses a broad band, and some absence cases lack a separate positive
witness. Those assertions were not weakened or redesigned during this
migration.

### Legacy Rollout Baseline: `pid_h02`

The target was audited and measured as the only live harness. The dependency
map records one dedicated consumer of `missions/pid_missions/pid_motion`. The
README, launcher order, case setup function, and all thirteen `pMissionEval`
mission modifiers agree. The matrix contains thirteen cases, twenty-two
`.xmoos` overlays, and three `.xbhv` overlays. Its five expected-degraded
`*_fail` cases already use ordinary mission-owned expected-negative semantics:
the evaluator writes `grade=pass` when the requested degraded motion is
observed. There is no shell `EXPECTED` table and no log-owned verdict.

Legacy measurements used time warp 10 and `--max_time=80`:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| batch-wave, `--jobs=2`, bases 48000, 50000, 52000 | 3 | 39 / 39 | 82.91, 84.69, 81.80 | clean |
| shared-stem serial, `--jobs=1 --port_base=54000` | 1 | 13 / 13 | 122.79 | clean |

The three two-job runs had an 83.13-second mean, 82.91-second median,
81.80-second minimum, 84.69-second maximum, and 2.89-second range. The first
run retained evidence long enough to inspect thirteen case directories,
thirteen intermediate rows, twenty-six MOOS targets, thirteen behavior
targets, all twenty-two `.moosx` and three `.bhvx` sidecars, twenty-six
original `.alog` files, twenty-six unique MOOSDB ports, and twenty-six unique
pShare ports. All rows passed in declared order, and no assigned listener or
scoped process remained. The retained tree was removed after inspection. An
unrelated application happened to listen on ports 50067 and 50068 during the
second sample; neither port belonged to the harness's stride-30 allocation,
and the unrelated process was left untouched.

Static inspection found form issues, but no shell-owned grading to migrate:

- The legacy parallel path uses batch barriers, while serial execution patches
  the shared source stem. Serial failure stops the matrix, `--just_make`
  suppresses errors, result parsing does not require exactly one row with one
  valid grade, and teardown uses the legacy helper without checking its return
  status.
- The harness bypasses its stem wrapper and calls `xlaunch.sh` directly. The
  wrapper does not accept the case modifier or explicit ports, does not
  preserve launch failure, and always exits zero after using the legacy
  teardown helper. The underlying `launch.sh` and sublaunchers already forward
  the four ports and use `nsplug -x` correctly.
- Seven evaluator configurations express arrival-or-deadline readiness as a
  compound lead condition. Runtime behavior is valid, but the current skill
  requires a simple lead variable. The existing `TEST_EVAL_READY` signal can
  retain both triggers: an `ARRIVED` mailflag sets it true while the existing
  timer continues to set it at the deadline. This needs no new variable or
  application.

The retained logs also establish two pre-existing diagnostics that are outside
this form migration. Twelve cases record a transient `No waypts given`
advisory exactly as the waypoint behavior spawns; the same timestamp contains
the populated waypoint settings, the advisory is retracted, and motion proceeds
normally. After their passing verdicts, `depth_step_pass` and
`depth_control_disabled_fail` can reach the later x=140 waypoint during
shutdown grace and emit `MissingDecVars:speed,course` from the still-active
depth behavior. Neither diagnostic is part of grading, and neither case's
coverage or lifecycle is being redesigned in this migration.

### Migrated Rollout: `pid_h02`

The migrated launcher preserves all thirteen case tokens, their declared
order, all twenty-two `.xmoos` and three `.xbhv` mappings, and the dedicated
`pid_motion` stem. Every selected case now runs from its own isolated mission
copy, including serial and one-case runs. Each copy receives a stride-30 port
block with MOOSDB at `+0/+1` and pShare at `+15/+16`. Bash 5.1 rolling
scheduling replaces legacy barrier waves, and the harness calls the copied
stem wrapper with `--max_time`, `--mmod`, GUI mode, time warp, and all four
ports. Exactly one normalized row is aggregated per selected case in declared
order. Ordinary mission failures do not stop the matrix; an unprovable
teardown stops refill and retains the diagnostic root plus lock. No watchdog,
process-group mechanism, application, or grading variable was added.

The migrated default base is the skill-standard 9000. The legacy launcher
declared 26000, but used that value only in its parallel path; default serial
execution silently reused the stem's 9000-range ports. Explicit
`--port_base=<n>` remains available for fresh or CI allocations.

The stem wrapper now accepts and forwards the existing modifier and port
contract, validates all three targets in `--just_make`, requires exactly one
live row with exactly one `grade=pass|fail` field, preserves a nonzero launch
return code, and uses canonical root-scoped teardown. The launch chain beneath
it was already correct and remains unchanged.

Seven full evaluator blocks formerly expressed readiness with a parenthesized
arrival-or-deadline OR. They now use the existing `TEST_EVAL_READY` signal as
a simple lead. `mailflag = @ARRIVED#TEST_EVAL_READY=true` provides the arrival
path, while each existing timer retains the deadline path. The current mission
has one one-shot `ARRIVED=true` producer and no false initialization, so this
is semantically equivalent apart from a sub-cycle delivery delay. Retained live
evidence showed every one of the seven cases receiving the mailflag output
roughly 0.25 through 0.59 mission-seconds after arrival and evaluating before
its deadline. Pass conditions, timer deadlines, report columns, PID settings,
behavior patches, and expected-degraded contracts were not changed.

Validation covered both skill static checkers, Bash syntax, ShellCheck, Bash
3.2 rejection and Homebrew re-execution, empty and unknown cases, invalid jobs,
port overflow, a retained full `--just_make` matrix, a standalone stem
preparation and 9.06-second live pass, nominal and expected-degraded harness
cases, an isolated serial
matrix, three rolling matrices, and runner/infrastructure failure paths. The
retained generation matrix contained thirteen copies, twenty-six MOOS targets,
thirteen behavior targets, all intended twenty-two `.moosx` and three `.bhvx`
sidecars, twenty-six unique MOOSDB ports, and twenty-six unique pShare ports.
The retained rolling matrix added twenty-six `.alog` files and showed same-
second refill after every completion. It had the known transient waypoint
advisory, no configuration warning, and no helm error; the shorter canonical
shutdown tail did not reach the two post-verdict legacy depth errors.

Complete live measurements at time warp 10 and `--max_time=80` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy shared-stem serial, `--jobs=1` | 1 | 13 / 13 | 122.79 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 13 / 13 | 142.47 | clean |
| legacy batch-wave, `--jobs=2` | 3 | 39 / 39 | 82.91, 84.69, 81.80 | clean |
| migrated rolling, `--jobs=2` | 3 | 39 / 39 | 79.41, 81.55, 84.10 | clean |
| final checkout-state rolling, clear base 30000 | 1 | 13 / 13 | 74.37 | clean |

The migrated rolling mean is 81.69 seconds, with an 81.55-second median,
79.41-second minimum, 84.10-second maximum, and 4.69-second range. It is 1.45
seconds, about 1.7 percent, below the matched 83.13-second legacy mean. The
migrated serial sample is 19.68 seconds, about 16 percent, slower because the
selected consistency contract now pays case-copy and scoped-cleanup costs on
every serial case. This cost is recorded rather than optimized through a
second direct-stem serial path.

Two additional successful checkout-state matrices, 76.88 and 74.37 seconds,
are not included in the matched three-run comparison. After the first of those,
an unrelated `Creative` process was listening on TCP 64346, numerically equal
to one case's UDP pShare slot. It was outside this repository, did not conflict
with pShare, and was left untouched. The definitive 74.37-second run therefore
used base 30000 only after every assigned slot was clear on both TCP and UDP;
the same dual-protocol check was clear after completion.

No-MOOS runner probes verified missing, duplicate, invalid-grade, nonzero-
launch, and preparation-error normalization. A valid mission row produced
before launch return code 7 was retained as `mission_*` provenance. A live
warp-1, one-second ceiling produced one `missing_result` row in 8.20 seconds
and cleaned all four ports. A forced teardown-discovery failure with
`--jobs=1 --just_make` started only the first case, wrote one `teardown_error`
plus twelve ordered `scheduler_aborted_after_teardown_error` rows, and retained
the root and safety lock until both were inspected and cleared with the real
helper.

### Legacy Rollout Baseline: `pid_h01`

PID H01 was audited and measured as the only live harness on July 14, 2026,
from revision `5b77ab600` on the Apple Silicon macOS host `becker`. The
dependency map records one dedicated consumer of
`missions/pid_missions/pid_unit`. The README, launcher order, and all
thirty-four `mission_mod` values agree exactly. `rudder_starboard_pass` is the
unpatched baseline; each of the other thirty-three cases maps one-to-one to a
shoreside `.xmoos` overlay. There are no behavior overlays or shared stem
consumers.

All thirty-four cases already follow mission-owned grading. Every evaluator
uses the existing simple `TEST_EVAL_READY=true` lead, writes one
`grade=$[GRADE]` report to `results.txt`, and sets `MISSION_EVALUATED=true`.
There is no shell `EXPECTED` table, `.alog` grading, or outer-versus-subject
result contract. The `simulation_mode_fail` and `thrust_cap_runtime_fail`
tokens are ordinary expected-negative scenarios: they grade pass when their
current documented bug evidence is observed. Existing hard-coded modifiers
already match the selected cases, so this migration does not need a new
grading variable, helper application, or `MMOD` launch path.

Legacy measurements used time warp 10, `--max_time=35`, explicit fresh port
bases, and the current canonical teardown implementation reached through the
legacy wrapper name:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| batch-wave, `--jobs=2`, bases 35000, 37000, 39000 | 3 | 102 / 102 | 110.77, 108.98, 108.92 | clean |
| shared-stem serial, `--jobs=1 --port_base=41000` | 1 | 34 / 34 | 170.52 | clean |

The three two-job samples have a 109.56-second mean, 108.98-second median,
108.92-second minimum, 110.77-second maximum, and 1.85-second range. The
first run was retained for inspection. It contains thirty-four case
directories, thirty-four intermediate result files, thirty-four targets,
thirty-four original `.alog` files, and the intended thirty-three sidecars.
Every mission wrote exactly one passing row, every modifier matched its case,
and the aggregate order matched the declared case order. Its thirty-four
MOOSDB ports were unique from 35000 through 35660. No assigned listener or
scoped process remained after any measured run.

This is a one-community mission and it does not launch pShare. The migrated
harness will retain the skill-standard stride-30 allocation and forward the
reserved shoreside pShare slot for launcher consistency, but it will not add a
pShare process. Live listener validation must therefore require only the one
MOOSDB listener per active case.

The retained baseline shows no migration-blocking runtime anomaly. Static
inspection found only the established legacy launcher weaknesses: batch
barriers rather than rolling refill; shared-stem serial preparation; stopping
serial execution after the first ordinary failure; loose result parsing;
blank aggregate lines; no run lock or final selected-result count assertion;
unchecked legacy teardown; broad process-group signal handling; and bypassing
the stem wrapper. The stem wrapper does not currently forward ports, preserve
the launch return code, validate its generated target, require one valid
mission row, or call the canonical helper directly.

Four existing coverage choices are recorded for later case-quality work and
will not be changed during form migration: `heading_debug_saturation_pass`
reports its debug string without grading it; `stale_nav_pass` reports the
stale message while grading zero outputs; some temporal or absence cases grade
the final value rather than the full publication history; and
`output_suffix_alt_pass` does not separately assert that unsuffixed outputs
are absent.

### Migrated Rollout: `pid_h01`

PID H01 now uses the settled one-community form of the PID H02 launcher. The
harness requires Bash 5.1 for rolling scheduling, creates an isolated mission
copy for every selected case, and calls that copy's stem wrapper. Each case
receives a stride-30 allocation with MOOSDB at `+0` and a reserved/forwarded
shoreside pShare slot at `+15`; no pShare process was added. The source stem is
never patched in place. The default port base is the skill-standard 9000.

The launcher preserves all thirty-four tokens and all thirty-three explicit
overlay mappings. It aggregates exactly one normalized row per selected case
in declared order, continues after ordinary mission failures, preserves a
valid mission row as `mission_*` evidence when launch fails, and stops refill
only when scheduler state or scoped teardown cannot be trusted. In that
infrastructure path it records ordered aborted rows and retains the diagnostic
root and safety lock. No watchdog, global cleanup, process-group extension,
application, modifier variable, or test-semantic change was introduced.

The stem wrapper now accepts the two one-community port arguments and forwards
them with time warp, `--max_time`, and display mode. It validates
`targ_shoreside.moos` under `--just_make`, requires exactly one live result row
with exactly one valid grade, preserves the `xlaunch.sh` return status, and
uses the canonical root-scoped teardown helper. The underlying `launch.sh`
already used `nsplug -x` and the explicit shoreside port substitutions, so it
did not change.

No pMissionEval configuration changed. All thirty-four evaluators still use
their existing simple `TEST_EVAL_READY=true` lead, pass conditions, report
columns, and hard-coded modifier. The two `_fail` cases still produce
mission-owned `grade=pass` only for `simulation_zero_output` and
`runtime_cap_ignored` evidence. This is a launcher-form migration, not a case-
coverage rewrite.

Validation covered both skill static checkers, Bash syntax, ShellCheck, Bash
3.2 rejection and safe Homebrew re-execution, invalid CLI and port bounds, a
retained full generation matrix, an independent 4.99-second stem pass, an
isolated nominal pass, both expected-negative cases, three complete rolling
matrices, one complete isolated serial matrix, and runner failure paths. The
retained generation matrix contained thirty-four targets, thirty-three
intended sidecars, correct modifier and port substitutions, and no pShare
process block. The retained live matrix added thirty-four original `.alog`
files, one mission result per case, exact aggregate order, no runtime error
flags, and no wrapper error. Every assigned TCP and UDP slot was clear after
each checked run.

Complete matched measurements at warp 10 and `--max_time=35` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy shared-stem serial, `--jobs=1` | 1 | 34 / 34 | 170.52 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 34 / 34 | 194.65 | clean |
| legacy batch-wave, `--jobs=2` | 3 | 102 / 102 | 110.77, 108.98, 108.92 | clean |
| migrated rolling, `--jobs=2` | 3 | 102 / 102 | 101.22, 96.81, 101.26 | clean |
| final checkout-state rolling, clear base 58000 | 1 | 34 / 34 | 102.86 | clean |

The migrated rolling samples have a 99.76-second mean, 101.22-second median,
96.81-second minimum, 101.26-second maximum, and 4.45-second range. Their mean
is 9.79 seconds, about 8.9 percent, faster than the 109.56-second legacy mean.
The migrated serial sample is 24.13 seconds, about 14.2 percent, slower because
the consistency contract now pays isolated-copy and scoped-cleanup costs for
every serial case.

No-MOOS probes verified missing file, missing row, duplicate rows, invalid and
duplicate grades, valid mission failure, nonzero launch with retained
provenance, and preparation failure. An injected teardown-discovery failure
started only the baseline case, produced one `teardown_error` plus thirty-three
ordered `scheduler_aborted_after_teardown_error` rows, and retained its root
and lock for inspection. A real warp-1, one-second ceiling produced one
`missing_result` row in 8.15 seconds and cleaned both reserved slots.

### Legacy Rollout Baseline: `usim_marine_h01`

uSimMarine H01 was audited and measured as the only live harness on July 14,
2026, from revision `5b77ab600` on the Apple Silicon macOS host `becker`. The
dependency map records one dedicated consumer of
`missions/usim_marine_missions/usim_marine_motion`. The README, launcher order,
and modifier values agree exactly. `thrust_forward_pass` is the unpatched
baseline; each of the other thirty-five cases maps one-to-one to a shoreside
`.xmoos` overlay. There are no behavior overlays or other stem consumers.

All thirty-six cases already follow mission-owned grading. Every evaluator
uses one existing `TEST_EVAL_READY=true` timer event, the simple
`lead_condition = TEST_EVAL_READY = true`, substantive pass conditions, one
`grade=$[GRADE]` report to `results.txt`, and a `mission_mod` matching the case
token. The shell's thirty-six `EXPECTED=pass` assignments merely compare the
mission grade with `pass`; they do not implement expected-negative semantics.
Pause, stop, disabled-state, and rate-suppression cases already grade pass when
the requested adverse or suppressive response is observed. No evaluator,
case patch, new variable, helper application, or outer-versus-subject verdict
is needed for the migration.

Legacy measurements used time warp 10, `--max_time=45`, explicit fresh port
bases, and the existing legacy scoped-teardown wrapper:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| batch-wave, `--jobs=2`, bases 30000, 32000, 34000 | 3 | 108 / 108 | 124.23, 134.17, 119.89 | clean |
| shared-stem serial, `--jobs=1 --port_base=36000` | 1 | 36 / 36 | 191.91 | clean |

The three two-job samples have a 126.10-second mean, 124.23-second median,
119.89-second minimum, 134.17-second maximum, and 14.28-second range. The
first run was retained for inspection. It contained thirty-six case
directories, thirty-six intermediate rows, thirty-six targets, thirty-six
original `.alog` files, and the intended thirty-five sidecars. Every mission
wrote exactly one passing row, every modifier matched its case, and aggregate
order matched the declared order. MOOSDB ports were unique from 30000 through
31050. All assigned TCP and UDP slots were clear after each measured run, and
no scoped MOOS process survived.

This is a one-community mission and it does not launch pShare. The legacy
launcher forwards a reserved `+10` pShare argument that the template does not
use. The migrated harness will use the skill-standard stride-30 layout with
MOOSDB at `+0` and the reserved/forwarded pShare slot at `+15`, without adding
a pShare process. The advertised legacy `--gui` path also has no viewer; the
migrated launcher will keep the option only for single-case wrapper parity and
will not claim that it creates a viewer.

The retained baseline shows no migration-blocking application anomaly. Static
inspection found the established launcher-form weaknesses: fixed pair waves
rather than rolling refill; shared-stem serial preparation; redundant expected
comparisons; loose last-line result parsing; stopping serial execution after
an ordinary failure; unchecked teardown; no lock or final row-count guard;
broad process-group signal handling; and bypassing a mission wrapper that does
not yet exist. A real launch-error bug can also preserve `grade=pass` when the
isolated child launcher returns nonzero because the parent ignores the worker
status and trusts the row. The migration will correct these runner mechanics
without changing the fixed-time snapshot coverage strategy or any substantive
case assertion.

### Migrated Rollout: `usim_marine_h01`

uSimMarine H01 now uses the settled one-community rolling launcher form. The
harness requires Bash 5.1, creates an isolated mission copy for every selected
case, and calls the copy's new stem wrapper. Each case receives a stride-30
allocation with MOOSDB at `+0` and a reserved/forwarded shoreside pShare slot
at `+15`; no pShare process was added. The default base is 9000, and the source
stem is never patched in place.

The launcher preserves all thirty-six case tokens, their declared order, the
unpatched baseline, and all thirty-five exact overlay mappings. It forwards the
selected case through the stem's existing `MMOD` substitution, aggregates one
validated row per case in selected order, continues after ordinary mission
failures, and preserves a valid mission grade as provenance if the wrapper
itself exits nonzero. Scheduler state and scoped teardown failures stop refill,
produce ordered aborted rows, and retain the diagnostic root and lock. No
process-group extension, watchdog, global cleanup, helper application, or new
grading variable was introduced.

The stem `zlaunch.sh` forwards time warp, `--max_time`, display mode, `MMOD`,
the one-community MOOSDB port, and the reserved pShare port to `xlaunch.sh`.
It validates `targ_shoreside.moos` under `--just_make`, requires exactly one
live result row with exactly one valid grade, preserves the launch return
status, and uses the canonical scoped-teardown helper. The underlying
`launch.sh` already used `nsplug -x` and accepted all required substitutions,
so it did not change.

No mission template or case overlay changed. All thirty-six evaluators still
use their original simple readiness lead, fixed evaluation time, substantive
pass conditions, report columns, and modifier defaults. Stop, pause, disabled,
and suppression scenarios remain ordinary mission-owned cases that grade pass
when their requested evidence is observed. This is a launcher-form migration,
not a case-coverage redesign.

Validation covered both skill static checkers, Bash syntax, ShellCheck, Bash
3.2 rejection and Homebrew re-execution, invalid CLI and port bounds, a
retained full generation matrix, an independent 5.74-second stem pass,
isolated nominal and obstacle-stop cases, three complete rolling matrices, one
complete isolated serial matrix, and runner/infrastructure failure paths. The
retained generation matrix contained thirty-six targets, the intended
thirty-five sidecars, exact modifiers and ports, and no pShare block. The
retained live matrix added thirty-six original `.alog` files and one mission
row per case, with exact aggregate order and no runtime error flags. Its 72
verbose scheduler events showed immediate refill after each of the first
thirty-four completions. Every assigned TCP and UDP slot was clear afterward.

Complete matched measurements at warp 10 and `--max_time=45` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy shared-stem serial, `--jobs=1` | 1 | 36 / 36 | 191.91 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 36 / 36 | 217.00 | clean |
| legacy batch-wave, `--jobs=2` | 3 | 108 / 108 | 124.23, 134.17, 119.89 | clean |
| migrated rolling, `--jobs=2` | 3 | 108 / 108 | 111.09, 111.03, 107.75 | clean |
| final checkout-state rolling, clear base 52000 | 1 | 36 / 36 | 114.14 | clean |

The migrated rolling samples have a 109.96-second mean, 111.03-second median,
107.75-second minimum, 111.09-second maximum, and 3.34-second range. Their
mean is 16.14 seconds, about 12.8 percent, faster than the 126.10-second legacy
mean. The migrated serial sample is 25.09 seconds, about 13.1 percent, slower
because the unified consistency contract pays case-copy and scoped-cleanup
costs for every serial case.

No-MOOS probes verified missing file, missing row, duplicate rows, invalid and
duplicate grades, valid mission failure, nonzero launch with retained
provenance, and preparation failure. An injected teardown-discovery failure
started only the baseline case, produced one `teardown_error` plus thirty-five
ordered `scheduler_aborted_after_teardown_error` rows, and retained its root
and lock for inspection. A real warp-1, one-second ceiling produced one
`missing_result` row in 6.93 seconds and cleaned both reserved slots.

### Legacy Rollout Baseline: `pnodereporter_h01`

pNodeReporter H01 was audited and measured as the only live harness on July 14,
2026, on the Apple Silicon macOS host `becker`. The dependency map records one
dedicated consumer of
`missions/pnodereporter_missions/pnodereporter_unit`. The README and launcher
contain the same thirty-three cases in the same order. The baseline case is
unpatched, and the other thirty-two cases map exactly to their kebab-case
`.xmoos` overlays. No overlays are missing or orphaned.

This audit found a substantive grading boundary that differs from the already
migrated launchers. All thirty-three missions run `pMissionEval`, but all
thirty-three launcher cases also check report-payload tokens in the shell.
Nine cases additionally check forbidden tokens. In total the launcher owns
127 required token fragments and thirteen forbidden fragments. For thirty of
the thirty-three cases, at least one of those payload checks proves documented
case behavior that the current `pMissionEval` conditions do not prove. Only
these three cases are already substantively decided by their evaluator:

- `blackout_interval_reset_fail`
- `mhash_odometer_pass`
- `extrap_gap_metric_pass`

The remaining evaluator conditions primarily establish that a node report,
first report, platform report, JSON report, or custom report was published.
They do not establish the requested fields or omissions inside the report.
This is not redundant expected-result plumbing: removing the token layer would
materially weaken thirty cases.

The limitation is structural. `NODE_REPORT_LOCAL` is a comma-delimited payload
with dynamic `TIME` and `INDEX` fields. The installed `pMissionEval` condition
grammar supports exact string relations and colon-field matching, but not
general substring matching, comma-field extraction, or regular expressions.
The raw report therefore cannot directly express the existing token contract
as ordinary pass conditions. No new application, scalar evidence variables,
or case-coverage redesign was introduced during the audit.

Legacy measurements used time warp 10, `--max_time=35`, explicit fresh port
bases, and the existing legacy scoped-teardown wrapper:

| configuration | run | exit | rows | launcher pass | launcher fail | wall seconds | cleanup |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| batch-wave, `--jobs=2 --port_base=30000`, retained | 1 | 1 | 33 | 32 | 1 | 116.95 | clean |
| batch-wave, `--jobs=2 --port_base=32000` | 2 | 0 | 33 | 33 | 0 | 110.37 | clean |
| batch-wave, `--jobs=2 --port_base=34000` | 3 | 1 | 33 | 32 | 1 | 117.49 | clean |
| shared-stem serial, `--jobs=1 --port_base=36000` | 1 | 1 | 33 | 32 | 1 | 160.52 | clean |

The three two-job samples have a 114.94-second mean, 116.95-second median,
110.37-second minimum, 117.49-second maximum, and 7.12-second range. They
published 97/99 launcher-passing rows, while the serial sample published
32/33 launcher-passing rows. Every mission evaluator involved in the three
outer failures still reported `grade=pass`; the payload layer correctly
identified that the requested report contents were absent.

The first two-job run failed `alt_nav_named_absolute_pass`: its report remained
the ordinary `NAME=reporter` record instead of the requested `truth_node`
alternate-navigation record. The retained `.alog` shows no alternate report
at any point. Five immediate isolated repetitions all passed in 5.61, 4.89,
5.25, 5.39, and 5.31 seconds. The full serial matrix later missed the same
alternate report again, so this is a pre-existing event/startup timing race,
not a rolling-scheduler or port-collision result.

The third two-job run failed `reverse_load_warning_pass`: the report contained
`THRUST_MODE_REVERSE=true` but omitted `LOAD_WARNING=pHelmIvP:5`. Five isolated
repetitions passed four times in 4.77-5.44 seconds; one repetition missed both
runtime fields and failed in 5.11 seconds. This case is independently
timing-sensitive even without parallel load. Both flakes remain coverage
findings to preserve and record; they do not justify altering case timing or
assertions during the launcher migration.

The retained first run contained thirty-three case directories, thirty-three
intermediate rows, thirty-three generated shoreside targets, thirty-two
intended `.moosx` sidecars, thirty-three original `.alog` files, and exactly
one mission result row per case. All thirty-three modifiers matched their case
tokens. MOOSDB ports were unique from 30000 through 30960 in stride-30 blocks;
the legacy `+10` pShare arguments were reserved but no pShare process ran. No
scoped process or assigned TCP/UDP listener survived any measured run.

The mechanically minimal launcher migration is otherwise clear: use the
settled Bash 5.1 rolling scheduler, isolated copies for every path, the thin
stem wrapper, canonical root-scoped teardown, a safety lock, `+15` reserved
pShare slots, exact result validation, ordered aggregation, and runner-owned
infrastructure failures. Before applying that migration, the verdict boundary
requires an explicit decision: retain and document these structured-payload
checks as a narrow legacy exception, or defer the harness until an approved
mission-owned structured-field mechanism exists. Silently dropping the checks
or introducing a new normalizer application is not acceptable.

### Migrated Rollout: `pnodereporter_h01`

The structured-payload checks were explicitly approved as a narrow exception.
The migrated launcher keeps every existing required and forbidden token, adds
no grading application or MOOS evidence variable, and otherwise follows the
skill 1.4.2 runner contract. Every path uses an isolated mission copy, Bash
5.1 rolling scheduling, stride-30 port blocks, canonical root-scoped teardown,
exact mission-result validation, and deterministic selected-case aggregation.

The legacy audit exposed two pre-existing timing mechanisms. Alternate report
mail could arrive before `pNodeReporter` was ready, and `LOAD_WARNING` is
cleared on each `Iterate`, so a final result snapshot could miss a report that
had genuinely been published. The two alternate-report overlays now repeat
their navigation mail at times 5, 10, and 15 and evaluate at time 19. The
reverse-warning overlay repeats thrust and warning mail at times 1, 3, and 5,
pauses at time 8, and evaluates at time 10. For these three publication-history
cases only, the payload supplement checks the final mission row first and then
the original `.alog` history for the same exact required report tokens. This
preserves the original assertion—whether the requested report was published—
without depending on which valid report happened to be the final snapshot.

Focused stabilization produced 10/10 passes for each alternate report case,
10/10 passes for the reverse-warning history check, and another 5/5 passes for
the final snapshot-first/history-fallback form. The stabilized legacy launcher
then passed three full two-job matrices in 108.35, 110.16, and 106.92 seconds
and one full serial matrix in 168.44 seconds. No timing failure recurred.

Complete matched measurements at warp 10 and `--max_time=35` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| original legacy batch-wave, `--jobs=2` | 3 | 97 / 99 | 116.95, 110.37, 117.49 | clean |
| stabilized legacy batch-wave, `--jobs=2` | 3 | 99 / 99 | 108.35, 110.16, 106.92 | clean |
| stabilized legacy shared-stem serial, `--jobs=1` | 1 | 33 / 33 | 168.44 | clean |
| migrated isolated serial, canonical defaults | 1 | 33 / 33 | 193 | clean |
| migrated isolated serial, supported one-second INT/TERM overrides | 1 | 33 / 33 | 178 | clean |
| migrated rolling, canonical defaults, `--jobs=2` | 3 | 99 / 99 | 101, 102, 98 | clean |
| final checkout-state rolling, clear base 24000 | 1 | 33 / 33 | 101 | clean |

The migrated rolling mean is 100.33 seconds, 8.15 seconds (about 7.5 percent)
faster than the stabilized legacy mean and 14.61 seconds (about 12.7 percent)
faster than the original legacy mean. Canonical migrated serial is 24.56
seconds (about 14.6 percent) slower than stabilized legacy serial. The
supported one-second cleanup-grace measurement removes 15 seconds of that
difference; the remaining approximately ten seconds is consistent with
copying and preparing every case under the unified isolation contract. The
canonical defaults remain unchanged.

Validation covered both skill static checkers, Bash syntax, ShellCheck, Bash
3.2 Homebrew re-execution and explicit rejection, invalid CLI values, a full
generation-only matrix, an independent stem pass, isolated nominal,
mission-owned negative, and history-sensitive cases, three complete rolling
matrices, one complete serial matrix, and a real missing-result path. The
retained rolling matrix contained thirty-three case directories, thirty-three
targets, the intended thirty-two sidecars, thirty-three original `.alog`
files, unique ports from 59000 through 59960, and exactly thirty-three result
rows in declared order. Targeted `aloggrep` checks confirmed the exact
alternate and reverse-thrust reports at mission times 24.09455, 24.17603, and
12.02941. A real warp-1, one-second ceiling produced exactly one
`grade=fail reason=missing_result` row, returned nonzero, and cleaned its port
and processes.

### Migrated Rollout: `upokedb_h01`

uPokeDB H01 was audited and migrated as the only live harness on July 14,
2026. It has twenty-eight documented cases, no patch overlays, and a dedicated
single-community stem. The harness writes each copied case's full
`pMissionEval` block and invokes the corresponding direct, alias, mission-file,
cache, mixed, duplicate, or time-macro `uPokeDB` command. The retained legacy
matrix contained twenty-eight targets, twenty-eight evaluator configs, the
expected seven cache fixtures, twenty-eight original `.alog` files, and unique
MOOSDB ports from 30000 through 30540.

All thirty-two substantive value assertions were already represented as
`pMissionEval` pass conditions. The legacy `EXPECTED=pass`, required-token,
absent-token, and numeric-greater-than checks only repeated those conditions
against pMissionEval's own report columns. They were removed without changing
case coverage. No grading application or MOOS evidence variable was added;
pMissionEval remains the sole ordinary verdict owner.

The migrated stem wrapper owns `xlaunch.sh`/`uMayFinish`, exact one-row result
validation, forwarded one-community ports, and canonical scoped teardown. Its
standalone path performs the baseline numeric poke. The explicit
`--external_stimulus` path lets the harness invoke a case's exact `uPokeDB`
command after MOOSDB startup while still using the stem lifecycle. A direct
child deadline, bounded by the existing `--max_time` infrastructure ceiling,
prevents a `uPokeDB` client from retrying forever after its MOOSDB disappears;
it does not supervise a process group or participate in grading.

Legacy measurements at warp 10 and `--max_time=30` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=2` | 3 | 84 / 84 | 65.16, 66.18, 66.58 | clean |
| legacy isolated serial, `--jobs=1` | 1 | 28 / 28 | 121.64 | clean |

One other legacy serial attempt was externally interrupted by `ktm` after
twenty passing rows. It was stopped, verified clean, and excluded from every
timing and correctness statistic.

The initial migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=2` | 3 | 84 / 84 | 84, 85, 82 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 28 / 28 | 163 | clean |
| final checkout-state rolling, clear base 52000 | 1 | 28 / 28 | 82 | clean |

The migrated rolling mean is 83.67 seconds, 17.70 seconds (about 26.8 percent)
slower than the 65.97-second legacy mean. Migrated serial is 41.36 seconds
(about 34 percent) slower than legacy serial. This is the expected cost of
moving from a harness-owned shortcut to the standard `xlaunch.sh` lifecycle,
whose two-second graceful shutdown interval is paid by every case and can only
overlap in rolling mode. The three-second migrated rolling range and complete
passing matrices show stable lifecycle overhead rather than case flakiness.

Validation covered full generation, standalone stem execution, direct, cache,
alias, duplicate, mixed cache/CLI, and positive-time cases, three complete
rolling matrices, a complete serial matrix, invalid CLI values, Bash 3.2
re-execution and rejection, exact result ordering, distinct ports, and scoped
cleanup. A warp-1, one-second delayed-overwrite probe produced one normalized
`missing_result` row in eight seconds, returned nonzero, and left no app or
listener. The harness static checker, Bash syntax, and ShellCheck pass. The
eval static checker retains one known false positive because it only recognizes
`pAutoPoke` or `uTimerScript` initialization; this CLI mission intentionally
has the app under test post `TEST_EVAL_READY=true` together with the graded
value, as confirmed by standalone and harness runs.

### Migrated Rollout: `uxms_h01`

uXMS H01 was audited and migrated as the only live harness on July 14, 2026.
It has thirty-two documented cases, no overlays, and a dedicated
single-community publisher stem. All README tokens, launcher tokens, and case
mappings matched before migration.

This harness is a narrow CLI-output exception rather than an ordinary
mission-only grader. pMissionEval owns the deterministic fixture-readiness
grade by checking the existing `XMS_ALPHA=alpha` and `XMS_NUM=42` facts. The
harness retains the original required and forbidden terminal-token assertions
because source columns, community columns, truncation, color, history,
filtering, and clean scoping exist only in `uXMS` stdout. No helper application,
new grading variable, or case-coverage change was introduced.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
every path, 30-port blocks, a harness lock, canonical scoped teardown, exact
one-row validation, deterministic selected-case aggregation, and normalized
runner failures. The stem wrapper now owns xlaunch/uMayFinish, explicit port
forwarding, a brief post-xlaunch grade wait, and canonical teardown. The
harness posts the existing `TEST_EVAL_READY` only after its capture completes.
The two existing prereport fields became regular report columns so pMissionEval
writes the complete row in one operation; their names, values, and order did
not change.

Legacy measurements at warp 10 and `--max_time=30` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=2` | 3 | 96 / 96 | 106.33, 106.33, 105.85 | clean |
| legacy isolated serial, `--jobs=1` | 1 | 32 / 32 | 200.21 | clean |

Final migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=2` | 3 | 96 / 96 | 126.65, 127.46, 125.63 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 32 / 32 | 249.00 | clean |
| final checkout-state rolling, clear base 63000 | 1 | 32 / 32 | 127.26 | clean |

The migrated rolling mean is 126.58 seconds, 20.41 seconds (about 19.2
percent) slower than the 106.17-second legacy mean. Migrated serial is 48.79
seconds (about 24.4 percent) slower. The cost comes from an extra second of
startup allowance needed by the xlaunch path plus the standard per-case
xlaunch shutdown lifecycle; rolling execution overlaps part of that cost.

Two provisional rolling runs passed before a third found empty captures in the
last two cases. Preserved focused runs then exposed a separate pMissionEval
prereport/xlaunch race: xlaunch could return before the grade append, or insert
a newline between prereport and report fragments. Those provisional runs are
excluded from final timing statistics. A two-second capture-start delay, the
brief wrapper grade wait prescribed by the skill, and moving `form`/`mmod` into
the ordinary report block removed both races. The formerly failing
`colormap_pair_pass` and `clean_shortcut_scope_pass` cases each passed three
consecutive focused repetitions before the three final full matrices.

Validation covered all-case generation, standalone stem execution,
representative filtering/truncation/config/source displays, three final rolling
matrices, one final serial matrix, exact order, one physical pMissionEval row
per case, 32 distinct retained ports from 54000 through 54930, invalid CLI and
port bounds, lock contention, Bash 3.2 re-execution and rejection, and scoped
cleanup. An injected no-output `uXMS` returned zero while the mission graded
pass; the harness correctly returned nonzero with
`reason=output_check_failed`, proving the supplemental CLI oracle remains
effective.

### Migrated Rollout: `uquerydb_h01`

uQueryDB H01 was audited and migrated as the only live harness on July 15,
2026. It has thirty documented cases, no overlays, and a dedicated
single-community publisher stem. README tokens, launcher tokens, setup
mappings, and final result order all reconcile exactly.

This remains the documented CLI-result exception identified in the initial
inventory. pMissionEval owns the deterministic publisher-readiness grade using
the existing `TEST_EVAL_READY`, `QUERY_NUM`, `QUERY_STR`, and `QUERY_LATE`
facts. The harness owns only the subject contract that cannot be represented as
MOOS state: the configured `uQueryDB` return code and optional `.checkvars`
contents. Eight cases intentionally require return code 1. No application,
MOOS grading variable, or case assertion was added or removed.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling paths, stride-30 port blocks, a harness lock, canonical
scoped teardown, exact one-row mission-result validation, deterministic
aggregation, and normalized runner failures. The stem wrapper owns the normal
xlaunch/uMayFinish lifecycle and explicit port forwarding. The harness posts
the existing readiness flag after its CLI subject finishes; the standalone
stem posts the same flag itself. As in the accepted uXMS correction, the two
stable prefix fields are ordinary report columns so pMissionEval emits one
physical result row before xlaunch completes.

Legacy measurements at warp 10 and `--max_time=30` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=2` | 3 | 90 / 90 | 68.75, 68.65, 67.65 | clean |
| legacy isolated serial, `--jobs=1` | 1 | 30 / 30 | 116.78 | clean |

Final migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=2` | 3 | 90 / 90 | 107.64, 109.22, 108.42 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 30 / 30 | 216.21 | clean |

The migrated rolling mean is 108.43 seconds, 40.08 seconds (about 58.6
percent) slower than the 68.35-second legacy mean. Migrated serial is 99.43
seconds (about 85.1 percent) slower. The principal cost is deliberate: every
case now pays xlaunch's fixed two-second shutdown interval and a 0.75-second
client-start allowance instead of the legacy direct lifecycle and 0.25-second
allowance. Rolling execution overlaps part of that cost. A 607.24-second
passing sample is excluded from every timing statistic because macOS power logs
show the laptop entered clamshell sleep during the run and spent roughly five
hundred seconds asleep or in maintenance wakes before resuming.

An adversarial no-mission probe exposed a real legacy failure-path weakness.
The old eight-second `uQueryDB` deadline sent `TERM` and then waited forever if
a disconnected client did not exit; it could subsequently attempt an unbounded
readiness poke against the same absent server. The migrated runner keeps the
same eight-second subject deadline, waits two seconds after `TERM`, sends
`KILL` only to that exact CLI PID if necessary, and skips the readiness poke
when the subject reports infrastructure timeout 124. This is bounded CLI
subject cleanup, not mission process-group supervision. The corrected probe
returned in 9.45 seconds with one `reason=missing_result` row and no surviving
process or listener.

Validation covered both skill static checkers, Bash syntax, ShellCheck,
standalone explicit-port execution, full retained generation, nominal and
expected-return-code-1 cases, every `.checkvars` format, three full rolling
matrices, one full serial matrix, rolling refill timestamps, invalid CLI and
port bounds, lock contention, Bash 3.2 re-execution and rejection, and scoped
cleanup. The retained live matrix contained thirty mission copies, thirty
targets, thirty `.alog` files, eight expected `.checkvars` files, one physical
pMissionEval row per case, no sidecars, and unique MOOSDB ports from 46000
through 46870. Injected subject-return and missing-checkvars failures produced
the intended `subject_result_mismatch` and `checkvars_mismatch` rows while
preserving the mission-owned pass evidence.

### Migrated Rollout: `utermcommand_h01`

uTermCommand H01 was audited and migrated as the only live harness on July 15,
2026. It has fifteen documented cases, no overlays, and a dedicated
single-community stem. README tokens, launcher tokens, setup mappings, and
final result order reconcile exactly.

This is an ordinary mission-owned grading harness, not a CLI-output exception.
All positive command publications, duplicate/exact-selection behavior, and
expected-absence outcomes were already expressed as pMissionEval pass and fail
conditions. The legacy `EXPECTED=pass` and `CHECK_TOKENS` shell comparisons
only repeated pMissionEval report fields and were removed. No application,
grading variable, or behavioral assertion was added or removed. The existing
`TEST_EVAL_READY` value is posted as false to confirm MOOSDB readiness before
terminal input and true after input; pMissionEval remains the only verdict
owner.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling paths, stride-30 port blocks, a harness lock, canonical
scoped teardown, exact one-row mission-result validation, deterministic
aggregation, and normalized runner failures. The stem wrapper owns the normal
xlaunch/uMayFinish lifecycle and explicit port forwarding. The stable
`form`/`mmod` fields are ordinary report columns so pMissionEval emits one
physical result row before xlaunch completes. A brief wrapper grade wait covers
the accepted result-file visibility race without manufacturing a grade.

Adopting xlaunch changed when the outer timeout begins: legacy started
uMayFinish only after all terminal input, while xlaunch starts it with the
mission. The default ceiling therefore increased from 30 to 180 MOOS seconds,
which is about eighteen wall seconds at the documented warp 10. The startup
client has a twenty-wall-second connection bound, and each terminal session has
a twelve-wall-second exact-PID bound. These bounds do not delay successful
cases and do not supervise process groups; they prevent a missing MOOSDB or a
stuck terminal client from hanging the worker.

Legacy awake measurements at warp 10 and `--max_time=30` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=4` | 3 | 45 / 45 | 36.89, 39.25, 35.48 | clean |

One legacy serial attempt took 2,510.48 seconds and returned 14/15 passing
rows. macOS power logs show repeated maintenance sleeps during the sample,
including a 314-second sleep, and several terminal clients hit their wall-clock
deadline while the machine was asleep. That sample is excluded from all
performance and correctness statistics.

Final migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=4` | 3 | 45 / 45 | 44.37, 44.71, 44.49 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 15 / 15 | 166.16 | clean |

The migrated rolling mean is 44.52 seconds, 7.32 seconds (about 19.7 percent)
slower than the 37.21-second legacy mean. The stable three-sample range shows
lifecycle overhead rather than case flakiness. The main costs are the standard
two-second xlaunch shutdown interval, explicit readiness confirmation, and
copy/cleanup work for every isolated case; rolling execution overlaps much of
that cost. No valid awake legacy serial sample exists for a serial percentage
comparison.

Three provisional migrated matrices are excluded from final statistics. They
were intentionally used to diagnose the standard-lifecycle transition: the
old 30-second MOOS-time ceiling began before external terminal input, the final
readiness poke could still be confirming its post while xlaunch shut down, the
result row could become visible just after xlaunch returned, and the legacy
eight-second client-start bound was too tight under four-case startup load.
The final timeout, readiness-confirmation, and grade-wait design removed those
races; the three final rolling matrices and complete serial matrix all passed.

Validation covered harness structural checks, Bash syntax, ShellCheck on the
changed launchers, full generation, standalone stem execution, nominal and
expected-absence cases, three final rolling matrices, one complete serial
matrix, rolling refill timestamps, invalid CLI and port bounds, lock
contention, Bash 3.2 re-execution and rejection, exact result ordering,
distinct retained ports from 57000 through 57420, no sidecar leakage, and
scoped cleanup. A no-stimulus external stem run returned nonzero with zero
grade rows, proving the wrapper treats missing pMissionEval output as an
infrastructure failure. The eval static checker retains the same known false
positive as other externally stimulated unit missions because it recognizes
only pAutoPoke or uTimerScript initialization; the app-under-test workflow owns
readiness here, as permitted by the skill and confirmed live.

After the initial closure review, the mission-level wrapper was reduced from
280 to 240 lines by consolidating duplicated terminal/readiness PID handling
into one bounded-client helper and removing unused multi-session plumbing. The
wrapper remains about sixty lines longer than the externally stimulated
uPokeDB, uXMS, and uQueryDB stems because its standalone scenario must safely
drive an interactive terminal pipeline. Target generation, standalone grading,
the missing-grade path, and a complete 15/15 rolling matrix were revalidated;
the post-simplification matrix took 45.93 seconds. An audit of all other twelve
migrated mission wrappers found no similar overgrowth: they range from 155 to
191 lines and contain only argument forwarding, xlaunch/result validation,
scoped teardown, and any small scenario-specific stimulus.

### Migrated Rollout: `hostinfo_h01`

HostInfo H01 was audited and migrated as the only live harness on July 15,
2026. It has seven documented cases, one baseline, and six explicit shoreside
patches. README tokens, launcher tokens, case-to-patch mappings, and final
result order reconcile exactly.

This is an ordinary mission-owned grading harness. The baseline and all six
patches already express their complete contracts as pMissionEval conditions:
the forced host IP, generated MOOSDB port, exact normalized route payloads,
invalid-route omission, mixed-route filtering, and non-UDP token retention.
The legacy shell's `EXPECTED=pass` comparison therefore duplicated the
mission verdict and was removed. No pMissionEval block, case assertion,
mission variable, application, or route stimulus changed.

The migrated harness uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling execution, explicit case-to-patch mappings, stride-30 port
blocks with the midpoint pShare reservation, a harness lock, canonical scoped
teardown, exact one-row mission-result validation, deterministic aggregation,
and normalized runner failures. The mission wrapper grew from 83 to 158 lines
only to forward explicit ports and mission labels, validate one pMissionEval
row, and use the canonical teardown helper. It contains no stimulus,
scheduler, case mapping, grading logic, or custom supervision and is within
the 155-191-line range of the other ordinary migrated wrappers.

Legacy measurements at warp 10 and `--max_time=40` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=3` | 3 | 21 / 21 | 19.10, 18.36, 17.07 | clean |
| legacy shared-stem serial, `--jobs=1` | 1 | 7 / 7 | 35.34 | clean |

Final migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=3` | 3 | 21 / 21 | 18.77, 17.13, 18.91 | clean |
| migrated isolated serial, `--jobs=1` | 2 | 14 / 14 | 41.86, 41.03 | clean |

The rolling mean changed from 18.18 to 18.27 seconds, an immaterial 0.09
seconds (about 0.5 percent). The migrated serial mean is 41.45 seconds, 6.11
seconds (about 17.3 percent) above the single 35.34-second legacy sample. A
focused run with teardown diagnostics enabled showed no signal or grace-period
wait in normal completion. The serial difference is therefore the expected
visible cost of making every case take the same isolated-copy path and of the
mission and harness independently verifying their scoped cleanup; rolling
execution overlaps that per-case filesystem and process-discovery work. No
three-second cleanup grace was paid by successful cases.

Validation covered both skill static checkers, Bash syntax, ShellCheck,
standalone target generation and live execution on explicit ports, retained
all-case generation, three full rolling matrices, two full serial matrices,
rolling refill timestamps, exact result ordering, unknown-case and port-bound
errors, lock contention, Bash 3.2 re-execution and rejection, and a live
missing-result path. The retained live matrix contained seven mission copies,
seven targets, seven `.alog` files, six intended sidecars, no baseline
sidecar, one physical pMissionEval row per case, and distinct MOOSDB ports from
42000 through 42180. Logs contained no warnings or configuration errors, and
no tested listener or mission process remained.

### Migrated Rollout: `loadwatch_h01`

LoadWatch H01 was audited and migrated as the only live harness on July 15,
2026. It has twelve documented cases and twelve explicit shoreside patches.
README tokens, launcher tokens, case-to-patch mappings, and final result order
reconcile exactly.

This is an ordinary mission-owned grading harness. Every patch already
expressed the complete uLoadWatch contract as pMissionEval conditions,
including near and hard breach counts, trigger holdoff, named-app filtering,
case-sensitive threshold matching, exact threshold boundaries, ignored low
thresholds, and clamped near thresholds. The legacy shell's `EXPECTED=pass`
comparison duplicated the mission verdict and was removed. No evaluator
condition, grading variable, app configuration, or scripted mail changed.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling execution, explicit case-to-patch mappings, stride-30 port
blocks, a harness lock, canonical scoped teardown, exact one-row mission-result
validation, deterministic aggregation, and normalized runner failures. The
mission wrapper is the same 158-line thin wrapper validated for HostInfo: it
forwards explicit ports and mission labels, checks the pMissionEval row, and
applies the standalone scoped-cleanup backstop. It adds no stimulus, grading,
scheduling, or custom supervision.

Legacy measurements at warp 10 and `--max_time=40` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=3` | 3 | 36 / 36 | 31.66, 27.27, 30.22 | clean |
| legacy shared-stem serial, `--jobs=1` | 1 | 12 / 12 | 60.61 | clean |

Final migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=3` | 3 | 36 / 36 | 29.46, 25.43, 24.49 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 12 / 12 | 70.45 | clean |

The rolling mean improved from 29.72 to 26.46 seconds, 3.26 seconds (about
11.0 percent). No case timing was shortened; the work-conserving scheduler
starts pending work as individual cases finish instead of waiting for each
three-case wave. Serial increased 9.84 seconds (about 16.2 percent), consistent
with the visible per-case cost of isolated copying plus independent mission and
harness cleanup verification.

Validation covered both skill static checkers, Bash syntax, ShellCheck,
standalone generation and live execution on explicit ports, retained all-case
generation, three full rolling matrices, one full serial matrix, rolling
refill timestamps, exact result ordering, unknown-case and port-bound errors,
lock contention, Bash 3.2 re-execution and rejection, and a live missing-result
path. The retained live matrix contained twelve mission copies, twelve targets,
twelve sidecars, twelve `.alog` files, one physical pMissionEval row per case,
and distinct MOOSDB ports from 54000 through 54330. Logs contained no warnings
or configuration errors, and no tested listener or mission process remained.

### Migrated Rollout: `processwatch_h01`

ProcessWatch H01 was audited and migrated as the only live harness on July 15,
2026. It has seven documented cases and seven explicit shoreside patches.
README tokens, launcher tokens, case-to-patch mappings, and final result order
reconcile exactly.

This is an ordinary mission-owned grading harness. All seven patches already
expressed their complete uProcessWatch contracts as pMissionEval conditions.
The expected-AWOL case remains a harness pass only when the mission observes
`PROC_WATCH_ALL_OK=false`, `GHOST_PRESENT=false`, and the expected missing-app
summary. No condition, watched process, stimulus, or grading variable changed.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling execution, explicit case-to-patch mappings, stride-30 port
blocks, a harness lock, canonical scoped teardown, exact one-row
mission-result validation, deterministic aggregation, and normalized runner
failures. Stable `form` and `mmod` fields moved from prereport fragments into
the ordinary pMissionEval report so every case writes one physical result row.
The thin mission wrapper waits up to three seconds for `grade=` before
validating that row; it does not create or alter the mission verdict.

The audit exposed a legacy lifecycle defect around the existing timeout. The
legacy launcher parsed `--max_time=40` correctly, and uMayFinish could reach
that ceiling, but shared `xlaunch.sh` did not propagate the uMayFinish status or
fully stop the descendant mission processes. The legacy harness then waited up
to thirty more wall seconds for a result, allowing the still-running mission to
reach the expected-AWOL timer at mission time 125 and write a grade after the
nominal ceiling. The migrated wrapper's strict missing-result check and scoped
teardown made 40 an effective end-to-end ceiling and exposed that it was too
short. The mission-appropriate default and documented validation value are now
180 mission seconds. This changes only the existing runner ceiling; the case
timer and assertions remain unchanged.

Legacy measurements at warp 10 with `--max_time=40`, which the legacy
post-timeout wait allowed the mission to overrun, were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=3` | 3 | 21 / 21 | 30.85, 29.37, 29.45 | clean |
| legacy shared-stem serial, `--jobs=1` | 1 | 7 / 7 | 45.68 | clean |

Final migrated measurements at warp 10 with an enforced `--max_time=180`
were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=3` | 3 | 21 / 21 | 31.86, 29.02, 30.72 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 7 / 7 | 52.81 | clean |

The rolling mean changed from 29.89 to 30.53 seconds, a 0.64-second increase
of about 2.1 percent. Serial increased 7.13 seconds, about 15.6 percent,
consistent with per-case isolated copying plus independent mission and harness
cleanup verification. Three focused expected-AWOL repetitions completed in
20, 19, and 18 seconds. The small rolling difference and stable focused runs
show no performance or timing regression after correcting the timeout.

Validation covered both skill static checkers, Bash syntax, ShellCheck,
standalone target generation and live execution, retained all-case generation,
three focused expected-AWOL repetitions, three full rolling matrices, one full
serial matrix, rolling refill timestamps, exact result ordering, unknown-case
and port-bound errors, lock contention, Bash 3.2 re-execution and rejection,
and a live missing-result path. The retained live matrix contained seven
mission copies, seven targets, seven intended sidecars, seven `.alog` files,
and one physical pMissionEval row per case. Logs contained no warnings or
configuration errors, and no tested listener or mission process remained.

### Migrated Rollout: `pshare_h01`

pShare H01 was audited and migrated as the only live harness on July 15, 2026.
It has thirteen documented cases: one explicit no-patch baseline and twelve
cases with paired shoreside and peer patches. README tokens, launcher tokens,
case mappings, patch pairs, and final result order reconcile exactly.

This is an ordinary mission-owned grading harness. Every case already grades
the routed pShare value through pMissionEval, covering direct and renamed
routes, share limits, wildcard forms, duration and frequency controls,
shorthand and CLI routes, fanout, source-app filtering, multicast aliases, and
wildcard destination rewriting. The legacy shell's `EXPECTED=pass` comparison
duplicated the mission verdict and was removed. No pShare route, timer,
evaluator condition, mission variable, or evidence column changed.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling execution, explicit paired-patch mappings, stride-30 port
blocks, a harness lock, canonical scoped teardown, exact one-row
mission-result validation, deterministic aggregation, and normalized runner
failures. Each case receives shoreside and peer MOOSDB ports at offsets zero
and one and pShare ports at midpoint offsets fifteen and sixteen. The thin
mission wrapper only forwards those four ports and the mission label, validates
the pMissionEval row, and applies the standalone scoped-cleanup backstop.

Legacy measurements at warp 10 and `--max_time=25` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=3` | 3 | 39 / 39 | 35.54, 35.54, 34.83 | clean |
| legacy shared-stem serial, `--jobs=1` | 1 | 13 / 13 | 73.16 | clean |

Final migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=3` | 3 | 39 / 39 | 33.01, 31.78, 33.61 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 13 / 13 | 84.37 | clean |

The rolling mean improved from 35.30 to 32.80 seconds, about 7.1 percent,
because pending cases refill open scheduler slots instead of waiting for a
whole three-case wave. Serial increased 11.21 seconds, about 15.3 percent,
consistent with isolated copying plus independent mission and harness scoped
cleanup verification for two communities per case. No timer, startup delay, or
timeout value changed.

Validation covered both skill static checkers, Bash syntax, ShellCheck,
standalone target generation and live execution on four explicit ports,
retained all-case generation, three full rolling matrices, one full serial
matrix, rolling refill timestamps, exact result ordering, unknown-case,
port-bound, multi-case GUI, lock, and Bash 3.2 probes, plus a live
missing-result path at warp one. The retained live matrix contained thirteen
mission copies, twenty-six targets, twenty-four intended sidecars, and one
physical pMissionEval row per case. Logs contained no warnings or configuration
errors, and no tested listener or mission process remained.

### Migrated Rollout: `plogger_h01`

pLogger H01 was audited and migrated as the only live harness on July 15,
2026. It has ten documented single-community cases: one no-patch baseline and
nine cases with explicit shoreside patches. README tokens, launcher tokens,
case mappings, patches, and final result order reconcile exactly.

This harness is a narrow filesystem-evidence exception to ordinary
mission-only grading. pMissionEval continues to own every live MOOS verdict.
After shutdown, the launcher verifies the existing `.alog`, `.slog`, `.xlog`,
fixed-name log, or copied-file contract because those artifacts are not
available as live MOOS variables. A clean result requires both the one-row
mission verdict and the intended artifact; an artifact failure becomes a
runner-owned `grade=fail reason=artifact_check_failed` row with the mission
grade retained as provenance. No pMissionEval condition, publisher stimulus,
logger configuration, case variable, or timing value changed.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling execution, explicit patch and artifact mappings, stride-30
port blocks, a harness lock, canonical scoped teardown, exact one-row mission
result validation, deterministic aggregation, and normalized runner failures.
The thin mission wrapper only forwards the port and execution controls,
validates the pMissionEval row, and applies the standalone cleanup backstop.

The audit found one defect in the legacy filesystem checker. Its
`find -exec grep` form returned success when no file matched, so a missing
`.slog`, `.xlog`, or copied file could satisfy the content test vacuously. The
checker now requires at least one matching file containing the expected
content. A focused negative probe removed a retained copied artifact and
correctly produced `artifact_check_failed`; all affected live cases and the
full final matrix passed with their real artifacts. This strengthens the
existing intended assertion without adding a case or changing coverage scope.

Legacy measurements at warp 10 and `--max_time=20` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=2` | 3 | 30 / 30 | 29.65, 28.61, 29.23 | clean |
| legacy shared-stem serial, `--jobs=1` | 1 | 10 / 10 | 50.56 | clean |

Migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=2` | 4 | 40 / 40 | 29.39, 31, 31, 29.73 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 10 / 10 | 56 | clean |

The rolling mean changed from 29.16 to 30.28 seconds, about 3.8 percent and
within the observed run-to-run range. Serial increased 5.44 seconds, about
10.8 percent, consistent with isolated copying plus independent mission and
harness cleanup verification. No startup delay, mission timer, or maximum time
changed.

Validation covered both skill static checkers, Bash syntax, ShellCheck,
standalone target generation and live execution on explicit ports, retained
all-case generation, four full rolling matrices, one full serial matrix, exact
result ordering, unknown-case, job-bound, port-bound, multi-case GUI, lock, and
Bash 3.2 probes, plus a live missing-result path. The final retained matrix
contained ten mission copies, ten targets, nine intended sidecars, ten `.alog`
files, two `.slog` files, one `.xlog`, one copied file, and one physical
pMissionEval row per case. Logs contained no warnings or configuration errors,
and no tested listener remained.

### Migrated Rollout: `pdeadmanpost_h01`

pDeadManPost H01 was audited and migrated as the only live harness on July 15,
2026. Its twenty documented cases, launcher tokens, explicit generated case
configurations, pMissionEval conditions, heartbeat schedules, deadflags, and
final result order reconcile exactly.

pMissionEval owns all live MOOS verdicts. Eleven cases retain their existing
post-run `.alog` count contracts because current state cannot represent repeated
identical publications, exact posting policy counts, or distinguish an explicit
`false` post from no post. The launcher no longer repeats pMissionEval's scalar
value comparisons; it only verifies those publication counts. A count failure
becomes `grade=fail reason=artifact_check_failed` with the mission row preserved
as provenance. No app, case, condition, heartbeat, deadflag, report variable, or
grading variable was added.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling execution, explicit generated fixture mapping, stride-30
port blocks, a harness lock, canonical scoped teardown, exact one-row mission
result validation, deterministic selected-case aggregation, and normalized
runner failures. The thin mission wrapper only forwards execution controls,
validates the pMissionEval row, and applies the standalone cleanup backstop.
Stable `form` and `mmod` fields moved from prereport fragments into the ordinary
report so pMissionEval writes one complete row at grading time.

The first migrated matrix exposed one lifecycle race in
`active_start_keepalive_suppresses_pass`. pMissionEval passed at mission time
4.56, the final heartbeat arrived at 4.60, and the existing 3.50-second stale
threshold let pDeadManPost publish at 8.15 while standard xlaunch was shutting
down. The legacy runner counted the log before this interval completed and
therefore saw zero. The original `max_noheart=3.50` value is retained. All
publication counts are now bounded by the first `MISSION_EVALUATED=true`
timestamp in each `.alog`, matching the actual test window instead of the
launcher's later shutdown window. Five focused reruns passed in 6.00, 6.03,
6.07, 6.04, and 6.12 seconds with a bounded count of zero. One retained log
showed evaluation at 4.38 and the correct stale-heartbeat deadflag later at
8.20, proving that post-test watchdog behavior remains active rather than being
delayed or suppressed. No case value changed.

Legacy measurements at warp 10 and `--max_time=30` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=4` | 3 | 60 / 60 | 21.20, 20.79, 20.49 | clean |
| legacy isolated serial, `--jobs=1` | 1 | 20 / 20 | 65.13 | clean |

Migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=4` | 3 | 60 / 60 | 32.67, 31.92, 32.43 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 20 / 20 | 116.13 | clean |

The rolling mean increased from 20.83 to 32.34 seconds, about 55.3 percent.
Serial increased 51.00 seconds, about 78.3 percent. The legacy harness launched
the mission and uMayFinish directly, while the migrated path uses the standard
xlaunch lifecycle plus independent mission and harness scoped-cleanup
verification for every case. Across four rolling slots, that fixed per-case
cost appears across five refill waves. Raw logs may contain later watchdog
activity during shutdown, but publication-count grading now stops at the
mission-owned evaluation boundary.

Validation covered both skill static checkers, Bash syntax, ShellCheck,
standalone target generation and live execution on explicit ports, retained
all-case generation, three successful full rolling matrices, one full serial
matrix, five focused keepalive repetitions, exact result ordering, distinct
ports, unknown-case, job-bound, port-bound, multi-case GUI, lock, and Bash 3.2
probes, live launch-error provenance, direct empty-result normalization, and a
missing-alog count failure. The final retained matrix contained twenty mission
copies, twenty targets, twenty generated configurations of each case-file type,
twenty `.alog` files, and one physical pMissionEval row per case. Logs contained
no warnings or configuration errors, and no tested listener remained.

### Migrated Rollout: `pechovar_h01`

pEchoVar H01 was audited and migrated as the only live harness on July 15,
2026. Its thirty-three documented cases, launcher tokens, generated pEchoVar,
uTimerScript, and pMissionEval configurations, publication-evidence contracts,
and final result order reconcile exactly.

This is a narrow log-evidence exception rather than a wholly mission-only
harness. pMissionEval owns ordinary numeric, string, boolean, fanout, chain,
and latest-value verdicts. The launcher no longer repeats simple
variable-presence checks for those cases. Existing `.alog` checks remain for
exact serialized EFlipper payloads, suppressed or cycle-blocked publications,
and three exact publication-count contracts because those histories cannot be
represented by current MOOS state without adding grading variables. Every log
check is bounded by the first `MISSION_EVALUATED=true` timestamp. No app or
grading variable was added.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling execution, generated case fixtures, stride-30 port blocks,
a harness lock, canonical scoped teardown, exact one-row mission-result
validation, deterministic aggregation, and normalized runner failures. The
thin mission wrapper only forwards execution controls, validates the
pMissionEval row, and applies the standalone cleanup backstop. Stable `form`
and `mmod` fields moved from prereport fragments into the ordinary report so
pMissionEval writes one complete row at grading time.

The first untouched legacy rolling matrix had one failure in
`bool_switch_true_pass`: `MISSION_EVALUATED=true` arrived at mission time 4.726
and the expected `ECHO_OUT_BOOL=false` followed at 4.777. Five focused legacy
repetitions then passed, as did the next two full legacy matrices and the full
legacy serial matrix, identifying a low-rate startup/evaluation race rather
than a pEchoVar contract failure. The existing `TEST_EVAL_READY` event moved
from mission time 1.5 to 2.0, giving pEchoVar a deterministic publication
margin without changing any source event, mapping, expected value, or
assertion. Five focused migrated repetitions and four final full rolling
matrices all passed.

Legacy measurements at warp 10 and `--max_time=20` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=4` | 3 | 98 / 99 | 38.38, 37.96, 37.17 | clean |
| legacy shared-stem serial, `--jobs=1` | 1 | 33 / 33 | 108.06 | clean |

The first rolling failure was the documented evaluation race. Five immediate
focused legacy reruns of that case passed in 5.14, 5.29, 5.33, 5.29, and 5.48
seconds.

Final migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=4` | 4 | 132 / 132 | 50.48, 51.97, 53.78, 51.58 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 33 / 33 | 188.68 | clean |

Using all three legacy samples, including the one flaky result, the rolling
mean increased from 37.84 to 51.95 seconds, 14.12 seconds (about 37.3 percent).
Serial increased 80.62 seconds (about 74.6 percent). The migrated path routes
every case through standard xlaunch and independent mission and harness scoped
cleanup; its roughly two-second fixed lifecycle cost is visible for each of
thirty-three short cases and overlaps only partially across rolling slots. No
case deadline, maximum time, source schedule other than the evaluation-ready
margin, or pEchoVar behavior changed.

Validation covered both skill static checkers, Bash syntax, ShellCheck for the
modified launchers, standalone generation, five focused race-case repetitions,
four final rolling matrices, one full serial matrix, rolling refill timestamps,
exact result ordering,
distinct generated ports, unknown-case, port-bound, multi-case GUI, lock, and
Bash 3.2 rejection probes. A retained full matrix contained thirty-three
isolated mission copies, thirty-three targets, thirty-three `.alog` files, and
one physical pMissionEval row per case. Logs contained no warnings or
configuration errors, and no tested listener remained.

### Migrated Rollout: `utimerscript_h01`

uTimerScript H01 was audited and migrated as the only live harness on July 15,
2026. Its thirty-three documented cases, launcher tokens, generated
uTimerScript and pMissionEval configurations, external control schedules, and
final result order reconcile exactly.

pMissionEval owns the ordinary current-state verdicts. The launcher no longer
repeats scalar value checks already enforced by pMissionEval. Narrow bounded
`.alog` checks remain for exact comma-bearing payloads, status metadata,
expected status-variable absence, and publication histories that final MOOS
state cannot express. Every retained history check ends at the first
`MISSION_EVALUATED=true` timestamp. No application or grading variable was
added. The space-containing `UTS_STATUS` and custom-status values were removed
from result columns so each pMissionEval result is one whitespace-parseable row;
their existing case-specific assertions remain in bounded log evidence.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling execution, generated case fixtures, stride-30 port blocks,
a harness lock, canonical scoped teardown, bounded exact-PID `uPokeDB` control
calls, exact one-row mission-result validation, deterministic aggregation, and
normalized runner failures. The new thin mission wrapper only forwards
execution controls to `xlaunch.sh`, validates the pMissionEval row, and applies
the standalone cleanup backstop.

Validation exposed three timing/evidence assumptions in the legacy runner:

- `pause_toggle_external_pass` could finish evaluation before its second toggle
  because the legacy runner delayed `uMayFinish` until after both pokes. Under
  xlaunch, successful evaluation correctly begins shutdown immediately. The
  existing output and evaluation times moved from `0.5/0.8` to `10/12`, leaving
  both toggle pokes, variables, expected output, and grading condition
  unchanged. Five focused repetitions passed, and retained logs place both
  toggles before output and evaluation.
- `reset_time_all_posted_pass` may publish its intermediate counter values too
  quickly for pLogger to retain every value. Its existing final value is
  deterministically `2`, so pMissionEval now checks `UTS_REPEAT = 2` directly
  instead of relying on a shell log-count check.
- Repeated identical `UTS_AMOUNT=burst` posts may be coalesced in the log. A
  validation attempt to replace the existing status counter with direct `.alog`
  row counting was therefore rejected and reverted. The original
  `posted=4` status assertion remains, bounded by evaluation.

The external-control path keeps the legacy half-second startup allowance.
An initial readiness-poke experiment was removed because a very short autonomous
case could evaluate before the readiness client connected. Each actual
`uPokeDB` call is now time-bounded so an unavailable server cannot leave the
harness waiting indefinitely. This does not add a mission variable or alter
case stimulus.

Legacy measurements at warp 10 and `--max_time=40` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=4` | 3 | 99 / 99 | 99.85, 99.50, 99.65 | clean |
| legacy isolated serial, `--jobs=1` | 1 | 33 / 33 | 241.62 | clean |

Final migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=4` | 4 | 132 / 132 | 82.02, 81.15, 84.34, 80.41 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 33 / 33 | 308.83 | clean |

The migrated rolling mean is 81.98 seconds, 17.69 seconds (about 17.7 percent)
faster than the 99.67-second legacy mean because rolling refill avoids waiting
for the slowest case in a fixed wave. Serial increased 67.21 seconds (about
27.8 percent) because the standard xlaunch lifecycle and independent mission
and harness cleanup checks cannot overlap at `--jobs=1`.

Validation covered both skill static checkers, Bash syntax, ShellCheck,
standalone generation and live execution on an explicit port, retained
all-case generation, four final full rolling matrices, one final full serial
matrix, five focused pause-toggle repetitions, exact result ordering, distinct
ports, rolling-refill timestamps, unknown-case, job-bound, port-bound,
multi-case GUI, lock, Bash 3.2
rejection, and Homebrew Bash re-execution probes. A retained final matrix had
thirty-three isolated mission copies, thirty-three targets, thirty-three
`.alog` files, one physical pMissionEval row per case, no warning-bearing worker
logs, and no surviving tested listener.

### Migrated Rollout: `pspoofnode_h01`

pSpoofNode H01 was audited and migrated as the only live harness on July 15,
2026. Its thirty-three documented cases, launcher tokens, generated
pSpoofNode, uTimerScript, and pMissionEval configurations, structured report
contracts, and final result order reconcile exactly.

This harness is a narrow structured-log evidence exception. pMissionEval owns
the evaluation boundary and authoritative mission row. It requires an existing
`NODE_REPORT` for the twenty-three positive-output cases and uses the same
existing variable as a fail condition for the ten rejected-request cases, so
an unexpected report makes the mission itself fail. A final MOOS value still
cannot express exact `NODE_REPORT` fields, multiple contacts, position changes,
expiration, or cancellation. The launcher therefore retains those existing
`.alog` assertions as supplements. Every assertion is bounded by the first
`MISSION_EVALUATED=true`, so refresh reports produced during shutdown cannot
alter the case verdict. No application or grading variable was added.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling execution, generated case fixtures, stride-30 port blocks,
a harness lock, canonical scoped teardown, exact one-row mission-result
validation, deterministic aggregation, and normalized runner failures. The
thin mission wrapper forwards execution controls to `xlaunch.sh`, validates
the pMissionEval row, and applies the standalone cleanup backstop. Stable
`form` and `mmod` fields moved from prereport fragments into the ordinary
report so pMissionEval writes one complete row at grading time.

Untouched legacy measurements at warp 10 and `--max_time=30` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=4` | 3 | 99 / 99 | 36.15, 35.39, 35.91 | clean |
| legacy shared-stem serial, `--jobs=1` | 1 | 33 / 33 | 109.14 | clean |

Final migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=4` | 5 | 165 / 165 | 54.08, 51.52, 53.76, 54.37, 52.75 | clean |
| migrated isolated serial, `--jobs=1` | 2 | 66 / 66 | 190.10, 191.85 | clean |

The rolling mean increased from 35.82 to 53.30 seconds, 17.48 seconds (about
48.8 percent). The serial mean increased from 109.14 to 190.98 seconds, 81.84
seconds (about 75.0 percent). These
thirty-three cases are unusually short, so the standard xlaunch lifecycle and
independent mission and harness cleanup checks form a large share of each
case's wall time. Rolling execution overlaps that fixed cost only partially;
serial execution cannot overlap it. No case deadline, source schedule,
pSpoofNode behavior, or assertion was weakened to improve timing.

Validation covered both skill static checkers, Bash syntax, ShellCheck,
standalone generation and live execution on explicit ports, retained all-case
generation, five full rolling matrices, two full serial matrices, representative
static, moving, expiration, cancellation, and rejected-request cases, rolling
refill timestamps, exact result ordering, distinct generated ports,
unknown-case, job-bound, port-bound, multi-case GUI, lock, Bash 3.2 rejection,
and Homebrew Bash re-execution probes. A retained full matrix contained
thirty-three isolated mission copies, targets, `.alog` files, and physical
pMissionEval rows, with no warning-bearing worker logs or surviving tested
listener. Sampling confirmed that large numbers of normal post-evaluation
refresh reports were excluded from grading. A targeted injected-report probe
also proved that pMissionEval, rather than the shell, fails a rejected-request
case when any unexpected `NODE_REPORT` appears.

### Migrated Rollout: `pantler_h01`

pAntler H01 was audited and migrated as the only live harness on July 15,
2026. Its six documented cases, launcher tokens, patch files, generated
pAntler and aliased uTimerScript configurations, pMissionEval conditions, and
final result order reconcile exactly.

This is an ordinary mission-owned harness. Each graded flag is published only
by a process that pAntler must successfully launch. The baseline, alias,
dual-alias, launch-filter, system-path, and extra-process-parameter contracts
remain in pMissionEval without shell evidence checks or expected-versus-actual
grading. Stable `form` and `mmod` fields moved from prereport fragments into
the ordinary report so each verdict is written as one physical row.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling execution, explicit case-to-patch mapping, stride-30 port
blocks, a harness lock, canonical scoped teardown, exact one-row mission-result
validation, deterministic aggregation, and normalized runner failures. The
thin mission wrapper forwards execution controls to `xlaunch.sh`, validates
the pMissionEval row, and applies the standalone cleanup backstop. The ordinary
`launch.sh` and all case stimuli and pass/fail conditions are unchanged.

Untouched legacy measurements at warp 10 and `--max_time=20` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=2` | 3 | 18 / 18 | 18.04, 17.61, 16.16 | clean |
| legacy shared-stem serial, `--jobs=1` | 1 | 6 / 6 | 29.65 | clean |

Final migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=2` | 5 | 30 / 30 | 18.21, 17.32, 18.61, 17.89, 18.34 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 6 / 6 | 34.25 | clean |

The rolling mean increased from 17.27 to 18.07 seconds, 0.80 seconds (about
4.7 percent), which remains within the observed run-to-run range. Serial
increased 4.60 seconds (about 15.5 percent) because isolated preparation and
the standard mission and harness cleanup checks cannot overlap at
`--jobs=1`.

Validation covered both skill static checkers, Bash syntax, ShellCheck,
standalone generation and live execution, retained all-case generation, five
full rolling matrices, one full serial matrix, focused alias, dual-alias, and
filter cases, rolling-refill timestamps, exact result ordering, distinct
generated ports, unknown-case, job-bound, port-bound, multi-case GUI, lock,
Bash 3.2 rejection, and Homebrew Bash re-execution probes. A retained full
matrix contained six isolated mission copies, targets, `.alog` files, and
physical pMissionEval rows, with no warning-bearing worker logs or surviving
tested listener.

The first migrated all-case generation attempt failed preparation for the five
patched cases because the newly composed `nspatch` command contained two shell
line-continuation backslashes. The baseline case still generated. The typo was
corrected before any migrated live mission ran; clean regeneration and all
subsequent validation passed.

### Migrated Rollout: `pshare_h02`

pShare H02 was audited and migrated as the only live harness on July 15,
2026. Its twelve documented cases, twenty-eight community patch files,
four-community target topology, pMissionEval conditions, and final result
order reconcile exactly.

This is an ordinary mission-owned harness. The shore pMissionEval instance
directly grades fan-in, competing updates, multiple inputs, multicast,
dynamic routes, custom multicast settings, relay branches, route lists,
alias commands, and runtime commands from existing mission variables. Relay
evaluators publish existing proof flags where needed, but the shell performs
no topology or expected-versus-actual grading. Stable `form` and `mmod`
fields moved from prereport fragments into the ordinary report so the
authoritative verdict is written as one physical row. No application,
grading variable, stimulus, condition, or ordinary `launch.sh` behavior was
added or changed.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling execution, explicit per-community case mapping, stride-50
port blocks, a harness lock, canonical scoped teardown, exact one-row mission
result validation, deterministic aggregation, and normalized runner failures.
Each case receives four MOOSDB ports at offsets 0 through 3 and four pShare
ports at offsets 10 through 13. The custom multicast base is derived from the
case's relay pShare port, so it remains inside the isolated block. The thin
mission wrapper forwards all eight ports to xlaunch, validates all four target
files in generation mode, validates the live pMissionEval row, and applies the
standalone cleanup backstop.

Untouched legacy measurements at warp 10 and `--max_time=65` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=2` | 3 | 36 / 36 | 62.83-63.69 | clean |
| legacy shared-stem serial, `--jobs=1` | 1 | 12 / 12 | 115.22 | clean |

Final migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=2` | 4 | 48 / 48 | 63.81, 64.09, 64.00, 63.41 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 12 / 12 | 125.45 | clean |
| migrated serial, one-second cleanup-grace control | 1 | 12 / 12 | 110.50 | clean |

The 63.83-second migrated rolling mean is effectively unchanged from the
observed legacy range. Default serial increased 10.23 seconds (about 8.9
percent). The controlled serial run with the scoped helper's supported
one-second interrupt and terminate grace was 14.95 seconds faster than the
canonical-default run and 4.72 seconds faster than legacy. This isolates the
serial delta to the conservative shared three-second cleanup policy rather
than pShare execution, case isolation, or grading. The canonical skill
defaults remain unchanged.

Validation covered the skill static checker, Bash syntax, ShellCheck,
standalone generation and live execution on eight explicit ports, retained
all-case generation, four full rolling matrices, one canonical serial matrix,
one cleanup-grace control matrix, focused multicast, custom-multicast, relay,
and runtime-route cases, rolling-refill timestamps, exact result ordering,
distinct generated port blocks, unknown-case, job-bound, port-bound,
multi-case GUI, lock, Bash 3.2 rejection, Homebrew Bash re-execution,
missing-result, and preparation-error probes. A retained full matrix contained
twelve isolated mission copies, forty-eight targets, forty-eight `.alog`
files, and one physical shore pMissionEval row per case, with no warning-bearing
worker logs or surviving tested listener.

### Migrated Rollout: `psearchgrid_h01`

pSearchGrid H01 was audited and migrated as the only live harness on July 15,
2026. Its twenty-three documented cases, generated pSearchGrid and
uTimerScript configurations, pMissionEval conditions, structured grid checks,
and final result order reconcile exactly.

This is an ordinary history-aware harness. pMissionEval directly grades
whether each required existing grid variable was published, or whether an
expected-absent variable remained absent. The launcher supplements that
mission-owned verdict only for contracts that require structured grid payload
contents, publication counts, or state before and after a reset. Supplemental
evidence is bounded at the first `MISSION_EVALUATED=true`; there is no
expected-versus-actual grade comparison and no new MOOS application or grading
variable.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling execution, generated per-case configuration, stride-20
port blocks, a harness lock, canonical scoped teardown, exact one-row
mission-result validation, deterministic aggregation, and normalized runner
failures. The thin mission wrapper forwards execution controls to
`xlaunch.sh`, validates generation and the pMissionEval row, and applies the
standalone cleanup backstop.

Two narrowly scoped corrections were required and retain the cases' intended
test value. The untouched standalone stem claimed `node_local_delta_pass` but
sent no node report and graded only readiness. Its default is now
`initial_grid_publish_pass`, using the existing automatic `VIEW_GRID`
publication as the pMissionEval condition. The harness's separate
`node_local_delta_pass` case still sends and validates a node report and its
delta. During rolling validation, `repeated_cell_delta_pass` sometimes split
its two same-time reports across adjacent 10 Hz pSearchGrid cycles, producing
two valid `x,1` deltas instead of the case's intended batched `x,2` delta. That
case alone now uses the existing pSearchGrid `AppTick=1` setting so the two
same-time reports are deterministically processed in one cycle. The distinct
`sequential_delta_cleared_pass` case remains at 10 Hz and still proves that
separated reports produce two cleared `x,1` deltas.

Untouched legacy measurements at warp 10 and `--max_time=30` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=4` | 3 | 69 / 69 | 25.74, 24.35, 24.71 | clean |
| legacy shared-stem serial, `--jobs=1` | 1 | 23 / 23 | 76.18 | clean |

Final migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=4` | 3 | 69 / 69 | 37.75, 38.02, 37.38 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 23 / 23 | 129.90 | clean |

The rolling mean increased from 24.93 to 37.72 seconds, 12.79 seconds (about
51.3 percent). Serial increased 53.72 seconds (about 70.5 percent). Both
deltas are approximately two seconds per case or per rolling wave and align
with the standard `xlaunch` shutdown lifecycle that the legacy launcher
bypassed; the cases perform the same stimuli and checks apart from the two
corrections above.

Validation covered both skill static checkers, Bash syntax, ShellCheck,
standalone generation and live execution, retained all-case generation, three
final full rolling matrices, one full serial matrix, five focused repetitions
of the stabilized same-cell case, focused nominal, expected-absence, reset,
sequential-history, and custom-variable cases, rolling-refill events, exact
result ordering, distinct generated ports, unknown-case, job-bound,
port-bound, multi-case GUI, lock, Bash 3.2 rejection, Homebrew Bash
re-execution, missing-result, and preparation-error probes. A retained full
matrix contained twenty-three isolated mission copies, targets, `.alog`
files, and one physical pMissionEval row per case, with no warning-bearing
worker logs or surviving tested listener.

### Migrated Rollout: `testfailure_h01`

TestFailure H01 was audited and migrated as the only live harness on July 15,
2026. Its nine documented cases, eight shoreside patches, eight behavior
patches, generated evaluator conditions, behavior parameters, and final result
order reconcile exactly.

This remains an ordinary mission-owned harness even though two subjects are
expected to crash. pMissionEval directly grades the existing process-health,
helm-gap, behavior-completion, and load-watch signals. The legacy
`crash_on_complete_fail` and `unknown_type_default_crash_fail` tokens are
preserved, but their authoritative mission result is
`grade=pass expected=process_loss` when uProcessWatch observes the requested
pHelmIvP loss. The launcher neither compares expected and actual grades nor
inspects logs for a second verdict.

The migrated launcher uses Bash 5.1 rolling scheduling, isolated copies for
serial and rolling execution, explicit shoreside-and-behavior patch mapping,
stride-20 port blocks, a harness lock, canonical scoped teardown, exact
one-row mission-result validation, deterministic aggregation, and normalized
runner failures. The thin mission wrapper forwards execution controls to
`xlaunch.sh`, validates generation and the pMissionEval row, and applies the
standalone cleanup backstop. No stimulus, evaluator condition, behavior
parameter, application, or grading variable changed. Stable `form` and `mmod`
fields moved from prereport fragments into each evaluator's ordinary report
so the authoritative verdict is written in one physical operation.

Untouched legacy measurements at warp 10 and `--max_time=150` were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| legacy batch-wave, `--jobs=4` | 3 | 23 / 27 | 122.86, 98.49, 92.53 | clean |
| legacy shared-stem serial, `--jobs=1` | 1 | 9 / 9 | 121.66 | clean |

The first legacy rolling run lacked results for three burn/hang cases, the
second lacked the default-duration burn result, and the third passed all nine.
The crash cases were stable in all three. These missing rows and the wide
timing range are recorded as pre-existing burn/hang sensitivity rather than a
migration blocker.

Final migrated measurements were:

| configuration | runs | passing rows / rows | wall seconds | cleanup |
| --- | ---: | ---: | --- | --- |
| migrated rolling, `--jobs=4` | 5 | 45 / 45 | 36.32, 38.89, 37.95, 38.28, 39.31 | clean |
| migrated isolated serial, `--jobs=1` | 1 | 9 / 9 | 112.80 | clean |

The rolling mean decreased from 104.63 to 38.15 seconds, 66.48 seconds (about
63.5 percent), while also eliminating the observed missing rows. Rolling
refill avoids waiting for every case in a legacy wave, isolated copies prevent
sidecar and artifact reuse, and the one-write pMissionEval rows remove the
prereport race. Serial decreased 8.86 seconds (about 7.3 percent), which is
small relative to the legacy burn/hang variance.

Validation covered both skill static checkers, Bash syntax, ShellCheck,
standalone generation and live execution, retained all-case generation, five
full rolling matrices, one full serial matrix, focused burn and expected-crash
cases, rolling-refill events, exact result ordering, distinct generated ports
and paired patch overlays, unknown-case, job-bound, port-bound, multi-case GUI,
lock, Bash 3.2 rejection, Homebrew Bash re-execution, missing-result, and
preparation-error probes. A retained full matrix contained nine isolated
mission copies, shoreside targets, behavior targets, `.alog` files, and one
physical pMissionEval row per case, with no warning-bearing worker logs or
surviving tested listener.

### Completed Migration: `fixedturn_h01`

FixedTurn H01 baseline evidence was gathered as the only live harness on July
15, 2026. Its twenty-four cases passed 72/72 rows across three untouched
legacy `--jobs=4` wave matrices in 84.01, 74.07, and 74.40 seconds. The
untouched shared-stem serial matrix passed 24/24 in 232.86 seconds. No tested
listener remained.

The migrated Bash 5.1 launcher uses isolated mission copies, rolling slot
refill, deterministic selected-case aggregation, root-scoped cleanup, and the
existing configurable minimum-12 `--port_stride`. It preserves the +0/+1
MOOSDB and +10/+11 pShare layout and every shore MOOS, vehicle MOOS, and
vehicle behavior overlay mapping. The mission wrapper remains case-agnostic:
it forwards launch arguments, requires exactly one pMissionEval row, and does
not grade or reinterpret that row. No case patch, pass condition, stimulus,
or coverage claim changed.

The installed evaluator checker reports one known false positive for the
existing compound `(FT_DONE=true) or (MISSION_TIMEOUT=true)` and
`(TURN_TWO=true) or (MISSION_TIMEOUT=true)` lead conditions. Local
`lib_logic` documentation and implementation both accept this fully
parenthesized syntax and evaluate it as boolean OR, and live runs proved both
the completion and timeout branches. The expressions were therefore retained
without adding a helper variable or delaying successful cases to the timeout
window. The harness static checker, Bash syntax, and ShellCheck pass; the eval
checker has no other finding.

Three migrated `--jobs=4` timing matrices passed 72/72 rows in 64.02, 65.60,
and 65.92 seconds. Their 65.18-second mean is 12.31 seconds, about 15.9
percent, faster than the 77.49-second legacy wave mean. The migrated isolated
serial matrix passed 24/24 in 233.71 seconds, 0.85 seconds or about 0.4 percent
slower than the 232.86-second legacy serial result and operationally
unchanged. A fourth retained rolling matrix passed 24/24 in 65.98 seconds and
provided artifact evidence rather than another timing sample.

Validation also covered standalone stem generation and live execution; an
all-case generation matrix; nominal, multi-turn, and expected-invalid focused
runs; exact case order; rolling refill; one physical pMissionEval row per case;
distinct generated MOOSDB and pShare ports; 48 intended sidecars with none in
the shared stem; unknown-case, numeric-bound, multi-case GUI, active-lock,
Bash 3.2 rejection, Homebrew Bash re-execution, missing-result,
preparation-error, teardown-error, and 24-row failure-aggregation probes. No
tested listener survived. Retained logs contained only the unchanged waypoint
completion advisory and the deliberately malformed update warning in the
recovery case.

### Completed Migration: `periodic_speed_h01`

PeriodicSpeed H01 baseline evidence was gathered as the only live harness on
July 15-16, 2026. Its twenty-four cases passed 72/72 rows across three
untouched legacy `--jobs=4` wave matrices in 60.96, 58.32, and 59.59 seconds.
The untouched shared-stem serial matrix passed 24/24 in 179.16 seconds. No
tested listener remained.

The migrated Bash 5.1 launcher uses isolated mission copies for serial and
rolling execution, rolling slot refill, deterministic selected-case
aggregation, the existing configurable minimum-12 port stride, root-scoped
cleanup, a harness lock, and strict one-row pMissionEval result validation. It
preserves every shoreside, vehicle, and behavior overlay mapping, including
the explicit helm-malconfiguration evaluator overlay for the twelve invalid
configuration cases. The manual visual case remains selectable but outside
the default twenty-four-case CI matrix. The mission wrapper only forwards
launch controls, validates the mission-owned result, and applies the normal
standalone cleanup backstop. No case patch, evaluator condition, stimulus,
behavior value, grading variable, or coverage claim changed.

The installed evaluator checker reports only its known false positive for the
existing fully parenthesized boolean-OR lead conditions. Local `lib_logic`
documentation and source accept that syntax, live runs exercised the
expressions, and the conditions were retained rather than introducing helper
variables. The harness static checker, Bash syntax, and ShellCheck pass.

Three migrated `--jobs=4` timing matrices passed 72/72 rows in 53.71, 55.83,
and 55.14 seconds. Their 54.89-second mean is 4.73 seconds, about 7.9 percent,
faster than the 59.62-second legacy wave mean. The improvement comes from
rolling refill; no test timing changed. The migrated isolated serial matrix
passed 24/24 in 188.16 seconds, 9.00 seconds or about 5.0 percent slower than
the legacy serial result. That is about 0.38 seconds per case for copying the
mission, using the thin wrapper, strictly validating its result, and applying
scoped cleanup.

Validation also covered standalone stem generation and live execution; an
all-case generation matrix; nominal, reset/reactivation, expected-invalid,
and manual-visual focused cases; exact case order; rolling refill; one
physical pMissionEval row per case; distinct generated MOOSDB and pShare
ports; all 56 intended sidecars with none in the shared stem; unknown-case,
numeric-bound, multi-case GUI, active-lock, Bash 3.2 rejection, Homebrew Bash
re-execution, missing-result, preparation-error, teardown-error, and 24-row
failure-aggregation probes. A retained full rolling matrix passed 24/24 in
53.54 seconds and confirmed the artifact contract. No tested process or
listener survived. One listener initially seen inside the broad numeric port
range was identified as an unrelated Adobe Creative Cloud process, not a
MOOS or harness process.

### Completed Migration: `zigzag_h01`

ZigZag H01 baseline evidence was gathered as the only live harness on July
16, 2026. Its thirty-five cases passed 105/105 rows across three untouched
legacy `--jobs=4` wave matrices in 121.61, 117.98, and 118.44 seconds. Their
mean was 119.34 seconds. The untouched shared-stem serial matrix passed 35/35
in 343.96 seconds. No tested listener remained.

The migrated Bash 5.1 launcher uses isolated mission copies for every serial
and rolling case, rolling slot refill, deterministic selected-case
aggregation, a configurable minimum-12 port stride, root-scoped cleanup, a
harness lock, and strict one-row pMissionEval result validation. It preserves
all thirty-five cases and every shoreside, vehicle-MOOS, and vehicle-behavior
overlay mapping, including the explicit helm-malconfiguration overlay for the
ten expected-invalid cases. The mission wrapper only forwards launch
controls, validates the mission-owned result, and applies the normal
standalone cleanup backstop. No case patch, evaluator condition, stimulus,
behavior value, grading variable, or coverage claim changed.

The installed evaluator checker reports only its known false positive for the
existing fully parenthesized boolean-OR lead conditions. Local `lib_logic`
documentation and source accept that syntax, and live runs exercised both the
normal completion family and the intentionally timeout-owned low-speed case.
The expressions were retained rather than adding helper variables. The
harness static checker, Bash syntax, and ShellCheck pass.

Three migrated `--jobs=4` timing matrices passed 105/105 rows in 93.75,
95.24, and 94.11 seconds. Their 94.37-second mean is 24.97 seconds, about 20.9
percent, faster than the legacy mean. A retained full rolling matrix passed
35/35 in 93.11 seconds. The improvement comes from rolling refill; no test
timing changed. The migrated isolated serial matrix passed 35/35 in 353.62
seconds, 9.66 seconds or about 2.8 percent slower than legacy. That is about
0.28 seconds per case for copying the mission, using the thin wrapper,
strictly validating the result, and applying scoped cleanup.

Validation also covered standalone stem generation and live execution; an
all-case generation matrix; nominal, timeout-owned speed clipping,
expected-invalid, and GUI focused cases; exact case order; rolling refill;
one physical pMissionEval row per case; distinct generated MOOSDB and pShare
ports; all 77 intended sidecars with none in the shared stem; unknown-case,
numeric-bound, multi-case GUI, active-lock, Bash 3.2 rejection, Homebrew Bash
re-execution, missing-result, preparation-error, teardown-error, and 35-row
failure-aggregation probes. The retained live matrix contained 70 `.alog`
files and 35 mission-owned passing rows. Its only behavior warnings were the
unchanged completed-approach advisory and the deliberately malformed runtime
update in the recovery case. No tested process or listener survived.

### Completed Migration: `memoryturnlimit_h01`

MemoryTurnLimit H01 baseline evidence was gathered as the only live harness on
July 16, 2026. Its twenty-one cases passed 63/63 rows across three untouched
legacy `--jobs=4` wave matrices in 84.48, 84.74, and 84.24 seconds, for an
84.49-second mean. The legacy serial matrix passed 21/21 in 181.58 seconds.
The five existing invalid-configuration cases remained explicitly solo because
their README contract already requires isolated helm failure observation.

The migrated Bash 5.1 launcher uses isolated mission copies for serial and
rolling execution, rolling refill for ordinary cases, deterministic selected-
case aggregation, distinct port blocks, root-scoped cleanup, a harness lock,
and strict one-row pMissionEval result validation. Before each of the five solo
cases, it drains the rolling slots; each solo case runs alone, after which
rolling refill resumes. All twenty-one case mappings and all mission-owned
grading conditions were preserved. The mission wrapper only forwards launch
controls, validates the pMissionEval row, and applies the standalone cleanup
backstop.

One narrow pre-existing timing edge was found in `short_memory_pass`. One full
matrix and one of ten focused repetitions sampled `NAV_HEADING` just below its
unchanged 112-degree lower bound, while all other motion, memory-average,
mismatch, speed, and health conditions passed. The checked-in legacy result
was also close to that boundary. The case-specific existing `EVAL_TIME` event
was moved from mission time 16 to 17; no threshold, stimulus, grading variable,
or behavior parameter changed. Ten post-fix focused repetitions passed with
headings from 114.4 through 117.8 degrees, and the full 21-case matrix then
passed again. This retains the case's test value while removing a sampling
race rather than weakening its assertion.

Three migrated rolling matrices passed 63/63 rows in 84.13, 86.06, and 86.36
seconds. Their 85.52-second mean is 1.03 seconds, about 1.2 percent, slower
than legacy and within the observed run-to-run range. The five serialized
failure-contract cases dominate completion time, so rolling refill is not
expected to provide a large gain. The isolated serial matrix passed 21/21 in
189.77 seconds, 8.19 seconds or about 4.5 percent slower than legacy, roughly
0.39 seconds per case for copy, wrapper validation, and scoped cleanup.

Validation covered all-case generation and live matrices, nominal and both
helm-malconfiguration and behavior-warning failure contracts, exact case
order, solo scheduling, rolling refill, one physical pMissionEval row per
case, distinct MOOSDB and pShare ports, standalone stem generation and live
execution, unknown-case and numeric-bound rejection, multi-case GUI rejection,
active-lock behavior, Bash 3.2 rejection, and Homebrew Bash re-execution. Bash
syntax, ShellCheck, and the harness checker pass. The evaluator checker has
only its known false positive for the existing, valid, fully parenthesized OR
lead conditions. No tested process survived cleanup.

### Completed Migration: `timer_h01`

Timer H01 baseline evidence was gathered as the only live harness on July 16,
2026. Its eighteen cases passed 54/54 rows across three untouched legacy
`--jobs=4` wave matrices in 47.97, 47.76, and 47.12 seconds, for a 47.62-second
mean. The untouched serial matrix passed 18/18 in 125.92 seconds. No baseline
failure or surviving test process was observed.

The migrated Bash 5.1 launcher uses isolated mission copies for serial and
rolling cases, rolling slot refill, deterministic selected-case aggregation,
the existing configurable minimum-12 port stride, root-scoped cleanup, a
harness lock, and strict one-row pMissionEval result validation. It preserves
all eighteen case mappings and every shoreside, vehicle-MOOS, and behavior
overlay. The mission wrapper forwards launch controls, validates the mission-
owned pMissionEval row, and provides the normal standalone cleanup backstop.
No case patch, event time, pass condition, behavior parameter, grading
variable, or coverage claim changed.

Three migrated `--jobs=4` matrices passed 54/54 rows in 43.19, 43.89, and
43.98 seconds. Their 43.69-second mean is 3.93 seconds, about 8.3 percent,
faster than the legacy wave mean. The isolated serial matrix passed 18/18 in
144.60 seconds, 18.68 seconds or about 14.8 percent slower than legacy. The
roughly 1.04-second-per-case serial difference comes from the modern mission
wrapper and scoped-cleanup lifecycle that the legacy harness bypassed; case
mission timing and evaluator conditions were unchanged.

Validation covered all-case generation, three full rolling matrices, one full
serial matrix, all mission-owned expected-absence verdicts, standalone stem
generation and live execution, exact case order, rolling refill, distinct
MOOSDB and pShare ports, one physical pMissionEval row per case, unknown-case
and numeric-bound rejection, multi-case GUI rejection, active-lock behavior,
Bash 3.2 rejection, and Homebrew Bash re-execution. Bash syntax, ShellCheck,
and the harness checker pass. The evaluator checker has only its known false
positive for the existing, valid, fully parenthesized OR lead conditions. No
tested process survived cleanup.

### Completed Migration: `collision_h01`

Collision H01 baseline evidence was gathered as the only live harness on July
16, 2026. Its twenty-one cases passed 63/63 rows across three untouched legacy
`--jobs=4` wave matrices in 65.34, 65.38, and 63.87 seconds, for a 64.86-second
mean. The untouched serial matrix passed 21/21 in 172.71 seconds. This includes
the expected-collision and helm-malconfiguration cases; pMissionEval already
turns the required negative subject evidence into a harness-level pass.

The migrated Bash 5.1 launcher uses isolated mission copies for every serial
and rolling case, rolling slot refill, deterministic selected-case
aggregation, configurable minimum-12 port spacing, root-scoped cleanup, a
harness lock, and strict one-row pMissionEval result validation. It preserves
all twenty-one shoreside, vehicle-MOOS, and behavior overlay mappings. The
mission wrapper forwards launch controls, validates the mission-owned result,
and supplies the normal standalone cleanup backstop. No case patch, timing,
geometry, pass condition, grading variable, or coverage claim changed.

Three migrated rolling matrices passed 63/63 rows in 57.11, 59.35, and 56.79
seconds. Their 57.75-second mean is 7.11 seconds, about 11.0 percent, faster
than the legacy wave mean. The isolated serial matrix passed 21/21 in 201.62
seconds, 28.91 seconds or about 16.7 percent slower than legacy, roughly 1.38
seconds per case for the standard wrapper and scoped-cleanup lifecycle that
the legacy harness bypassed.

Validation covered all-case generation, three full rolling matrices, one full
serial matrix, nominal resolution, expected collision, expected absence, and
helm-malconfiguration verdicts, standalone stem generation and live
execution, exact case order, rolling refill, distinct MOOSDB and pShare ports,
one physical pMissionEval row per case, unknown-case and numeric-bound
rejection, multi-case GUI rejection, active-lock behavior, Bash 3.2 rejection,
and Homebrew Bash re-execution. Bash syntax, ShellCheck, and the harness
checker pass. No tested process survived cleanup.

### Completed Migration: `loiter_h01`

Loiter H01 baseline evidence was gathered as the only live harness on July 16,
2026. Its thirty-four cases passed 102/102 rows across three untouched legacy
`--jobs=4` wave matrices in 104.56, 108.30, and 105.76 seconds, for a
106.21-second mean. The untouched serial matrix passed 34/34 in 278.67
seconds. No baseline failure was observed.

The migrated Bash 5.1 launcher uses isolated mission copies for serial and
rolling execution, rolling slot refill, deterministic selected-case
aggregation, configurable minimum-12 port spacing, root-scoped cleanup, a
harness lock, and strict one-row pMissionEval result validation. It preserves
all thirty-four case mappings and every shoreside, vehicle-MOOS, and behavior
overlay. The mission wrapper is now a thin, case-agnostic forwarder and result
validator; it no longer routes `--case` or `--jobs` back into the harness. No
case patch, evaluator condition, event time, behavior value, grading variable,
or coverage claim changed.

Three migrated rolling matrices passed 102/102 rows in 85.71, 87.65, and
89.98 seconds. Their 87.78-second mean is 18.43 seconds, about 17.4 percent,
faster than the legacy wave mean. The isolated serial matrix passed 34/34 in
318.03 seconds, 39.36 seconds or about 14.1 percent slower than legacy,
roughly 1.16 seconds per case for the standard wrapper and scoped-cleanup
lifecycle.

Validation covered all-case generation, three full rolling matrices, one full
serial matrix, nominal acquisition, runtime updates, output suffixing,
expected behavior-warning and helm-malconfiguration verdicts, standalone stem
generation and live execution, exact case order, rolling refill, distinct
MOOSDB and pShare ports, one physical pMissionEval row per case, unknown-case
and numeric-bound rejection, multi-case GUI rejection, active-lock behavior,
Bash 3.2 rejection, and Homebrew Bash re-execution. Bash syntax, ShellCheck,
and the harness checker pass. No tested process survived cleanup.

### Completed Migration: `obstacle_behavior_h01`

Obstacle Behavior H01 baseline evidence was gathered as the only live harness
on July 16, 2026. Three clean legacy `--jobs=4` matrices passed 63/63 rows in
75.30, 74.89, and 74.85 seconds, for a 75.01-second mean. Two additional
concurrent legacy matrices each failed only `two_obstacles_clean_pass`, which
reported a real mission-owned collision. That case then passed 10/10 when run
alone. The untouched serial matrix passed 21/21 in 208.48 seconds.

The migrated Bash 5.1 launcher uses isolated mission copies, rolling slot
refill, deterministic selected-case aggregation, configurable minimum-12 port
spacing, root-scoped cleanup, a harness lock, and strict one-row pMissionEval
result validation. It preserves all twenty-one case mappings and every
shoreside, vehicle-MOOS, and behavior overlay. The mission wrapper only
forwards launch controls, validates the mission-owned row, and applies the
standalone cleanup backstop. The initial migration did not change any case
patch, geometry, event time, evaluator condition, behavior value, grading
variable, or coverage claim.

Initial migrated rolling validation reproduced the same pre-existing
`two_obstacles_clean_pass` collision once in five matrices, while the case
remained 10/10 alone. The first conservative scheduler therefore drained
active slots and ran that one load-sensitive case alone, then resumed rolling
refill. Three clean matrices passed 63/63 rows in 83.83, 83.70, and 83.03
seconds, for an 83.52-second mean. The initial migrated serial matrix passed
21/21 in 223.51 seconds.

One final-scheduler matrix also produced a one-off `launch_rc=1` with no
pMissionEval row for `default_auto_request_pass`. It did not recur in 10
focused repetitions, the next full matrix, or the retained evidence matrix,
and no process remained. It is recorded as an isolated runner/startup outlier,
not hidden or converted into a pass.

Validation covered all-case generation, repeated full rolling and serial
matrices, the explicitly solo scheduling trace, nominal avoidance, expected
collision, and helm-malconfiguration verdicts, standalone stem generation and
live execution, exact case order, distinct MOOSDB and pShare ports, one
physical pMissionEval row per case, unknown-case and numeric-bound rejection,
multi-case GUI rejection, active-lock behavior, Bash 3.2 rejection, and
Homebrew Bash re-execution. Bash syntax, ShellCheck, and the harness checker
pass. No tested process survived cleanup.

A July 16 case-quality follow-up removed that temporary solo exception. The
old two-obstacle evaluator required only one `OBSTACLE_ALERT`, so one spawned
avoidance instance could satisfy the documented two-obstacle claim. The case
now uses pMissionEval's existing mail-count support to require at least two
alerts. `BHV_AvoidObstacleV24` also posts its existing `$[OID]` label through
a `spawnxflag`; the case-specific node and shore broker overlays carry the one
new `OBSTACLE_SPAWNED` evidence variable to pMissionEval, which requires the
later `ob_two` instance. No application was added.

The original case speed of 1.8 still produced one real collision during
isolated repetition. Reducing only this case's existing transit speed to 1.6
provided more helm/control margin without changing either obstacle, arrival,
or zero-collision requirements. Ten focused repeats then passed 10/10. Five
final ordinary rolling matrices passed 105/105 rows in 62, 62, 62, 61, and 63
seconds, for a 62.0-second mean. This is 13.01 seconds, about 17.3 percent,
faster than the clean legacy mean and 21.52 seconds faster than the temporary
solo scheduler mean. The final serial matrix passed 21/21 in 222 seconds,
13.52 seconds or about 6.5 percent slower than legacy.

A deliberate mutation removed the `ob_two` vehicle stimulus. The vehicle
still arrived, but pMissionEval reported `obstacle_alert_count=1`,
`obstacle_spawned=ob_one`, and `grade=fail`, proving that one obstacle no
longer satisfies the case. One pre-fix concurrent matrix had two unrelated
one-obstacle collision failures in an abnormal 141 seconds, and another was
invalidated by a confirmed macOS sleep/wake interval; neither is included in
the final performance or repeatability statistics. The original stimulus was
restored before all final rolling and serial evidence was gathered.

### Completed Migration: `opregion_h01`

OpRegion H01 baseline evidence was gathered as the only live harness on July
16, 2026. Its twenty-five cases passed 75/75 rows across three untouched
legacy `--jobs=4` wave matrices in 80.03, 79.25, and 79.64 seconds, for a
79.64-second mean. The untouched serial matrix passed 25/25 in 203.60 seconds.
No baseline failure was observed.

The migrated Bash 5.1 launcher uses isolated mission copies, rolling slot
refill, deterministic selected-case aggregation, configurable minimum-12 port
spacing, root-scoped cleanup, a harness lock, and strict one-row pMissionEval
result validation. It preserves all twenty-five cases and every shoreside,
vehicle-MOOS, and behavior overlay. The mission wrapper is now a thin,
case-agnostic forwarder and result validator; it no longer routes `--case` or
`--jobs` back into the harness. No case patch, evaluator condition, safety
timing, behavior value, grading variable, or coverage claim changed.

Three migrated rolling matrices passed 75/75 rows in 62.97, 64.19, and 63.91
seconds. Their 63.69-second mean is 15.95 seconds, about 20.0 percent, faster
than the legacy wave mean. The isolated serial matrix passed 25/25 in 233.99
seconds, 30.39 seconds or about 14.9 percent slower than legacy, roughly 1.22
seconds per case for the standard wrapper and scoped-cleanup lifecycle.

Validation covered all-case generation, three full rolling matrices, one full
serial matrix, nominal containment, save and halt breaches, timing gates,
runtime region updates, behavior errors, and helm-malconfiguration verdicts,
standalone stem generation and live execution, exact case order, rolling
refill, distinct MOOSDB and pShare ports, one physical pMissionEval row per
case, unknown-case and numeric-bound rejection, multi-case GUI rejection,
active-lock behavior, Bash 3.2 rejection, and Homebrew Bash re-execution. Bash
syntax, ShellCheck, and the harness checker pass. No tested process survived
cleanup.

### Completed Migration: `shadow_h01`

Shadow H01 baseline evidence was gathered as the only live harness on July 16,
2026. Its thirty-five cases passed 105/105 rows across three untouched legacy
`--jobs=4` wave matrices in 127, 127, and 126 seconds, for a 126.67-second
mean. The untouched serial matrix passed 35/35 in 377 seconds. No baseline
failure was observed.

The migrated Bash 5.1 launcher uses isolated mission copies for serial and
rolling execution, rolling slot refill, deterministic selected-case
aggregation, three MOOSDB and three pShare ports per two-vehicle case,
root-scoped cleanup, a harness lock, and strict one-row pMissionEval result
validation. All thirty-five README tokens, launcher cases, and explicit case
mappings reconcile exactly. All existing patches, event times, behavior
parameters, evaluator conditions, grading variables, and coverage claims were
preserved. The mission wrapper is now a thin single-scenario forwarder rather
than routing harness arguments back into the harness.

Initial migrated validation exposed two intermittent infrastructure failures:
`turn_north_shadow_pass` once produced no result during rolling execution, and
`heading_widths_pass` once did the same during serial execution. The retained
serial failure showed both vehicle communities operating normally, while the
shoreside log contained remote vehicle traffic but none of the local evaluator
state. This means pMissionEval did not reject either case; pAntler launched the
shoreside only partway through its process list. Both cases then passed 10/10
focused repetitions with unchanged test inputs and grading.

The faster rolling lifecycle was repeatedly starting twelve shoreside
processes at the existing 100-millisecond pAntler spacing. The existing
`MSBetweenLaunches` setting was increased from 100 to 200 milliseconds for the
shoreside only. This changes wall-clock process startup pacing, not MOOS event
time, mission behavior, stimulus, or grading. Three valid post-fix rolling
matrices passed 105/105 rows in 110, 117, and 112 seconds, for a 113.0-second
mean. That is 13.67 seconds, about 10.8 percent, faster than the legacy wave
mean. The post-fix isolated serial matrix passed 35/35 in 422 seconds, 45
seconds or about 11.9 percent slower than legacy due to per-case copies, the
thin wrapper lifecycle, scoped cleanup, and the additional startup spacing.

One additional rolling attempt was invalidated by a macOS sleep/wake event:
the shell wall clock jumped to 367 seconds while only about 30 seconds of
active tool time elapsed. The four cases live across suspension evaluated with
stale Shadow contact state; all cases launched after wake passed. This run is
recorded as environmental contamination and excluded from performance and
repeatability statistics.

Validation covered all-case generation, three clean final rolling matrices,
one clean final serial matrix, twenty focused repeats, standalone stem
generation and live execution, exact case order and mapping reconciliation,
all expected-warning and expected-malconfiguration contracts, one physical
pMissionEval row per case, 105 unique MOOSDB listeners, 105 unique pShare
listeners, zero port overlap, intended sidecars, unknown-case and numeric-bound
rejection, multi-case GUI rejection, active-lock behavior, Bash 3.2 rejection,
Homebrew Bash re-execution, and a forced missing-result row. Bash syntax,
ShellCheck, and both skill static checkers pass. No tested process survived
cleanup.

### Completed Migration: `stationkeep_h01`

StationKeep H01 baseline evidence was gathered as the only live harness on
July 16, 2026. Its thirty-four cases passed 102/102 rows across three
untouched legacy `--jobs=4` wave matrices in 119.90, 103.66, and 105.09
seconds, for a 109.55-second mean. The untouched serial matrix passed 34/34
in 294.81 seconds. No baseline failure or flaky case was observed.

The migrated Bash 5.1 launcher uses isolated mission copies for serial and
rolling execution, rolling slot refill, deterministic selected-case
aggregation, configurable minimum-12 port spacing, root-scoped cleanup, a
harness lock, and strict one-row pMissionEval result validation. It preserves
all thirty-four explicit case mappings and every shoreside, vehicle-MOOS, and
behavior overlay. The mission wrapper is now a thin, case-agnostic launch
forwarder and result validator. No case patch, evaluator condition, event
time, behavior value, grading variable, or coverage claim changed.

Three migrated rolling matrices passed 102/102 rows in 91.86, 93.28, and
92.54 seconds. Their 92.56-second mean is 16.99 seconds, about 15.5 percent,
faster than the legacy wave mean. The isolated serial matrix passed 34/34 in
334.71 seconds, 39.90 seconds or about 13.5 percent slower than legacy,
roughly 1.17 seconds per case for isolated copying and the standard wrapper
and scoped-cleanup lifecycle.

Validation covered all-case generation, three full rolling matrices, one
full serial matrix, nominal station acquisition, hibernation, runtime-update,
warning-recovery, and expected-inactive verdicts, exact case order, rolling
refill, distinct generated ports, intended sidecars, one physical
pMissionEval row per case, unknown-case rejection, active-lock behavior, and
Bash 3.2 rejection. Disposable fault injection verified normalized
`missing_result`, `duplicate_results`, and `prepare_error` rows. Bash syntax
and both skill static checkers pass. No tested MOOS process survived cleanup.

### Completed Migration: `waypoint_h01`

Waypoint H01 baseline evidence was gathered as the only live harness on July
16, 2026. Its forty-three cases passed 129/129 rows across three untouched
legacy `--jobs=4` wave matrices in 145.90, 134.84, and 137.24 seconds, for a
139.33-second mean. The untouched serial matrix passed 43/43 in 378.15
seconds. No baseline failure or flaky case was observed.

The migrated Bash 5.1 launcher uses isolated mission copies for serial and
rolling execution, rolling slot refill, deterministic selected-case
aggregation, configurable minimum-12 port spacing, root-scoped cleanup, a
harness lock, and strict one-row pMissionEval result validation. It preserves
all forty-three explicit case mappings and every shoreside, vehicle-MOOS, and
behavior overlay. The mission wrapper is now a thin, case-agnostic launch
forwarder and result validator. No case patch, evaluator condition, event
time, behavior value, grading variable, or coverage claim changed.

Three migrated rolling matrices passed 129/129 rows in 107.78, 108.83, and
106.01 seconds. Their 107.54-second mean is 31.79 seconds, about 22.8 percent,
faster than the legacy wave mean. The isolated serial matrix passed 43/43 in
398.52 seconds, 20.37 seconds or about 5.4 percent slower than legacy, roughly
0.47 seconds per case for isolated copying and the standard lifecycle.

Validation covered all-case generation, three full rolling matrices, one full
serial matrix, nominal completion, timed non-completion, runtime updates,
repeat-cycle, custom-output, and helm-malconfiguration verdicts, standalone
stem generation and live execution, exact matrix reconciliation, rolling
refill, distinct generated ports, intended sidecars, unknown-case rejection,
active-lock behavior, and Bash 3.2 rejection. Disposable fault injection
verified normalized `missing_result`, `duplicate_results`, and `prepare_error`
rows. Bash syntax, ShellCheck, and both skill static checkers pass. No tested
MOOS process survived cleanup.

### Completed Migration: `legrun_h01`

Legrun H01 baseline evidence was gathered as the only live harness on July
16, 2026. Its thirty-three cases passed 99/99 rows across three untouched
legacy `--jobs=4` wave matrices in 151.88, 154.83, and 153.62 seconds, for a
153.44-second mean. The untouched serial matrix passed 33/33 in 525.35
seconds. No baseline failure or flaky case was observed.

The migrated Bash 5.1 launcher uses isolated mission copies for serial and
rolling execution, rolling slot refill, deterministic selected-case
aggregation, configurable minimum-12 port spacing, root-scoped cleanup, a
harness lock, and strict one-row pMissionEval result validation. It preserves
all thirty-three explicit case mappings, the 200-second harness ceiling, the
15,400 default port base, and every shoreside, vehicle-MOOS, and behavior
overlay. No case patch, evaluator condition, event time, behavior value,
grading variable, or coverage claim changed.

Three migrated rolling matrices passed 99/99 rows in 144.85, 148.77, and
140.21 seconds. Their 144.61-second mean is 8.83 seconds, about 5.8 percent,
faster than the legacy wave mean. The isolated serial matrix passed 33/33 in
553.39 seconds, 28.04 seconds or about 5.3 percent slower than legacy, roughly
0.85 seconds per case for isolated copying and the standard lifecycle.

Strict standalone validation exposed one pre-existing wrapper defect: the
stem's 70-second default could end before pMissionEval graded the baseline,
while the legacy wrapper still returned success without checking for a row.
The stem default now matches the harness's existing 200-second ceiling. No
mission event, condition, stimulus, or behavior parameter changed. A
200-second override and two subsequent default standalone runs all passed.

Validation covered all-case generation, three full rolling matrices, one full
serial matrix, normal leg/turn completion, flag aliases, runtime updates, and
expected no-progress timeout verdicts, standalone generation and execution,
exact matrix reconciliation, rolling refill, distinct generated ports,
intended sidecars, unknown-case rejection, active-lock behavior, and Bash 3.2
rejection. Disposable fault injection verified normalized `missing_result`,
`duplicate_results`, and `prepare_error` rows. Bash syntax, ShellCheck, and
both skill static checkers pass. No tested MOOS process survived cleanup.

### Completed Migration: `trail_h01`

Trail H01 baseline evidence was gathered as the only live harness on July 16,
2026. Its forty-two cases passed 126/126 rows across three untouched legacy
`--jobs=4` wave matrices in 165.79, 156.58, and 156.77 seconds, for a
159.71-second mean. The untouched serial matrix passed 42/42 in 446.54
seconds. No baseline failure or flaky case was observed.

The migrated Bash 5.1 launcher uses isolated mission copies for serial and
rolling execution, rolling slot refill, deterministic selected-case
aggregation, three MOOSDB and three pShare ports per two-vehicle case,
root-scoped cleanup, a harness lock, and strict one-row pMissionEval result
validation. All forty-two README tokens, launcher cases, and explicit case
mappings reconcile exactly. All existing patches, event times, behavior
parameters, evaluator conditions, grading variables, and coverage claims were
preserved. The mission wrapper is now a thin single-scenario forwarder rather
than routing harness arguments back into the harness.

Three clean migrated rolling matrices passed 126/126 rows in 132.18, 131.90,
and 131.63 seconds. Their 131.90-second mean is 27.81 seconds, about 17.4
percent, faster than the legacy wave mean. The isolated serial matrix passed
42/42 in 498.45 seconds, 51.91 seconds or about 11.6 percent slower than
legacy, roughly 1.24 seconds per case for isolated copying and the standard
wrapper and scoped-cleanup lifecycle.

One additional rolling matrix had one `bad_decay_fail` mission exit with
`launch_rc=1` before pMissionEval wrote a result. The case passed immediately
with its workdir retained, then passed 5/5 focused repetitions, the retained
full matrix, and the final full matrix with unchanged test inputs and grading.
No process remained. The attempt is recorded as an isolated launch/lifecycle
outlier and is excluded from clean performance statistics; the strict runner
correctly reported it as `missing_result` rather than hiding it.

Validation covered all-case generation, three clean full rolling matrices,
one full serial matrix, nominal pursuit, warning-only, and expected-inactive
verdicts, standalone stem generation and live execution, exact matrix
reconciliation, rolling refill, 126 unique MOOSDB ports, paired pShare ports,
intended sidecars, unknown-case rejection, active-lock behavior, and Bash 3.2
rejection. Disposable fault injection verified normalized `missing_result`,
`duplicate_results`, and `prepare_error` rows. Bash syntax, ShellCheck, and
both skill static checkers pass. No tested MOOS process survived cleanup.

### Completed Migration: `convoy_h01`

Convoy H01 baseline evidence was gathered as the only live harness on July 16,
2026. Three clean untouched legacy `--jobs=4` wave matrices passed 144/144
rows in 188.17, 185.39, and 186.20 seconds, for a 186.59-second mean. The
untouched serial matrix passed 48/48 in 552.52 seconds.

One additional untouched rolling matrix had `lead_s_turn_pass` fail its
existing `ABE_NAV_SPEED >= 0.35` condition with a measured 0.34. Every other
condition passed. The unchanged case then passed 5/5 focused legacy
repetitions with measured speed from 0.35 through 0.46. The failed matrix is
recorded as a pre-existing timing-sensitive outlier and excluded from the
clean timing mean; no threshold or event time was changed for migration.

The migrated Bash 5.1 launcher uses isolated mission copies for serial and
rolling execution, rolling slot refill, deterministic selected-case
aggregation, three MOOSDB and three pShare ports per two-vehicle case,
root-scoped cleanup, a harness lock, and strict one-row pMissionEval result
validation. All forty-eight README tokens, launcher cases, and explicit case
mappings reconcile exactly. Every patch, evaluator condition, behavior value,
grading variable, five custom chaser positions, and the S-turn's existing
170-second ceiling are preserved. The mission wrapper is now a thin
single-scenario forwarder rather than routing harness arguments back into the
harness.

Three migrated rolling matrices passed 144/144 rows in 150.14, 157.45, and
148.09 seconds. Their 151.89-second mean is 34.70 seconds, about 18.6 percent,
faster than the clean legacy wave mean. The isolated serial matrix passed
48/48 in 564.49 seconds, 11.97 seconds or about 2.2 percent slower than
legacy, roughly 0.25 seconds per case. The unchanged S-turn passed its focused
migrated run, all three rolling matrices, and serial.

Validation covered all-case generation, nominal, warning-expected,
helm-malconfiguration-expected, custom-geometry, and S-turn verdicts,
standalone generation and live execution, exact matrix reconciliation,
rolling refill, 144 unique MOOSDB ports, 144 non-overlapping pShare ports,
intended sidecars, unknown-case rejection, active-lock behavior, and Bash 3.2
rejection. Disposable fault injection verified normalized `missing_result`,
`duplicate_results`, and `prepare_error` rows. Bash syntax, ShellCheck, and
both skill static checkers pass. No tested MOOS process survived cleanup.

### Completed Migration: `cutrange_h01`

CutRange H01 baseline evidence was gathered as the only live harness on July
16, 2026. Its thirty-nine cases passed 117/117 rows across three untouched
legacy `--jobs=4` wave matrices in 168.68, 169.94, and 171.12 seconds, for a
169.91-second mean. The untouched serial matrix passed 39/39 in 500.78
seconds. No baseline failure or flaky case was observed.

The migrated Bash 5.1 launcher uses isolated mission copies for serial and
rolling execution, rolling slot refill, deterministic selected-case
aggregation, three MOOSDB and three pShare ports per two-vehicle case,
root-scoped cleanup, a harness lock, and strict one-row pMissionEval result
validation. All thirty-nine unique README tokens, launcher cases, and explicit
case mappings reconcile exactly. Every patch, evaluator condition, event time,
behavior value, grading variable, and four custom vehicle-start configurations
are preserved. The mission wrapper is now a thin single-scenario forwarder
rather than routing harness arguments back into the harness.

The first migrated matrix correctly reported missing results for
`right_turn_target_pass` and `s_turn_target_pass`. A retained focused run showed
the target still approaching the evaluator's turn geometry when the existing
120-second `uMayFinish` ceiling ended. The legacy harness had allowed the live
mission to continue after that return while waiting up to fifteen real seconds
for a late row, so its apparent 120-second ceiling was not enforced. Both cases
passed with a 180-second override and then passed again with the harness and
mission's existing `MAX_TIME` defaults changed from 120 to 180. No evaluator
condition, event time, position, stimulus, or behavior parameter changed;
ordinary cases still terminate on their earlier pMissionEval result.

Three clean post-fix migrated rolling matrices passed 117/117 rows in 141.66,
138.08, and 140.40 seconds. Their 140.05-second mean is 29.86 seconds, about
17.6 percent, faster than the legacy wave mean. The isolated serial matrix
passed 39/39 in 519.74 seconds, 18.96 seconds or about 3.8 percent slower than
legacy, roughly 0.49 seconds per case.

Validation covered all-case generation, nominal, two-position geometry, turn
geometry, and expected-inactive verdicts, standalone generation and live
execution, exact matrix reconciliation, rolling refill, 117 unique MOOSDB
ports, 117 non-overlapping pShare ports, intended sidecars, unknown-case
rejection, active-lock behavior, and Bash 3.2 rejection. Disposable fault
injection verified normalized `missing_result`, `duplicate_results`, and
`prepare_error` rows. Bash syntax, ShellCheck, and both skill static checkers
pass. No tested MOOS process survived cleanup.

### Completed Migration: `depth_constant_h01`

Constant-depth H01 baseline evidence was gathered as the only live harness on
July 16, 2026. Its nineteen cases passed 57/57 rows across three untouched
legacy `--jobs=4` wave matrices in 122.06, 123.24, and 123.30 seconds, for a
122.87-second mean. The untouched serial matrix passed 19/19 in 155.30
seconds. No baseline failure or flaky case was observed.

The migrated Bash 5.1 launcher uses isolated mission copies for serial and
rolling execution, rolling slot refill, deterministic selected-case
aggregation, two MOOSDB and two pShare ports per case, root-scoped cleanup, a
harness lock, and strict one-row pMissionEval result validation. The five
pre-existing solo cases retain exclusive slots through skill 1.4.3's
solo-slot scheduling pattern. All nineteen README tokens, launcher cases, and
explicit patch mappings reconcile exactly. Every patch, evaluator condition,
event time, behavior value, grading variable, and coverage claim is
unchanged.

The shared depth mission wrapper is now a thin case-agnostic forwarder and
strict result validator. The other four depth harnesses invoke `xlaunch.sh`
directly rather than this wrapper, so their current execution is unaffected.
The wrapper's existing 55-second standalone default was deliberately
preserved; H01 continues to pass its existing 80-second case ceiling
explicitly.

Three migrated rolling matrices passed 57/57 rows in 116.66, 116.16, and
116.48 seconds. Their 116.43-second mean is 6.44 seconds, about 5.2 percent,
faster than the legacy wave mean. The isolated serial matrix passed 19/19 in
181.14 seconds, 25.84 seconds or about 16.6 percent slower than legacy,
roughly 1.36 seconds per case. A controlled serial run with the teardown
helper's supported one-second INT and TERM grace overrides passed in 164.97
seconds. About 16.17 seconds of the default serial difference therefore came
from the canonical three-second graceful cleanup budgets; the remaining 9.67
seconds, about 0.51 seconds per case, came from isolated copying and the
standard mission-wrapper lifecycle. Canonical defaults remain unchanged.

Validation covered all-case generation, three full rolling matrices, one full
serial matrix, nominal depth hold, expected helm-malconfiguration, all five
solo-slot cases, standalone generation and live execution with both the
preserved 55-second default and an explicit 80-second ceiling, exact matrix
and patch-map reconciliation, rolling refill, 38 unique MOOSDB ports, 38
non-overlapping pShare ports, intended sidecars, unknown-case rejection,
active-lock behavior, and Bash 3.2 rejection. Disposable failure probes
verified normalized `missing_result`, `duplicate_results`, and `prepare_error`
rows. Bash syntax, ShellCheck, and both skill static checkers pass. No tested
MOOS process survived cleanup.

### Completed Migration: `depth_goto_h02`

GoToDepth H02 baseline evidence was gathered as the only live harness on July
16, 2026. Its eighteen cases passed 54/54 rows across three untouched legacy
`--jobs=4` wave matrices in 90.80, 92.31, and 92.18 seconds, for a 91.76-second
mean. The legacy serial matrix finished in 158.06 seconds but failed both
repeat cases with three rather than four arrivals. Each repeat failure then
reproduced once in five focused legacy attempts, establishing a pre-existing
fixed-snapshot race.

The migrated Bash 5.1 launcher uses isolated mission copies for serial and
rolling execution, rolling slot refill, deterministic selected-case
aggregation, two MOOSDB and two pShare ports per case, root-scoped cleanup, a
harness lock, and strict one-row pMissionEval result validation. The two
pre-existing crossing-sensitive cases retain exclusive slots through skill
1.4.3's solo-slot pattern. All eighteen README tokens, launcher cases, and
explicit patch mappings reconcile exactly.

Existing fixed time gates remain where elapsed time is part of the case, but
pMissionEval also waits for each case's substantive arrival count before
grading. This prevents a time snapshot from failing a still-progressing valid
sequence. The two repeat cases use a shorter but still substantive 6/10 depth
traversal. `goto_depth_repeat_pass` proves four arrivals plus its existing
horizontal checkpoint. `goto_depth_repeat_exhaustion_pass` additionally uses
the behavior's standard `endflag` as `GTD_DONE`; case-specific vehicle and
shoreside patches bridge that completion evidence without changing the shared
mission. It proves exactly four arrivals, behavior completion, and the original
horizontal checkpoint. Both cases passed 10/10 focused migrated repetitions.

Three migrated rolling matrices passed 54/54 rows in 78, 83, and 79 seconds.
Their 80.0-second mean is 11.76 seconds, about 12.8 percent, faster than the
legacy rolling mean. A clean isolated serial matrix passed 18/18 in 179
seconds. It is 20.94 seconds slower than the failed 158.06-second legacy serial
attempt, which is not an equivalent clean timing sample.

The first migrated serial matrix had one
`goto_depth_bad_update_preserve_pass` mission exit before writing a result.
The unchanged case passed 10/10 focused repetitions and a complete 179-second
serial rerun; it had also passed all three rolling matrices. The miss is
recorded as an isolated launch/lifecycle outlier rather than suppressed with a
case change.

Validation covered all-case generation, three full rolling matrices, two full
serial attempts, focused repeat and runtime-update sweeps, exact matrix and
patch-map reconciliation, solo-slot behavior, 36 unique MOOSDB ports, 36
unique pShare ports, unknown-case rejection, active-lock behavior, Homebrew
Bash re-execution, and explicit Bash 3.2 rejection. Bash syntax, ShellCheck,
the harness checker, and all eighteen generated-case evaluator checks pass.
No tested MOOS process survived cleanup.

### Completed Migration: `depth_periodic_surface_h03`

PeriodicSurface H03 baseline evidence was gathered as the only live harness on
July 16, 2026. Two precisely timed clean untouched legacy `--jobs=4` matrices
passed 48/48 rows in 77.692 and 77.808 seconds, for a 77.750-second mean. An
additional untimed console-output matrix passed 24/24, and the untouched
serial matrix passed 24/24 in 199.477 seconds.

A third precisely timed untouched rolling matrix finished in 77.358 seconds
but had the long-form status-variable alias case grade before its required
timeout flag arrived. It had already surfaced, and the unchanged case then
passed 10/10 focused legacy repetitions. This pre-existing fixed-snapshot
outlier is excluded from clean timing data.

The migrated Bash 5.1 launcher uses isolated mission copies in every mode,
rolling slot refill, deterministic selected-case aggregation, two MOOSDB and
two pShare ports per case, root-scoped cleanup, a harness lock, and strict
one-row pMissionEval result validation. All twenty-four README tokens,
launcher cases, and explicit patch mappings reconcile exactly. No shared-stem
file changed.

Positive surfacing cases retain their existing time gates but also wait for the
already-required sticky surfacing and timeout evidence. The alias case passed
10/10 focused migrated runs. The timeout-reset case now has two ordered
pMissionEval aspects: observe the timeout, then observe `TIME_AT_SURFACE <= 1`.
Its existing timer is the reset deadline, preserving a mission-owned failure
if reset never occurs. It passed 10/10 focused runs with `timeout_seen=true`
and `surface_time=0`. No behavior parameter, stimulus, threshold, or event
time changed.

The first migrated rolling matrix exposed the same one-iteration sampling
problem in the timeout-reset case: the timeout flag arrived while
`TIME_AT_SURFACE` was still two. The ordered evaluator is the correction; no
test threshold was relaxed.

Three clean migrated rolling matrices passed 72/72 rows in 64, 63, and 63
seconds. Their 63.33-second mean is 14.42 seconds, about 18.5 percent, faster
than the legacy wave mean. The isolated serial matrix passed 24/24 in 222
seconds, 22.52 seconds or about 11.3 percent slower than legacy, roughly 0.94
seconds per case.

Validation covered all-case generation, focused alias and reset sweeps, three
clean full rolling matrices, one full serial matrix, exact matrix and patch-map
reconciliation, rolling refill, 48 unique MOOSDB ports, 48 unique pShare ports,
intended sidecars, unknown-case rejection, active-lock behavior, Homebrew Bash
re-execution, and explicit Bash 3.2 rejection. Bash syntax, ShellCheck, the
harness checker, and all twenty-four generated-case evaluator checks pass. No
tested MOOS process survived cleanup.

### Completed Migration: `depth_max_h04`

MaxDepth H04 baseline evidence was gathered as the only live harness on July
16, 2026. Three untouched legacy `--jobs=4` matrices passed 45/45 rows in
49.27, 50.73, and 54.56 seconds, for a 51.52-second mean. The untouched serial
matrix passed 15/15 in 131.17 seconds. No baseline outlier occurred.

The migrated Bash 5.1 launcher uses isolated mission copies in every mode,
rolling slot refill, deterministic selected-case aggregation, two MOOSDB and
two pShare ports per case, root-scoped cleanup, a harness lock, and strict
one-row pMissionEval result validation. All fifteen README tokens, launcher
cases, and explicit patch mappings reconcile exactly. No case overlay,
stimulus, threshold, evaluator condition, or shared-stem file changed.

Three migrated rolling matrices passed 45/45 rows in 54.58, 45.62, and 49.26
seconds. Their 49.82-second mean is 1.70 seconds, about 3.3 percent, faster than
legacy and within normal run-to-run variation. The isolated serial matrix
passed 15/15 in 158.67 seconds, 27.50 seconds or about 21.0 percent slower than
legacy, roughly 1.83 seconds per case for isolated copying, xlaunch lifecycle,
and verified scoped cleanup.

Validation covered all-case generation, three full rolling matrices, one full
serial matrix, all existing nominal and expected-failure contracts, exact
matrix and patch-map reconciliation, rolling refill, 30 unique MOOSDB ports,
30 unique pShare ports, unknown-case rejection, active-lock behavior,
Homebrew Bash re-execution, and explicit Bash 3.2 rejection. Bash syntax,
ShellCheck, the harness checker, and all fifteen generated-case evaluator
checks pass. No tested MOOS process survived cleanup.

### Completed Migration: `depth_min_altitude_h05`

MinAltitude H05 baseline evidence was gathered as the only live harness on
July 16, 2026. Two clean untouched legacy `--jobs=4` matrices passed 26/26
rows in 59.42 and 52.89 seconds, for a 56.16-second mean. A third untouched
matrix finished in 51.83 seconds but had `min_altitude_low_threshold_pass`
grade before reaching its required near-bottom state. The unchanged case
passed 9/10 focused legacy repetitions. The untouched serial matrix passed
13/13 in 118.51 seconds.

The migrated Bash 5.1 launcher uses isolated mission copies in every mode,
rolling slot refill, deterministic selected-case aggregation, two MOOSDB and
two pShare ports per case, root-scoped cleanup, a harness lock, and strict
one-row pMissionEval result validation. All thirteen README tokens, launcher
cases, and explicit patch mappings reconcile exactly. No behavior parameter,
stimulus, pass threshold, or shared-stem file changed.

Three fixed-snapshot races were converted to event-or-deadline evaluation
using only existing navigation values. `zero_min_pass` waits for depth at least
16 and x at least 45, `low_threshold_pass` waits for altitude at most 4 and x
at least 45, and `clearance_boundary_pass` waits for depth at least 10 and x at
least 45. Their existing timer is now a 75-mission-second failure deadline.
Each final form passed 10/10 focused runs. An intermediate 60-second snapshot
for the low-threshold case passed only 8/10 and was rejected. A depth-only
boundary trigger was also rejected because it could fire before the existing x
condition; neither intermediate design remains.

The unconstrained-deep-bottom case intermittently reports its existing sticky
`bhv_error=true` field but does not grade that field. It did so in every legacy
baseline matrix and in some migrated runs. This pre-existing coverage choice
is recorded for later case-quality review and was not changed during migration.

After the three repairs, three rolling matrices passed 39/39 rows in 43.86,
44.24, and 42.76 seconds, for a 43.62-second mean, 12.54 seconds or about 22.3
percent faster than the clean legacy mean. An exact final-source rolling
confirmation passed 13/13 in 43.55 seconds. The exact final-source serial
matrix passed 13/13 in 132.98 seconds, 14.47 seconds or about 12.2 percent
slower than legacy, roughly 1.11 seconds per case.

Validation covered final-source generation and execution, focused repeat
sweeps, full rolling and serial matrices, exact matrix and patch-map
reconciliation, rolling refill, 26 unique MOOSDB ports, 26 unique pShare ports,
unknown-case rejection, active-lock behavior, Homebrew Bash re-execution, and
explicit Bash 3.2 rejection. Bash syntax, ShellCheck, the harness checker, and
all thirteen generated-case evaluator checks pass. No tested MOOS process
survived cleanup.

## Immediate Next Step

Forty-seven of the sixty-seven registered harnesses are now migrated against skill
1.4.3: `cmgr_h01`, `cmgr_h02`, `collision_h01`, `convoy_h01`, `cutrange_h01`, `depth_constant_h01`, `depth_goto_h02`, `depth_max_h04`, `depth_min_altitude_h05`, `depth_periodic_surface_h03`, `hostinfo_h01`, `legrun_h01`, `loadwatch_h01`, `loiter_h01`, `obmgr_h01`,
`obmgr_h02`, `obstacle_behavior_h01`, `opregion_h01`, `fixedturn_h01`, `memoryturnlimit_h01`, `pantler_h01`, `pechovar_h01`, `pid_h01`, `pid_h02`, `pnodereporter_h01`,
`periodic_speed_h01`, `processwatch_h01`, `pdeadmanpost_h01`, `plogger_h01`, `pshare_h01`, `pshare_h02`, `pspoofnode_h01`,
`psearchgrid_h01`, `testfailure_h01`, `upokedb_h01`, `uquerydb_h01`, `usim_marine_h01`, `utermcommand_h01`,
`shadow_h01`, `stationkeep_h01`, `timer_h01`, `trail_h01`, `utimerscript_h01`, `uxms_h01`, `ufld_obstacle_sim_h01`, `waypoint_h01`, and `zigzag_h01`. Each has source checks, live serial
and rolling evidence, cleanup checks, failure-path probes, and timing records.
Twenty registered harnesses remain. Continue one harness at a time with
the remaining shared-stem families, changing shared stem content only when a
contract violation is demonstrated and validating every affected consumer.
Temporary `.parallel_*`, `.harness_runs`, generated MOOS logs, result files,
targets, and Python cache trees are removed after their useful evidence is
recorded so they do not inflate source review.

Preserve the settled verdict boundary: a clean wrapper preserves the
mission-owned row unchanged, while a nonzero wrapper, missing result, malformed
result, preparation failure, or teardown failure is a runner failure with
available mission grade and return-code evidence retained as provenance. Do
not add an independent watchdog or process-tree supervisor without a separate
skill-level decision, and do not overlap live harness processes even when
nominal port ranges differ.
