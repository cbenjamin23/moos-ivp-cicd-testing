# cmgr_tests

Fast contact-manager stem mission for `pContactMgrV20`.

This mission is the one shared stem for the contact-manager CI suite. It is
kept intentionally small and deterministic, and the harness patches this same
stem into both single-contact and multi-contact cases, plus a few report-only
branches such as `CONTACT_RANGES`, `CONTACT_CLOSEST_RELBNG`, `BHV_ABLE_FILTER`,
and the early-warning flag path.

## Default Stem Behavior

- One simulated ownship runs a short straight transit.
- The ownship starts at `(0,-60)` and the goal waypoint is `(60,-60)`.
- A vehicle-local `uTimerScript` posts one spoofed contact through `pSpoofNode`.
- A shoreside `uTimerScript` posts `TEST_EVAL_READY=true` after a short window.
- `pAutoPoke` auto-deploys the vehicle on ordinary `launch.sh` and `zlaunch.sh`
  runs.
- `pMissionEval` grades at the timer checkpoint, not on arrival.
- The default standalone mission passes when `CONTACT_SEEN=true` by the
  evaluation checkpoint.
- The shoreside viewer uses `MIT_SP.tif` with `set_pan_x=0`, `set_pan_y=-300`,
  and `zoom=1.0`.

This keeps the standalone mission useful as a very fast sanity check while
still letting the harness build richer cases from the same stem.

## Why This Stem Is Short

- the contact-manager behavior is the thing under test
- the evaluation is driven directly by a timer checkpoint
- arrival is not the main grading trigger
- the transit exists only to give simple, repeatable geometry

## Typical Runs

```bash
./launch.sh 10
./launch.sh --nogui 10
./zlaunch.sh 10
./zlaunch.sh --gui 10
```

`./zlaunch.sh` defaults to `--nogui`, which matches its role as the automation
wrapper for this CI stem. Use `--gui` only when you explicitly want to inspect
the run in `pMarineViewer`.

## What The Harness Patches

The harness built on top of this mission, [`harnesses/cmgr_harnesses/H01-cmgr_unit`](../../../harnesses/cmgr_harnesses/H01-cmgr_unit),
may change:

- `pContactMgrV20` parameters such as `alert_range`, `cpa_range`, `contact_max_age`,
  `reject_range`, or `decay`
- spoof timing and spoof geometry
- the number of spoofed contacts
- runtime `BCM_ALERT_REQUEST` and `BCM_REPORT_REQUEST` mail
- alert-filter configuration such as type matching
- the `pMissionEval` pass/fail rule
- the reporting columns written to `results.txt`

That is how the same stem supports both the single-contact and multi-contact
test families.

## Typical Result Line

```text
form=cmgr_tests  mmod=detect_baseline_pass  grade=pass  eval=true  seen=true  closest=intruder  range=14  mhash=[INKY-RUST]
```

Simple field meanings:

- `form`: mission family
- `mmod`: mission mode or case name
- `grade`: pass/fail from `pMissionEval`
- `eval`: whether the checkpoint fired
- `seen`: whether contact detection happened by the checkpoint
- `closest`: closest reported contact name, when reported
- `range`: closest reported contact range, when reported
- `ranges`: full `CONTACT_RANGES` list, when reported
- `relbng`: closest-contact relative bearing, when reported
- `filter`: contact enable/disable requests, when reported
- `warn`: early-warning flag, when reported
- `mhash`: short mission hash

## Current Harness On This Stem

- harness: `harnesses/cmgr_harnesses/H01-cmgr_unit`
- matrix size: `33` cases
- validation on March 19, 2026: full matrix passed at warp `10`

Most of that wall-clock time is launch and teardown overhead rather than mission
logic.
