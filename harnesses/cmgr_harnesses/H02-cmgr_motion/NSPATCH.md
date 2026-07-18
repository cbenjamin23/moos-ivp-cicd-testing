# NSPATCH in H02-cmgr_motion

This harness uses `nspatch` the same way as the other repo harnesses:

- `meta_vehicle.moos` is the stem encounter mission
- case `.xmoos` files replace whole `ProcessConfig` blocks
- the generated `meta_vehicle.moosx` or `meta_shoreside.moosx` overlay is what
  `nsplug -x` actually uses to build the final `targ_*.moos`

In this moving contact suite, patches mainly do three things:

- replace the vehicle `uTimerScript` block to change contact geometry or timing
- replace the vehicle `pContactMgrV20` block to change alert timing or
  downstream wiring
- replace the shoreside `uTimerScript` or `pMissionEval` block when a case
  needs a different evaluation window or a stricter encounter-count rule

Important practical point:

- every case starts from a separate cleaned copy of the stem, so old sidecars
  cannot leak between serial or rolling cases
- `nspatch` writes sidecars only inside that case's copied mission directory
- each case is launched with `--mmod=<case_name>` so the stem `pMissionEval`
  result line already carries the case name even when no shoreside patch is
  needed
- `baseline_crossing_pass` explicitly uses no patch; every other case maps to
  its historical shoreside and/or vehicle patch in `zlaunch.sh`
