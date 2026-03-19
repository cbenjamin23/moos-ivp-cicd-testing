# moos-ivp-cicd-testing

Small MOOS-IvP CI/CD mission workspace for fast, self-evaluating mission tests.

## What Is Here

- `missions/first_draft`
  Fast obstacle-avoidance CI mission used as a local baseline for `pAutoPoke`, `pMissionEval`, and `uMayFinish`.
- `missions/cmgr_smoke`
  Fast contact-manager stem mission for `pContactMgrV20`.
  The default evaluation is timer-driven and contact-focused rather than arrival-driven.
- `missions/cmgr_multi`
  Fast multi-contact stem mission for `pContactMgrV20`.
  The default evaluation is timer-driven and checks both two-contact counting and closest-contact selection.
- `harnesses/H01-cmgr_smoke`
  `nspatch` harness over `missions/cmgr_smoke` for a 14-case contact-manager matrix.
- `harnesses/H02-cmgr_multi`
  `nspatch` harness over `missions/cmgr_multi` for an 8-case multi-contact matrix.
- `src/`
  Legacy example app/behavior code from the original template that still builds in this repo but is not the main CI mission content.

## Build

```bash
./build.sh
```

## Core Mission Runs

```bash
cd missions/first_draft
./zlaunch.sh 10

cd ../cmgr_smoke
./zlaunch.sh 10
```

## Contact-Manager Harness

```bash
cd harnesses/H01-cmgr_smoke
./zlaunch.sh 10
./zlaunch.sh --case=detect_baseline_pass 10
./zlaunch.sh --just_make 10

cd ../H02-cmgr_multi
./zlaunch.sh 10
./zlaunch.sh --case=count_two_pass 10
```

`./zlaunch.sh` in the harness directory runs the whole contact-manager matrix by
default. Use `--case=<name>` to run one case only.
See [`harnesses/H01-cmgr_smoke/NSPATCH.md`](./harnesses/H01-cmgr_smoke/NSPATCH.md)
for the patching and cleanup details.
See [`harnesses/H02-cmgr_multi/NSPATCH.md`](./harnesses/H02-cmgr_multi/NSPATCH.md)
for the multi-contact patching flow.

## What `zlaunch.sh` Does

In a mission directory such as `missions/cmgr_smoke`, `./zlaunch.sh` now does
one short headless mission run by default:

1. Cleans old local mission artifacts.
2. Launches the mission through the shared MOOS-IvP `xlaunch.sh` flow.
3. Lets `pAutoPoke` deploy the mission.
4. Lets `pMissionEval` decide pass/fail.
5. Lets `uMayFinish` stop the run once `MISSION_EVALUATED=true`.
6. Leaves a single result line in the mission-local `results.txt`.

In the harness directory `harnesses/H01-cmgr_smoke`, `./zlaunch.sh` does more:

1. Chooses one case or the full case list.
2. Backs up any existing `meta_*.moosx` / `meta_*.bhvx` files in the stem.
3. Applies `nspatch` overlays to `missions/cmgr_smoke`.
4. Runs the stem mission once per case through shared `xlaunch.sh`.
5. Reads the mission-local `results.txt`.
6. Compares actual vs expected outcome.
7. Appends one summary line per case to the harness `results.txt`.
8. Restores any original overlay files on exit.

## Results Files

There are stem-local and harness-local `results.txt` files after harness runs:

- `missions/cmgr_smoke/results.txt`
  One line from the most recent mission run.
- `missions/cmgr_multi/results.txt`
  One line from the most recent multi-contact mission run.
- `harnesses/H01-cmgr_smoke/results.txt`
  One line per harness case, including expected vs actual.
- `harnesses/H02-cmgr_multi/results.txt`
  One line per multi-contact harness case, including expected vs actual.

Typical mission result line:

```text
form=cmgr_harness  mmod=detect_edge_pass  grade=pass  eval=true  seen=true  closest=intruder  range=14  mhash=[MILD-ROWE]
```

Field anatomy:

- `form`
  The mission family or harness family name.
- `mmod`
  The specific case or mission variant.
- `grade`
  The outcome decided by `pMissionEval`.
- `eval`
  Whether the timer-driven evaluation checkpoint was reached.
- `seen`
  Whether `pContactMgrV20` posted `CONTACT_SEEN=true` by the checkpoint.
- `closest`
  The contact name reported in `CONTACT_CLOSEST`.
- `range`
  The numeric `CONTACT_CLOSEST_RANGE` at evaluation time.
- `mhash`
  The short mission hash for the generated mission files.

Typical harness summary line:

```text
case=detect_edge_pass  expected=pass  actual=pass  status=ok  form=cmgr_harness  mmod=detect_edge_pass  grade=pass  eval=true  seen=true  closest=intruder  range=14  mhash=[MILD-ROWE]
```

Extra harness-only fields:

- `case`
  The harness case name requested by `zlaunch.sh`.
- `expected`
  What the harness expected the mission to grade.
- `actual`
  What the mission actually graded.
- `status`
  `ok` if expected matches actual, otherwise `mismatch`.

Optional fields such as `count` and `list` appear only on the cases that are
actually testing those published variables.

For the multi-contact stem, note that:

- `count` is the number of alerted contacts
- `list` is the known-contact list still tracked by contact manager

For the current contact-manager matrix, every case is expected to end with
mission `grade=pass`. Negative cases are written that way on purpose: they pass
when the mission correctly confirms non-detection or another expected absence.

`grade` and `actual` are both shown because they come from different layers:

- `grade`
  The mission's own result from `pMissionEval`.
- `actual`
  The harness's extracted view of that mission result for comparison against
  `expected`.

## Notes

- These missions are designed to auto-deploy, self-evaluate, write `results.txt`, and exit cleanly through `uMayFinish`.
- The contact-manager harness uses per-case `pMissionEval` patches, so each case can grade the specific contact-manager behavior under test.
- The current `H01-cmgr_smoke` suite contains 14 cases and validated at about `143` seconds wall clock on March 18, 2026 at warp `10`.
- The current `H02-cmgr_multi` suite contains 8 cases and validated at about `83` seconds wall clock on March 18, 2026 at warp `10`.
- Most of that wall-clock cost is launch and teardown overhead rather than the mission logic itself.
- Generated mission artifacts can be removed with the mission-local `clean.sh` scripts.
