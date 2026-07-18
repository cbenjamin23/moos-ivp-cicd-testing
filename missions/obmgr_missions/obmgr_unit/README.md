# obmgr_tests

Fast obstacle-manager stem mission for `pObstacleMgr`.

This mission is the source stem for the obstacle-manager CI suite. Unlike the
older `first_draft` mission, this stem is deliberately unit-test-like: the
ownship stays stationary in the lower band of the map and the obstacle-manager
behavior is driven almost entirely by timer-scripted obstacle mail.

## Default Stem Behavior

- One simulated ownship stays effectively stationary at `(0,-60)`.
- A vehicle-local `uTimerScript` posts an `OBM_ALERT_REQUEST`, then injects one
  near `GIVEN_OBSTACLE`.
- A shoreside `uTimerScript` posts `TEST_EVAL_READY=true` after a short window.
- The vehicle-side test script now self-starts once `pObstacleMgr` reports
  `OBM_CONNECT=true`, so ordinary `launch.sh` runs begin automatically without
  needing a manual deploy click.
- `pAutoPoke` still posts the normal shoreside startup flags used by the
  headless wrappers.
- The shoreside viewer uses `MIT_SP.tif` with `set_pan_x=0`, `set_pan_y=-300`,
  and `zoom=1.0`.
- `pMissionEval` grades at the timer checkpoint, not on waypoint arrival.
- The default standalone mission passes when the given obstacle is accepted and
  `OBM_DIST_TO_OBJ` reports the expected near range.

This keeps the standalone mission useful as a quick sanity check while still
letting the harness patch isolated copies into given-obstacle, point-cluster,
general-alert, resolution, and obstacle-modification cases. The harness also
uses copies of this stem for the more niche branches around default
general-alert naming, missing-duration rejection, `post_dist_to_polys=false`,
custom point input names, and point-cluster trimming.

## Why This Stem Is Short

- `pObstacleMgr` is the thing under test
- the ownship position is intentionally simple and stable
- evaluation is timer-driven rather than transit-driven
- obstacle inputs are injected directly instead of relying on a larger mission

## Typical Runs

```bash
./launch.sh 10
./launch.sh --log=full 10
./launch.sh --nogui 10
./zlaunch.sh 10
./zlaunch.sh --log=full 10
./zlaunch.sh --gui 10
./zlaunch.sh --shore_mport=22000 --veh_mport=22001 \
  --shore_pshare=22015 --veh_pshare=22016 10
./zlaunch.sh --just_make --mmod=given_baseline_pass 10
```

`./zlaunch.sh` defaults to `--nogui`, which matches its role as the automation
wrapper for this CI stem. Use `--gui` only when you explicitly want to inspect
the run in `pMarineViewer`. Logging defaults to `minimal` with no active
`pLogger`; `--log=full` restores the previous shoreside and vehicle logger
configuration from stem-local patches.

## Evaluation-Wrapper Contract

The stem's `zlaunch.sh` is intentionally thin. It forwards `--max_time`,
`--mmod`, display mode, time warp, and all four explicit port arguments through
`xlaunch.sh` to the ordinary launch path:

- `--shore_mport`
- `--veh_mport`
- `--shore_pshare`
- `--veh_pshare`

After a live run, it requires exactly one nonempty `pMissionEval` result row
containing exactly one `grade=pass|fail` field. It does not manufacture or
reinterpret a grade. On exit it calls the canonical repository teardown helper
against only the current mission root.

`--just_make` instead verifies that `targ_shoreside.moos`, `targ_abe.moos`, and
`targ_abe.bhv` were generated. The harness uses that path to inspect patches
and forwarded ports without launching MOOS processes.

## What The Harness Patches

The harness built on top of this mission, [`harnesses/obmgr_harnesses/H01-obmgr_unit`](../../../harnesses/obmgr_harnesses/H01-obmgr_unit),
copies this stem once per selected case and may change inside each copy:

- `pObstacleMgr` parameters such as `alert_range`, `ignore_range`,
  `given_max_duration`, `max_age_per_point`, `post_dist_to_polys`, and `lasso`
- obstacle input family: `GIVEN_OBSTACLE` versus `TRACKED_FEATURE`
- obstacle geometry, timing, and duration
- runtime `OBM_ALERT_REQUEST` mail
- `general_alert` and obstacle-modification variables
- the `pMissionEval` pass/fail rule
- the reporting columns written to `results.txt`

Four payload-format cases use the existing shoreside `uTimerScript` to publish
one reused runtime expectation variable, `OBM_EXPECTED_PAYLOAD`, for comparison
by `pMissionEval`. This accommodates structured strings containing their own
`=` characters without adding another app or moving the verdict into shell
code.

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

- harness: `harnesses/obmgr_harnesses/H01-obmgr_unit`
- matrix size: `30` cases
- execution: isolated stem copy and distinct `+0/+1/+15/+16` ports for every
  case, including serial runs
- scheduling: Bash 5.1+ rolling `--jobs`, with final rows restored to
  selected-case order
- grading: exactly one mission-owned `pMissionEval` row per ordinary case;
  harness-owned rows only normalize runner failures
- most recent migration validation: serial and rolling full matrices passed
  `30/30` at warp `10`

Most of the wall-clock cost comes from launch and teardown overhead rather than
the obstacle-manager logic itself. Detailed legacy and migrated timings are
kept in the harness README.

## Validation Snapshot

Validated during the July 14, 2026 harness migration:

- the copied stem generated all expected targets with explicit forwarded ports
- the copied stem passed alone and wrote one valid `pMissionEval` row
- all 30 harness copies generated their expected sidecars and targets
- the full isolated serial matrix passed `30/30`
- repeated rolling `--jobs=2` matrices passed `30/30`

The wrapper and harness static checks also passed. These checks validate the
migration form and the existing case behavior; they do not claim a redesign or
expansion of case coverage.
