# P01 Performance Baselines

This harness keeps an explicit performance baseline in
[`performance_baselines.tsv`](/Users/charlesbenjamin/moos-ivp-cicd-testing/harnesses/performance_harnesses/P01-obstacle_gauntlet/performance_baselines.tsv).

The baseline file is intentionally range-based, not exact-match:

- `baseline_field_pass` and `dense_field_pass` are deterministic comparison runs
- `endurance_random_pass` is stochastic and is checked against broader bands

The harness currently compares these result-snapshot metrics:

- `wall_time`
- `cycles`
- `collisions`
- `near_misses`
- `encounters`
- `obavoiding`
- `timeout`

It also scans the vehicle `.alog` `APP_LOG` output and fails the case if it sees
disallowed planner/error text such as:

- `BHV_ERROR`
- `obstacle unavoidable`
- `Obstacle Breached`
- `Unable to init AOF_AvoidObstacleV24`

Important: the metrics in `results.txt` are evaluation-snapshot metrics from
`pMissionEval`, not guaranteed end-of-teardown metrics. That distinction matters
most for the randomized endurance case, where the mission may continue briefly
after `MISSION_EVALUATED=true` before `uMayFinish` and launcher teardown finish.
