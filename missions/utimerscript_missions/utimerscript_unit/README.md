# utimerscript_unit

Headless single-community stem for exercising `uTimerScript` as an app-under-test.
The harness writes case-specific `uTimerScript` and `pMissionEval` blocks, launches
the live MOOSDB, and uses external `uPokeDB` only for runtime control mail such as
pause, forward, reset, and condition variables. This avoids using `uTimerScript` as
its own test driver.

Typical harness run:

```sh
../../../harnesses/utimerscript_harnesses/H01-utimerscript_unit/zlaunch.sh --jobs=4 --port_base=7300 10
```

Run one inspectable case:

```sh
../../../harnesses/utimerscript_harnesses/H01-utimerscript_unit/zlaunch.sh --case=timed_numeric_string_pass --port_base=7300 10
```
