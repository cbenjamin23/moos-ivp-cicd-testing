# H01-timer_behavior_motion

Patch-driven moving correctness harness for
`missions/timer_behavior_missions/timer_behavior_motion`.

This harness focuses on `BHV_Timer` as the behavior under test. The stem
mission runs one simulated vehicle on a short waypoint leg while the timer
behavior is held idle for a few seconds and then allowed to run. The verdict is
owned by `pMissionEval` and uses timer-posted duration counters:

- idle duration before `TIMER_ACTIVE=true`
- running duration after `TIMER_ACTIVE=true`
- configured status variable names
- configured status suffix behavior
- clean behavior state through `BHV_ERROR_SEEN=false`

## Current Matrix

- `baseline_idle_running_pass` Baseline case. The timer starts idle, then activates and must post both stock suffixed duration counters with sane elapsed values.
- `custom_status_vars_pass` Custom variable case. The timer uses custom idle and running variable names plus a suffix, and those custom counters must be populated.
- `default_suffix_pass` Default variable case. The timer keeps the stock `TIMER_IDLE` and `TIMER_RUNNING` names while using a pre-underscored suffix.
- `custom_no_suffix_pass` Unsuffixed custom-variable case. The timer uses custom idle and running variable names with no suffix, and the raw custom counters must be populated.
- `default_no_suffix_pass` Unsuffixed default-variable case. The timer uses the stock `TIMER_IDLE` and `TIMER_RUNNING` outputs with no suffix.
- `active_at_start_pass` Active-at-start timing case. `TIMER_ACTIVE` is asserted almost immediately after deploy, so the running counter should dominate by evaluation time.
- `delayed_activation_pass` Delayed activation case. The timer is held idle longer than the baseline before activation, so the idle counter must dominate while still proving nonzero running time.
- `pause_resume_pass` Pause/resume case. `TIMER_ACTIVE` is toggled true, false, then true again, and the final counters must show accumulated idle and running time across both intervals.
- `repeated_status_param_pass` Repeated-parameter case. Earlier status variable and suffix settings are replaced by later valid settings, and the final stock suffixed counters must be populated.
- `post_mapping_pass` Post-mapping case. The timer posts stock suffixed counter names, inherited `post_mapping` remaps them, and evaluation requires the remapped counters.
- `duration_complete_pass` Duration-complete case. The timer uses inherited duration support, posts an end flag, and stops with a bounded running counter and zero remaining time.
- `runtime_update_pass` Runtime update case. The timer starts with stock suffixed counters, then receives `TIMER_UPDATES` that change both status variable names and suffix; both pre-update and post-update counters must be present.
- `never_active_fail` Never-active case. The timer remains idle through evaluation, and the case passes when the idle counter grows while the running counter stays zero.
- `bad_idle_var_fail` Invalid idle-variable case. The timer is configured with a whitespace-bearing idle status variable, and the case passes when no stock status counters are emitted.
- `bad_running_var_fail` Invalid running-variable case. The timer is configured with a whitespace-bearing running status variable, and the case passes when no stock status counters are emitted.
- `bad_suffix_fail` Invalid suffix case. The timer is configured with a whitespace-bearing suffix, and the case passes when no stock status counters are emitted.
- `post_mapping_silent_fail` Silent post-mapping case. The running counter is remapped to `silent`, and the case passes when the idle counter is present while the stock running counter is absent.
- `unknown_param_fail` Unknown-parameter case. The timer receives an unsupported behavior parameter, and the case passes when no stock status counters are emitted.

## Running

```bash
./zlaunch.sh
./zlaunch.sh --case=custom_status_vars_pass 10
./zlaunch.sh --jobs=4 --port_base=15000 10
./zlaunch.sh --jobs=4 --port_base=29700 --port_stride=15 10
./zlaunch.sh --just_make --jobs=4 --port_base=15000 10
```

The default `--port_base` is `15000` to avoid the repository's older 9000-range
defaults during active development. Grouped runs use 30-port case blocks from
that base so each live case gets isolated MOOSDB and pShare ports. A smaller
`--port_stride` may be used when a coordinator assigns a tighter non-overlapping
port band.

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for a fully logged diagnostic case.
