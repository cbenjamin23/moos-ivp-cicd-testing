# H01-cmgr_tests

Patch-driven harness for [`missions/cmgr_missions/cmgr_unit`](../../../missions/cmgr_missions/cmgr_unit).

This harness turns one small stem mission into a broader `pContactMgrV20` test
matrix. Each case is intended to behave like a unit test expressed as one short
mission run. The harness patches mission parameters and, when needed, patches
the `pMissionEval` rule so each case grades the specific contact-manager
behavior under test. The stem mission uses the lower-band Forrest 19 / MIT_SP
framing with the ownship starting at `(0,-60)` and the default corridor set to
`(60,-60)`.

For the patching mechanics, see [`NSPATCH.md`](./NSPATCH.md).

## Current Matrix

### Single-contact cases

- `detect_baseline_pass`: Posts `intruder` at `(20,-44)` four seconds after
  deploy under the static `track` alert with `alert_range=80` and
  `cpa_range=100`, testing the normal alert `on_flag`; passes when
  `CONTACT_SEEN=true` at the 12-second checkpoint.
- `detect_strict_absent_pass`: Keeps the baseline contact but reduces both
  alert thresholds to 10 meters, exercising range/CPA exclusion; passes when
  the checkpoint is reached without `CONTACT_SEEN=true`.
- `detect_edge_pass`: Sets both alert thresholds to 18 meters for the baseline
  contact, testing admission near the threshold; passes when
  `CONTACT_SEEN=true`.
- `detect_edge_absent_pass`: Sets both alert thresholds to 14 meters while the
  reported closest range is 14 meters, exercising the alert boundary; the
  current evaluator passes when `CONTACT_SEEN` remains false but does not grade
  the reported range.
- `detect_delayed_spoof_pass`: Delays the baseline `SPOOF` from four to eight
  seconds and moves the checkpoint to 16 seconds, testing late contact intake;
  passes when `CONTACT_SEEN=true`.
- `detect_short_duration_absent_pass`: Sets the spoof duration to two seconds,
  posts `intruder` at `(50,-44)`, and uses 20-meter alert thresholds,
  exercising short-lived contact input; the current evaluator passes when
  `CONTACT_SEEN` remains false, but the reported 34-meter range also places the
  contact outside both thresholds.
- `detect_early_checkpoint_absent_pass`: Evaluates at two seconds before the
  baseline spoof is posted at four seconds, testing pre-contact state; passes
  when `TEST_EVAL_READY=true` and `CONTACT_SEEN` has not become true.
- `detect_far_spoof_absent_pass`: Posts `intruder` at `(20,-30)` with
  20-meter alert thresholds, exercising an out-of-range tracked contact;
  passes when `CONTACT_SEEN` remains false.
- `detect_near_spoof_pass`: Moves `intruder` to `(20,-50)` under the baseline
  alert, exercising close-range detection; passes when `CONTACT_SEEN=true`.
- `detect_cpa_only_pass`: Posts a closing contact at `(35,-35)` with speed
  `2.0`, `alert_range=20`, and `cpa_range=40`, testing CPA-based detection
  while the current range remains outside the alert range; passes when
  `CONTACT_SEEN=true`, `CONTACT_CLOSEST=intruder`, and
  `CONTACT_CLOSEST_RANGE>20`.
- `closest_contact_pass`: Uses the baseline contact to test closest-contact
  naming; passes when `CONTACT_SEEN=true` and
  `CONTACT_CLOSEST=intruder`.
- `post_all_ranges_pass`: Enables `post_all_ranges=true` for alerted contacts
  `alpha` and `bravo`, exercising `CONTACT_RANGES` publication; the current
  evaluator passes on `CONTACTS_COUNT=2` and
  `CONTACTS_LIST=alpha,bravo` while recording, but not comparing, the ranges.
- `post_closest_relbng_pass`: Enables `post_closest_relbng=true` for the same
  two-contact geometry, exercising `CONTACT_CLOSEST_RELBNG` publication; the
  current evaluator passes on `CONTACTS_COUNT=2` and
  `CONTACT_CLOSEST=alpha` while recording, but not grading, the bearing.
- `runtime_alert_add_pass`: Removes the static alert, posts a complete
  `BCM_ALERT_REQUEST` at two seconds, and posts `intruder` at four seconds,
  testing runtime alert creation; passes when `CONTACT_SEEN=true` and
  `CONTACT_CLOSEST=intruder`.
