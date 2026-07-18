# H01-pdeadmanpost_unit

Live `pDeadManPost` watchdog harness. Each case writes a focused deadman
configuration, heartbeat schedule, and `pMissionEval` oracle, then launches the
same stem mission on isolated ports.

`pMissionEval` owns every live MOOS verdict. Eleven cases also preserve the
existing post-run `.alog` publication-count checks because repeated identical
posts, exact post counts, and an explicit `false` publication cannot be
distinguished from current MOOS state alone. A count mismatch is normalized as
`grade=fail reason=artifact_check_failed` with the mission row retained as
provenance. No additional app or grading variable is used.

Every selected case runs in its own mission copy and stride-30 port block. The
launcher uses Bash 5.1 rolling scheduling, deterministic selected-case result
ordering, root-scoped teardown, and the same isolated execution path for
serial and rolling runs.

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

## Migration Validation

The July 2026 migration preserved all 20 configurations, pMissionEval
conditions, heartbeat schedules, deadflags, and 11 count contracts. Three
legacy four-job runs passed 60/60 rows in 21.20, 20.79, and 20.49 seconds;
legacy serial passed 20/20 in 65.13 seconds. Three final migrated rolling runs
passed 60/60 in 32.67, 31.92, and 32.43 seconds; migrated isolated serial
passed 20/20 in 116.13 seconds.

The standard xlaunch shutdown interval exposed one legacy observation race in
`active_start_keepalive_suppresses_pass`: its final heartbeat was followed by a
deadflag during normal shutdown, after pMissionEval had already passed. The
original `max_noheart=3.50` contract is retained. Publication counts are now
bounded by the first `MISSION_EVALUATED=true` timestamp, so the case requires
zero deadflags during its evaluation window while allowing the watchdog to
correctly fire after heartbeats stop. Five focused reruns passed with a bounded
count of zero; a retained raw log confirmed the expected later deadflag.

Validation also covered both skill static checks, Bash syntax, ShellCheck,
standalone generation and live execution, all-case generation, exact result
ordering, distinct ports, old-Bash and option errors, lock contention,
launch-error and missing-result normalization, missing-alog rejection, retained
workdirs, log inspection, and post-run listener checks.
