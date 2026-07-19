# H01 pAntler Unit Harness

This harness verifies small pAntler launch-composition contracts. The graded
flags are emitted only by apps started through pAntler, so the mission grade
stays tied to process launch behavior.

## Cases

- `pantler_baseline_launch_pass` Starts an ordinary `uTimerScript` from the ANTLER `Run` list, testing unaliased process launch; passes when that process posts `ANTLER_BASE_FLAG=true` and the later evaluation trigger.
- `pantler_alias_launch_pass` Starts `uTimerScript` as `uTimerAlias`, testing MOOS-name aliasing and lookup of the matching `ProcessConfig=uTimerAlias` block; passes when the aliased process posts `ANTLER_ALIAS_FLAG=true` and the evaluation trigger.
- `pantler_multi_alias_launch_pass` Starts two copies of `uTimerScript` as `uTimerOne` and `uTimerTwo`, testing independent configuration of multiple aliases of one executable; passes when the two processes post `ANTLER_ONE_FLAG=true` and `ANTLER_TWO_FLAG=true`.
- `pantler_filter_launch_pass` Supplies a pAntler launch filter containing `uTimerAllowed` but not `uTimerBlocked`, testing selective launch from an ANTLER block that lists both aliases; passes when the allowed process posts `ANTLER_FILTER_ALLOWED=true` and any `ANTLER_FILTER_BLOCKED=true` post fails the case.
- `pantler_system_path_launch_pass` Starts `uTimerScript` as `uTimerSystemPath` with `PATH=SYSTEM`, exercising explicit system-path lookup; passes when the aliased process posts `ANTLER_PATH_FLAG=true` and the evaluation trigger.
- `pantler_extra_params_launch_pass` Defines `timer_args=--verbose=false` and starts `uTimerScript` as `uTimerExtra` with `ExtraProcessParams=timer_args`, exercising named extra-argument forwarding; passes when the aliased process posts `ANTLER_EXTRA_FLAG=true` and the evaluation trigger.

Typical commands:

```sh
./zlaunch.sh --jobs=2 --port_base=9000 --max_time=20 10
./zlaunch.sh --jobs=4 --log=full --port_base=9000 --max_time=20 10
./zlaunch.sh --case=pantler_multi_alias_launch_pass --port_base=9000 --max_time=20 10
```

Logging mode defaults to `minimal`; `--log=full` may be combined with the
whole matrix or one `--case`. Both modes intentionally preserve the same
case-defined ANTLER launch lists and logger processes in this subject harness.
Changing that topology for minimal mode would change the behavior under test.

The launcher requires Bash 5.1 or newer for rolling scheduling. Every case,
including serial runs, executes in its own mission copy and port block.
pMissionEval owns each verdict from flags published only by the processes that
pAntler starts; the harness only applies the case patch, validates the mission
row, aggregates results in case order, and performs scoped cleanup.