- `runtime_alert_disable_absent_pass`: Disables the static `track` alert with
  `BCM_ALERT_REQUEST` at two seconds before posting `intruder` at four seconds,
  exercising runtime alert disable; passes when `CONTACT_SEEN` remains false.
- `filter_match_type_absent_pass`: Adds `match_type=ship` to the alert while
  the spoofed contact retains the default `kayak` type, exercising type
  filtering; the current evaluator passes when `CONTACT_SEEN` remains false
  and records, but does not require, `CONTACTS_LIST=intruder`.
- `disable_contact_pass`: Detects `intruder` at four seconds and posts
  `CONTACT_DISABLE=intruder` at six seconds, exercising translation to a
  `BHV_ABLE_FILTER` disable message; the current evaluator requires
  `CONTACT_SEEN=true` and any filter mail, but does not compare the payload.
- `enable_contact_pass`: Detects `intruder` at four seconds and posts
  `CONTACT_ENABLE=intruder` at six seconds, exercising translation to a
  `BHV_ABLE_FILTER` enable message; the current evaluator requires
  `CONTACT_SEEN=true` and any filter mail, but does not compare the payload.
- `early_warning_pass`: Configures `early_warning_time=3`, disables warning
  radii, and sets `early_warning_flag=CONTACT_WARN=true`, testing the warning
  flag path; passes when `CONTACT_WARN=true` at the checkpoint.
- `count_one_pass`: Uses the baseline contact to test alerted-contact counting;
  passes when `CONTACT_SEEN=true` and `CONTACTS_COUNT=1`.
- `list_intruder_pass`: Uses the baseline contact to test the known-contact
  list; passes when `CONTACT_SEEN=true` and
  `CONTACTS_LIST=intruder`.
- `range_report_pass`: Uses the baseline contact to test the closest-range
  report; passes when `CONTACT_SEEN=true`, `CONTACT_CLOSEST=intruder`, and
  `10 < CONTACT_CLOSEST_RANGE < 30`.
- `report_request_pass`: Posts
  `BCM_REPORT_REQUEST=var=BCM_CONTACTS_20,range=20` two seconds before the
  baseline contact, testing the custom range-limited report subsystem; passes
  when `BCM_CONTACTS_20=intruder`.
- `range_tight_pass`: Applies a tighter check to the baseline closest-range
  report; passes when `CONTACT_SEEN=true`, `CONTACT_CLOSEST=intruder`, and
  `12 < CONTACT_CLOSEST_RANGE < 16`.

### Multi-contact cases on the same stem

- `count_two_pass`: Posts `alpha` at `(20,-50)` and `bravo` at `(20,-40)`
  under 30-meter range and 35-meter CPA thresholds, testing alerted-contact
  counting; passes when `CONTACTS_COUNT=2`.
- `list_alpha_bravo_pass`: Uses the same two contacts to test known-contact
  list construction; passes when `CONTACTS_COUNT=2` and
  `CONTACTS_LIST=alpha,bravo`.
- `closest_alpha_pass`: Uses the same geometry, where `alpha` is nearer, to
  test closest-contact selection; passes when `CONTACTS_COUNT=2` and
  `CONTACT_CLOSEST=alpha`.
- `closest_bravo_pass`: Moves `alpha` to `(20,-36)` while placing `bravo` at
  `(20,-50)`, testing closest-contact reselection; passes when
  `CONTACTS_COUNT=2` and `CONTACT_CLOSEST=bravo`.
- `bravo_far_count_one_pass`: Places `alpha` at `(20,-50)` and `bravo` at
  `(20,-20)`, testing the distinction between tracked and alerted contacts;
  passes when `CONTACTS_COUNT=1`, `CONTACTS_LIST=alpha,bravo`, and
  `CONTACT_CLOSEST=alpha`.
- `late_bravo_count_two_pass`: Posts `alpha` at four seconds, `bravo` at nine
  seconds, and evaluates at 16 seconds, testing delayed multi-contact intake;
  passes when `CONTACTS_COUNT=2` and `CONTACTS_LIST=alpha,bravo`.
- `stale_bravo_drop_pass`: Sets `contact_max_age=6`, gives `bravo` a
  three-second spoof duration, and evaluates at 18 seconds, testing stale
  contact removal while `alpha` remains current; passes when
  `CONTACTS_COUNT=1`, `CONTACTS_LIST=alpha`, and
  `CONTACT_CLOSEST=alpha`.
