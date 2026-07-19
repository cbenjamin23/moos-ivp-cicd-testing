# H02-pmissionhash_unit

Patch-driven unit harness for `pMissionHash`.

The harness uses isolated copies of the stem mission at
`missions/mission_utility_missions/mission_utility_unit`. Cases explicitly
patch the hash publication settings, run through the standard `xlaunch`
lifecycle, and use `pMissionEval` as the grading consumer for hash mail and
report rows. Publication-history and expected-absence checks remain
supplemental evidence because they cannot be proven from one final MOOS value.

Logging defaults to the shared grading allowlist. Use `--log=full` to restore
the original wildcard diagnostic logger for every selected case.

Exported harness rows use `grade=<pass|fail>` for the harness verdict and
`subject=pMissionHash` to identify the utility under test. The evaluator row is
preserved as `subject_grade=pass` plus the hash evidence fields.

## Current Matrix

- `hash_custom_vars_pass` Sets `mission_hash_var=CUSTOM_MISSION_HASH` and `mhash_short_var=CUSTOM_MHASH_SHORT`, testing non-default publication names; passes when the evaluator grades pass and reports a full `mhash=...,utc=...` value plus a two-word short hash through those custom variables.
- `hash_short_off_pass` Sets `mhash_short_var=off` while retaining `MISSION_HASH`, testing suppression of the standalone short-hash post and pMissionEval's derivation of `$[MHASH_SHORT]` from the full payload; passes when no `MHASH` mail appears and the row contains both a full hash and a derived two-word short hash.
- `hash_long_off_pass` Sets `mission_hash_var=off` while leaving `mhash_short_var=MHASH`, testing the long-hash disable path; passes when the evaluator grades pass and neither `MISSION_HASH` nor `MHASH` appears in the `.alog`.
- `hash_reset_changes_pass` Posts `RESET_MHASH=true`, testing runtime regeneration of the mission hash; passes when the evaluator grades pass and the `.alog` contains at least two distinct `mhash=` values published on `MISSION_HASH`.

## Running

```bash
./zlaunch.sh --case=hash_custom_vars_pass --port_base=9000 10
./zlaunch.sh --jobs=3 --port_base=9000 10
./zlaunch.sh --just_make --jobs=3 --port_base=9000 10
```

Serial and rolling runs use the same isolated-case path. Grouped runs use
30-port case blocks from `--port_base`; the default starts at `9000`.
