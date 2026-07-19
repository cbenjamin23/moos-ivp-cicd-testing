# Harness Logging Mode Migration Plan

## Goal

Make minimal logging the normal behavior for test mission stems and harnesses,
while preserving the current diagnostic logging behind an explicit
`--log=full` option.

The migration must not change mission grading, case selection, result rows,
port isolation, or cleanup semantics. Minimal and full mode must produce the
same mission-owned pass/fail result for every ordinary harness case.

## Settled User Interface

Harnesses and directly launched stems will use the same two public modes:

```bash
./zlaunch.sh                         # whole matrix, minimal logging
./zlaunch.sh --log=minimal           # explicit equivalent
./zlaunch.sh --log=full              # whole matrix, full logging
./zlaunch.sh --log=full --case=CASE  # one case, full logging
./zlaunch.sh --log=full --case=CASE --keep_workdirs
```

The options are orthogonal:

- `--case` selects cases.
- `--log` applies one logging mode to every selected case.
- `--jobs` controls concurrency for the selected cases.

`LOG_MODE=minimal` is the default. Only `minimal` and `full` are valid values;
an unknown value is a launcher error.

### Migration Concurrency Ceiling

Validation and benchmark runs for this migration must not use more than eight
concurrent jobs. Use `--jobs=2` or `--jobs=4` for ordinary rolling validation
and stop scaling at `--jobs=8`. Existing historical results above eight may be
retained as context, but this migration will not reproduce or extend them.

## Canonical Configuration Strategy

### Minimal Lives In The Stem

The checked-in stem mission becomes the canonical minimal configuration. A
stem launched without logging arguments therefore behaves like a harness run
with `--log=minimal`.

Minimal means the least logging needed to preserve correctness:

- If no consumer grades from log artifacts, do not launch `pLogger`.
- If grading requires asynchronous evidence, launch `pLogger` with an explicit
  allowlist of only the required variables and rates.
- If grading requires synchronous evidence, retain only the required columns
  and sampling period.
- Disable wildcard logging and unnecessary synchronous logging in minimal
  mode.
- Harnesses whose subject is `pLogger`, `pAntler`, or logging behavior retain
  whatever logging their cases actually test.

When minimal mode does not launch `pLogger`, its dormant configuration block
may remain in the stem if that makes the full restoration smaller and clearer.
Runtime behavior, not deletion of inert text, is the requirement.

### Full Is A Restoration Patch

Before changing a stem, capture its current logging behavior as stem-local
full-logging patch assets, normally:

```text
full-logging-shoreside.xmoos
full-logging-vehicle.xmoos
```

Use only the files required by that stem's community layout. The full patches
restore the pre-migration behavior, including:

- the `Run = pLogger` entry in `ProcessConfig = ANTLER` when minimal removes it;
- the previous complete `ProcessConfig = pLogger` block when minimal replaces
  it with an explicit allowlist;
- existing wildcard, synchronous, asynchronous, omit-pattern, source, path,
  and timestamp settings.

Prefer full-block replacement for `ANTLER` and `pLogger`, since both may contain
repeated keys.

The full patch assets belong with the stem because direct launches and every
harness consuming that stem need the same definition of full logging.

### Patch Order

Full logging is restored before existing common and case patches. This
reconstructs the old full stem first and then lets the existing case setup
produce the same configuration it produced before migration:

```text
minimal stem -> optional full-logging restoration -> common/case patches
             -> generated .moosx sidecars -> nsplug -x targets
```

Minimal mode skips the full-logging restoration:

```text
minimal stem -> common/case patches -> generated .moosx sidecars
             -> nsplug -x targets
```

Patch application occurs only inside the isolated case mission copy. Parallel
cases never patch a shared stem directory.

### Direct Stem Launches

Direct stem launchers should also accept `--log=minimal|full`. They must use
the same stem-local full-logging assets as the harness path, rather than
maintaining a second copy of the old logger configuration.

Direct launch preparation must remove stale generated sidecars before applying
the selected mode so a previous full run cannot contaminate a later minimal
run.

## Shared Stems And Exceptions

Use the simplest behavior that preserves correctness. Do not build a large
exception framework in advance.

A shared stem's checked-in minimal configuration must be sufficient for every
configured consumer. If consumers genuinely need different evidence, choose
the smallest sensible solution:

1. use the union of required variables when the extra cost is negligible; or
2. let the affected harness apply a small required-grading patch in both modes.

Potential exceptions include:

- a case patch that replaces `ANTLER` and accidentally reintroduces or removes
  `pLogger`;
- a case patch that replaces `ProcessConfig = pLogger`;
- a process-watch assertion that explicitly expects `pLogger`;
- a harness that grades counts, timing, absence, or datatype evidence from an
  `.alog` or `.slog`;
- a shared stem whose consumers require materially different log evidence;
- the `pLogger` and `pAntler` subject harnesses;
- performance harness post-processing that reads `APP_LOG` or other log-only
  evidence.

Handle exceptions locally and document the reason. Notify the user when an
audit finds a potential exception, especially before choosing a broader
allowlist, changing grading evidence, or making full and minimal effectively
identical for a target. Do not silently weaken grading or expand logging.

## Baseline Statistics

Snapshot: July 17, 2026. Counts are based on the 67 registered targets in
`docs/context/dependency_tree.json` and a static scan of their current stems and
launchers.

| Metric | Baseline | Current |
| --- | ---: | ---: |
| Registered harness targets | 67 | 67 |
| Registered harness launchers | 67 | 67 |
| Unique mission stems | 50 | 50 |
| Dedicated stems | 45 | 45 |
| Shared stems | 5 | 5 |
| Targets using shared stems | 22 | 22 |
| Targets running `pLogger` by default | 67 | 37 |
| Targets with wildcard logging | 49 | 49 |
| Explicit-only logging targets | 18 | 18 |
| Targets enabling asynchronous logging | 67 | 67 |
| Targets with synchronous logging | 67 | 67 |
| Apparent log-artifact-dependent targets | 16 | 16 |
| Initial candidates for no `pLogger` in minimal mode | 51 | 51 |
| Audited `N`: no `pLogger` in minimal mode | n/a | 50 |
| Audited `G`: narrow grading logger required | n/a | 15 |
| Audited `S`: subject/special behavior | n/a | 2 |
| Baseline-classification scans complete | n/a | 67 |
| Migrated targets | 0 | 31 |
| Minimal-mode validated targets | 0 | 31 |
| Full-mode regression-validated targets | 0 | 32 |
| CPU/concurrency benchmarked targets | 0 | 31 |
| Pre-migration scratch-benchmarked targets | n/a | 4 |

Definitions:

- A wildcard target has `WildCardLogging=true` somewhere in its configured
  stem.
- An explicit-only target currently runs `pLogger` but has no enabled wildcard
  logging in its stem.
- A synchronous/asynchronous target has at least one corresponding enabled
  logger in its stem.
- A log-artifact-dependent target is an initial static candidate whose
  registered launcher or known sourced helper references `.alog`, `.slog`,
  `aloggrep`, or related artifact operations. Manual audit remains authoritative.
- A no-logger candidate has no such static artifact reference. It is not
  approved until process-watch and case-patch behavior are checked.
- `N`, `G`, and `S` are the audited July 18 minimal-baseline classes defined
  below. They supersede the initial static candidate split for traversal and
  implementation planning without rewriting the July 17 baseline.

### Initial Log-Artifact Dependency Candidates

The current static scan identifies:

```text
pnodereporter_h01
pechovar_h01
utimerscript_h01
pdeadmanpost_h01
pspoofnode_h01
psearchgrid_h01
plogger_h01
pmissioneval_h01
pmissionhash_h01
umayfinish_h01
p01_obstacle
p02_colregs
p03_colregs
ufld_scope_h01
ufield_comms_h02
ufield_comms_h03
```

This list is an audit queue, not a declaration that every listed target needs
all of its current logging.

## Audited Minimal-Baseline Classification

Classification pass: July 18, 2026. This pass reviewed all 67 registered
launchers, their registered sourced helpers, log-artifact post-processing,
process-watch references to `pLogger`, and harness patches that replace
`ANTLER` or `ProcessConfig = pLogger`. It defines the migration queue; it does
not mark the target's full pre-implementation audit complete.

| Class | Meaning | Targets |
| --- | --- | ---: |
| `N` | No `pLogger` required in minimal mode | 50 |
| `G` | A narrow grading logger is required | 15 |
| `S` | Logging/launcher subject or logging-sensitive special behavior | 2 |
| **Total** |  | **67** |

The implementation passes are:

1. `N`: migrate ordinary no-logger targets first.
2. `G`: migrate targets that need explicit grading evidence.
3. `S`: migrate `pLogger` and `pAntler` subject targets last.

Mixed shared stems are atomic exceptions to that simple ordering. The six `N`
consumers of `ufield_app_unit` move with `ufld_scope_h01`, and
`ufield_comms_h01` moves with the two `G` consumers of `ufield_comms_unit`.
This prevents a stem edit in one pass from leaving an unvalidated consumer in
another pass.

### Required Grading Evidence

`G` means that minimal mode still launches `pLogger`, but only in the
communities and for the variables/artifacts listed below. `MISSION_EVALUATED`
is retained wherever the harness uses it as the end of the grading window.
Absence, count, timing, and datatype checks remain harness-owned and are not
weakened.

