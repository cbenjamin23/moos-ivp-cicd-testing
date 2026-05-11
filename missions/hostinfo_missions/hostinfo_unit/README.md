# hostinfo_unit

Headless shoreside mission for app-level `pHostInfo` checks. The stem runs one
MOOS community with `pHostInfo`, `uTimerScript`, `pAutoPoke`, `pMissionEval`,
and `pMissionHash`.

The default scenario forces a deterministic host IP and grades `PHI_HOST_IP`
and `PHI_HOST_PORT_DB`. Harness overlays change host-info configuration and add
exact `PHI_HOST_INFO` payload checks for pShare route reporting.

Run a single mission:

```sh
./launch.sh --nogui 10
```

Run through the mission automation wrapper:

```sh
./zlaunch.sh --max_time=30 --nogui 10
```
