# cmgr_multi

Fast multi-contact CI stem for `pContactMgrV20`.

- One simulated ownship runs the same short straight transit used by the smoke stem.
- A vehicle-local `uTimerScript` posts two spoofed contacts: `alpha` first and `bravo` second.
- `alpha` is intentionally closer than `bravo` in the default geometry.
- A shoreside `uTimerScript` posts `TEST_EVAL_READY=true` after a short window.
- `pAutoPoke` auto-deploys the vehicle on ordinary `launch.sh` and `zlaunch.sh` runs.
- `pMissionEval` evaluates at the timer checkpoint, not on arrival.
- The default standalone mission passes when both contacts are alerted and `CONTACT_CLOSEST=alpha`.

This stem exists to cover contact-manager behavior that is awkward to express in
the single-contact smoke mission:

- multi-contact counting
- multi-contact list reporting
- closest-contact selection
- delayed second-contact arrival
- stale-contact removal

Typical runs:

```bash
./launch.sh 10
./launch.sh --nogui 10
./zlaunch.sh 10
./zlaunch.sh --gui 10
```

`./zlaunch.sh` defaults to `--nogui`, matching its role as the automation
wrapper for this CI stem. Use `--gui` only when you explicitly want
`pMarineViewer`.

Typical standalone result line:

```text
form=cmgr_multi  mmod=multi_baseline_pass  grade=pass  eval=true  seen=true  closest=alpha  range=8  count=2  list=alpha,bravo  mhash=[EXAM-HASH]
```

Simple field meanings:

- `form`: mission family
- `mmod`: mission mode or case name
- `grade`: pass/fail from `pMissionEval`
- `eval`: whether the checkpoint fired
- `seen`: whether at least one alert was posted by the checkpoint
- `closest`: the closest reported contact name
- `range`: the closest reported contact range
- `count`: the alerted-contact count
- `list`: the known-contact list still tracked by contact manager
- `mhash`: short mission hash

For the current harness built on top of this stem:

- harness: `harnesses/H02-cmgr_multi`
- matrix size: 8 cases
- validated full run on March 18, 2026: about `83` seconds wall clock at warp `10`
