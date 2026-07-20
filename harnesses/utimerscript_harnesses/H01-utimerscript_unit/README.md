# H01-utimerscript_unit

This harness validates `uTimerScript` as a live MOOS app rather than as a helper
for other tests. The stem mission runs one shoreside MOOS community with
`uTimerScript`, `pMissionEval`, `pMissionHash`, and `pLogger`. The harness writes
case-specific script and evaluator config into isolated mission copies, then uses
external `uPokeDB` only for runtime control mail.

The launcher requires Bash 5.1 or newer and uses rolling `--jobs` scheduling,
distinct stride-30 port blocks, canonical root-scoped teardown, and one
deterministically ordered result row per selected case. Serial and rolling runs
use the same isolated-copy path.

`pMissionEval` owns each ordinary current-state verdict. Bounded post-run
`.alog` checks cover exact payloads, logical publication counts, release order,
polygon containment, and process exit where a final MOOS value cannot represent
the contract. Full logging retains the same grading allowlist before adding
wildcard diagnostics, and duplicate wildcard rows do not count as additional
publications. Every artifact check stops at the first
`MISSION_EVALUATED=true` timestamp.

Logging defaults to the seven-variable grading allowlist. Use `--log=full` for
wildcard diagnostic logging across every selected case.

Typical run:

```sh
./zlaunch.sh --jobs=4 --port_base=9000 10
```

Run one case:

```sh
./zlaunch.sh --case=timed_numeric_string_pass --port_base=9000 10
```

## Current Matrix

