# PID H01 nspatch Contract

The harness composes each case from the dedicated
`missions/pid_missions/pid_unit` stem. `rudder_starboard_pass` uses an
unmodified copy of the stem. Each of the other thirty-three cases applies the
same-named kebab-case `*-shoreside.xmoos` file to `meta_shoreside.moos`,
producing `meta_shoreside.moosx` inside that case's isolated mission copy.

There are no vehicle, behavior, or shared-stem patches in this harness. Patch
composition is:

- 17 overlays replace `uTimerScript` and `pMissionEval`.
- 5 overlays replace `pMarinePIDV22` and `pMissionEval`.
- 11 overlays replace all three blocks.

Every overlay contains its existing case-specific `mission_mod`; the harness
does not inject or rewrite a modifier. All evaluators retain the simple
`lead_condition = TEST_EVAL_READY = true` contract and write their verdict to
`results.txt`. The shell aggregates that verdict but does not reinterpret the
case's PID evidence.

Case tokens, filenames, and modifiers are intentionally exact. In particular,
the historical spelling `manual_overide_alias_zero_pass` is preserved across
all three. Run `./zlaunch.sh --just_make --keep_workdirs` to inspect all
generated sidecars and targets without launching MOOS.