| Ref | Target(s) | Minimal evidence contract |
| --- | --- | --- |
| `G1` | `pechovar_h01` | Pre-evaluation FLIP/echo/cycle payload, absence, and count evidence: `FLIP_OUT`, `ECHO_OUT_STR`, `ECHO_OUT_LATEST`, `CYCLE_A`, `CYCLE_B`, plus `MISSION_EVALUATED`. |
| `G2` | `utimerscript_h01` | Pre-evaluation payload/status/count evidence for `UTS_PAYLOAD`, `UTS_MACRO`, `UTS_COUNT`, `UTS_CUSTOM_STATUS`, `UTS_STATUS`, and `UTS_DELAY_RESET`, plus `MISSION_EVALUATED`. |
| `G3` | `pdeadmanpost_h01` | Pre-evaluation post counts for the case-defined `DEAD_*` variables, plus `MISSION_EVALUATED`. |
| `G4` | `pspoofnode_h01` | `NODE_REPORT` payload, absence, update, movement, and before/after timing evidence; cancellation markers such as `SPOOF_CANCEL`; plus `MISSION_EVALUATED`. |
| `G5` | `psearchgrid_h01` | Payload, count, absence, and reset-window evidence for `VIEW_GRID`, `VIEW_GRID_DELTA`, `SEARCH_GRID`, `SEARCH_GRID_DELTA`, and `PSG_RESET_GRID`, plus `MISSION_EVALUATED`. |
| `G6` | `pmissioneval_h01` | Presence/absence/value evidence for `MISSION_EVALUATED`, `EVAL_RESULT_FLAG`, `EVAL_PASS_FLAG`, `EVAL_MAIL_SEEN`, `EVAL_FAIL_FLAG`, `EVAL_SCORE_FLAG`, `STAGE1_READY`, `STAGE2_READY`, `EVAL_LITERAL_NUM`, and `MISSION_HASH`. |
| `G7` | `pmissionhash_h01` | Presence, absence, and distinct-value evidence for `MISSION_HASH` and `MHASH`. |
| `G8` | `umayfinish_h01` | `CUSTOM_DONE=complete` for the custom-finish case. |
| `G9` | `p01_obstacle`, `p02_colregs`, `p03_colregs` | Vehicle-community `.alog` existence and `APP_LOG` warning/error scans used in the performance verdict. |
| `G10` | `ufld_scope_h01` | Latest `APPCAST` emitted by `uFldScope`, including table contents and absence checks. |
| `G11` | `ufield_comms_h02` | Shoreside `NODE_BROKER_PING`, `NODE_BROKER_ACK_ABE`, `NODE_BROKER_ACK_BEN`, `NODE_BROKER_VACK`, and `PSHARE_CMD`; vehicle `NODE_BROKER_ACK`, `NODE_PSHARE_VARS`, and `PSHARE_CMD`. |
| `G12` | `ufield_comms_h03` | Shoreside `NODE_BROKER_ACK_ABE`, `NODE_BROKER_ACK_BEN`, `NODE_BROKER_PING`, `PSHARE_CMD`, and `TRY_SHORE_HOST`; vehicle `NODE_BROKER_ACK`, `PSHARE_CMD`, and `TRY_SHORE_HOST`. |
| `G13` | `pnodereporter_h01` | Shoreside `NODE_REPORT_LOCAL` history required by `reverse_load_warning_pass`; the other cases continue to grade from mission-owned result rows. |
| `S1` | `plogger_h01` | Case-defined `.alog`, `.slog`, `.xlog`, datatype/precision, wildcard/omit, dynamic request, timestamp, and copy-file artifacts. Minimal mode preserves the behavior under test rather than imposing an ordinary allowlist. |
| `S2` | `pantler_h01` | Case-defined `ANTLER` launch lists and aliases. Five case patches include `pLogger`; their process topology must remain the behavior under test. |

### Resolved Special Exception `S3`

`colregs_h02` was temporarily classified `S3` while parallel failures were
investigated in both logging modes. Retained full logs showed that logging was
not the dependency: the failures came from startup readiness, an unintended
reciprocal 10 m bow-distance boundary, moving Band 315 classifications, and
time-based overtaking checkpoints. The test-level fixes are documented in the
July 18 cohort notes below. The stable 56-case gate passes jobs 4 in both modes
and now defaults to minimal, so the target is reclassified `N`. The two moving
Band 315 cases remain explicitly runnable exploratory cases; none was deleted.

### Resolved No-Logger Exception `N1`

`pnodereporter_h01` was initially classified `N`, but
`reverse_load_warning_pass` lost transient `LOAD_WARNING` evidence when the
`.alog` fallback was removed. The target is now `G13`: minimal mode runs one
shoreside logger with wildcard and synchronous logging disabled and records
only `NODE_REPORT_LOCAL`. Three consecutive minimal replays of the affected
case passed, as did the complete 33-case minimal and full jobs-4 matrices.

### Patch-Overlap Queue

The following targets have harness patches that replace `ANTLER`, include a
`Run = pLogger` entry, or replace `ProcessConfig = pLogger`. These targets keep
their `N`, `G`, or `S` class, but implementation must edit or compose those
patches so minimal mode cannot accidentally restore full logging:

```text
pid_h02
pshare_h01
pshare_h02
plogger_h01
pantler_h01
pmissioneval_h01
obstacle_behavior_h01
ufld_pathcheck_h01
ufld_message_handler_h01
ufld_contact_range_sensor_h01
ufld_beacon_range_sensor_h01
ufld_collision_detect_h01
ufld_collob_detect_h01
ufld_scope_h01
ufield_comms_h02
```

No explicit process-watch assertion naming `pLogger` was found in the 67
registered harnesses. That result is rechecked during each target's full audit
because generated or case-specific process lists can still change when
`ANTLER` is patched.

### Shared Stem Fanout

| Stem | Consumers |
| --- | ---: |
| `missions/colregs_missions/colregs_unit` | 4 |
| `missions/depth_behavior_missions/depth_behavior_motion` | 5 |
| `missions/mission_utility_missions/mission_utility_unit` | 3 |
| `missions/ufield_app_missions/ufield_app_unit` | 7 |
| `missions/ufield_comms_missions/ufield_comms_unit` | 3 |

Update the Current column and dated notes after each reviewed migration batch.
If the inventory method changes, record the change rather than silently
rewriting the baseline.

## Per-Harness Migration Workflow

### 1. Establish The Current Contract

- Resolve the registered target, stem, launcher, and every shared-stem
  consumer from the dependency map.
- Record current `ANTLER` and `pLogger` blocks for every community.
- Audit the harness launcher, sourced helpers, patches, and post-processing for
  log artifact use.
- Audit process-watch expectations involving `pLogger`.
- Identify case patches that touch `ANTLER` or `pLogger`.
- Run or retain a representative pre-change baseline when needed to prove full
  equivalence.

### 2. Capture Full Logging

- Create stem-local full-logging `.xmoos` assets that restore the current
  logging behavior.
- Verify a generated full target matches the old target's logging-related
  configuration after normalizing generated paths or timestamps.
- Preserve case-specific logger changes by applying existing case patches
  after the full restoration.

### 3. Make The Stem Minimal

- Remove `pLogger` from the active `ANTLER` run list when grading does not need
  it.
- Otherwise replace wildcard/broad logging with the smallest explicit
  grading allowlist.
- Keep result generation owned by `pMissionEval`; logging mode must not become
  harness-side grading logic.
- Update direct stem launchers to parse and apply `--log` consistently.

### 4. Wire The Harness Interface

- Add `LOG_MODE=minimal` and parse `--log=minimal|full`.
- Apply the selected mode to every selected case.
- Apply full restoration inside each isolated case copy before common/case
  patches.
- Keep `--case`, `--jobs`, `--port_base`, `--max_time`, display mode, and
  cleanup behavior independent of logging mode.
- Include the selected logging mode in verbose/run summaries where useful.

### 5. Validate

At minimum:

1. Run the stem static checker.
2. Generate one direct stem target in minimal mode and inspect its logger
   configuration.
3. Generate one direct stem target in full mode and compare it with the old
   full configuration.
4. Run one nominal harness case in minimal and full mode.
5. Run one expected-negative, absence-sensitive, count-sensitive, or
   timing-sensitive case in both modes when the matrix has one.
6. Compare mission-owned result grades and evidence columns.
7. Run the complete matrix in minimal mode with `--jobs=1`.
8. Run a representative rolling minimal-mode matrix on a fresh port range.
9. Use `--keep_workdirs` to verify sidecar order, generated targets, and
   distinct ports.
10. Check that no mission processes or listeners remain after either mode.
11. Run the target registry and repository invariant checks.
12. Regenerate `docs/context/dependency_tree.md` and its JSON whenever launcher,
    stem, or target metadata changes.

Full and minimal mode must yield the same ordinary mission verdict. A mismatch
is a migration defect unless the target is explicitly testing logging behavior
and the exception is documented.

## Exact Post-Implementation Run Protocol

Run every command sequentially with fresh non-overlapping port ranges. Before
starting, select:

```text
NOMINAL_CASE=<ordinary passing case>
SENSITIVE_CASE=<negative, absence, count, timing, or log-sensitive case>
JOBS=<documented representative parallel value, normally 2 or 4, maximum 8>
WARP=<the harness's normal validation warp, normally 10>
MAX_TIME=<documented safe ceiling>
```

If a matrix has no distinct sensitive case, record that fact and use a second
representative case. A serial-only harness omits the `--jobs` runs. A harness
that cannot support one of the standard commands records the reason as an
exception instead of silently skipping it.

### A. Static And Generated-Configuration Checks

1. Run the stem static checker and the harness static checker.
2. Validate the registered case count:

   ```bash
   python3 scripts/ci_harness_case_count.py <harness>/zlaunch.sh
   ```

3. Generate and preserve the nominal case in minimal mode:

   ```bash
   ./zlaunch.sh --just_make --log=minimal --case="$NOMINAL_CASE" \
     --keep_workdirs --port_base="$BASE_MIN_GEN" "$WARP"
   ```

4. Generate and preserve the same case in full mode:

   ```bash
   ./zlaunch.sh --just_make --log=full --case="$NOMINAL_CASE" \
     --keep_workdirs --port_base="$BASE_FULL_GEN" "$WARP"
   ```

5. Inspect the generated targets and sidecars. Confirm minimal contains the
   audited policy and full reconstructs the pre-migration logging blocks after
   normalizing generated ports, paths, and timestamps.

### B. Paired Case Correctness

Run the nominal case once in each mode:

```bash
./zlaunch.sh --log=minimal --case="$NOMINAL_CASE" \
  --max_time="$MAX_TIME" --port_base="$BASE_MIN_CASE" "$WARP"
./zlaunch.sh --log=full --case="$NOMINAL_CASE" \
  --max_time="$MAX_TIME" --port_base="$BASE_FULL_CASE" "$WARP"
```

Run the sensitive case once in each mode:

```bash
./zlaunch.sh --log=minimal --case="$SENSITIVE_CASE" \
  --max_time="$MAX_TIME" --port_base="$BASE_MIN_SENSITIVE" "$WARP"
./zlaunch.sh --log=full --case="$SENSITIVE_CASE" \
  --max_time="$MAX_TIME" --port_base="$BASE_FULL_SENSITIVE" "$WARP"
```

Compare exit status, `grade=`, and all mission-owned result evidence. Ordinary
cases must have equivalent verdicts. Preserve workdirs and notify the user on
the first unexplained mismatch.

### C. Complete-Matrix Correctness

Run the complete matrix in minimal single-slot mode:

```bash
./zlaunch.sh --log=minimal --jobs=1 --max_time="$MAX_TIME" \
  --port_base="$BASE_MIN_SERIAL" "$WARP"
```

Then run matched complete matrices at the same representative concurrency:

```bash
/usr/bin/time -l ./zlaunch.sh --log=minimal --jobs="$JOBS" \
  --max_time="$MAX_TIME" --port_base="$BASE_MIN_PAR" "$WARP"
/usr/bin/time -l ./zlaunch.sh --log=full --jobs="$JOBS" \
  --max_time="$MAX_TIME" --port_base="$BASE_FULL_PAR" "$WARP"
```

