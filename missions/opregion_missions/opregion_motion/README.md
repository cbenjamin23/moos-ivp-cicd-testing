# opregion_motion

Logging is minimal by default in both communities and launches no `pLogger`.
Add `--log=full` to either launcher to restore both original loggers.

Deterministic moving safety-envelope stem mission for `BHV_OpRegionV24`.

This mission keeps the same one-vehicle MIT_SP framing as the other
motion-correctness stems. The vehicle starts at `(-18,-121)`, crosses the
centered region toward `(42,-121)`, and returns to `(12,-121)` before the default
pass evaluation. `BHV_OpRegionV24` runs alongside the nominal waypoint behavior
and owns the safety verdict through its save, halt, and time-limit flags.

The default core polygon is centered at `(12,-121)` with bounds
`{-23,-152:47,-152:47,-90:-23,-90}`. Shoreside also posts
long-duration `VIEW_POLYGON` copies of the core and halt regions so the viewer
keeps the safety envelope visible throughout headless or manual runs.
The fixed core and halt-envelope boxes are drawn in white; red is reserved for
halt-status breach markers.

Default evaluation:

- `pMissionEval` evaluates when `ARRIVED=true`

Default pass rule:

- `ARRIVED=true`
- `SAVE_REGION_BREACHED=false`
- `HALT_REGION_BREACHED=false`
- `TIME_LIMIT_BREACHED=false`
- `BHV_ERROR_SEEN=false`

The companion harness patches the same stem into save-region recovery,
halt-region breach, exit-debounce, max-time, dynamic-region update, and
dynamic-halt cases. Cases with a natural terminal event evaluate on arrival or
on the expected breach flag; the debounce/reset cases keep a short timed gate
because they test absence of a breach after a timing window.

Entry points:

```bash
./launch.sh
./launch.sh --just_make --nogui 10
./zlaunch.sh 10
./zlaunch.sh --case=trigger_exit_debounce_pass --gui 1
./zlaunch.sh --jobs=4 10
```

Companion harness:

- [`harnesses/opregion_harnesses/H01-opregion_safety`](../../../harnesses/opregion_harnesses/H01-opregion_safety)

The local `zlaunch.sh` keeps the stem mission convenient for default runs, but
forwards named cases and wave runs to the companion harness. In wave mode, the
harness creates isolated mission copies and assigns per-case MOOSDB/pShare
ports so cases can run in parallel without sharing a database.
