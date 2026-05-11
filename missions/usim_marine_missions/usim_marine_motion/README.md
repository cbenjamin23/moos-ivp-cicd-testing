# uSimMarineV22 Motion Mission

This stem mission runs one `uSimMarineV22` instance in a single MOOS community and lets harness cases patch the simulator, scripted actuator mail, and `pMissionEval` grading block.

Typical commands:

```bash
./launch.sh --nogui 10
../../..//scripts/xlaunch.sh --max_time=45 --shore_mport=30000 --shore_pshare=30010 --nogui 10
```

The mission is intended for `harnesses/usim_marine_harnesses/H01-usim_marine_motion`, where each case grades simulator publications such as `NAV_X`, `NAV_Y`, `NAV_SPEED`, `NAV_HEADING`, `NAV_DEPTH`, `USM_RESET_COUNT`, and `USM_DRIFT_SUMMARY`.