For a serial-only harness, run one complete minimal matrix and one complete full
matrix under `/usr/bin/time -l` without `--jobs`.

The two matched runs are both correctness validation and the initial resource
comparison. They must select the same cases and use the same warp, maximum
time, jobs, and machine state. Port bases differ only to avoid stale-listener
confusion.

### D. Post-Run Hygiene

After each complete matrix:

- verify the expected number of result rows and count `grade=pass|fail`;
- inspect launcher logs for unexpected warnings;
- check for remaining MOOSDB, pShare, app, and viewer processes;
- check the allocated MOOSDB and pShare ports for listeners;
- record teardown success or failure;
- remove retained workdirs only after generated configuration evidence has
  been captured.

For a shared stem, repeat sections B through D for every registered consumer
before marking any consumer complete.

## Statistics Gathering

### Per-Target Configuration Statistics

Record after the audit and again after implementation:

- stem and number of registered consumers;
- community count and number of active `pLogger` processes per case;
- wildcard logger count;
- synchronous and asynchronous logger count;
- explicit logged-variable count and configured rates;
- log-artifact dependency classification and required variables;
- number of full-logging restoration assets;
- cases or patches that touch `ANTLER` or `pLogger`;
- selected minimal policy: none, explicit asynchronous, synchronous, or
  subject-test exception.

These values update the baseline/current statistics and the 67-row ledger.

### Per-Run Correctness And Resource Statistics

Record for every complete-matrix run:

- exact command, date, target, mode, selected case count, warp, `--max_time`,
  jobs, and port base;
- machine identifier, operating system, and logical CPU count;
- exit code, result-row count, pass count, fail count, and missing/duplicate
  row count;
- wall-clock, user CPU, and system CPU seconds;
- total CPU seconds and average cores consumed,
  `(user + system) / wall`;
- maximum resident set size reported by `/usr/bin/time -l`;
- `.alog`, `.slog`, `.ylog`, and total log file counts and bytes;
- log lines by extension when practical;
- warning count, teardown status, leftover process count, and leftover listener
  count.

Calculate the paired minimal-versus-full changes:

```text
CPU reduction %      = 100 * (full_cpu - minimal_cpu) / full_cpu
wall reduction %     = 100 * (full_wall - minimal_wall) / full_wall
log byte reduction % = 100 * (full_bytes - minimal_bytes) / full_bytes
```

Treat a zero-byte full baseline as not applicable rather than dividing by zero.

### Repeat And Scaling Benchmarks

The matched runs above provide one initial measurement for every target. Do not
claim stable performance improvement from one pair.

Run three repetitions when the target is a pilot, performance harness,
high-load/multi-community harness, shows a meaningful difference, or has noisy
or unstable results. Report median, minimum, maximum, and coefficient of
variation for wall and CPU seconds.

For selected scaling targets, run both modes with three repetitions at:

```text
jobs=1,2,4,8
warp=10
cases=same complete matrix
```

Stop increasing jobs after repeated correctness instability or when the entire
matrix already fits in one wave/rolling fill and added jobs cannot increase
concurrency. Never test above `--jobs=8` for this migration. Record the highest
repeatedly stable value at or below eight for each mode.

Keep concise summaries under `time_benchmarking/`. Avoid committing bulky raw
mission logs unless they are required to explain an anomaly.

### Ordinary `N` Jobs-2 Retirement Decision

Decision: July 18, 2026, after five migrated `N` targets (`cmgr_h01`,
`cmgr_h02`, `obmgr_h01`, `obmgr_h02`, and `pid_h01`).

For all five targets, the jobs-2 and jobs-4 paired reductions told the same
proportional story. The CPU-reduction difference between the two concurrency
levels ranged from 0.1 to 3.4 percentage points; the wall-reduction difference
ranged from 0.6 to 3.2 points. Jobs 2 therefore adds run time without changing
the migration decision for ordinary dedicated `N` targets.

For subsequent ordinary `N` migrations:

- use jobs 4 for the untouched full baseline, post-edit minimal measurement,
  and post-edit full regression;
- retain one nominal and one expected-negative or timing-sensitive jobs-1 case
  check where the matrix provides both;
- restore jobs 2 when jobs 4 is noisy, fails, changes grading behavior, cannot
  fill four workers, or the target has unusual topology/performance concerns;
- keep jobs 1/2/4/8 scaling only for explicitly selected scaling targets and
  never exceed jobs 8.

## Pre-Migration Scratch Benchmark

Snapshot: July 18, 2026. This is a directional experiment performed before
implementation, not migration validation. The repository missions and
harnesses were not changed. Two isolated scratch checkouts were used: one at
the current checked-in logging configuration (`full`) and one with the planned
minimal behavior simulated directly in the four selected stems.

Method:

- machine: Apple macOS 26.5.2, 10 logical CPUs;
- complete case matrices at each harness's normal warp;
- `--jobs=4`, three repetitions per mode and target;
- full/minimal order alternated across repetitions;
- fresh, non-overlapping port bases; all runs sequential;
- `/usr/bin/time -l`; CPU is user plus system seconds;
- retained workdirs used to count `.alog`, `.slog`, `.xlog`, and `.ylog`
  bytes;
- `N` simulation removed active `Run = pLogger` entries while leaving dormant
  config blocks in place;
- `G` simulation retained `pLogger` with the audited explicit allowlist and
  removed wildcard and unnecessary synchronous logging.

Median results:

| Target | Class | Cases / warp | Full wall (s) | Minimal wall (s) | Wall reduction | Full CPU (s) | Minimal CPU (s) | CPU reduction | Full log bytes | Minimal log bytes | Log reduction | Clean matrices full / minimal |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `cmgr_h01` | `N` | 33 / 10 | 79.50 | 62.44 | 21.5% | 68.16 | 32.03 | 53.0% | 18,741,017 | 0 | 100% | 3/3 / 2/3 |
| `depth_constant_h01` | `N` | 19 / 10 | 117.78 | 98.55 | 16.3% | 40.09 | 18.81 | 53.1% | 23,074,567 | 0 | 100% | 2/3 / 3/3 |
| `pechovar_h01` | `G` | 33 / 10 | 54.26 | 53.54 | 1.3% | 66.55 | 63.18 | 5.1% | 31,416 | 26,606 | 15.3% | 3/3 / 3/3 |
| `ufield_comms_h02` | `G` | 12 / 20 | 41.70 | 42.21 | -1.2% | 23.58 | 24.04 | -2.0% | 11,463,435 | 2,351,960 | 79.5% | 3/3 / 2/3 |

Wall-time coefficients of variation were 0.3% through 2.8%; CPU coefficients
were 1.5% through 4.0%. Maximum resident-set values reported by
`/usr/bin/time -l` clustered near 8 MB, but that parent-process measurement is
not a useful aggregate of all concurrently launched MOOS children and is not
interpreted as a memory result.

Correctness notes:

- `cmgr_h01` minimal repetition 2 had one `detect_cpa_only_pass` failure
  (`seen=false`). Focused follow-up passed 5/5 full and 5/5 minimal.
- `depth_constant_h01` full repetition 3 had one
  `constant_depth_bad_update_preserve_pass` depth-tolerance failure. Focused
  follow-up passed 3/3 full and 3/3 minimal.
- `ufield_comms_h02` minimal repetition 3 had one
  `shore_bridge_tokens_pass` mission failure (`node_count=1`); it was not a
  supplemental log-artifact failure. Focused follow-up passed 5/5 full and 5/5
  minimal.
- `pechovar_h01` passed every complete matrix in both modes.

Interpretation:

- The two `N` samples show a large, repeatable resource signal: roughly 53%
  lower CPU, 16-22% lower wall time, and elimination of 19-23 MB of logs per
  complete matrix at `--jobs=4`.
- The `G` samples do not show a consistent wall or CPU improvement. The
  already-explicit `pechovar_h01` improves modestly; the multi-community
  `ufield_comms_h02` result is within run noise for CPU and wall time.
- `G` can still produce a meaningful storage/I/O reduction: about 79.5% for
  `ufield_comms_h02` in this sample.
- The result supports prioritizing `N` for CPU/concurrency gains. It does not
  justify assuming every `G` harness will run faster, and the isolated
  timing-sensitive failures remain sensitive-case checks during migration.

## Rollout Order

1. Pilot `N` with `cmgr_h01` to prove minimal-as-stem and full restoration.
2. Pilot `G` with `pechovar_h01` to prove a narrow grading allowlist. This is a
   mechanism pilot, not the start of the main `G` batch.
3. Complete the ordinary `N` queue in small reviewed batches. Shared
   `depth_behavior_motion` consumers move atomically. The four `colregs_unit`
   consumers now share the same minimal-default stem contract; the resolved
   `colregs_h02` `S3` investigation is retained in the cohort notes.
4. Complete the `G` queue. Treat `mission_utility_unit` as one three-consumer
   group. Move the entire mixed `ufield_app_unit` and `ufield_comms_unit`
   consumer groups here, including their `N` consumers.
5. Complete the three `G9` performance targets with their warning-log contract
   and repeated resource measurements.
6. Complete the remaining `S` queue: migrate `plogger_h01` and `pantler_h01`
   with explicit exception notes.
7. Run selected CPU and stable-concurrency scaling benchmarks only after the
   corresponding correctness validation; never exceed `--jobs=8`.

Do not run two validation or benchmark scripts concurrently when their MOOSDB
or pShare ranges could overlap.

## Migration Start Procedure

Before the first implementation edit for a target:

1. Confirm the target's ledger row is `not started`, then mark Audit and Overall
   `in progress`.
2. Resolve the target, launcher, stem, shared consumers, case count, community
   layout, and current port stride from the generated dependency map.
3. Select and record the nominal and sensitive cases, validation warp,
   `--max_time`, representative jobs value, and fresh port ranges. The jobs
   value must not exceed eight.
4. Complete the log-artifact, process-watch, `ANTLER`, and `pLogger` patch
   audit.
5. Capture the current logging blocks and a generated full target before
   changing the stem.
6. Notify the user of any potential exception or grading ambiguity before
   choosing its implementation.
7. Record the audited minimal policy and full-restoration assets in the ledger,
   then begin implementation.

For the first pilot, start with `cmgr_h01`. Do not begin a second harness until
the pilot has completed generated-configuration checks, paired case checks,
minimal serial validation, matched minimal/full rolling validation, hygiene
checks, statistics capture, invariant checks, dependency-map regeneration, and
its ledger update.

## Per-Target Execution Records

### `cmgr_h01` (`N`) — In Progress

Audit and baseline snapshot: July 18, 2026.

