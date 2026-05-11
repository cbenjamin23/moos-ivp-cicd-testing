# loadwatch_unit

Headless shoreside mission for app-level `uLoadWatch` checks. The stem runs one
MOOS community with `uLoadWatch`, `uTimerScript`, `pAutoPoke`, `pMissionEval`,
and `pMissionHash`.

The default scenario keeps all load-watch counters clear. Harness overlays vary
threshold configuration and timed `DB_UPTIME` mail so `pMissionEval` can grade
near-breach and hard-breach behavior directly.

Run a single mission:

```sh
./launch.sh --nogui 10
```

Run through the mission automation wrapper:

```sh
./zlaunch.sh --max_time=30 --nogui 10
```
