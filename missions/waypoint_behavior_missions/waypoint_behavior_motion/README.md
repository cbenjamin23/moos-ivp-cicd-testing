# waypoint_behavior_motion

Deterministic one-vehicle motion stem for `BHV_Waypoint`.

The vehicle starts at `(-18,-121)` and the default behavior drives it east to
`(42,-121)`. The waypoint behavior owns the mission verdict through its
completion, waypoint, cycle, and end flags. A station-keep behavior activates
only after `WPT_DONE=true` so manual runs settle instead of continuing to drive.

Default evaluation:

- `pMissionEval` evaluates when `WPT_DONE=true`

Default pass rule:

- `WPT_DONE=true`
- `WAYPOINT_END=true`
- `BHV_ERROR_SEEN=false`

The companion harness patches this stem into multi-point traversal, waypoint
flag, repeat/cycle, runtime update, large capture radius, slow-speed
non-arrival, route-ordering, nonzero current-index, lead-to-start, end-speed,
alternate-speed, reset-on-idle, capture-line, slip-radius, lead-condition,
startup waypoint-flag, custom-output, polygon-input, no-point timeout, and
malformed-update cases. Most pass cases evaluate on waypoint completion. Cases
that intentionally prove non-completion or in-transit output contracts use a
short `TEST_EVAL_READY` timer.

Entry points:

```bash
./launch.sh
./launch.sh --just_make --nogui 10
./zlaunch.sh 10
./zlaunch.sh --case=repeat_forever_cycle_pass --gui 1
./zlaunch.sh --jobs=4 10
```

Companion harness:

- [`/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/waypoint_behavior_harnesses/H01-waypoint_behavior_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/waypoint_behavior_harnesses/H01-waypoint_behavior_motion)

The local `zlaunch.sh` keeps the stem mission convenient for default runs, but
forwards named cases and wave runs to the companion harness. In wave mode, the
harness creates isolated mission copies and assigns per-case MOOSDB/pShare
ports so cases can run in parallel without sharing a database.
