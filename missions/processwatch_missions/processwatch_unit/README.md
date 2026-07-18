# processwatch_unit

Headless shoreside mission for app-level `uProcessWatch` checks. The stem runs
one MOOS community with `uProcessWatch`, two small watched peer utilities,
`uTimerScript`, `pAutoPoke`, `pMissionEval`, and `pMissionHash`.

The default scenario grades `PROC_WATCH_ALL_OK=true`. Harness overlays vary
watch lists, custom post names, summary/event publication names, and an expected
missing-process failure.

Run a single mission:

```sh
./launch.sh --nogui 10
```

Run through the mission automation wrapper:

```sh
./zlaunch.sh --max_time=180 --nogui 10
```