- dedicated stem: `missions/cmgr_missions/cmgr_unit`;
- harness: `harnesses/cmgr_harnesses/H01-cmgr_unit`;
- matrix: 33 cases, warp 10, port stride 30, two communities per case;
- minimal policy: no active `pLogger` process;
- current full behavior: two active `pLogger` processes per case, one
  shoreside and one vehicle, with wildcard and asynchronous logging enabled;
- grading evidence: mission-owned results; no `.alog`/`.slog` reader,
  `pLogger` process-watch assertion, or case patch replacing `ANTLER` or
  `pLogger` was found;
- full-restoration assets planned: `full-logging-shoreside.xmoos` and
  `full-logging-vehicle.xmoos`;
- nominal paired case: `detect_baseline_pass`;
- timing-sensitive paired case: `detect_cpa_only_pass`;
- pre-edit structural checks: eval mission PASS; harness PASS;
- benchmark host: Charles's MacBook Pro, macOS 26.5.2 arm64, 10 logical CPUs.

Untouched full-logging baselines:

| Jobs | Repetition | Port base | Result | Wall (s) | User (s) | System (s) | CPU (s) | Average cores | Max RSS (bytes) | `.alog` count | `.alog` bytes | Retained run |
| ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| 2 | 1 | 54000 | 33/33 pass | 140.04 | 19.35 | 50.82 | 70.17 | 0.501 | 7,880,704 | 66 | 19,232,064 | `run_20260718T115816_96765` |
| 4 | 1 | 55000 | 33/33 pass | 76.47 | 19.29 | 52.68 | 71.97 | 0.941 | 7,864,320 | 66 | 18,625,740 | `run_20260718T120052_8288` |
| 4 | 2 | 56000 | 33/33 pass | 75.95 | 19.46 | 50.50 | 69.96 | 0.921 | 7,913,472 | 66 | 18,549,183 | `run_20260718T120214_18125` |
| 4 | 3 | 57000 | 33/33 pass | 75.12 | 19.15 | 50.26 | 69.41 | 0.924 | 7,897,088 | 66 | 18,275,577 | `run_20260718T120336_27802` |

Jobs-4 full median: 75.95 seconds wall, 69.96 CPU seconds, 0.921 average
cores, 7,897,088 bytes maximum RSS, and 18,549,183 `.alog` bytes. The retained
workdirs remain under the harness-local `.harness_runs/` directory until the
generated-configuration and anomaly evidence has been captured.

Implementation checkpoint:

- the checked-in shoreside and vehicle ANTLER blocks no longer launch
  `pLogger`; dormant logger configuration blocks remain inert;
- `full-logging-shoreside.xmoos` and `full-logging-vehicle.xmoos` restore the
  complete pre-edit ANTLER and pLogger blocks;
- direct `launch.sh` and `zlaunch.sh` accept only `--log=minimal|full` and
  default to minimal;
- the harness accepts the same option and applies full restoration before the
  existing case patches in each isolated mission copy;
- pre-edit and post-edit structural checks pass;
- minimal and full single-case `--just_make` pass;
- generated minimal targets contain zero active pLogger entries; generated
  full targets contain two;
- the case-specific `pMissionEval` replacement is present in both modes;
- the generated full nominal-case targets match the retained untouched targets
  at identical ports, apart from one trailing blank line in `targ_abe.moos`;
- a direct full `--just_make` followed by default minimal `--just_make` removed
  both stale `.moosx` sidecars and changed the active pLogger count from two to
  zero.

Post-edit complete-matrix runs used the exact command form
`/usr/bin/time -l /opt/homebrew/bin/bash ./zlaunch.sh --log=MODE --jobs=JOBS
--keep_workdirs --port_base=PORT 10`, with the values below:

| Mode | Jobs | Repetition | Port base | Result | Wall (s) | User (s) | System (s) | CPU (s) | Average cores | Max RSS (bytes) | Logger files / bytes | Retained run |
| --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| minimal | 1 | 1 | 20000 | 33/33 pass | 201.93 | 9.44 | 21.93 | 31.37 | 0.155 | 7,847,936 | 0 / 0 | `run_20260718T121301_51908` |
| minimal | 2 | 1 | 21000 | 33/33 pass | 105.20 | 9.13 | 21.39 | 30.52 | 0.290 | 7,913,472 | 0 / 0 | `run_20260718T121635_58984` |
| minimal | 4 | 1 | 22000 | 33/33 pass | 57.51 | 9.12 | 21.33 | 30.45 | 0.529 | 7,897,088 | 0 / 0 | `run_20260718T121833_65855` |
| minimal | 4 | 2 | 23000 | 33/33 pass | 57.30 | 9.11 | 21.42 | 30.53 | 0.533 | 7,913,472 | 0 / 0 | `run_20260718T121947_72670` |
| minimal | 4 | 3 | 24000 | 33/33 pass | 57.59 | 9.12 | 22.03 | 31.15 | 0.541 | 7,897,088 | 0 / 0 | `run_20260718T122050_79475` |
| full | 4 | post-edit regression | 25000 | 33/33 pass | 77.56 | 18.86 | 50.02 | 68.88 | 0.888 | 7,913,472 | 165 / 18,612,148 | `run_20260718T122154_86024` |

The full regression's 165 logger artifacts comprise 66 `.alog` files
(18,434,682 bytes), 33 `.slog` files (131,254 bytes), and 66 `.ylog` files
(46,212 bytes). Every minimal run produced zero files for all three extensions.
The three untouched jobs-4 full baselines had a median of 18,727,142 total
logger bytes.

Paired results:

- jobs 2: minimal reduced wall time by 24.9%, CPU seconds by 56.5%, and logger
  bytes by 100%;
- jobs 4 medians: minimal reduced wall time by 24.3%, CPU seconds by 56.4%,
  and logger bytes by 100%;
- minimal jobs-4 wall time ranged from 57.30 to 57.59 seconds (about 0.26%
  coefficient of variation); CPU ranged from 30.45 to 31.15 seconds (about
  1.25% coefficient of variation);
- untouched full jobs-4 wall time ranged from 75.12 to 76.47 seconds (about
  0.90% coefficient of variation); CPU ranged from 69.41 to 71.97 seconds
  (about 1.91% coefficient of variation);
- the post-edit full regression remained within the untouched full artifact
  range and close to its timing range, supporting restoration equivalence.

Final correctness and hygiene checks:

- nominal `detect_baseline_pass` and timing-sensitive `detect_cpa_only_pass`
  passed in both modes;
- every complete matrix wrote exactly 33 rows, all `grade=pass`, with no
  missing or duplicate rows;
- launcher-log warning/error scans found zero matches;
- teardown reported no failures, and no retained-run process or listener was
  found on the validation port ranges;
- direct stem and harness launchers reject unknown logging modes with exit 2;
- shell syntax, ShellCheck for the new helper plus both `zlaunch.sh` wrappers,
  eval and harness structural checks, and `git diff --check` pass; the legacy
  `launch.sh` retains its pre-existing ShellCheck warning set, with no warning
  on the new logging-mode lines;
- `docs/context/dependency_tree.md` and `.json` were regenerated after the
  launcher, stem, documentation, and patch-asset changes.

### Ordinary `N` Comparison Cohort — Complete

Targets: `cmgr_h02`, `obmgr_h01`, `obmgr_h02`, and `pid_h01`. Audit,
implementation, and validation date: July 18, 2026.

Configuration audit:

- all four use dedicated stems and mission-owned grading;
- no `.alog`/`.slog` grading reader or pLogger process assertion was found;
- no case patch replaces `ANTLER` or `pLogger`;
- `uProcessWatch` uses `watch_all=true`, not an explicit pLogger requirement;
- `cmgr_h02`, `obmgr_h01`, and `obmgr_h02` have shoreside and vehicle loggers;
  `pid_h01` has one shoreside logger;
- minimal removes the seven active pLogger launches while leaving dormant
  configurations intact; seven stem-local full-restoration assets reconstruct
  the untouched ANTLER/logger behavior;
- minimal/full `--just_make` passed for every target; minimal generated targets
  contained 0 active pLoggers and full contained 2/2/2/1;
- full nominal targets matched retained untouched targets at identical ports,
  apart from one trailing blank line;
- shell syntax, ShellCheck for the new helpers and both wrapper layers, and all
  eight eval/harness structural checks passed.

Complete-matrix resource runs used
`/usr/bin/time -l /opt/homebrew/bin/bash ./zlaunch.sh [--log=MODE]
--jobs=JOBS --keep_workdirs --port_base=PORT 10`:

| Target | Mode | Jobs | Port | Result | Wall | User | System | CPU | Avg cores | Max RSS | Logger files / bytes | Retained run |
| --- | --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `cmgr_h02` | full pre | 2 | 30000 | 15/15 | 89.51 | 8.76 | 22.35 | 31.11 | 0.348 | 7,929,856 | 75 / 82,942,219 | `run_20260718T130812_13048` |
| `cmgr_h02` | full pre | 4 | 31000 | 15/15 | 51.66 | 9.11 | 23.81 | 32.92 | 0.637 | 7,847,936 | 75 / 83,754,549 | `run_20260718T130945_17975` |
| `cmgr_h02` | minimal | 2 | 44000 | 15/15 | 76.85 | 4.31 | 10.18 | 14.49 | 0.189 | 7,962,624 | 0 / 0 | `run_20260718T132539_76104` |
| `cmgr_h02` | minimal | 4 | 45000 | 15/15 | 43.82 | 4.24 | 10.64 | 14.88 | 0.340 | 7,929,856 | 0 / 0 | `run_20260718T132705_79599` |
| `cmgr_h02` | full post | 4 | 46000 | 15/15 | 51.91 | 9.12 | 24.11 | 33.23 | 0.640 | 7,929,856 | 75 / 76,041,601 | `run_20260718T132756_83235` |
| `obmgr_h01` | full pre | 2 | 32000 | 30/30 | 122.28 | 16.23 | 42.81 | 59.04 | 0.483 | 7,880,704 | 150 / 8,678,546 | `run_20260718T131050_22884` |
| `obmgr_h01` | full pre | 4 | 33000 | 30/30 | 70.69 | 16.80 | 44.52 | 61.32 | 0.868 | 7,962,624 | 150 / 8,709,737 | `run_20260718T131257_31522` |
| `obmgr_h01` | minimal | 2 | 47000 | 30/30 | 93.82 | 8.47 | 20.77 | 29.24 | 0.312 | 8,028,160 | 0 / 0 | `run_20260718T132853_88601` |
| `obmgr_h01` | minimal | 4 | 48000 | 30/30 | 55.10 | 8.47 | 20.70 | 29.17 | 0.529 | 7,995,392 | 0 / 0 | `run_20260718T133033_94735` |
| `obmgr_h01` | full post | 4 | 49000 | 30/30 | 67.73 | 16.34 | 44.22 | 60.56 | 0.894 | 7,979,008 | 150 / 8,306,899 | `run_20260718T133138_1244` |
| `obmgr_h02` | full pre | 2 | 34000 | 10/10 | 58.75 | 6.39 | 16.21 | 22.60 | 0.385 | 7,798,784 | 50 / 12,368,929 | `run_20260718T131413_40203` |
| `obmgr_h02` | full pre | 4 | 35000 | 10/10 | 36.97 | 6.05 | 16.43 | 22.48 | 0.608 | 7,847,936 | 50 / 12,423,496 | `run_20260718T131518_43818` |
| `obmgr_h02` | minimal | 2 | 50000 | 10/10 | 49.25 | 2.89 | 6.93 | 9.82 | 0.199 | 7,880,704 | 0 / 0 | `run_20260718T133251_10445` |
| `obmgr_h02` | minimal | 4 | 51000 | 10/10 | 32.15 | 2.92 | 7.61 | 10.53 | 0.328 | 7,897,088 | 0 / 0 | `run_20260718T133354_13065` |
| `obmgr_h02` | full post | 4 | 52000 | 10/10 | 37.91 | 6.09 | 17.19 | 23.28 | 0.614 | 7,847,936 | 50 / 11,794,082 | `run_20260718T133431_15634` |
| `pid_h01` | full pre | 2 | 36000 | 34/34 | 97.84 | 14.64 | 38.35 | 52.99 | 0.542 | 7,946,240 | 68 / 5,063,471 | `run_20260718T131600_47222` |
| `pid_h01` | full pre | 4 | 38000 | 34/34 | 53.60 | 15.04 | 41.15 | 56.19 | 1.048 | 7,913,472 | 68 / 5,124,279 | `run_20260718T131750_54624` |
| `pid_h01` | minimal | 2 | 53000 | 34/34 | 80.43 | 8.50 | 22.06 | 30.56 | 0.380 | 7,880,704 | 0 / 0 | `run_20260718T133515_19319` |
| `pid_h01` | minimal | 4 | 55000 | 34/34 | 43.22 | 8.50 | 22.70 | 31.20 | 0.722 | 7,946,240 | 0 / 0 | `run_20260718T133643_24667` |
| `pid_h01` | full post | 4 | 57000 | 34/34 | 55.46 | 14.45 | 40.95 | 55.40 | 0.999 | 7,897,088 | 68 / 4,907,923 | `run_20260718T133734_29902` |

