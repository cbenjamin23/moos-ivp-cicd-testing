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

- `baseline_cycle_pass`
  Baseline lazy/busy cycle. Requires two busy transitions and positive vehicle
  speed during the second busy window.
- `lazy_wait_pass`
  Samples the initial lazy window. Requires no busy transition, positive
  pending-active time, zero pending-inactive time, and near-zero speed.
- `first_busy_pass`
  Samples the first busy window. Requires one busy transition, positive
  pending-inactive time, and positive vehicle speed.
- `busy_to_lazy_pass`
  Starts busy and samples after the initial busy window returns to lazy.
  Requires pending-active evidence without depending on vehicle deceleration
  dynamics.
- `deprecated_zaic_aliases_pass`
  Uses deprecated ZAIC alias parameters and requires the behavior to enter busy
  mode with positive vehicle speed.
- `short_cycle_three_count_pass`
  Uses short lazy/busy periods and requires at least three busy transitions plus
  speed evidence in the third busy window.
- `explicit_initially_lazy_pass`
  Sets `initially_busy=false` explicitly and verifies the behavior starts in the
  lazy window with pending-active evidence and near-zero speed.
- `zaic_base_period_peak_aliases_pass`
  Uses the `zaic_basewidth` and `period_peakwidth` aliases and requires the
  behavior to enter busy mode with positive vehicle speed.
- `reset_on_reactivation_pass`
  Temporarily disables the behavior condition and verifies the default
  `reset_upon_running=true` policy restarts the lazy timer on reactivation.
- `reset_false_continuous_pass`
  Temporarily disables the behavior condition with `reset_upon_running=false`
  and verifies idle time continues to advance the periodic state.
- `initially_busy_pass`
  Starts the behavior in the busy state and requires immediate speed evidence
  while the busy counter remains at zero.
- `zero_speed_pass`
  Starts busy with `period_speed=0`, proving zero is accepted and the vehicle
  remains effectively stopped while the behavior is active.
- `bad_period_lazy_fail`
  Expected negative for `period_lazy=0`. Passes only when the helm reports
  `HELM_MALCONFIG=true`.
- `bad_period_lazy_nonnumeric_fail`
  Expected negative for a nonnumeric `period_lazy` value. Passes only when the
  helm reports `HELM_MALCONFIG=true`.
- `bad_period_busy_fail`
  Expected negative for `period_busy=0`. Passes only when the helm reports
  `HELM_MALCONFIG=true`.
- `bad_period_speed_fail`
  Expected negative for negative `period_speed`. Passes only when the helm
  reports `HELM_MALCONFIG=true`.
- `bad_initially_busy_fail`
  Expected negative for a non-boolean `initially_busy` value. Passes only when
  the helm reports `HELM_MALCONFIG=true`.
- `bad_reset_upon_running_fail`
  Expected negative for a non-boolean `reset_upon_running` value. Passes only
  when the helm reports `HELM_MALCONFIG=true`.
- `bad_basewidth_fail`
  Expected negative for negative `basewidth`. Passes only when the helm reports
  `HELM_MALCONFIG=true`.
- `bad_zaic_basewidth_fail`
  Expected negative for negative `zaic_basewidth`. Passes only when the helm
  reports `HELM_MALCONFIG=true`.
- `bad_peakwidth_fail`
  Expected negative for negative `peakwidth`. Passes only when the helm reports
  `HELM_MALCONFIG=true`.
- `bad_period_peakwidth_fail`
  Expected negative for negative `period_peakwidth`. Passes only when the helm
  reports `HELM_MALCONFIG=true`.
- `bad_summit_delta_fail`
  Expected negative for negative `summit_delta`. Passes only when the helm
  reports `HELM_MALCONFIG=true`.
- `bad_zaic_summit_delta_fail`
  Expected negative for negative `zaic_summit_delta`. Passes only when the helm
  reports `HELM_MALCONFIG=true`.

## Manual Inspection

- `reset_false_visual_pass`
  Slower visual version of the reset-false case. `RUN_PS_ALL` is turned off at
  5 seconds, back on at 20 seconds, and the mission evaluates at 27 seconds.
  The case is meant for `--gui` runs and is not part of the default CI matrix.

## Manual Commands

```bash
./zlaunch.sh --case=baseline_cycle_pass --port_base=15000 10
./zlaunch.sh --case=initially_busy_pass --gui --port_base=15000 10
./zlaunch.sh --case=reset_false_visual_pass --gui --port_base=15000 1
./zlaunch.sh --jobs=4 --port_base=34000 --port_stride=12 10
```
