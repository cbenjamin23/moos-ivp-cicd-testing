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

`pMissionEval` owns each ordinary current-state verdict. A narrow set of
post-run `.alog` checks remains for publication counts, exact structured
payload/status text, and expected absence because those histories cannot be
represented by the final value of a MOOS variable. These checks are bounded by
the first `MISSION_EVALUATED=true` timestamp.

Typical run:

```sh
./zlaunch.sh --jobs=4 --port_base=9000 10
```

Run one case:

```sh
./zlaunch.sh --case=timed_numeric_string_pass --port_base=9000 10
```

## Current Matrix

- `timed_numeric_string_pass` Posts one numeric value and one string value on a short schedule, proving the basic event publication path.
- `quoted_comma_payload_pass` Posts a quoted comma-bearing payload and checks the alog for the exact structured string.
- `macro_count_nav_pass` Publishes nav mail and then expands `NAV_X`, `NAV_Y`, local count, and total count macros into a later payload.
- `condition_gate_external_pass` Holds the script behind a logic condition until an external poke releases it.
- `pause_unpause_external_pass` Starts paused, receives runtime unpause mail, and then posts the scheduled event.
- `forward_jump_next_pass` Uses `UTS_FORWARD=0` to jump directly to a late pending posting rather than waiting for the full script time.
- `reset_var_repost_pass` Uses runtime reset mail and checks that the same scripted event posts again after reset.
- `reset_time_all_posted_pass` Uses `reset_time=all-posted` and a finite reset limit to prove automatic reposting after all events are complete.
- `delay_start_pass` Adds an initial delay and verifies that delayed events still publish and grade correctly.
- `status_var_initial_pass` Uses a custom status variable and checks that status mail includes the configured script name and pending count.
- `silent_status_pass` Sets `status_var=silent` and verifies that default status mail is absent while scripted completion still works.
- `randvar_uniform_bounds_pass` Expands a runtime random variable and grades that the value remains inside the configured bounds.
- `randvar_gaussian_bounds_pass` Expands a gaussian runtime random variable and grades that the clamped sample stays inside its configured bounds.
- `randpair_poly_bounds_pass` Expands a random point sampled from a polygon and grades both coordinates against the polygon bounds.
- `block_on_peer_pass` Blocks until `pMissionHash` is visible as a DB client, then proves the pending event is released.
- `atomic_condition_drop_pass` Uses `script_atomic=true` to prove a script that has started continues after its condition drops midstream.
- `pause_toggle_external_pass` Uses two runtime `UTS_PAUSE=toggle` pokes to pause and later resume a pending event.
- `forward_positive_skip_pass` Uses positive `UTS_FORWARD` mail to skip elapsed script time and release a later event early.
- `custom_control_vars_pass` Rebinds pause, forward, and reset control variables, then proves those custom controls drive the script.
- `time_warp_accelerates_pass` Sets the app-level timer-script time warp and checks that a delayed script-time event publishes.
- `delay_reset_pass` Combines `reset_time=all-posted` with `delay_reset` and verifies the delayed second pass posts.
- `reset_max_one_limit_pass` Uses a finite reset limit and checks that automatic all-posted reset stops after one reset.
- `amount_multiple_posts_pass` Uses `amt=3` on one event and verifies three live publications.
- `dbtime_utc_macro_pass` Posts `DBTIME` and `UTC` macros as numeric values and grades that both are positive.
- `upon_awake_reset_pass` Drops and restores a condition with `upon_awake=reset`, proving the script resets and reposts.
- `upon_awake_restart_pass` Drops and restores a condition with `upon_awake=restart`, proving the script restarts without incrementing reset count.
- `math_expression_pass` Expands a nested math expression at runtime and grades the resulting numeric post.
- `idx_amount_expansion_pass` Uses `amt=3` with the IDX macro and verifies the final indexed payload.
- `time_window_event_pass` Uses an event time window and verifies the scheduled event publishes before evaluation.
- `reset_time_numeric_pass` Uses a numeric `reset_time` and finite reset limit to prove timed automatic reset behavior.
- `multi_condition_gate_pass` Requires two independent logic conditions before a gated event may publish.
- `shuffle_false_status_pass` Disables shuffle and verifies status reports the non-shuffled script mode.
- `quit_event_pass` Runs the `quit` event path after posting a readiness flag, then grades completion externally.
