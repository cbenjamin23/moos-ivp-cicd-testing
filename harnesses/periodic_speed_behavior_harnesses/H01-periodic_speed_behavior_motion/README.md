# H01-periodic_speed_behavior_motion

Harness for `BHV_PeriodicSpeed` using the stem mission at
`missions/periodic_speed_behavior_missions/periodic_speed_behavior_motion`.

The stem keeps a single simulated vehicle on a constant heading while
`BHV_PeriodicSpeed` alternates between lazy and busy speed windows. The mission
grades behavior-owned status variables and observed vehicle speed.

The drawn track segment is only a visual reference. These cases evaluate when
`uTimerScript` posts `MISSION_DONE`; they do not wait for the vehicle to reach
the end of the line.

## Current Matrix

- `baseline_cycle_pass`: Uses `period_lazy=4`, `period_busy=4`, and `period_speed=1.4`; at the scheduled 18.5-second sample, passes when at least two busy transitions have been counted and `NAV_SPEED>0.4`.
- `lazy_wait_pass`: Samples the stock four-second initial lazy window at 3.5 seconds; passes when `PS_BUSY_COUNT=0`, pending-active time is positive, pending-inactive time is zero, and `NAV_SPEED<0.3`.
- `first_busy_pass`: Samples the stock first busy window at seven seconds; passes when `PS_BUSY_COUNT=1`, pending-inactive time is positive, and `NAV_SPEED>0.4`.
- `busy_to_lazy_pass`: Sets `initially_busy=true`, `reset_upon_running=false`, `period_busy=2`, `period_lazy=6`, and speed 1.2, then samples at 5.5 seconds; passes when the initial busy state has returned to lazy with busy count zero, pending-active time at least three seconds, and pending-inactive time zero.
- `deprecated_zaic_aliases_pass`: Uses `period_lazy=2`, `period_busy=8`, `period_speed=1.5`, plus deprecated `period_basewidth=0.2`, `zaic_peakwidth=0`, and `zaic_summit_delta=25`; passes at 6.5 seconds when busy count is one, pending-inactive time is positive, and speed exceeds 0.4.
- `short_cycle_three_count_pass`: Sets both periods to 1.5 seconds and speed to 1.3; passes at 12.5 seconds when at least three busy transitions have occurred and speed exceeds 0.4.
- `explicit_initially_lazy_pass`: Explicitly sets `initially_busy=false` on the stock four-second cycle; passes at 3.5 seconds when busy count is zero, pending-active time is positive, pending-inactive time is zero, and speed is below 0.3.
- `zaic_base_period_peak_aliases_pass`: Uses `period_lazy=2`, `period_busy=8`, `period_speed=1.5`, `zaic_basewidth=0.2`, `period_peakwidth=0`, and `summit_delta=25`; passes at 6.5 seconds when busy count is one, pending-inactive time is positive, and speed exceeds 0.4.
- `reset_on_reactivation_pass`: With default reset behavior, disables `RUN_PS_ALL` at one second and re-enables it at 5.5 seconds; passes at 7.2 seconds when the restarted lazy window still has busy count zero, pending-active time above one second, pending-inactive time zero, and speed below 0.3.
- `reset_false_continuous_pass`: Sets `reset_upon_running=false`, `period_lazy=8`, and `period_busy=10`, disables the behavior from three through 12 seconds, then samples at 15 seconds; passes when the periodic clock advanced through the inactive interval far enough to produce at least one busy transition and speed above 0.4.
- `initially_busy_pass`: Sets `initially_busy=true`, `reset_upon_running=false`, five-second periods, and speed 1.6; passes at 3.5 seconds when the initial busy window has busy count zero, pending-inactive time is positive, and speed exceeds 0.4.
- `zero_speed_pass`: Starts busy with `reset_upon_running=false`, five-second periods, and `period_speed=0`; passes at 3.5 seconds when busy count remains zero, pending-inactive time is positive, and speed is below 0.3.
- `bad_period_lazy_fail`: Sets `period_lazy=0`; this expected-negative case passes when `IVPHELM_STATE` reports `HELM_MALCONFIG=true`.
- `bad_period_lazy_nonnumeric_fail`: Sets `period_lazy=soon`; this expected-negative case passes when helm malconfiguration is reported.
- `bad_period_busy_fail`: Sets `period_busy=0`; this expected-negative case passes when helm malconfiguration is reported.
- `bad_period_speed_fail`: Sets `period_speed=-1`; this expected-negative case passes when helm malconfiguration is reported.
- `bad_initially_busy_fail`: Sets `initially_busy=maybe`; this expected-negative case passes when helm malconfiguration is reported.
- `bad_reset_upon_running_fail`: Sets `reset_upon_running=maybe`; this expected-negative case passes when helm malconfiguration is reported.
- `bad_basewidth_fail`: Sets `basewidth=-0.1`; this expected-negative case passes when helm malconfiguration is reported.
- `bad_zaic_basewidth_fail`: Sets `zaic_basewidth=-0.1`; this expected-negative case passes when helm malconfiguration is reported.
- `bad_peakwidth_fail`: Sets `peakwidth=-0.1`; this expected-negative case passes when helm malconfiguration is reported.
- `bad_period_peakwidth_fail`: Sets `period_peakwidth=-0.1`; this expected-negative case passes when helm malconfiguration is reported.
- `bad_summit_delta_fail`: Sets `summit_delta=-1`; this expected-negative case passes when helm malconfiguration is reported.
- `bad_zaic_summit_delta_fail`: Sets `zaic_summit_delta=-1`; this expected-negative case passes when helm malconfiguration is reported.

## Manual Inspection

- `reset_false_visual_pass`: Manual-only `reset_upon_running=false` case with 14-second lazy and 16-second busy periods; disables `RUN_PS_ALL` from five through 20 seconds and passes at 27 seconds when at least one busy transition occurred and speed exceeds 0.4.

## Manual Commands

```bash
./zlaunch.sh --case=baseline_cycle_pass --port_base=15000 10
./zlaunch.sh --case=initially_busy_pass --gui --port_base=15000 10
./zlaunch.sh --case=reset_false_visual_pass --gui --port_base=15000 1
./zlaunch.sh --jobs=4 --port_base=34000 --port_stride=12 10
```

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for a fully logged diagnostic case.
