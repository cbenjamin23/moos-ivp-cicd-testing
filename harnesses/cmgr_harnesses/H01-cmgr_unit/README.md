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

- `detect_baseline_pass`
  Default geometry should see the spoofed contact before the checkpoint.
- `detect_strict_absent_pass`
  Tighter alert thresholds should keep the same spoof unseen.
- `detect_edge_pass`
  Near-threshold geometry should still detect the contact before the
  checkpoint.
- `detect_edge_absent_pass`
  Slightly tighter near-threshold geometry should stay unseen.
- `detect_delayed_spoof_pass`
  A later spoof and later checkpoint should still detect.
- `detect_short_duration_absent_pass`
  A short-lived distant spoof should stay unseen.
- `detect_early_checkpoint_absent_pass`
  An intentionally early checkpoint should occur before detection.
- `detect_far_spoof_absent_pass`
  A farther spoof should stay outside the alert window.
- `detect_near_spoof_pass`
  A nearer spoofed contact should be detected quickly and reported by the
  checkpoint.
- `detect_cpa_only_pass`
  This is a CPA-specific case rather than just another range threshold case.
  The contact is still outside `alert_range` at grading time, but it is on a
  closing course tight enough that `cpa_range` should still make the alert
  fire.
- `closest_contact_pass`
  The closest-contact report should identify `intruder`.
- `post_all_ranges_pass`
  This case turns on the `CONTACT_RANGES` report. The mission should publish
  the full list of current alerted ranges instead of only the closest one.
- `post_closest_relbng_pass`
  This turns on the closest-relative-bearing report. The mission should publish
  `CONTACT_CLOSEST_RELBNG`, proving that bearing output works as well as the
  closest-contact selection itself.
- `runtime_alert_add_pass`
  No static alert is configured in this case. Instead
  the harness posts a `BCM_ALERT_REQUEST` at runtime and expects that dynamic
  alert definition to drive normal contact detection.
- `runtime_alert_disable_absent_pass`
  A runtime `BCM_ALERT_REQUEST` disables the normal alert before the spoofed
  contact arrives. The correct outcome is non-detection.
- `filter_match_type_absent_pass`
  This is a representative alert-filter case. The alert is constrained to
  `match_type=ship`, while the spoofed contact remains the default `kayak`, so
  the contact should still be tracked but should not count as alerted.
- `disable_contact_pass`
  The contact is already seen before the disable request is posted, so this
  case checks that contact manager emits the expected `BHV_ABLE_FILTER`
  disable message. It is not trying to suppress an alert that already fired.
- `enable_contact_pass`
  The matching enable case should emit the expected `BHV_ABLE_FILTER` enable
  request for a named contact under controlled timing.
- `early_warning_pass`
  The configured warning flag should post before the main contact evaluation
  completes.
- `count_one_pass`
  The alerted-contact count should be exactly `1`.
- `list_intruder_pass`
  The known-contact list should contain only `intruder`.
- `range_report_pass`
  `CONTACT_CLOSEST_RANGE` should land in a reasonable broad band.
- `report_request_pass`
  This exercises the separate `BCM_REPORT_REQUEST` subsystem. The case asks
  contact manager to publish a custom range-limited contact list variable and
  verifies that the requested report contains `intruder`.
- `range_tight_pass`
  `CONTACT_CLOSEST_RANGE` should land in a tighter band around the baseline.

### Multi-contact cases on the same stem

- `count_two_pass`
  The alerted-contact count should be exactly `2`.
- `list_alpha_bravo_pass`
  The known-contact list should be `alpha,bravo`.
- `closest_alpha_pass`
  The closest-contact report should identify `alpha`.
- `closest_bravo_pass`
  The closest-contact report should identify `bravo`.
- `bravo_far_count_one_pass`
  The contact `bravo` should remain tracked, but it should stay outside the
  alert count.
- `late_bravo_count_two_pass`
  The contact `bravo` arrives later, and the delayed checkpoint should still
  catch both contacts.
- `stale_bravo_drop_pass`
  The contact `bravo` should expire and drop out of the tracked list before
  grading.
- `reject_range_retire_pass`
  The contact is accepted first, then a very small `reject_range` should cause
  contact manager to retire it and post `CONTACTS_RETIRED=intruder`.
- `alpha_far_bravo_only_pass`
  The contact `alpha` should remain tracked, but only `bravo` should qualify as
  alerted and closest.

## Typical Runs

```bash
./zlaunch.sh
./zlaunch.sh --jobs=4 --port_base=20000 10
./zlaunch.sh --case=detect_baseline_pass 10
./zlaunch.sh --case=count_two_pass 10
./zlaunch.sh --just_make 10
./zlaunch.sh --max_time=65 10
```

Wave mode notes:

- `--jobs=<N>` runs the matrix in waves of up to `N` isolated case copies
- each live case in a wave gets its own temp mission directory and unique port
  block
- the harness uses one `ktm` barrier between waves, not after every case
- this mode is intended for CI wall-clock reduction, not for interactive use
  alongside other MOOS missions

## What `./zlaunch.sh` Does Here

When run from this harness directory, `./zlaunch.sh` does not call the stem
mission's own `zlaunch.sh`. Instead it owns the full run:

1. Choose one case or the full default case list.
2. Run the stem mission `clean.sh` and `ktm` so the next case starts clean.
3. Apply the selected `nspatch` overlays to `missions/cmgr_missions/cmgr_unit`.
4. Call shared `xlaunch.sh`, which in turn calls the stem mission `launch.sh`.
5. Read the mission-local `results.txt`.
6. Compare `actual` against `expected`.
7. Append one summary line to the harness `results.txt`.
8. On harness exit, run one final `clean.sh` and `ktm`.

That means the harness owns patching, cleanup, and result comparison.

In wave mode, those same steps happen inside per-case temporary mission copies.
The shared stem directory remains the source for patches and the default serial
execution path.

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

Why both `actual` and `grade` appear in harness output:

- `grade` is the mission's own result, written by `pMissionEval`
- `actual` is the harness's extracted view of that result, used for comparison
  against `expected`
- today `actual` is usually the same value as `grade`
- they are kept separate because they come from different layers:
  mission result versus harness observation of that result

## Wall-Clock Efficiency

Validated on March 20, 2026:

- full 33-case matrix passed at warp `10`
- serial wall clock: `239.45` seconds
- `--jobs=4 --port_base=20000` wall clock: `110.56` seconds
- the suite is still dominated by launch and teardown overhead rather than the
  case logic itself

The suite exits on `MISSION_EVALUATED`, so fixed launch and teardown work is a
larger wall-clock cost than the case logic itself. Lowering `--max_time`
further is not likely to save much unless cases are timing out.
