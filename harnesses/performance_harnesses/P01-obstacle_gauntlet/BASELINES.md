# P01 Performance Checks

P01 no longer keeps a separate `performance_baselines.tsv` file.

Current split:

- `pMissionEval` owns the MOOS-visible case verdict
- shell owns only:
  - `wall_time` bands
  - disallowed warning text in the newest vehicle `.alog`

The mission-side checks now cover:

- `cycles`
- `collisions`
- `near_misses`
- `encounters`
- `obavoiding`
- `timeout`

Shell still checks:

- `wall_time`
- warning-count scans for:
  - `BHV_ERROR`
  - `obstacle unavoidable`
  - `Obstacle Breached`
  - `Unable to init AOF_AvoidObstacleV24`

Important: the metrics in `results.txt` are evaluation-snapshot metrics from
`pMissionEval`, not guaranteed end-of-teardown metrics. That distinction matters
most for the randomized endurance case, where the mission may continue briefly
after `MISSION_EVALUATED=true` before `uMayFinish` and launcher teardown finish.
