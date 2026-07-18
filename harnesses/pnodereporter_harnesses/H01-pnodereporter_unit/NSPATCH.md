# pNodeReporter H01 nspatch Contract

The harness composes every case from the dedicated
`missions/pnodereporter_missions/pnodereporter_unit` stem.
`nav_report_baseline_pass` uses an unmodified stem copy. Each of the other
thirty-two cases applies its explicitly mapped kebab-case `.xmoos` file to
`meta_shoreside.moos`, producing `meta_shoreside.moosx` inside that case's
isolated mission copy.

This is a one-community harness. It has no vehicle or behavior overlays and no
other harness consumes the stem. The harness forwards `--mmod=<case>` and one
MOOSDB/reserved-pShare port pair to the copied mission wrapper. `launch.sh`
uses `nsplug -x`, so each generated sidecar is consumed without modifying the
source stem.

`pMissionEval` writes every mission grade. Because the subject of these tests
is the exact structured node-report representation, the harness retains its
narrow payload supplement: it verifies the documented required and forbidden
fragments after a mission reports `grade=pass`. The two alternate-navigation
cases and the ephemeral load-warning case accept a matching result snapshot or
a matching logged `NODE_REPORT_LOCAL` publication. No application or MOOS
grading variable was added.

Case order, patch selection, required/forbidden fragments, and evaluator
configuration remain explicit in `zlaunch.sh`. Run
`./zlaunch.sh --just_make --keep_workdirs` to inspect all generated targets and
sidecars without launching MOOS.