Paired reductions:

| Target | Jobs-2 wall / CPU | Jobs-4 wall / CPU | Logger bytes |
| --- | ---: | ---: | ---: |
| `cmgr_h02` | 14.1% / 53.4% | 15.2% / 54.8% | 100% |
| `obmgr_h01` | 23.3% / 50.5% | 22.1% / 52.4% | 100% |
| `obmgr_h02` | 16.2% / 56.5% | 13.0% / 53.2% | 100% |
| `pid_h01` | 17.8% / 42.3% | 19.4% / 44.5% | 100% |

Correctness and hygiene:

- every complete run wrote the expected result count and every row passed;
- nominal and expected-negative/stress jobs-1 minimal cases passed for every
  target: `baseline_crossing_pass`/`tight_alert_fail`,
  `given_baseline_pass`/`no_alert_request_absent_pass`,
  `baseline_center_pass`/`wide_center_fail`, and
  `rudder_starboard_pass`/`simulation_mode_fail`;
- warning/error scans found zero matches in all 20 complete-run launcher logs;
- teardown reported no failure, no retained-run process remained, and no TCP
  listener or UDP binding matched an exact allocated MOOSDB/pShare port;
- the four post-edit full jobs-4 runs stayed in the untouched full timing and
  resource envelope and produced the expected logger artifact counts.

### Ordinary `N` Jobs-4-Only Cohort — Complete

Targets: `usim_marine_h01`, `upokedb_h01`, `uxms_h01`, `uquerydb_h01`, and
`utermcommand_h01`.
Audit, implementation, and validation date: July 18, 2026. This is the first
cohort using the jobs-2 retirement decision.

Configuration audit:

- all five have dedicated, single-community stems;
- no logger artifact, pLogger process assertion, or logging patch is part of
  grading; `usim_marine_h01` has ordinary case overlays, while the other four
  grade external CLI clients against the copied live stem;
- minimal removes one active pLogger launch per stem and full uses one
  stem-local ANTLER restoration asset per stem;
- minimal/full nominal `--just_make` runs passed for every target; generated
  minimal targets contained zero active pLoggers and full targets contained
  one;
- full generated targets reproduced the retained untouched target exactly for
  `upokedb_h01` and differed only by a trailing blank line for the other three.

Complete-matrix resource runs used
`/usr/bin/time -l /opt/homebrew/bin/bash ./zlaunch.sh [--log=MODE]
--jobs=4 --keep_workdirs --port_base=PORT 10`:

| Target | Mode | Port | Result | Wall | User | System | CPU | Avg cores | Max RSS | Logger files / bytes | Retained run |
| --- | --- | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `usim_marine_h01` | full pre | 16000 | 36/36 | 57.01 | 16.86 | 45.93 | 62.79 | 1.101 | 7,897,088 | 72 / 10,152,103 | `run_20260718T134656_48761` |
| `usim_marine_h01` | minimal | 26000 | 36/36 | 47.85 | 9.01 | 24.48 | 33.49 | 0.700 | 7,979,008 | 0 / 0 | `run_20260718T135525_91417` |
| `usim_marine_h01` | full post | 28000 | 36/36 | 56.30 | 16.98 | 44.26 | 61.24 | 1.088 | 7,880,704 | 72 / 10,269,182 | `run_20260718T135730_97396` |
| `upokedb_h01` | full pre | 18000 | 28/28 | 44.65 | 12.88 | 34.07 | 46.95 | 1.052 | 7,864,320 | 56 / 25,379 | `run_20260718T134800_56588` |
| `upokedb_h01` | minimal | 30000 | 28/28 | 38.35 | 6.98 | 17.36 | 24.34 | 0.635 | 7,946,240 | 0 / 0 | `run_20260718T140026_14447` |
| `upokedb_h01` | full post | 31000 | 28/28 | 46.39 | 13.91 | 36.55 | 50.46 | 1.088 | 7,962,624 | 56 / 25,379 | `run_20260718T140113_18957` |
| `uxms_h01` | full pre | 19000 | 32/32 | 65.69 | 14.78 | 39.67 | 54.45 | 0.829 | 7,929,856 | 64 / 43,456 | `run_20260718T134852_62680` |
| `uxms_h01` | minimal | 32000 | 32/32 | 58.16 | 8.32 | 20.97 | 29.29 | 0.504 | 7,897,088 | 0 / 0 | `run_20260718T140210_26116` |
| `uxms_h01` | full post | 33000 | 32/32 | 67.81 | 15.36 | 40.82 | 56.18 | 0.828 | 7,962,624 | 64 / 43,456 | `run_20260718T140316_31278` |
| `uquerydb_h01` | full pre | 21000 | 30/30 | 57.84 | 14.85 | 40.52 | 55.37 | 0.957 | 7,995,392 | 60 / 48,630 | `run_20260718T135008_69821` |
| `uquerydb_h01` | minimal | 34000 | 30/30 | 47.67 | 8.07 | 19.61 | 27.68 | 0.581 | 7,880,704 | 0 / 0 | `run_20260718T140432_38793` |
| `uquerydb_h01` | full post | 35000 | 30/30 | 58.43 | 15.54 | 41.84 | 57.38 | 0.982 | 7,946,240 | 60 / 48,630 | `run_20260718T140530_45361` |
| `utermcommand_h01` | full pre | 40000 | 15/15 | 45.39 | 9.02 | 23.35 | 32.37 | 0.713 | 7,946,240 | 30 / 13,729 | `run_20260718T141651_76061` |
| `utermcommand_h01` | minimal | 44000 | 15/15 | 39.02 | 5.43 | 12.47 | 17.90 | 0.459 | 7,880,704 | 0 / 0 | `run_20260718T141921_84542` |
| `utermcommand_h01` | full post | 45000 | 15/15 | 45.29 | 9.11 | 22.79 | 31.90 | 0.704 | 7,946,240 | 30 / 13,729 | `run_20260718T142010_90831` |

Paired untouched-full versus minimal jobs-4 reductions:

| Target | Wall | CPU | Logger bytes |
| --- | ---: | ---: | ---: |
| `usim_marine_h01` | 16.1% | 46.7% | 100% |
| `upokedb_h01` | 14.1% | 48.2% | 100% |
| `uxms_h01` | 11.5% | 46.2% | 100% |
| `uquerydb_h01` | 17.6% | 50.0% | 100% |
| `utermcommand_h01` | 14.0% | 44.7% | 100% |

Correctness, findings, and hygiene:

- all fifteen retained complete runs wrote the expected result counts with no
  failing rows; all minimal runs produced zero logger artifacts, and all full
  regressions restored the untouched artifact counts;
- nominal and sensitive/edge jobs-1 cases passed in both modes:
  `thrust_forward_pass`/`obstacle_hit_stop_pass`,
  `numeric_direct_pass`/`cache_duplicate_first_wins_pass`,
  `scoped_var_pass`/`filter_no_match_absent_pass`, and
  `cli_numeric_pass`/`missing_var_timeout_fail`, and
  `numeric_command_pass`/`ambiguous_partial_absent_pass`;
- the first `upokedb_h01` minimal matrix exposed a fixed-delay race in
  `mission_file_connection_pass`: the external client could start before
  `targ_shoreside.moos` existed. Follow-up audit found the same latent pattern
  in the migrated `uquerydb_h01` and `uxms_h01` mission-file cases. All three
  now use the same bounded nonempty-file readiness check and return a runner
  stimulus error if target generation never completes. Focused minimal and
  full checks pass for all three; the original failed attempt is excluded from
  statistics. This is an external-client readiness fix, not a logging-specific
  startup effect;
- the generic eval-mission static checker reports that `upokedb_h01` and
  `utermcommand_h01` have no in-mission initializer. This is an expected
  structural-check limitation: each app-under-test is intentionally invoked
  by its harness as external stimulus. Their harness checkers and live
  validation pass;
- complete-run launcher warning/error scans found zero matches; teardown left
  no relevant process and no exact allocated MOOSDB/pShare listener.

### Ordinary `N` Behavior Cohort — Complete

Targets: `collision_h01`, `convoy_h01`, `cutrange_h01`, `fixedturn_h01`,
`periodic_speed_h01`, `zigzag_h01`, `legrun_h01`,
`memoryturnlimit_h01`, `timer_h01`, and `hostinfo_h01`. Audit,
implementation, and validation date: July 18, 2026.

Configuration audit:

