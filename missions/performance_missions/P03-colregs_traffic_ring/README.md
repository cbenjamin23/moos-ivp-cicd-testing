# P03-colregs_traffic_ring

Seeded five-vehicle COLREGS traffic-ring mission.

Purpose:
- broader multi-vessel integration pressure than `P02-colregs_joust`
- repeated reassignment of fresh waypoint targets on a ring
- deterministic replay through checked-in seeds and mission modes

Mission modes:
- `baseline_circle_pass`
  - five vehicles
  - equal speeds
  - medium runtime
- `mixed_speed_circle_pass`
  - five vehicles
  - fixed mixed speeds
  - medium runtime
- `endurance_circle_pass`
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

Notes:
- `TRAFFIC_ASSIGN_FAILS` is reported as coordinator health but is not a mission fail on its own.
- `NEAR_MISS_TOTAL` and `closest_range_ever` are reported for analysis, but the primary soak gate is collision-free sustained activity.
- The supported modes are tuned for replayable seeded traffic pressure, not exact deterministic geometry.

Typical runs:

```bash
./launch.sh --mmod=baseline_circle_pass --nogui 10
./launch.sh --mmod=mixed_speed_circle_pass --nogui 10
./launch.sh --mmod=endurance_circle_pass --nogui 10
```
