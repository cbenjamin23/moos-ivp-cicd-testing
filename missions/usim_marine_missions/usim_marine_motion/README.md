# uSimMarineV22 Motion Mission

This stem mission runs one `uSimMarineV22` instance in a single MOOS community and lets harness cases patch the simulator, scripted actuator mail, and `pMissionEval` grading block.

Typical commands:

```bash
./launch.sh --nogui 10
./zlaunch.sh 10
./zlaunch.sh --just_make --shore_mport=30000 --shore_pshare=30015 10
```

The mission is intended for `harnesses/usim_marine_harnesses/H01-usim_marine_motion`, where each case grades simulator publications such as `NAV_X`, `NAV_Y`, `NAV_SPEED`, `NAV_HEADING`, `NAV_DEPTH`, `USM_RESET_COUNT`, and `USM_DRIFT_SUMMARY`.

`zlaunch.sh` is the self-evaluating wrapper used by the harness. It forwards
time warp, `--max_time`, display mode, `--mmod`, an explicit shoreside MOOSDB
port, and a reserved shoreside pShare port to `xlaunch.sh`. In `--just_make`
mode it requires `targ_shoreside.moos`; after a live run it requires exactly
one `pMissionEval` row with exactly one valid grade. It preserves a nonzero
launch status and performs canonical root-scoped teardown.

This is a single-community mission. It launches one MOOSDB and no pShare
application, so the pShare argument is reserved for launcher-interface
consistency rather than consumed by the target mission. The low-level
`launch.sh` remains available for interactive mission work; the harness calls
`zlaunch.sh` for bounded, result-checked execution.