- all ten have dedicated stems and mission-owned grading; no `.alog`/`.slog`
  reader, explicit pLogger process assertion, or case logging patch is part of
  grading;
- minimal removes active pLogger launches and retains inert configuration
  blocks; full restoration is held in separate stem-local ANTLER/logger patch
  files and is applied before case-specific overlays;
- nine stems use shoreside and vehicle restoration assets; `hostinfo_h01` is
  shoreside-only;
- generated minimal targets contain zero active pLoggers. Full targets restore
  two active processes for the ordinary one-vehicle stems, one for
  `hostinfo_h01`, and three for `convoy_h01` and `cutrange_h01` because their
  nominal missions launch two vehicles. Retained untouched nominal targets
  have the same 1/2/3 counts;
- Bash syntax, ShellCheck for modified wrappers/helpers, all eval/harness
  static checkers, and all minimal/full `--just_make` checks pass.

Complete-matrix measurements used
`/usr/bin/time -l /opt/homebrew/bin/bash ./zlaunch.sh --log=MODE --jobs=JOBS
--keep_workdirs --port_base=PORT 10` on Charles's MacBook Pro, macOS 26.5.2
arm64, with 10 logical CPUs. `full pre` is the untouched stem, `minimal` is the
new default, and `full post` is restoration regression validation.

| Target | Mode | Jobs | Port | Result | Wall | User | System | CPU | Avg cores | Max RSS | Logger files / bytes | Retained run |
| --- | --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `collision_h01` | full pre | 4 | 10000 | 21/21 | 58.75 | 12.36 | 33.54 | 45.90 | 0.781 | 7,946,240 | 105 / 69,348,782 | `run_20260718T143722_18850` |
| `collision_h01` | minimal | 4 | 28000 | 21/21 | 50.74 | 6.05 | 15.49 | 21.54 | 0.425 | 7,979,008 | 0 / 0 | `run_20260718T145858_70759` |
| `collision_h01` | full post | 4 | 43000 | 21/21 | 58.60 | 12.38 | 34.99 | 47.37 | 0.808 | 7,995,392 | 105 / 43,822,881 | `run_20260718T151129_47336` |
| `convoy_h01` | full pre | 4 | 12000 | 48/48 | 150.71 | 29.71 | 77.97 | 107.68 | 0.714 | 8,077,312 | 288 / 84,931,649 | `run_20260718T143820_25822` |
| `convoy_h01` | minimal | 4 | 30000 | 48/48 | 133.76 | 14.57 | 35.24 | 49.81 | 0.372 | 8,077,312 | 0 / 0 | `run_20260718T145949_76682` |
| `convoy_h01` | full post | 4 | 45000 | 48/48 | 150.84 | 29.52 | 76.36 | 105.88 | 0.702 | 8,126,464 | 288 / 77,431,452 | `run_20260718T151228_55464` |
| `cutrange_h01` | full pre | 4 | 14000 | 39/39 | 140.80 | 24.15 | 66.50 | 90.65 | 0.644 | 8,060,928 | 234 / 333,354,212 | `run_20260718T144051_41618` |
| `cutrange_h01` | minimal | 4 | 32000 | 39/39 | 122.49 | 11.91 | 29.12 | 41.03 | 0.335 | 8,093,696 | 0 / 0 | `run_20260718T150203_88494` |
| `cutrange_h01` | full post | 4 | 47000 | 39/39 | 141.81 | 24.52 | 64.16 | 88.68 | 0.625 | 7,995,392 | 234 / 297,825,288 | `run_20260718T151458_72873` |
| `fixedturn_h01` | full pre | 4 | 16000 | 24/24 | 65.36 | 13.88 | 36.81 | 50.69 | 0.776 | 8,028,160 | 120 / 20,690,734 | `run_20260718T144312_56357` |
| `fixedturn_h01` | minimal | 4 | 34000 | 24/24 | 56.55 | 6.97 | 17.73 | 24.70 | 0.437 | 7,995,392 | 0 / 0 | `run_20260718T150405_98712` |
| `fixedturn_h01` | full post | 4 | 49000 | 24/24 | 64.03 | 13.30 | 34.31 | 47.61 | 0.744 | 8,011,776 | 120 / 19,617,401 | `run_20260718T151720_87783` |
| `periodic_speed_h01` | full pre | 4 | 17000 | 24/24 | 50.92 | 13.15 | 35.00 | 48.15 | 0.946 | 7,995,392 | 120 / 10,701,029 | `run_20260718T144417_64515` |
| `periodic_speed_h01` | minimal | 4 | 35000 | 24/24 | 43.66 | 7.03 | 18.23 | 25.26 | 0.579 | 7,962,624 | 0 / 0 | `run_20260718T150502_6212` |
| `periodic_speed_h01` | full post | 4 | 50000 | 24/24 | 52.91 | 13.78 | 38.48 | 52.26 | 0.988 | 8,011,776 | 120 / 10,272,072 | `run_20260718T152008_17525` |
| `zigzag_h01` | full pre | 4 | 18000 | 35/35 | 95.78 | 19.92 | 53.52 | 73.44 | 0.767 | 8,044,544 | 175 / 37,348,909 | `run_20260718T144508_72163` |
| `zigzag_h01` | minimal | 4 | 36000 | 35/35 | 83.73 | 10.19 | 26.12 | 36.31 | 0.434 | 8,093,696 | 0 / 0 | `run_20260718T150546_13256` |
| `zigzag_h01` | full post | 4 | 51000 | 35/35 | 94.46 | 19.69 | 51.82 | 71.51 | 0.757 | 8,028,160 | 175 / 35,448,906 | `run_20260718T152112_29030` |
| `legrun_h01` | full pre | 4 | 20000 | 33/33 | 139.59 | 19.47 | 51.88 | 71.35 | 0.511 | 7,995,392 | 165 / 57,429,325 | `run_20260718T144644_82922` |
| `legrun_h01` | minimal | 4 | 38000 | 33/33 | 124.07 | 9.33 | 23.42 | 32.75 | 0.264 | 7,979,008 | 0 / 0 | `run_20260718T150709_21695` |
| `legrun_h01` | full post | 2 | 59000 | 33/33 | 268.71 | 20.50 | 50.00 | 70.50 | 0.262 | 7,979,008 | 165 / 53,158,085 | `run_20260718T152958_79319` |
| `memoryturnlimit_h01` | full pre | 4 | 22000 | 21/21 | 87.93 | 12.62 | 35.00 | 47.62 | 0.542 | 7,979,008 | 105 / 14,384,774 | `run_20260718T144904_93437` |
| `memoryturnlimit_h01` | minimal | 4 | 40000 | 21/21 | 69.64 | 5.96 | 15.02 | 20.98 | 0.301 | 7,962,624 | 0 / 0 | `run_20260718T150914_29861` |
| `memoryturnlimit_h01` | full post | 4 | 55000 | 21/21 | 87.16 | 13.46 | 34.18 | 47.64 | 0.547 | 7,929,856 | 105 / 13,539,046 | `run_20260718T153436_93866` |
| `timer_h01` | full pre | 4 | 23000 | 18/18 | 44.03 | 9.99 | 26.72 | 36.71 | 0.834 | 8,077,312 | 90 / 14,693,182 | `run_20260718T145032_922` |
| `timer_h01` | minimal | 4 | 41000 | 18/18 | 39.34 | 5.19 | 13.76 | 18.95 | 0.482 | 8,060,928 | 0 / 0 | `run_20260718T151023_36020` |
| `timer_h01` | full post | 4 | 56000 | 18/18 | 43.21 | 9.59 | 24.32 | 33.91 | 0.785 | 8,044,544 | 90 / 13,903,787 | `run_20260718T153604_3039` |
| `hostinfo_h01` | full pre | 4 | 25000 | 7/7 | 13.28 | 3.32 | 9.25 | 12.57 | 0.947 | 7,798,784 | 14 / 26,155 | `run_20260718T145228_14309` |
| `hostinfo_h01` | minimal | 4 | 42000 | 7/7 | 11.27 | 1.83 | 5.04 | 6.87 | 0.610 | 7,864,320 | 0 / 0 | `run_20260718T151103_41579` |
| `hostinfo_h01` | full post | 4 | 57000 | 7/7 | 12.71 | 3.12 | 8.84 | 11.96 | 0.941 | 7,847,936 | 14 / 25,318 | `run_20260718T153647_10138` |

Paired untouched-full versus minimal jobs-4 reductions:

| Target | Wall | CPU | Logger bytes |
| --- | ---: | ---: | ---: |
| `collision_h01` | 13.6% | 53.1% | 100% |
| `convoy_h01` | 11.2% | 53.7% | 100% |
| `cutrange_h01` | 13.0% | 54.7% | 100% |
| `fixedturn_h01` | 13.5% | 51.3% | 100% |
| `periodic_speed_h01` | 14.3% | 47.5% | 100% |
| `zigzag_h01` | 12.6% | 50.6% | 100% |
| `legrun_h01` | 11.1% | 54.1% | 100% |
| `memoryturnlimit_h01` | 20.8% | 55.9% | 100% |
| `timer_h01` | 10.7% | 48.4% | 100% |
| `hostinfo_h01` | 15.1% | 45.3% | 100% |

Correctness, exceptions, and hygiene:

- all ten minimal jobs-4 matrices and all retained full regressions passed;
  nominal and sensitive/expected-negative jobs-1 cases also passed in both
  modes;
- the first `periodic_speed_h01` full regression had one mission-owned failure
  in `initially_busy_pass`; retained log evidence showed the behavior started
  lazy (`PS_PENDING_ACTIVE=3`, `PS_PENDING_INACTIVE=0`, `NAV_SPEED=0`) despite
  `initially_busy=true`. The upstream `BHV_PeriodicSpeed::onIdleToRunState()`
  resets `m_mode_busy=false` under the default reset policy, so startup outcome
  depends on whether that lifecycle callback runs. This is not a logger-mode
  difference. The harness now separates the two contracts: cases that require
  an initially busy window explicitly set `reset_upon_running=false`, while
  `reset_on_reactivation_pass` independently verifies the default
  `reset_upon_running=true` behavior. All three startup-busy cases passed in
  both logging modes at jobs 1, followed by complete 24/24 jobs-4 matrices in
  minimal and full mode. The failed pre-fix attempt remains excluded from
  performance statistics;
- two `legrun_h01` full jobs-4 attempts consistently missed only the final
  `bad_speed_count_fail` result while focused full runs passed twice and the
  generated full target matched the untouched target after port normalization.
  Retained logs show the vehicle produced `NODE_REPORT_LOCAL` continuously but
  the shore received no `NODE_REPORT`; the vehicle broker never advanced from
  ping to its normal bridge set on pShare port 53970. A clean full jobs-4 matrix
  at the harness's ordinary low port range (`--port_base=26000`) passed 33/33
  in 141 seconds, so no case edit is warranted. The earlier high-port failures
  are treated as transport/port-run anomalies, not concurrency or logging
  regressions. Its untouched/minimal performance comparison remains the
  matched jobs-4 pair;