- `reject_range_retire_pass`: Sets `reject_range=12` and posts `intruder` at
  `(20,-30)`, testing range-based retirement after contact intake; passes when
  `CONTACTS_RETIRED=intruder`.
- `alpha_far_bravo_only_pass`: Places tracked `alpha` at `(20,-20)` and alerted
  `bravo` at `(20,-50)`, testing alert-count and closest-contact selection with
  a farther background contact; passes when `CONTACTS_COUNT=1`,
  `CONTACTS_LIST=alpha,bravo`, and `CONTACT_CLOSEST=bravo`.

## Typical Runs

```bash
./zlaunch.sh
./zlaunch.sh --log=full
./zlaunch.sh --jobs=4 --port_base=20000 10
./zlaunch.sh --case=detect_baseline_pass 10
./zlaunch.sh --log=full --case=detect_baseline_pass 10
./zlaunch.sh --case=count_two_pass 10
./zlaunch.sh --just_make 10
./zlaunch.sh --max_time=65 10
```

Execution notes:

- every selected case, including single-case and `--jobs=1` runs, uses an
  isolated mission copy and a unique port block
- logging defaults to `minimal` for every selected case; `--log=full` restores
  the stem's previous logger configuration for the whole selection
- `--jobs=<N>` uses rolling scheduling: the next pending case starts whenever
  any active case finishes
- every case writes its own log and intermediate result row beneath a unique
  harness-owned invocation root
- final rows are aggregated in the matrix order, not completion order
- `--keep_workdirs` preserves only the current invocation root for inspection
- `--gui` requires one explicit `--case`; full matrices remain headless
- cleanup is scoped to that invocation root; the harness does not use a
  machine-wide `ktm`, `pkill`, or `killall`

## What `./zlaunch.sh` Does Here

When run from this harness directory, `./zlaunch.sh` owns case selection,
isolation, scheduling, and aggregation while the copied stem owns mission
launch and grading:

1. Choose one case or the full default case list.
2. Copy the stem mission into a per-case directory and run its local
   `clean.sh`.
3. Apply the optional full-logging restoration first, followed by the selected
   case overlays, only inside that copy.
4. Assign explicit MOOSDB and pShare ports from the case's 30-port block.
5. Call the copied stem's `zlaunch.sh`.
6. Read the single mission-owned `pMissionEval` row from its `results.txt`.
7. Prepend `case=<name>` and retain the mission's `grade=` and evidence fields.
8. After every selected case finishes, write the normalized rows to the
   harness `results.txt` in selected-case order.

Runner failures use `grade=fail reason=<runner_reason>`. Ordinary case verdicts
come directly from `pMissionEval`; the harness no longer compares a redundant
shell `expected=pass` value with an extracted `actual` value.

## Results Lines

Typical detection-style harness line:

```text
case=detect_edge_pass grade=pass form=cmgr_tests mmod=detect_edge_pass eval=true seen=true closest=intruder range=14 mhash=[RUDE-SLAB]
```

Typical multi-contact reporting line:

```text
case=count_two_pass grade=pass form=cmgr_tests mmod=count_two_pass eval=true count=2 list=alpha,bravo closest=alpha range=8 mhash=[MILD-SVEN]
```

Typical extended reporting line:

```text
case=post_all_ranges_pass grade=pass form=cmgr_tests mmod=post_all_ranges_pass eval=true count=2 list=alpha,bravo ranges=8.7,18.9 closest=alpha mhash=[TAME-THOR]
```

Typical dynamic/reporting line:

```text
case=report_request_pass grade=pass form=cmgr_tests mmod=report_request_pass eval=true report=intruder seen=true range=14 mhash=[COOL-JEFF]
```

Typical retirement line:

```text
case=reject_range_retire_pass grade=pass form=cmgr_tests mmod=reject_range_retire_pass eval=true retired=intruder mhash=[WARY-AVEN]
```

Field anatomy:

- `case`
  The harness case name that was run.
- `grade`
  Pass/fail result written by the mission itself.
- `form`
  Harness or mission family name.
- `mmod`
  The mission mode written by `pMissionEval` for the selected case.
- `eval`
  Whether the case's evaluation checkpoint was reached.
- `seen`
  Whether `CONTACT_SEEN=true` by the checkpoint, when included by the case.
