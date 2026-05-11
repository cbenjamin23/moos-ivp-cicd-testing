# ufld_obstacle_sim_unit

Headless shoreside mission for app-level `uFldObstacleSim` checks. The stem
runs one MOOS community with `uFldObstacleSim`, scripted input mail,
`pMissionEval`, `pMissionHash`, and logging.

The default scenario loads a small obstacle file and uses scripted
`PMV_CONNECT` mail to trigger publication. Harness overlays change obstacle
files and simulator configuration to isolate source-side behavior: ground-truth
obstacle output, vehicle-facing obstacle output, visual toggles, point-mode
sensor simulation, runtime point-size updates, and reset publication.

Run a single mission:

```sh
./launch.sh --nogui 10
```

Run through the mission automation wrapper:

```sh
./zlaunch.sh --max_time=30 --nogui 10
```