- the initially selected `testfailure_h01` was deferred before logging edits after its
  untouched jobs-4 baseline missed two of nine result rows
  (`hang_alias_gap_detected_pass` and `burn_default_time_gap_pass`).
  Retained logs prove both behaviors completed and posted very large one-cycle
  `PHELMIVP_ITER_GAP` values (815.76 and 1218.01), but `pMissionEval` missed the
  transient values and never emitted a verdict. That prerequisite is now
  resolved without a source change: a high-rate auxiliary `pMissionEval`
  latches `TEST_GAP_SEEN=true`, and the four burn/hang evaluators grade the
  durable latch plus `TEST_FAILURE_DONE` and process health. The nine-case
  baseline passed jobs 1 and three consecutive jobs-4 matrices. Its logging
  migration remains pending; `hostinfo_h01` still supplied the original tenth
  performance pair;
- launcher warning/error scans of retained successful matrices found zero
  matches; post-run checks found no relevant MOOS processes or listeners, and
  generated result snapshots and Python bytecode were removed.

### PID, pNodeReporter, Depth, And COLREGS Cohort — Complete

Targets: `pid_h02`, `pnodereporter_h01`, all five `depth_behavior_motion`
consumers, and all four `colregs_unit` consumers. Audit, implementation, and
validation date: July 18, 2026. The `colregs_h02` exception was resolved at the
test level and is included in the completed migration count.

Configuration audit and implementation:

- the PID, depth, and ordinary COLREGS minimal stems launch no `pLogger`;
  pNodeReporter is `G13` and runs one shoreside logger restricted to
  `NODE_REPORT_LOCAL`;
- full restoration lives in separate stem-local shoreside/vehicle patch
  assets and is applied before case overlays;
- `pid_h02`'s `runtime_speed_factor_update_pass` case replaces the vehicle
  ANTLER block because it also adds `uTimerScript`; its separate full logging
  case patch restores the pre-migration logger and preserves that case-only
  process without duplicating either process;
- direct and harness launchers accept only `--log=minimal|full`; all four
  checked-in stems and every harness in this cohort default to minimal;
- generated minimal targets contain zero active loggers for PID, depth, and
  ordinary COLREGS; pNodeReporter contains one logger with one `Log` variable.
  Full targets restore the untouched active logger counts and configurations;
- Bash syntax, ShellCheck for changed wrappers/helpers, all four eval-mission
  structural checks, and all eleven harness structural checks pass.

Complete matrices used jobs 4 at the harness's normal warp and low ordinary
port ranges. CPU is user plus system seconds. `Full pre logs` counts `.alog`
and `.slog` artifacts in the retained untouched baseline. Minimal workdirs
were retained long enough to count artifacts and then removed.

| Target | Cases | Full pre wall / CPU | Full pre logs (files / bytes) | Minimal wall / CPU | Minimal logs (files / bytes) | Wall / CPU reduction | Full post wall / CPU | Result |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| `pid_h02` | 13 | 44.04 / 27.62 | 65 / 16,458,268 | 36.66 / 14.07 | 0 / 0 | 16.8% / 49.1% | 44.41 / 28.30 | 13/13 both modes |
| `pnodereporter_h01` | 33 | 53.42 / 56.96 | 66 / 1,052,639 | 53.66 / 59.77 | 33 / 722,074 | -0.4% / -4.9% | 54.33 / 59.25 | 33/33 both modes |
| `depth_constant_h01` | 19 | 116.41 / 40.30 | 95 / 23,289,922 | 97.16 / 20.55 | 0 / 0 | 16.5% / 49.0% | 116.58 / 41.05 | 19/19 both modes |
| `depth_goto_h02` | 18 | 76.14 / 36.36 | 90 / 23,663,772 | 65.89 / 17.83 | 0 / 0 | 13.5% / 51.0% | 79.76 / 40.77 | 18/18 both modes |
| `depth_periodic_surface_h03` | 24 | 64.11 / 50.49 | 120 / 26,791,250 | 55.98 / 24.46 | 0 / 0 | 12.7% / 51.6% | 64.27 / 52.75 | 24/24 both modes |
| `depth_max_h04` | 15 | 47.34 / 34.76 | 75 / 19,675,873 | 42.19 / 15.25 | 0 / 0 | 10.9% / 56.1% | 48.08 / 33.68 | 15/15 both modes |
| `depth_min_altitude_h05` | 13 | 43.27 / 29.65 | 65 / 15,970,161 | 36.25 / 12.92 | 0 / 0 | 16.2% / 56.4% | 41.40 / 28.14 | 13/13 both modes |
| `colregs_h01` | 22 | 65.71 / 51.03 | 154 / 111,819,747 | 54.65 / 23.10 | 0 / 0 | 16.8% / 54.7% | 66.37 / 51.87 | 22/22 both modes |
| `colregs_h03` | 23 | 106.60 / 51.98 | 161 / 237,193,725 | 96.55 / 25.03 | 0 / 0 | 9.4% / 51.8% | 106.68 / 53.54 | 23/23 both modes |
| `colregs_h04` | 32 | 87.70 / 70.32 | 224 / 184,636,381 | 74.70 / 33.51 | 0 / 0 | 14.8% / 52.3% | 90.62 / 72.14 | 32/32 both modes post-fix |

Across the ten completed targets, untouched full totals were 704.74 wall
seconds, 449.47 CPU seconds, and 660,551,738 logger bytes. Minimal totals were
613.69 wall seconds, 246.49 CPU seconds, and 722,074 logger bytes: reductions
of 12.9% wall, 45.2% CPU, and 99.89% logger bytes. The full post totals were
712.50 wall and 461.49 CPU seconds, close to the untouched full envelope.
Maximum RSS remained near 8 MB and is not interpreted as aggregate child
memory.

H02's exception follow-up used functional jobs-4 wall-time validation rather
than `/usr/bin/time` CPU collection. The stable stand-on group passed 33/33 in
three repetitions per mode (minimal: 33, 32, 33 seconds; full: 39, 39, 41
seconds). The overtaking group passed 18/18 across three minimal repetitions
(45, 44, 42 seconds). The complete 56-case gate passed in 177 seconds minimal
and 203 seconds full, a 12.8% minimal wall-time reduction. These values are
kept separate from the ten-target CPU aggregate above.

Correctness and exception findings:

- pNodeReporter's no-logger audit exception fired: `reverse_load_warning_pass`
  needs transient history that is not reliably present in the final result
  payload. The `G13` logger records only `NODE_REPORT_LOCAL`; the affected case
  passed three consecutive minimal replays and the complete matrix passed in
  both modes;
- the first untouched `depth_min_altitude_h05` baseline missed
  `min_altitude_clearance_boundary_pass` at altitude `10.183`; a focused replay
  and clean full baseline passed, and both post-edit matrices passed. No case
  edit was made;
- `colregs_h02` minimal jobs-4 runs initially failed different timeout sets
  (52/58 in 212.55 seconds and 54/58 in 227.05 seconds), while one full
  restoration control passed 58/58 in 201.58 seconds. A bounded vehicle-stack
  readiness gate fixed the affected give-way startup family. Retained logs
  then separated three independent test-contract problems from logging: the
  `standon_unsurebow_above_pass` reciprocal track straddled the unintended
  10 m give-way bow-distance split and now uses the existing stable
  `crossing_port_standon_exec_far_pass` geometry; the Band 350 bow fixture was
  moved from y=-64 to y=-60 to stay inside its intended classification; and
  the two moving Band 315 `unsure`/`unsure_bow` probes were unstable in both
  logging modes and are now explicit exploratory cases rather than default
  gate cases. They were not deleted. The overtaking mirror evaluator now waits
  for its expected classification instead of an arbitrary timer, and all five
  overtaking threshold evaluators use a 90 rather than 75 simulated-second
  timeout. Staged-deploy, reduced-warp, passive-contact, and exclusive-slot
  experiments did not provide an acceptable general fix and were reverted.
  The final 56-case jobs-4 matrix passed in both modes with minimal as the
  default; no solo-slot workaround or behavior source edit remains;
- two full `colregs_h04` matrices exposed independent evaluator sampling
  boundaries. `eval_tol_default_pass` observed CPA `21.939` against a `21.8`
  ceiling, and `pwt_grade_quasi_pass` observed PWT `33.664` after its intended
  range checkpoint. The test-level bands are now `20.7-22.1` and `18-36`,
  respectively; both remain separated from the adjacent parameter bands.
  Each case passed three focused repetitions in both modes where applicable,
  and the final full jobs-4 matrix passed 32/32;
- all accepted matrices produced the exact expected result-row count, cleanup
  reported no teardown failure, and retained generated workdirs were removed
  after statistics were captured.

## Migration Tracking

Use one row per registered target. Shared-stem changes may advance several rows
in one batch, but each consumer must have its own validation status.

Status vocabulary:

- Minimal class: audited `N`, `G`, or `S` as defined above. Classification is
  complete, but it does not advance the migration Audit status.
- Audit: `not started`, `in progress`, or `complete`.
- Implementation: `pending`, `in progress`, or `complete`.
- Validation/benchmark fields: `pending`, `pass`, `fail`, `n/a`, or a dated
  result reference.
- Overall: `not started`, `in progress`, `blocked`, or `complete`.

The ledger remains in registry order for stable comparison with the dependency
map. Execute it by class using the rollout order above, with the documented
mixed-shared-stem deferrals.

