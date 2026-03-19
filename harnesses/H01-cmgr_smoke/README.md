# H01-cmgr_smoke

Patch-driven harness for [`missions/cmgr_smoke`](../../missions/cmgr_smoke).

Each case is intended to behave like a small unit test expressed as one mission
run. The harness patches mission parameters and, when needed, patches the
`pMissionEval` rule so each case grades the specific contact-manager behavior
under test.

For the patching mechanics themselves, see [`NSPATCH.md`](./NSPATCH.md).

## Current Matrix

- `detect_baseline_pass`
  Default geometry should see the spoofed contact before the checkpoint.
- `detect_strict_absent_pass`
  Tighter alert thresholds should keep the same spoof unseen.
- `detect_edge_pass`
  Near-threshold geometry should still detect.
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
  A nearer spoof should be detected quickly.
- `closest_contact_pass`
  `CONTACT_CLOSEST` should be `intruder`.
- `count_one_pass`
  `CONTACTS_COUNT` should be exactly `1`.
- `list_intruder_pass`
  `CONTACTS_LIST` should be `intruder`.
- `range_report_pass`
  `CONTACT_CLOSEST_RANGE` should land in a reasonable broad band.
- `range_tight_pass`
  `CONTACT_CLOSEST_RANGE` should land in a tighter band around the baseline.

## Typical Runs

```bash
./zlaunch.sh
./zlaunch.sh --case=detect_baseline_pass 10
./zlaunch.sh --case=count_one_pass 10
./zlaunch.sh --just_make 10
./zlaunch.sh --max_time=50 10
```

## What `./zlaunch.sh` Does Here

When run from this harness directory, `./zlaunch.sh` does not call the stem
mission's own `zlaunch.sh`. Instead it controls the whole run itself:

1. Choose one case or the full default case list.
2. Back up any pre-existing `meta_*.moosx` / `meta_*.bhvx` files in the stem.
3. Remove active overlay files from the previous case.
4. Apply the selected `nspatch` overlays to `missions/cmgr_smoke`.
5. Call shared `xlaunch.sh`, which in turn calls the stem mission's `launch.sh`.
6. Read the mission-local `results.txt`.
7. Compare `actual` against `expected`.
8. Append one summary line to the harness `results.txt`.
9. Restore any original overlay files on exit.

That means the harness owns patching, cleanup, and result comparison.

## Results Lines

Typical detection-style harness line:

```text
case=detect_edge_pass  expected=pass  actual=pass  status=ok  form=cmgr_harness  mmod=detect_edge_pass  grade=pass  eval=true  seen=true  closest=intruder  range=14  mhash=[RUDE-SLAB]
```

Typical reporting-style harness line:

```text
case=count_one_pass  expected=pass  actual=pass  status=ok  form=cmgr_harness  mmod=count_one_pass  grade=pass  eval=true  seen=true  count=1  list=intruder  closest=intruder  range=14  mhash=[PUNY-WHIT]
```

Field anatomy:

- `case`
  The harness case name that was run.
- `expected`
  What the harness expected the mission to grade.
- `actual`
  What the mission actually graded after parsing the mission result line.
- `status`
  `ok` if expected equals actual, otherwise `mismatch`.
- `form`
  Harness or mission family name.
- `mmod`
  The case-specific mission mode written by `pMissionEval`.
- `grade`
  Pass/fail result written by the mission itself.
- `eval`
  Whether the case's evaluation checkpoint was reached.
- `seen`
  Whether `CONTACT_SEEN=true` by the checkpoint.
- `closest`
  The closest reported contact, when that column is included by the case.
- `range`
  The closest reported range, when that column is included by the case.
- `count`
  The alerted-contact count, when that column is included by the case.
- `list`
  The alerted-contact list, when that column is included by the case.
- `mhash`
  Short mission hash.

Not every case reports every optional field. Detection-only cases stay shorter.
Count/list columns are included only when they help explain the specific case.

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
  contact should stay unseen

Why both `actual` and `grade` appear in harness output:

- `grade` is the mission's own result, written by `pMissionEval`
- `actual` is the harness's extracted view of that result, used for comparison
  against `expected`
- today `actual` is usually the same value as `grade`
- they are kept separate because they come from different layers:
  mission result versus harness observation of that result

## Wall-Clock Efficiency

Validated on March 18, 2026:

- full 14-case matrix at warp `10`: about `143` seconds wall clock
- roughly `10` seconds per case on average

The suite exits on `MISSION_EVALUATED`, so the fixed launch and teardown work is
now a larger wall-clock cost than the case logic itself. Lowering `--max_time`
further is not likely to save much unless cases are timing out.
