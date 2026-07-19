# pdeadmanpost_unit

Headless single-community stem for exercising `pDeadManPost` watchdog posting.
The harness writes case-specific heartbeat schedules, deadman configuration, and
evaluation rules before launching the mission.

The case matrix covers active-at-start behavior, inactive-until-first-heartbeat
behavior, stale-heartbeat posting, live-heartbeat suppression, custom heartbeat
variables, zero-threshold posting, `once`/`repeat`/`reset` post policies with
posting-count checks, invalid policy fallback, numeric/string/false deadflags,
and multiple deadflags.

Direct `zlaunch.sh` runs default to minimal count-evidence logging. Pass
`--log=full` to restore the original diagnostic variable set.

The mission's `pMissionEval` configuration writes the live verdict. Its thin
`zlaunch.sh` forwards the requested ports, mission label, display mode, time
warp, and maximum time to shared `xlaunch.sh`, requires exactly one valid result
row, and applies the repository's scoped-cleanup backstop when run directly.
Publication-count checks remain a harness responsibility because they require
the completed `.alog` file.

Typical harness run:

```sh
../../../harnesses/pdeadmanpost_harnesses/H01-pdeadmanpost_unit/zlaunch.sh --jobs=4 --port_base=15800 10
```

Run one inspectable case:

```sh
../../../harnesses/pdeadmanpost_harnesses/H01-pdeadmanpost_unit/zlaunch.sh --case=active_start_once_posts_pass --port_base=15800 10
```
