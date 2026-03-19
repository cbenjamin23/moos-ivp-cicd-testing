# cmgr_smoke

Fast CI stem for `pContactMgrV20`.

- One simulated ownship runs a short straight transit.
- A vehicle-local `uTimerScript` posts a `SPOOF` event to `pSpoofNode`.
- A shoreside `uTimerScript` posts `TEST_EVAL_READY=true` after a short window.
- `pAutoPoke` auto-deploys the vehicle on ordinary `launch.sh` and `zlaunch.sh` runs.
- `pMissionEval` evaluates at the timer checkpoint, not on arrival.
- The default standalone mission passes when `CONTACT_SEEN=true` by the evaluation checkpoint.

Why this mission is short:

- it is meant to behave like a small unit-style mission test
- the contact-manager behavior is the thing under test
- arrival is no longer the primary grading trigger
- the transit only exists to give the vehicle a simple, repeatable geometry

Typical runs:

```bash
./launch.sh 10
./launch.sh --nogui 10
./zlaunch.sh 10
./zlaunch.sh --gui 10
```

`./zlaunch.sh` now defaults to `--nogui`, which matches its role as the
automation wrapper for this CI stem. Use `--gui` only when you explicitly want
`pMarineViewer`.

This mission is intended to be the fast stem for patch-driven contact-manager
tests. Case-specific harness patches may change:

- `pContactMgrV20` alert parameters
- spoof timing and geometry
- the `pMissionEval` pass/fail rule itself

Typical standalone result line:

```text
form=cmgr_smoke  mmod=detect_baseline_pass  grade=pass  eval=true  seen=true  closest=intruder  range=14  mhash=[INKY-RUST]
```

Simple field meanings:

- `form`: mission family
- `mmod`: mission mode or case name
- `grade`: pass/fail from `pMissionEval`
- `eval`: whether the checkpoint fired
- `seen`: whether contact detection happened by the checkpoint
- `closest`: the closest reported contact name
- `range`: the closest reported contact range
- `mhash`: short mission hash

For the current harness built on top of this stem:

- harness: `harnesses/H01-cmgr_smoke`
- matrix size: 14 cases
- validated full run on March 18, 2026: about `143` seconds wall clock at warp `10`
