# loiter_behavior_motion

Deterministic one-vehicle motion stem for `BHV_Loiter`.

The vehicle starts west of a compact loiter polygon centered near `(12,-121)`.
The default behavior drives into the region, acquires a loiter vertex, and
stabilizes on the polygon. The mission grades on behavior-owned loiter outputs
rather than on an unrelated waypoint-arrival event.

Default evaluation:

- `pAutoPoke` deploys the vehicle and initializes loiter status variables.
- `uTimerScript` sets `TEST_EVAL_READY=true` after the default acquisition
  window.
- `pMissionEval` evaluates only after `TEST_EVAL_READY=true`.

Default pass rule:

- `LOITER_INDEX >= 0`
- `LOITER_ACQUIRE = 0`
- `LOITER_DIST_TO_POLY <= 10`
- `BHV_ERROR_SEEN=false`

The companion harness patches this stem into direction, polygon-shape,
activation, acquisition, runtime-update, output-contract, parameter, and
negative-configuration cases. Most pass cases evaluate after the vehicle has
entered stable loiter. Early acquisition and slow-speed cases intentionally
evaluate while the vehicle is still outside the polygon.

Entry points:

```bash
./launch.sh
./launch.sh --just_make --nogui 10
./zlaunch.sh 10
./zlaunch.sh --case=radial_clockwise_pass --gui 1
./zlaunch.sh --jobs=4 10
```

Companion harness:

- [`/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/loiter_behavior_harnesses/H01-loiter_behavior_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/loiter_behavior_harnesses/H01-loiter_behavior_motion)

The local `zlaunch.sh` keeps the stem mission convenient for default runs, but
forwards named cases and wave runs to the companion harness. In wave mode, the
harness creates isolated mission copies and assigns per-case MOOSDB/pShare
ports so cases can run in parallel without sharing a database.