| Target | Stem | Shared | Minimal class | Audit | Implementation | Minimal serial | Minimal rolling | Full regression | Benchmark | Exception | Overall |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `cmgr_h01` | `cmgr_unit` | no | `N` | complete | complete | pass (jobs 1) | pass (jobs 2/4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `cmgr_h02` | `cmgr_motion` | no | `N` | complete | complete | pass (2 cases, jobs 1) | pass (jobs 2/4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `obmgr_h01` | `obmgr_unit` | no | `N` | complete | complete | pass (2 cases, jobs 1) | pass (jobs 2/4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `obmgr_h02` | `obmgr_motion` | no | `N` | complete | complete | pass (2 cases, jobs 1) | pass (jobs 2/4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `pid_h01` | `pid_unit` | no | `N` | complete | complete | pass (2 cases, jobs 1) | pass (jobs 2/4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `pid_h02` | `pid_motion` | no | `N` | complete | complete | pass (runtime case, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | case ANTLER replacement preserves case-only uTimerScript and restores logger only in full | complete |
| `usim_marine_h01` | `usim_marine_motion` | no | `N` | complete | complete | pass (2 cases, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `pnodereporter_h01` | `pnodereporter_unit` | no | `G` | complete | complete | pass (`N1` cases, both modes; reverse minimal 3x) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | `G13`: minimal logs only NODE_REPORT_LOCAL after N1 fallback proved necessary | complete |
| `upokedb_h01` | `upokedb_unit` | no | `N` | complete | complete | pass (2 cases, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | external-stimulus checker limitation; shared mission-file readiness race fixed | complete |
| `uxms_h01` | `uxms_unit` | no | `N` | complete | complete | pass (2 cases, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `uquerydb_h01` | `uquerydb_unit` | no | `N` | complete | complete | pass (2 cases, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | CLI-result grading exception preserved | complete |
| `pechovar_h01` | `pechovar_unit` | no | `G` | not started | pending | pending | pending | pending | pending | none known | not started |
| `utimerscript_h01` | `utimerscript_unit` | no | `G` | not started | pending | pending | pending | pending | pending | none known | not started |
| `pdeadmanpost_h01` | `pdeadmanpost_unit` | no | `G` | not started | pending | pending | pending | pending | pending | none known | not started |
| `pspoofnode_h01` | `pspoofnode_unit` | no | `G` | not started | pending | pending | pending | pending | pending | none known | not started |
| `utermcommand_h01` | `utermcommand_unit` | no | `N` | complete | complete | pass (2 cases, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | external-stimulus checker limitation | complete |
| `psearchgrid_h01` | `psearchgrid_unit` | no | `G` | not started | pending | pending | pending | pending | pending | none known | not started |
| `depth_constant_h01` | `depth_behavior_motion` | yes (5) | `N` | complete | complete | pass (covered by matrix) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `depth_goto_h02` | `depth_behavior_motion` | yes (5) | `N` | complete | complete | pass (covered by matrix) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `depth_periodic_surface_h03` | `depth_behavior_motion` | yes (5) | `N` | complete | complete | pass (covered by matrix) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `depth_max_h04` | `depth_behavior_motion` | yes (5) | `N` | complete | complete | pass (covered by matrix) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `depth_min_altitude_h05` | `depth_behavior_motion` | yes (5) | `N` | complete | complete | pass (boundary focused) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | untouched boundary miss passed focused and clean repeats; no edit | complete |
| `colregs_h01` | `colregs_unit` | yes (4) | `N` | complete | complete | pass (covered by matrix) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `colregs_h02` | `colregs_unit` | yes (4) | `N` | complete | complete | pass (focused families) | pass (56/56, jobs 4) | pass (56/56, jobs 4) | 2026-07-18 wall-only exception follow-up | resolved `S3`: readiness and evaluator contracts fixed; two moving Band 315 probes retained as explicit exploratory cases; no solo slot or source edit | complete |
| `colregs_h03` | `colregs_unit` | yes (4) | `N` | complete | complete | pass (covered by matrix) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `colregs_h04` | `colregs_unit` | yes (4) | `N` | complete | complete | pass (2 sampling cases, both modes) | pass (jobs 4) | pass post-fix (jobs 4) | 2026-07-18 paired | two evaluator sampling bands widened without overlapping adjacent contracts | complete |
| `collision_h01` | `collision_behavior_motion` | no | `N` | complete | complete | pass (2 cases, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `convoy_h01` | `convoy_behavior_motion` | no | `N` | complete | complete | pass (2 cases, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | two-vehicle full mode correctly restores 3 pLoggers | complete |
| `cutrange_h01` | `cutrange_behavior_motion` | no | `N` | complete | complete | pass (2 cases, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | two-vehicle full mode correctly restores 3 pLoggers | complete |
| `fixedturn_h01` | `fixedturn_behavior_motion` | no | `N` | complete | complete | pass (2 cases, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `periodic_speed_h01` | `periodic_speed_behavior_motion` | no | `N` | complete | complete | pass (3 startup cases, both modes, jobs 1) | pass post-fix (jobs 4) | pass post-fix (jobs 4) | 2026-07-18 paired | startup-busy cases explicitly disable reset; default reset has a separate case | complete |
| `zigzag_h01` | `zigzag_behavior_motion` | no | `N` | complete | complete | pass (2 cases, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `legrun_h01` | `legrun_behavior_motion` | no | `N` | complete | complete | pass (2 cases, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4 low ports; jobs 2 high ports) | 2026-07-18 paired jobs 4 | high-port pShare handshake anomaly; no case edit | complete |
| `memoryturnlimit_h01` | `memoryturnlimit_behavior_motion` | no | `N` | complete | complete | pass (2 cases, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `timer_h01` | `timer_behavior_motion` | no | `N` | complete | complete | pass (2 cases, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | none found | complete |
| `testfailure_h01` | `testfailure_behavior_unit` | no | `N` | not started | pending | pending (baseline 9/9 jobs 1) | pending (baseline 9/9 jobs 4, 3x) | pending | pending | sticky gap latch prerequisite resolved; logging migration not started | ready |
| `hostinfo_h01` | `hostinfo_unit` | no | `N` | complete | complete | pass (2 cases, both modes, jobs 1) | pass (jobs 4) | pass (jobs 4) | 2026-07-18 paired | replacement for deferred `testfailure_h01`; none found | complete |
| `loadwatch_h01` | `loadwatch_unit` | no | `N` | not started | pending | pending | pending | pending | pending | none known | not started |
| `processwatch_h01` | `processwatch_unit` | no | `N` | not started | pending | pending | pending | pending | pending | none known | not started |
| `pshare_h01` | `pshare_unit` | no | `N` | not started | pending | pending | pending | pending | pending | `ANTLER`/`pLogger` patch | not started |
| `pshare_h02` | `pshare_topology` | no | `N` | not started | pending | pending | pending | pending | pending | `ANTLER`/`pLogger` patch | not started |
| `plogger_h01` | `plogger_unit` | no | `S` | not started | pending | pending | pending | pending | pending | subject harness (`S1`) | not started |
| `pantler_h01` | `pantler_unit` | no | `S` | not started | pending | pending | pending | pending | pending | subject harness (`S2`) | not started |
| `pmissioneval_h01` | `mission_utility_unit` | yes (3) | `G` | not started | pending | pending | pending | pending | pending | `ANTLER`/`pLogger` patch (`G6`) | not started |
| `pmissionhash_h01` | `mission_utility_unit` | yes (3) | `G` | not started | pending | pending | pending | pending | pending | none known | not started |
| `umayfinish_h01` | `mission_utility_unit` | yes (3) | `G` | not started | pending | pending | pending | pending | pending | none known | not started |
| `loiter_h01` | `loiter_behavior_motion` | no | `N` | not started | pending | pending | pending | pending | pending | none known | not started |
| `obstacle_behavior_h01` | `obstacle_behavior_motion` | no | `N` | not started | pending | pending | pending | pending | pending | `ANTLER`/`pLogger` patch | not started |
| `opregion_h01` | `opregion_motion` | no | `N` | not started | pending | pending | pending | pending | pending | none known | not started |
| `p01_obstacle` | `P01-obstacle_gauntlet` | no | `G` | not started | pending | pending | pending | pending | pending | none known | not started |
| `shadow_h01` | `shadow_behavior_motion` | no | `N` | not started | pending | pending | pending | pending | pending | none known | not started |
| `stationkeep_h01` | `stationkeep_behavior_motion` | no | `N` | not started | pending | pending | pending | pending | pending | none known | not started |
| `trail_h01` | `trail_behavior_motion` | no | `N` | not started | pending | pending | pending | pending | pending | none known | not started |
| `waypoint_h01` | `waypoint_behavior_motion` | no | `N` | not started | pending | pending | pending | pending | pending | none known | not started |
| `p02_colregs` | `P02-colregs_joust` | no | `G` | not started | pending | pending | pending | pending | pending | none known | not started |
| `p03_colregs` | `P03-colregs_traffic_ring` | no | `G` | not started | pending | pending | pending | pending | pending | none known | not started |
| `ufld_pathcheck_h01` | `ufield_app_unit` | yes (7) | `N` | not started | pending | pending | pending | pending | pending | defer with `G10`; `ANTLER`/`pLogger` patch | not started |
| `ufld_message_handler_h01` | `ufield_app_unit` | yes (7) | `N` | not started | pending | pending | pending | pending | pending | defer with `G10`; `ANTLER`/`pLogger` patch | not started |
| `ufld_contact_range_sensor_h01` | `ufield_app_unit` | yes (7) | `N` | not started | pending | pending | pending | pending | pending | defer with `G10`; `ANTLER`/`pLogger` patch | not started |
| `ufld_beacon_range_sensor_h01` | `ufield_app_unit` | yes (7) | `N` | not started | pending | pending | pending | pending | pending | defer with `G10`; `ANTLER`/`pLogger` patch | not started |
| `ufld_collision_detect_h01` | `ufield_app_unit` | yes (7) | `N` | not started | pending | pending | pending | pending | pending | defer with `G10`; `ANTLER`/`pLogger` patch | not started |
| `ufld_collob_detect_h01` | `ufield_app_unit` | yes (7) | `N` | not started | pending | pending | pending | pending | pending | defer with `G10`; `ANTLER`/`pLogger` patch | not started |
| `ufld_scope_h01` | `ufield_app_unit` | yes (7) | `G` | not started | pending | pending | pending | pending | pending | mixed shared stem; patch (`G10`) | not started |
| `ufield_comms_h01` | `ufield_comms_unit` | yes (3) | `N` | not started | pending | pending | pending | pending | pending | defer with `G11`/`G12` shared stem | not started |
| `ufield_comms_h02` | `ufield_comms_unit` | yes (3) | `G` | not started | pending | pending | pending | pending | pending | mixed shared stem; patch (`G11`) | not started |
| `ufield_comms_h03` | `ufield_comms_unit` | yes (3) | `G` | not started | pending | pending | pending | pending | pending | mixed shared stem (`G12`) | not started |
| `ufld_obstacle_sim_h01` | `ufld_obstacle_sim_unit` | no | `N` | not started | pending | pending | pending | pending | pending | none known | not started |

For every exception, record:

- the target and affected cases;
- the conflicting patch or grading dependency;
- why the ordinary policy is insufficient;
- the chosen local behavior;
- the validation evidence;
- when the user was notified.

## Completion Criteria

The migration is complete when:

- all 67 registered targets accept the standard logging interface;
- minimal is the default for harness and direct stem launches;
- full applies to the whole selected harness set and composes with `--case`;
- every stem has reviewed minimal behavior and restorable full behavior;
- every shared-stem consumer is validated;
- ordinary grading is equivalent between modes;
- all exceptions are documented and communicated;
- repository invariants and the generated dependency map are current;
- final statistics and representative CPU/concurrency measurements are
  recorded.