- `timed_numeric_string_pass`: Schedules numeric `UTS_NUM=42` at 0.2 and string `UTS_STR=alpha` at 0.3, testing basic typed event publication; passes when pMissionEval observes both exact values after the 0.5 readiness event.
- `quoted_comma_payload_pass`: Schedules quoted `name=alpha,mode=survey`, testing that a comma-bearing value remains one event field; passes when the pre-evaluation `UTS_PAYLOAD` alog line contains the exact structured string.
- `macro_count_nav_pass`: Posts `NAV_X=12.5` and `NAV_Y=-4.25`, then expands both navigation values and the local and total event counters; passes only when `UTS_MACRO` exactly equals `x=12.50,y=-4.25,count=2,tcount=2`.
- `condition_gate_external_pass`: Holds `UTS_GATED=released` behind `condition=UTS_GATE = true`, then pokes the gate externally; passes when the first output occurs after the `UTS_GATE=true` mail and the final value is `released`.
- `pause_unpause_external_pass`: Starts paused with `UTS_PAUSED_OUT=after_pause` pending, then pokes `UTS_PAUSE=false`; passes when the first output occurs after that unpause mail and its value is `after_pause`.
- `forward_jump_next_pass`: Places `UTS_LATE=jumped` at script time 100, sends `UTS_FORWARD=0`, and evaluates from a later external poke while natural script time is still below 100; passes when the jump-to-next control releases the event.
- `reset_var_repost_pass`: Posts `UTS_COUNT=$[TCOUNT]`, sends one `UTS_RESET=true`, and evaluates after the second run; passes when the final value is `1` and exactly two logical `UTS_COUNT` publications occurred.
- `reset_time_all_posted_pass`: Sets `reset_time=all-posted` and `reset_max=2` for an event valued with `TCOUNT`; passes when the third run publishes `UTS_REPEAT=2`.
- `delay_start_pass`: Sets `delay_start=1.0` before a time-0.1 `UTS_DELAYED=true` event; passes when that value is present by the time-1.3 readiness event, without grading its publication time.
- `status_var_initial_pass`: Rebinds status output to `UTS_CUSTOM_STATUS` for script `status_case`; passes when its alog line contains both `name=status_case` and `pending=0`.
- `silent_status_pass`: Sets `status_var=silent` and schedules only the readiness event; passes when evaluation completes with zero `UTS_STATUS` publications before the cutoff.
- `randvar_uniform_bounds_pass`: Defines uniform `RV` on `[5,7]` with `key=at_post` and publishes its expansion; passes when `UTS_RAND` lies in the configured bounds.
- `randvar_gaussian_bounds_pass`: Defines gaussian `GV` with `mu=12`, `sigma=0.5`, and clamp bounds `[10,14]`; passes when the single `UTS_GAUSS` sample lies in `[10,14]`.
- `randpair_poly_bounds_pass`: Publishes 20 paired `x,y` samples from the nonrectangular triangle `{0,0:10,0:0,2}` in single events so each pair comes from one `at_post` draw; passes when all 20 values lie inside that triangle.
- `block_on_peer_pass`: Sets `block_on=pMissionHash` before scheduling `UTS_BLOCKED_OUT=unblocked`; passes when the event publishes after the concurrently launched peer is present, without checking an earlier blocked interval.
- `atomic_condition_drop_pass`: Starts under `UTS_RUN=true` with `script_atomic=true`, then drops the condition after the first event but before the time-10 second event; passes when both events publish and the final gate value is false.
- `pause_toggle_external_pass`: Schedules `UTS_TOGGLE_OUT=after_toggle` at time 10 and sends two `UTS_PAUSE=toggle` pokes; passes when the first output occurs after the second toggle, proving the intervening paused interval suppressed it.
- `forward_positive_skip_pass`: Places `UTS_FORWARD_POS=skipped` at script time 100, sends `UTS_FORWARD=100`, and evaluates from a later external poke while natural script time is still below 100; passes when the positive skip releases the event.
- `custom_control_vars_pass`: Rebinds pause, forward, and reset to `UTS_HOLD`, `UTS_SKIP`, and `UTS_REDO`; passes when the first probe follows the hold release, each 100-second event is released by its skip, reset status is `1/any`, and both the probe and control output publish exactly twice with final local count `1`.
- `time_warp_accelerates_pass`: Sets uTimerScript `time_warp=4` and schedules output at script time 2; passes when `UTS_WARPED=true` and status contains `time_warp=4`, without measuring acceleration.
- `delay_reset_pass`: Sets all-posted reset, `reset_max=1`, and `delay_reset=1.0`; passes when exactly two `UTS_DELAY_RESET` posts occur and status contains `delay_reset=1`.
- `reset_max_one_limit_pass`: Sets `reset_time=all-posted` and `reset_max=1` with a `TCOUNT` payload; passes when final `UTS_LIMITED_REPEAT=1` and status reports `resets=1/1`.
- `amount_multiple_posts_pass`: Sets `amt=3` on `UTS_AMOUNT=burst`; passes when the final value is `burst` and status reports `posted=4`, comprising three amount posts plus readiness.
- `dbtime_utc_macro_pass`: Publishes `$[DBTIME]` and `$[UTC]` in the same iteration and currently requires both values to be positive; this is smoke coverage only because the tested upstream build reports the same epoch-like value for both macros when its `DB_UPTIME` baseline remains unset.
- `upon_awake_reset_pass`: Gates the script, sets `upon_awake=reset`, then performs true-false-true gate transitions; passes when final `UTS_AWAKE_RESET=1` and status reports one reset.
- `upon_awake_restart_pass`: Repeats the gate transition with `upon_awake=restart`; passes when final `UTS_AWAKE_RESTART=1` and status reports zero resets.
- `math_expression_pass`: Schedules `{{3+2}*4}` as an event value, testing nested runtime math expansion; passes when pMissionEval observes `UTS_MATH=20`.
- `idx_amount_expansion_pass`: Uses `amt=3` with value `idx_$[IDX]`, testing zero-based index expansion across a burst; passes when the final value is `idx_002` and status reports four total posts including readiness.
- `time_window_event_pass`: Schedules `UTS_WINDOWED=true` in the `0.2:0.8` time window; passes when the value exists by time 1.1, without checking where inside the window it posted.
- `reset_time_numeric_pass`: Sets numeric `reset_time=0.6` and `reset_max=1` for a `TCOUNT` event; passes when final `UTS_NUMERIC_RESET=1` and status reports `resets=1/1`.
- `multi_condition_gate_pass`: Requires `UTS_GATE_A=true` and then `UTS_GATE_B=ready`; passes when the first `UTS_MULTI_GATED=released` publication occurs only after the second gate mail and both final gate values match.
- `shuffle_false_status_pass`: Sets `shuffle=false` and schedules `UTS_SHUFFLE_FALSE=true`; passes when the value publishes and status contains `shuffle=false`, without comparing event order.
- `quit_event_pass`: Posts `UTS_QUIT_READY=true`, executes `quit` at time 0.4, and leaves pMissionEval running until an external readiness poke; passes when the ready post is sourced by `uTimerScript` and a later `DB_CLIENTS` update no longer contains that client.
