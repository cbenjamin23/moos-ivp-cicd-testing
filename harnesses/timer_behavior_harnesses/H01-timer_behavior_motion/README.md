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

- `baseline_idle_running_pass`
  Uses `TIMER_IDLE_SECONDS_base` and `TIMER_RUNNING_SECONDS_base`, activates the
  timer after four seconds, and evaluates at ten seconds; passes when both
  counters exceed two seconds and no behavior error occurs.
- `custom_status_vars_pass`
  Sets `var_status_idle=IDLE_SECONDS`, `var_status_running=RUN_SECONDS`, and
  `status_suffix=custom`; passes when `IDLE_SECONDS_custom` and
  `RUN_SECONDS_custom` both exceed two seconds and no behavior error occurs.
- `default_suffix_pass`
  Omits both status-variable parameters and sets `status_suffix=_survey`;
  passes when default outputs `TIMER_IDLE_survey` and `TIMER_RUNNING_survey`
  both exceed two seconds and no behavior error occurs.
- `custom_no_suffix_pass`
  Sets custom outputs `IDLE_RAW` and `RUN_RAW` with no suffix; passes when both
  counters exceed two seconds and no behavior error occurs.
- `default_no_suffix_pass`
  Omits status-variable and suffix parameters; passes when default unsuffixed
  outputs `TIMER_IDLE` and `TIMER_RUNNING` both exceed two seconds and no
  behavior error occurs.
- `active_at_start_pass`
  Posts `TIMER_ACTIVE=true` at 0.5 seconds and evaluates at eight seconds;
  passes when `TIMER_RUNNING_SECONDS_base>5` and no behavior error occurs.
- `delayed_activation_pass`
  Delays `TIMER_ACTIVE=true` until seven seconds and evaluates at twelve;
  passes when idle time exceeds five seconds, running time exceeds two seconds,
  and no behavior error occurs.
- `pause_resume_pass`
  Activates at two seconds, pauses at five, resumes at eight, and evaluates at
  thirteen; passes when accumulated idle time exceeds three seconds, running
  time exceeds five seconds, and no behavior error occurs.
- `repeated_status_param_pass`
  Sets unused status names and suffix first, then replaces them with
  `TIMER_IDLE_SECONDS`, `TIMER_RUNNING_SECONDS`, and `base`; passes when the
  final suffixed counters both exceed two seconds and no behavior error occurs.
- `post_mapping_pass`
  Maps the stock suffixed counters to `TIMER_IDLE_MAPPED` and
  `TIMER_RUNNING_MAPPED`; passes when both mapped counters exceed two seconds
  and no behavior error occurs.
- `duration_complete_pass`
  Sets `duration=2`, `duration_status=TIMER_REMAINING`, and end flag
  `TIMER_DONE=true`; passes when the end flag is true, remaining time is below
  0.5 seconds, running time is between one and four seconds, and no behavior
  error occurs.
- `runtime_update_pass`
  Activates at four seconds, then at six changes both status names and suffix
  through `TIMER_UPDATES`; passes when the original idle/running counters
  exceed two/one seconds, the updated suffixed counters both exceed two
  seconds, and no behavior error occurs.
- `never_active_fail`
  Removes the vehicle-side activation event so the behavior condition remains
  false through evaluation; the harness passes when idle time exceeds six
  seconds, running time equals zero, and no behavior error occurs.
- `bad_idle_var_fail`
  Sets whitespace-bearing `var_status_idle=BAD VAR`; the harness passes when
  evaluation occurs with `TIMER_ACTIVE=true`, no stock suffixed idle or running
  counter has appeared, and no behavior error occurs.
- `bad_running_var_fail`
  Sets whitespace-bearing `var_status_running=BAD VAR`; the harness passes when
  evaluation occurs with `TIMER_ACTIVE=true`, no stock suffixed idle or running
  counter has appeared, and no behavior error occurs.
- `bad_suffix_fail`
  Sets whitespace-bearing `status_suffix=bad suffix`; the harness passes when
  evaluation occurs with `TIMER_ACTIVE=true`, no stock suffixed idle or running
  counter has appeared, and no behavior error occurs.
- `post_mapping_silent_fail`
  Maps `TIMER_RUNNING_SECONDS_base` to `silent`; the harness passes when the
  idle counter exceeds two seconds, the stock running counter never appears,
  and no behavior error occurs.
- `unknown_param_fail`
  Adds unsupported `unsupported_timer_param=true`; the harness passes when
  evaluation occurs with `TIMER_ACTIVE=true`, no stock suffixed idle or running
  counter has appeared, and no behavior error occurs.

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
