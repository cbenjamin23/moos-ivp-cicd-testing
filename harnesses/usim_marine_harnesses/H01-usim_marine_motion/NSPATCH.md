# uSimMarine H01 nspatch Contract

The harness composes each case from the dedicated
`missions/usim_marine_missions/usim_marine_motion` stem.
`thrust_forward_pass` uses an unmodified copy of the stem. Each of the other
thirty-five cases applies the same-named kebab-case `.xmoos` file to
`meta_shoreside.moos`, producing `meta_shoreside.moosx` inside that case's
isolated mission copy.

There are no vehicle, behavior, or shared-stem patches in this harness. Patch
composition is:

- 12 overlays replace `uTimerScript` and `pMissionEval`.
- 23 overlays replace `uSimMarineV22`, `uTimerScript`, and `pMissionEval`.

The harness forwards `--mmod=<case>` to the mission wrapper. Each stem or
overlay already uses the matching `$(MMOD:=case_name)` default, so this labels
the existing result contract without rewriting evaluator content. All
evaluators retain one simple `lead_condition = TEST_EVAL_READY = true`, their
existing substantive pass conditions, and one report to `results.txt`.
`pMissionEval` owns the verdict; the shell only validates and aggregates it.

Case tokens, patch filenames, order, simulator configuration, scripted events,
evaluation timing, pass conditions, and report columns are intentionally
unchanged by the harness-skill migration. Run
`./zlaunch.sh --just_make --keep_workdirs` to inspect all generated sidecars
and targets without launching MOOS.
