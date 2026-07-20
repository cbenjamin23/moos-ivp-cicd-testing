# H02-pmissionhash_unit

This is an intentionally non-visual unit harness. `--gui` is unsupported
because the stem has no `pMarineViewer` configuration.

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

- `hash_custom_vars_pass` Sets `mission_hash_var=CUSTOM_MISSION_HASH` and `mhash_short_var=CUSTOM_MHASH_SHORT`, testing that both hash publications move to the configured names rather than being duplicated under the defaults; passes when the evaluator receives a full `mhash=...,utc=...` value and a two-word short hash through the custom variables and the `.alog` contains neither `MISSION_HASH` nor `MHASH`.
- `hash_short_off_pass` Sets `mhash_short_var=off` while retaining `MISSION_HASH`, testing suppression of the standalone short-hash post and pMissionEval's derivation of `$[MHASH_SHORT]` from the full payload; passes when no `MHASH` mail appears and the row contains both a full hash and a derived two-word short hash.
- `hash_long_off_pass` Sets `mission_hash_var=off` while leaving `mhash_short_var=MHASH`, testing the long-hash disable path; passes when the evaluator grades pass and neither `MISSION_HASH` nor `MHASH` appears in the `.alog`.
- `hash_reset_changes_pass` Posts `RESET_MHASH=true` at mission time 5 and evaluates after pMissionHash's next 30-second publication, testing runtime hash regeneration; passes when the reported `prehash` and `reset_hash` are different two-word hashes and the `.alog` contains at least two distinct `MISSION_HASH` payloads sourced by `pMissionHash`.

## Running

```bash
./zlaunch.sh --case=hash_custom_vars_pass --port_base=9000 10
./zlaunch.sh --jobs=3 --port_base=9000 10
./zlaunch.sh --just_make --jobs=3 --port_base=9000 10
```

Serial and rolling runs use the same isolated-case path. Grouped runs use
30-port case blocks from `--port_base`; the default starts at `9000`.

## Coverage Validation

The July 20, 2026 oracle pass completed all four cases with both minimal and
full logging at three concurrent jobs. Injecting `MISSION_HASH` and `MHASH`
posts while both custom hash values remained valid left `subject_grade=pass`
but made `hash_custom_vars_pass` fail with `reason=evidence_mismatch`.
Removing only the case-owned `RESET_MHASH` event likewise left
`subject_grade=pass`, but the identical `prehash` and `reset_hash` values made
`hash_reset_changes_pass` fail with `reason=evidence_mismatch`.

The reset case's `uTimerScript` exclusively owns the reset and evaluation
deadline. The shared runner supplies only the initial pass and report inputs,
so it cannot issue a second poke after the mission has already shut down.