- `closest`
  The closest reported contact, when that column is included by the case.
- `range`
  The closest reported range, when that column is included by the case.
- `ranges`
  The complete `CONTACT_RANGES` list, when that column is included by the case.
- `relbng`
  The closest contact's rounded relative bearing, when that column is included
  by the case.
- `count`
  The alerted-contact count, when that column is included by the case.
- `list`
  The known-contact list still tracked by contact manager, when that column is
  included by the case.
- `report`
  The value posted by a `BCM_REPORT_REQUEST` for the selected case, when included.
- `filter`
  The `BHV_ABLE_FILTER` message for contact enable/disable cases.
- `filter_seen`
  Mission-side confirmation that the selected case's filter message was observed.
- `warn`
  The early-warning flag value when the warning branch is enabled.
- `retired`
  The retired-contact publication, when included.
- `mhash`
  Short mission hash.

Not every case reports every optional field. Detection-only cases stay shorter.
Count/list/report/retired columns are included only when they help explain the
specific case.

Important multi-contact note:

- `count` is the number of alerted contacts
- `list` is the known-contact list still tracked by contact manager

That means a case can correctly report `count=1` and `list=alpha,bravo` at the
same time.

## Expected Outcomes

The harness treats each case like a small mission-level unit test:

- a case passes if the expected contact-manager behavior happened
- a case fails if the mission graded something else
- some cases confirm detection
- other cases confirm correct non-detection
- reporting cases confirm the right published contact-manager outputs

Important note about the current matrix:

- every current case is expected to end with mission `grade=pass`
- that includes the absence cases
- in those cases, `grade=pass` means the mission correctly confirmed that a
  contact should stay unseen or should stay out of the alerted set

The normalized harness row contains one authoritative `grade` field. The
legacy `actual` and `expected` shell fields were redundant because every case
in this matrix already makes the intended outcome, including non-detection,
produce mission-owned `grade=pass`.

## Wall-Clock Efficiency

Legacy wave implementation, validated on March 20, 2026:

- full 33-case matrix passed at warp `10`
- serial wall clock: `239.45` seconds
- `--jobs=4 --port_base=20000` wall clock: `110.56` seconds
- the suite is still dominated by launch and teardown overhead rather than the
  case logic itself

The suite exits on `MISSION_EVALUATED`, so fixed launch and teardown work is a
larger wall-clock cost than the case logic itself. Lowering `--max_time`
further is not likely to save much unless cases are timing out.

Migrated rolling implementation, validated on July 13, 2026:

- unified isolated `--jobs=1`: 33/33 passed in `257.51` seconds
- three matching `--jobs=2 --port_base=20000` matrices: 99/99 rows passed
- rolling wall times: `132.84`, `136.17`, and `135.12` seconds
- rolling mean: `134.71` seconds; median: `135.12` seconds
- the matching legacy three-run mean was `146.20` seconds
- the migrated mean was `11.49` seconds, or about `7.9%`, faster

The retained rolling evidence was inspected for generated ports, sidecars, and
logs, then removed so generated mission trees do not inflate source review. An
intentional warp-1/one-second timeout matrix also confirmed that all 33
missing-result failures are reported before the launcher exits nonzero.

Skill 1.4.2 alignment, validated on July 14, 2026:

- the stem's Apple Bash 3.2 paths safely handle empty GUI/forwarding arrays and
  report zero or duplicate mission rows without an unbound-variable error
- runner-owned launch failures retain a valid mission row as renamed
  `mission_*` provenance instead of discarding it
- `detect_baseline_pass` passed five consecutive focused repetitions, and
  `detect_strict_absent_pass` again passed with `seen=false`
- the isolated serial matrix passed 33/33 in `282.27` seconds
- three clean rolling matrices passed 99/99 in `154`, `140`, and `153.88`
  seconds; their mean was `149.29` seconds
- one apparent `570`-second sample included a documented `429`-second macOS
  clamshell sleep and is excluded from timing statistics
- a diagnostic run using the helper's supported one-second INT/TERM grace
  overrides passed 33/33 in `134.38` seconds, essentially matching the earlier
  `134.71`-second mean

The refreshed helper's canonical three-second grace defaults therefore account
for the current timing increase. They are retained unchanged here; the cost is
cleanup patience rather than slower mission evaluation, copy preparation, or
scheduler refill.
