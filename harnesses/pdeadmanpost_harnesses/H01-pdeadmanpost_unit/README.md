# H01-pdeadmanpost_unit

Live `pDeadManPost` watchdog harness. Each case writes a focused deadman
configuration, heartbeat schedule, and `pMissionEval` oracle, then launches the
same stem mission on isolated ports.

```sh
./zlaunch.sh --jobs=4 --port_base=15800 10
./zlaunch.sh --case=active_start_once_posts_pass --port_base=15800 10
```

## Cases

- `active_start_once_posts_pass`: Active-at-start watchdog posts after stale startup.
- `active_start_false_no_initial_post_pass`: Inactive-at-start watchdog does not post before the first heartbeat.
- `heartbeat_before_dead_suppresses_pass`: Regular heartbeat mail suppresses a deadflag.
- `stop_heartbeat_posts_pass`: Deadflag posts after heartbeat mail stops.
- `active_false_after_first_heartbeat_posts_pass`: Inactive-at-start watchdog arms after first heartbeat, then posts when stale.
- `numeric_deadflag_pass`: Numeric deadflag payload is posted and graded.
- `string_deadflag_pass`: String deadflag payload is posted and graded.
- `multiple_deadflags_pass`: Multiple deadflags post from one deadman trip.
- `once_policy_single_post_pass`: `post_policy=once` posts exactly once by alog count.
- `repeat_policy_reposts_pass`: `post_policy=repeat` reposts while active-at-start stale.
- `reset_policy_reposts_after_heartbeat_pass`: `post_policy=reset` reposts after a later heartbeat resets the posting latch.
- `repeat_policy_inactive_reposts_pass`: `post_policy=repeat` reposts after inactive startup is armed by a heartbeat.
- `reset_policy_no_repost_without_heartbeat_pass`: `post_policy=reset` remains single-shot without a later heartbeat.
- `active_start_keepalive_suppresses_pass`: Active-at-start watchdog stays quiet when heartbeat mail remains fresh.
- `custom_heartbeat_var_pass`: Custom `heartbeat_var` wiring arms and trips correctly.
- `negative_deadflag_pass`: Negative numeric deadflag payload is posted and graded.
- `zero_max_noheart_posts_pass`: Zero `max_noheart` trips promptly.
- `false_deadflag_pass`: Boolean-looking false deadflag payload is posted and graded.
- `invalid_post_policy_falls_back_once_pass`: Invalid `post_policy` is ignored and the default once policy remains in effect.
- `reset_policy_multiple_reposts_pass`: `post_policy=reset` reposts exactly once after each of two later heartbeats.
