# processwatch_unit

Headless shoreside mission for app-level `uProcessWatch` checks. The stem runs
one MOOS community with `uProcessWatch`, two small watched peer utilities,
`pEchoVar`, `uTimerScript`, `pMissionEval`, and `pMissionHash`.

The default scenario evaluates when `PROC_WATCH_ALL_OK=true` and requires a
nonempty short summary before a six-second fallback deadline. Harness overlays
vary watch lists, custom post names, summary/event publication names, and an
expected missing-process outcome. `pEchoVar` reduces exact full-summary
`name(here/gone)` entries to scalar evaluator inputs.

Run a single mission:

```sh
./launch.sh --nogui 10
```

Run through the mission automation wrapper:

```sh
./zlaunch.sh --max_time=180 --nogui 10
```
Logging is minimal by default and does not launch `pLogger`. Add
`--log=full` to either launcher to restore the original shoreside logger.
