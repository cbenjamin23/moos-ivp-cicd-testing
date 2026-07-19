# H01-pdeadmanpost_unit

Live `pDeadManPost` watchdog harness. Each case writes a focused deadman
configuration, heartbeat schedule, and `pMissionEval` oracle, then launches the
same stem mission on isolated ports.

`pMissionEval` owns every live MOOS verdict. Post-run `.alog` checks also grade
publication counts and timing where repeated identical posts, an explicit
`false` publication, or the time of a watchdog trip cannot be distinguished
from current MOOS state alone. An artifact mismatch is normalized as
`grade=fail reason=artifact_check_failed` with the mission row retained as
provenance. No additional app or grading variable is used.

Every selected case runs in its own mission copy and stride-30 port block. The
launcher uses Bash 5.1 rolling scheduling, deterministic selected-case result
ordering, root-scoped teardown, and the same isolated execution path for
serial and rolling runs.

Logging defaults to the variables and MOOSDB connection evidence needed by the
artifact checks, plus the evaluation cutoff. Use `--log=full` for the original
diagnostic set.

```sh
./zlaunch.sh --jobs=4 --port_base=15800 10
./zlaunch.sh --case=active_start_once_posts_pass --port_base=15800 10
```

## Cases

- `active_start_once_posts_pass`: Starts active with `max_noheart=0.5`, no heartbeat mail, and `DEAD_ONCE=true`, testing startup arming; passes when pMissionEval observes the deadflag by time 1.5.
- `active_start_false_no_initial_post_pass`: Starts inactive with no heartbeat and `DEAD_NO_START=true`, testing that the watchdog remains unarmed; passes when evaluation reaches time 1.5 without that deadflag.
- `heartbeat_before_dead_suppresses_pass`: Starts inactive with a one-second timeout and sends heartbeats through time 1.4, testing that fresh mail prevents a trip; passes at time 1.6 when `DEAD_KEEPALIVE` remains absent.
- `stop_heartbeat_posts_pass`: Starts inactive, uses the heartbeat at 0.2 to arm the watchdog and the heartbeat at 0.4 to refresh its 0.45-second timeout, then stops sending mail; passes when `DEAD_STOP=true` is first published 0.35-0.70 seconds after the second heartbeat.
- `active_false_after_first_heartbeat_posts_pass`: Starts inactive, sends one heartbeat at 0.2, and then exceeds the 0.45-second timeout, testing first-heartbeat arming; passes when `DEAD_AFTER_FIRST=true` is observed by time 1.1.
- `numeric_deadflag_pass`: Configures numeric deadflag `DEAD_NUM=7.5`, testing numeric payload parsing and publication; passes when pMissionEval observes exact numeric `7.5`.
- `string_deadflag_pass`: Configures string deadflag `DEAD_STR=halt`, testing string payload publication; passes when pMissionEval observes exact string `halt`.
- `multiple_deadflags_pass`: Configures `DEAD_MULTI_A=true` and `DEAD_MULTI_B=alpha` on one watchdog, testing fan-out from a single trip; passes when both exact values are present.
- `once_policy_single_post_pass`: Uses `post_policy=once` with an active stale watchdog, testing the single-shot latch; passes when `DEAD_ONCE_SINGLE=true` and the pre-evaluation alogs contain exactly one publication.
- `repeat_policy_reposts_pass`: Uses `post_policy=repeat` with an active stale watchdog through time 1.4, testing continuous reposting; passes when `DEAD_REPEAT=true` and at least two publications occur.
- `reset_policy_reposts_after_heartbeat_pass`: Uses `post_policy=reset`, allows one initial stale-timeout post, sends a heartbeat at 0.8 to clear the posting latch, and goes stale once more; passes when the alog contains exactly two `DEAD_RESET=true` publications before evaluation.
- `repeat_policy_inactive_reposts_pass`: Starts inactive under `post_policy=repeat`, arms with one heartbeat at 0.2, then goes stale; passes when `DEAD_REPEAT_AFTER_FIRST=true` and at least two publications occur.
- `reset_policy_no_repost_without_heartbeat_pass`: Uses `post_policy=reset` without any heartbeat after the initial trip, testing that the latch is not reset merely by elapsed time; passes when `DEAD_RESET_SINGLE=true` is published exactly once.
- `active_start_keepalive_suppresses_pass`: Starts active with `max_noheart=3.5` and sends heartbeats through time 1.8, testing fresh-mail suppression during the evaluation window; passes at time 1.6 with exactly zero `DEAD_ACTIVE_KEEPALIVE` publications.
- `custom_heartbeat_var_pass`: Rebinds the heartbeat to `ALT_HEART`, starts inactive, and sends one `ALT_HEART` post before going stale; passes when `DEAD_ALT_HEART=true` is published at least once.
- `negative_deadflag_pass`: Configures numeric deadflag `DEAD_NEG=-3.25`, testing negative-number parsing; passes when pMissionEval observes exact numeric `-3.25`.
- `zero_max_noheart_posts_pass`: Sets `max_noheart=0` on an active watchdog, testing that zero is accepted as an immediate timeout rather than replaced by the positive default; passes when `DEAD_ZERO_MAX=true` is published once and its first publication is no more than 0.90 seconds after `pDeadManPost` appears in `DB_CLIENTS`.
- `false_deadflag_pass`: Configures `DEAD_FALSE=false`, testing publication of a boolean-looking false string that final-state evaluation alone could confuse with absence; passes when the value is false and the alogs contain at least one publication.
- `invalid_post_policy_falls_back_once_pass`: Supplies `post_policy=invalid_policy`, testing fallback to the default once policy; passes when `DEAD_INVALID_POLICY=true` is published exactly once.
- `reset_policy_multiple_reposts_pass`: Uses `post_policy=reset`, trips once, then sends heartbeats at 1.2 and 2.2 to re-arm two more stale intervals; passes when `DEAD_RESET_MULTI=true` is published exactly three times by time 3.4.

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
ordering, distinct ports, old-Bash and option errors,
launch-error and missing-result normalization, missing-alog rejection, retained
workdirs, log inspection, and post-run listener checks.

The July 2026 coverage-strengthening pass corrected
`stop_heartbeat_posts_pass` to start inactive after timestamp evidence showed
the old active-at-start configuration could trip before either heartbeat. The
case now proves that the second heartbeat refreshes the timeout. The reset case
now requires exactly two posts, and the zero-timeout case bounds its first post
relative to the app's MOOSDB connection. Focused mutations to a 0.10-second
stop timeout, `post_policy=repeat`, and `max_noheart=0.5` were all rejected by
the corresponding artifact contracts.

The final four-job family run passed 20/20 cases in 33 seconds. Its retained
workdirs contained 20 distinct forwarded MOOSDB ports and no post-run listeners;
the strengthened zero-timeout case also passed in `--log=full` mode.
