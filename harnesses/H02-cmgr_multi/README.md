# H02-cmgr_multi

Patch-driven harness for [`missions/cmgr_multi`](../../missions/cmgr_multi).

This harness exists for contact-manager behaviors that need more than one
contact in play at the same time. Each case is still meant to behave like a
small unit test expressed as one mission run.

For the patching mechanics themselves, see [`NSPATCH.md`](./NSPATCH.md).

## Current Matrix

- `count_two_pass`
  Both baseline contacts should be alerted by the checkpoint.
- `list_alpha_bravo_pass`
  The known-contact list should be exactly `alpha,bravo`.
- `closest_alpha_pass`
  In the baseline geometry, `alpha` should be the closest contact.
- `closest_bravo_pass`
  After swapping the geometry, `bravo` should be the closest contact.
- `bravo_far_count_one_pass`
  `bravo` should stay out of alert range, leaving one alerted contact.
- `late_bravo_count_two_pass`
  `bravo` arrives later, but both contacts should still be active by a later checkpoint.
- `stale_bravo_drop_pass`
  `bravo` is short-lived and should expire before the later checkpoint.
- `alpha_far_bravo_only_pass`
  `alpha` should stay out of alert range, leaving `bravo` as the only alerted contact.

## Simple Semantics

The important distinction in this stem is:

- `CONTACTS_COUNT`
  Number of alerted contacts.
- `CONTACTS_LIST`
  Known contacts still tracked by contact manager.

That means a one-alert case can still show `list=alpha,bravo` if both contacts
are known but only one is inside the alert rules. Only the stale-expiry case is
expected to shrink the list from two names down to one.

## Typical Runs

```bash
./zlaunch.sh
./zlaunch.sh --case=count_two_pass 10
./zlaunch.sh --case=stale_bravo_drop_pass 10
./zlaunch.sh --just_make 10
./zlaunch.sh --max_time=65 10
```

## What `./zlaunch.sh` Does Here

When run from this harness directory, `./zlaunch.sh` does not call the stem
mission's own `zlaunch.sh`. Instead it:

1. Chooses one case or the full default case list.
2. Backs up any pre-existing `meta_*.moosx` / `meta_*.bhvx` files in the stem.
3. Removes active overlay files from the previous case.
4. Applies the selected `nspatch` overlays to `missions/cmgr_multi`.
5. Calls shared `xlaunch.sh`, which in turn calls the stem mission `launch.sh`.
6. Reads the mission-local `results.txt`.
7. Compares `actual` against `expected`.
8. Appends one summary line to the harness `results.txt`.
9. Restores any original overlay files on exit.

## Results Lines

Typical line:

```text
case=closest_bravo_pass  expected=pass  actual=pass  status=ok  form=cmgr_multi_harness  mmod=closest_bravo_pass  grade=pass  eval=true  count=2  list=alpha,bravo  closest=bravo  range=8  mhash=[COOL-PLAN]
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
- `count`
  Alerted-contact count.
- `list`
  Known-contact list.
- `closest`
  The closest reported contact.
- `range`
  The closest reported range.
- `mhash`
  Short mission hash.

## Expected Outcomes

- every current case is expected to end with mission `grade=pass`
- the harness is comparing expected mission behavior, not expecting some cases
  to mission-fail on purpose
- negative-style multi-contact cases still pass when the mission correctly
  confirms one contact stayed out of alert range or one contact expired

## Wall-Clock Efficiency

Validated on March 18, 2026:

- full 8-case matrix at warp `10`: about `83` seconds wall clock
- roughly `10` seconds per case on average

Like the smoke harness, most of the wall-clock time here is launch and teardown
overhead rather than the mission logic itself.
