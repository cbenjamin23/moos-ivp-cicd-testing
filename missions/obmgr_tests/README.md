# obmgr_tests

Fast obstacle-manager stem mission for `pObstacleMgr`.

This mission is the shared stem for the obstacle-manager CI suite. Unlike the
older `first_draft` mission, this stem is deliberately unit-test-like: the
ownship stays stationary near the origin and the obstacle-manager behavior is
driven almost entirely by timer-scripted obstacle mail.

## Default Stem Behavior

- One simulated ownship stays effectively stationary at the origin.
- A vehicle-local `uTimerScript` posts an `OBM_ALERT_REQUEST`, then injects one
  near `GIVEN_OBSTACLE`.
- A shoreside `uTimerScript` posts `TEST_EVAL_READY=true` after a short window.
- The vehicle-side test script now self-starts once `pObstacleMgr` reports
  `OBM_CONNECT=true`, so ordinary `launch.sh` runs begin automatically without
  needing a manual deploy click.
- `pAutoPoke` still posts the normal shoreside startup flags used by the
  headless wrappers.
- `pMissionEval` grades at the timer checkpoint, not on waypoint arrival.
- The default standalone mission passes when the given obstacle is accepted and
  `OBM_DIST_TO_OBJ` reports the expected near range.

This keeps the standalone mission useful as a quick sanity check while still
letting the harness patch the same stem into given-obstacle, point-cluster,
general-alert, resolution, and obstacle-modification cases. The harness also
uses this stem for the more niche branches around default general-alert naming,
missing-duration rejection, `post_dist_to_polys=false`, custom point input
names, and point-cluster trimming.

## Why This Stem Is Short

- `pObstacleMgr` is the thing under test
- the ownship position is intentionally simple and stable
- evaluation is timer-driven rather than transit-driven
- obstacle inputs are injected directly instead of relying on a larger mission

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

The harness built on top of this mission, [`harnesses/H02-obmgr_tests`](../../harnesses/H02-obmgr_tests),
may change:

- `pObstacleMgr` parameters such as `alert_range`, `ignore_range`,
  `given_max_duration`, `max_age_per_point`, `post_dist_to_polys`, and `lasso`
- obstacle input family: `GIVEN_OBSTACLE` versus `TRACKED_FEATURE`
- obstacle geometry, timing, and duration
- runtime `OBM_ALERT_REQUEST` mail
- `general_alert` and obstacle-modification variables
- the `pMissionEval` pass/fail rule
- the reporting columns written to `results.txt`

## Typical Result Line

```text
form=obmgr_tests  mmod=given_baseline_pass  grade=pass  eval=true  new_given=true  dist=OB_NEAR,18.0  mindist=18  mhash=[BOLD-ONYX]
```

Simple field meanings:

- `form`: mission family
- `mmod`: mission mode or case name
- `grade`: pass/fail from `pMissionEval`
- `eval`: whether the checkpoint fired
- `new_given`: whether the mail-injected given obstacle was accepted
- `dist`: the current `OBM_DIST_TO_OBJ` publication when included
- `mindist`: `OBM_MIN_DIST_EVER` when included
- `mhash`: short mission hash

## Current Harness On This Stem

- harness: `harnesses/H02-obmgr_tests`
- matrix size: `20` cases
- most recent full harness validation: full matrix passed at warp `10`

Most of the wall-clock cost comes from launch and teardown overhead rather than
the obstacle-manager logic itself.

## Validation Snapshot

Validated on March 19, 2026:

- `./launch.sh --just_make --nogui 10`
- `./zlaunch.sh 10`
- `harnesses/H02-obmgr_tests/zlaunch.sh 10`

The standalone stem and the full harness both passed in those runs.
