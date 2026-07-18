# H01 pAntler Unit Harness

This harness verifies small pAntler launch-composition contracts. The graded
flags are emitted only by apps started through pAntler, so the mission grade
stays tied to process launch behavior.

## Cases

- `pantler_baseline_launch_pass` Launches a normal `uTimerScript` and expects its flag to reach `pMissionEval`.
- `pantler_alias_launch_pass` Launches `uTimerScript` under the `uTimerAlias` MOOS name and expects the alias-specific config block to run.
- `pantler_multi_alias_launch_pass` Launches two aliases of the same binary and expects both alias-specific config blocks to run.
- `pantler_filter_launch_pass` Starts pAntler with a launch filter and expects the allowed timer to run while the blocked timer never posts.
- `pantler_system_path_launch_pass` Launches an aliased timer with an explicit `PATH=SYSTEM` option and expects the alias config block to run.
- `pantler_extra_params_launch_pass` Launches an aliased timer with `ExtraProcessParams` and expects the timer still to run with the added option.

Typical commands:

```sh
./zlaunch.sh --jobs=2 --port_base=9000 --max_time=20 10
./zlaunch.sh --case=pantler_multi_alias_launch_pass --port_base=9000 --max_time=20 10
```

The launcher requires Bash 5.1 or newer for rolling scheduling. Every case,
including serial runs, executes in its own mission copy and port block.
pMissionEval owns each verdict from flags published only by the processes that
pAntler starts; the harness only applies the case patch, validates the mission
row, aggregates results in case order, and performs scoped cleanup.
