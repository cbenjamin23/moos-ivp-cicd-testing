# ufld_obstacle_sim_unit

Headless shoreside mission for app-level `uFldObstacleSim` checks. The stem
runs one MOOS community with `uFldObstacleSim`, scripted input mail,
`pMissionEval`, `pMissionHash`, and a narrow upstream `pEchoVar`
adapter.

Logging defaults to `--log=minimal`, which leaves the dormant pLogger
configuration in place but does not launch the process. Use `--log=full` to
restore the pre-migration logger for direct diagnostic runs.

The default scenario loads a fixed obstacle field and uses `PMV_CONNECT` to
trigger publication. Its evaluator grades exact truth and vehicle-facing
obstacle payloads, minimum range, the final region visual, and the expected
visual count. Harness overlays change the obstacle field, scripted mail,
simulator parameters, and evaluator conditions for the remaining cases.

`pMissionEval` starts before the subject and owns the sole `grade=` written to
`results.txt`. Timers wait for `pMissionEval`, `pEchoVar`, and
`uFldObstacleSim` to appear in `DB_CLIENTS` before their event clocks begin.
Expected-absence cases use fail conditions on the original MOOS variables;
mailflag counters exist only for repeat-publication assertions.

`pMissionEval` compares simple current values directly. Its condition grammar
does not accept a structured literal containing relation characters such as
`=`. For exact structured reports, `uTimerScript` publishes the expected
payload to a case-owned `UFOS_EXPECT_*` variable and the evaluator uses the
parser-safe right-variable form `$ (UFOS_EXPECT_*)`. This compares the runtime
strings without placing the payload itself in the condition grammar.

A single stock `pEchoVar` process contains narrowly scoped EFlipper rules only
for assertions that need field selection or a retained match across multiple
publications. These include obstacle and vehicle labels, tracked keys, point
colors and sizes, polygon labels, and reset evidence. The outputs are plain
components or presence sentinels except for one exact inactive-record
composite, which is compared through a runtime expected variable. They are not
grades. No custom grading application or generic evidence-rule system is part
of the mission.

The mission wrapper accepts harness port aliases, validates target creation in
`--just_make` mode, requires a mission grade after live runs, and performs
root-scoped teardown. The low-level launcher also uses scoped cleanup for
manual interactive exits. The H01 harness forwards `--max_time` to this wrapper
without adding an outer wall-clock watchdog; as in the skill baseline,
`uMayFinish` can apply that ceiling only after it connects to MOOSDB.

Run the default case directly:

```sh
./zlaunch.sh --max_time=30 --nogui 10
./zlaunch.sh --log=full --max_time=30 --nogui 10
```

Harness overlays are applied by the H01 launcher for nondefault cases; passing
only `--mmod=<case>` changes the result label but does not apply an overlay.
