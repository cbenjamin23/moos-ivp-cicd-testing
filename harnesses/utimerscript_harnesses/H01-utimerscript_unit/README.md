# H01-utimerscript_unit

This harness validates `uTimerScript` as a live MOOS app rather than as a helper
for other tests. The stem mission runs one shoreside MOOS community with
`uTimerScript`, `pMissionEval`, `pMissionHash`, and `pLogger`. The harness writes
case-specific script and evaluator config into isolated mission copies. It uses
external `uPokeDB` for runtime control mail and one delayed `uXMS` client for
the `block_on` connection gate.

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

Logging defaults to a focused grading allowlist. Use `--log=full` for
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
- `delay_start_pass`: Sets `delay_start=1.0` before a time-0.1 `UTS_DELAYED=true` event; the first output must occur 0.9–1.4 MOOS seconds after the script's initial status.
- `status_var_initial_pass`: Rebinds status output to `UTS_CUSTOM_STATUS` for script `status_case`; passes when its alog line contains both `name=status_case` and `pending=0`.
- `silent_status_pass`: Sets `status_var=silent` and schedules only the readiness event; passes when evaluation completes with zero `UTS_STATUS` publications before the cutoff.
- `randpair_poly_bounds_pass`: Publishes 20 paired `x,y` samples from the nonrectangular triangle `{0,0:10,0:0,2}` in single events so each pair comes from one `at_post` draw; passes when all 20 values lie inside that triangle.
- `block_on_peer_pass`: Sets `block_on=UTSDelayedPeer`, starts that client only after a 1.2-second launcher delay, and requires `UTS_BLOCKED_OUT=unblocked` after the first logged `DB_CLIENTS` value containing the delayed peer.
- `atomic_condition_drop_pass`: Starts under `UTS_RUN=true` with `script_atomic=true`, then drops the condition after the first event but before the time-10 second event; passes when both events publish and the final gate value is false.
- `pause_toggle_external_pass`: Schedules `UTS_TOGGLE_OUT=after_toggle` at time 10 and sends two `UTS_PAUSE=toggle` pokes; passes when the first output occurs after the second toggle, proving the intervening paused interval suppressed it.
- `forward_positive_skip_pass`: Places `UTS_FORWARD_POS=skipped` at script time 100, sends `UTS_FORWARD=100`, and evaluates from a later external poke while natural script time is still below 100; passes when the positive skip releases the event.
- `custom_control_vars_pass`: Rebinds pause, forward, and reset to `UTS_HOLD`, `UTS_SKIP`, and `UTS_REDO`; passes when the first probe follows the hold release, each 100-second event is released by its skip, reset status is `1/any`, and both the probe and control output publish exactly twice with final local count `1`.
- `time_warp_accelerates_pass`: Sets uTimerScript `time_warp=4` and schedules output at script time 2; status must report warp 4 and the output must follow initial status by 0.35–0.8 MOOS seconds.
- `delay_reset_pass`: Sets all-posted reset, `reset_max=1`, and `delay_reset=1.0`; exactly two `UTS_DELAY_RESET` posts must occur 1.0–1.6 MOOS seconds apart.
- `reset_max_one_limit_pass`: Sets `reset_time=all-posted` and `reset_max=1` with a `TCOUNT` payload; passes when final `UTS_LIMITED_REPEAT=1` and status reports `resets=1/1`.
- `amount_multiple_posts_pass`: Sets `amt=3` on `UTS_AMOUNT=burst`; passes when the final value is `burst` and status reports `posted=4`, comprising three amount posts plus readiness.
- `dbtime_utc_macro_pass`: Publishes `$[DBTIME]` and `$[UTC]` in the same iteration and currently requires both values to be positive; this is smoke coverage only because the tested upstream build reports the same epoch-like value for both macros when its `DB_UPTIME` baseline remains unset.
- `upon_awake_reset_pass`: Gates the script, sets `upon_awake=reset`, then performs true-false-true gate transitions; passes when final `UTS_AWAKE_RESET=1` and status reports one reset.
- `upon_awake_restart_pass`: Repeats the gate transition with `upon_awake=restart`; passes when final `UTS_AWAKE_RESTART=1` and status reports zero resets.
- `math_expression_pass`: Schedules `{{3+2}*4}` as an event value, testing nested runtime math expansion; passes when pMissionEval observes `UTS_MATH=20`.
- `idx_amount_expansion_pass`: Uses `amt=3` with value `idx_$[IDX]`, testing zero-based index expansion across a burst; passes when the final value is `idx_002` and status reports four total posts including readiness.
- `time_window_event_pass`: Schedules `UTS_WINDOWED=true` in the `0.2:0.8` time window; its first publication must follow initial status by 0.15–0.95 MOOS seconds, including one app-tick margin around the configured window.
- `reset_time_numeric_pass`: Sets numeric `reset_time=0.6` and `reset_max=1` for a `TCOUNT` event; passes when final `UTS_NUMERIC_RESET=1` and status reports `resets=1/1`.
- `multi_condition_gate_pass`: Requires `UTS_GATE_A=true` and then `UTS_GATE_B=ready`; passes when the first `UTS_MULTI_GATED=released` publication occurs only after the second gate mail and both final gate values match.
- `quit_event_pass`: Posts `UTS_QUIT_READY=true`, executes `quit` at time 0.4, and leaves pMissionEval running until an external readiness poke; passes when the ready post is sourced by `uTimerScript` and a later `DB_CLIENTS` update no longer contains that client.

## Direct CTest coverage

The removed one-sample uniform and gaussian missions could prove only bounds,
not a distribution. `RandVarUniformTest` and `RandVarGaussianTest` now draw
2,000 samples and require broad mean and variance bands; the uniform test also
requires both outer tenths of its range to be reached. The removed
`shuffle=false` status mission was likewise non-discriminating: the source
contract is that `scheduleEvents(false)` preserves an already assigned event
timestamp across reset, which `TS_MOOSAppEventTest` checks exactly.

## Latest validation

- July 20, 2026
- generated-file matrix: `30/30` completed
- minimal-logging matrix: `30/30` passed
- full-logging matrix: `30/30` passed
- focused `utimerscript` CTest family: `55/55` passed
- eight deliberate mutations failed at their intended evidence: omitted
  `block_on`, zero start delay, disabled app time warp, zero reset delay, an
  event outside the configured time window, collapsed uniform range,
  collapsed gaussian variance, and shuffled rescheduling
- warp: `10`
