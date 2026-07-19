# cutrange_behavior_motion

Two-vehicle motion stem for `BHV_CutRange`. The default mission launches `abe`
as the chaser and `ben` as the target on an eastbound leg. `abe` runs
`BHV_CutRange`, receives contact data for `ben` through `pContactMgrV20`, and is
graded by shoreside `pMissionEval`.

The default evaluation waits until `TEST_EVAL_READY=true`, then requires:
`CUTRANGE_PURSUIT=true`, `CUTRANGE_GIVEUP=false`, closest-ever vehicle range at
or below 50 meters, the target still moving, and no settled chaser-side behavior
warning or error. Additional
harness cases patch the stem for geometry, parameter, runtime-update, giveup,
and invalid-config scenarios.

## Running

```bash
./zlaunch.sh 10
./zlaunch.sh --case=static_cutrange_pass 10
./zlaunch.sh --case=headon_cutrange_pass --gui 1
./zlaunch.sh --case=runtime_giveup_update_pass --gui 1
./zlaunch.sh --jobs=4 --port_base=25000 10
```

Logging is minimal by default. Add `--log=full` to restore the original
shoreside and two-vehicle `pLogger` configuration for diagnostic runs.

Named cases are forwarded to
[`harnesses/cutrange_behavior_harnesses/H01-cutrange_behavior_motion`](../../../harnesses/cutrange_behavior_harnesses/H01-cutrange_behavior_motion).
