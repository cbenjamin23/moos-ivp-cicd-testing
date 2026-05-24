# H02-pmissionhash_unit

Patch-driven unit harness for `pMissionHash`.

The harness uses the stem mission at
`missions/mission_utility_missions/mission_utility_unit`. Cases patch the hash
publication settings and use `pMissionEval` only as the mission-owned grading
consumer for hash mail and report rows.

Exported harness rows use `grade=<pass|fail>` for the harness verdict and
`subject=pMissionHash` to identify the utility under test. The evaluator row is
preserved as `subject_grade=pass` plus the hash evidence fields.

## Current Matrix

- `hash_custom_vars_pass` Custom variable case. `pMissionHash` publishes non-default long and short hash variables, and the evaluator expands them into the report row.
- `hash_short_off_pass` Short-hash disable case. `pMissionHash` publishes only `MISSION_HASH`, while the evaluator still derives the short hash from the full hash payload.
- `hash_long_off_pass` Long-hash disable case. `pMissionHash` is configured with `mission_hash_var=off`, so neither long nor short hash mail should be published.
- `hash_reset_changes_pass` Hash reset case. A scripted `RESET_MHASH=true` must cause more than one distinct mission hash publication during the run.

## Running

```bash
./zlaunch.sh --case=hash_custom_vars_pass --port_base=7300 10
./zlaunch.sh --jobs=3 --port_base=7300 10
./zlaunch.sh --just_make --jobs=3 --port_base=7300 10
```

Grouped runs use 10-port case blocks from `--port_base`. The default starts at
`7300`.
