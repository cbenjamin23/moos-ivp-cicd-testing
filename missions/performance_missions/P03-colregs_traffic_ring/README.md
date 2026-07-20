# P03-colregs_traffic_ring

Seeded five-vehicle COLREGS traffic-ring mission.

Purpose:
- broader multi-vessel integration pressure than `P02-colregs_joust`
- repeated reassignment of fresh waypoint targets on a ring
- fixed scenario parameters and RNG seeds for comparable repeated runs

Mission scenarios:
- `baseline_circle`
  - five vehicles
  - equal speeds
  - medium runtime
- `mixed_speed_circle`
  - five vehicles
  - fixed mixed speeds
  - medium runtime
- `endurance_circle`
  - five vehicles
  - fixed mixed speeds
  - longer runtime
  - wider assignment ring and stricter predicted-CPA filtering for CI replay

The mission uses `pTrafficManager` on shoreside to:
- wait for all vehicles to report in
- assign fresh single-point waypoints asynchronously from a seeded RNG
- reject candidate targets whose naive straight-line predicted CPA is too small
- post `WPT_UPDATE_<UP_VNAME>` back to each vehicle

Pass contract:
- `MISSION_DONE = true`
- `MISSION_TIMEOUT = false`
- `COLLISION_TOTAL = 0`
- `UCD_CLOSEST_RANGE_EVER > 0`
- `TRAFFIC_BATCHES >=` the case-specific floor
- the five helm communities report the scenario's exact `TRAFFIC_STOCK_SPEED`
  values
- cooperative scenarios keep every live `AVOID` state true; the noncooperative
  scenario requires only Eve's state to be false

Notes:
- `TRAFFIC_ASSIGN_FAILS` is reported as coordinator health but is not a mission fail on its own.
- `NEAR_MISS_TOTAL` and `closest_range_ever` are reported for analysis, but the primary soak gate is collision-free sustained activity.
- Each helm initializes `TRAFFIC_STOCK_SPEED` from the same `STOCK_SPD` used by
  its waypoint behavior, and the node broker gives both that value and live
  `AVOID` state a vehicle-specific shoreside name for grading.
- The supported scenarios use fixed RNG seeds, but asynchronous vehicle
  completion can change the order in which assignments consume that stream.
  Runs are comparable traffic samples, not exact event-by-event replays.

Typical runs:

```bash
./launch.sh --scenario=baseline_circle --nogui 10
./launch.sh --scenario=mixed_speed_circle --nogui 10
./launch.sh --scenario=endurance_circle --nogui 10
./zlaunch.sh --log=full --scenario=endurance_circle --nogui 10
```

Direct launches default to `--log=minimal`, with only vehicle `APP_LOG`
evidence retained for performance warning scans. `--log=full` restores the
original shoreside and vehicle wildcard logging.
