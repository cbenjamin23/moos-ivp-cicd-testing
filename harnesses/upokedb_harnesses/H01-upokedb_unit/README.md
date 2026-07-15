# H01-upokedb_unit

Patch-free harness for
[`missions/upokedb_missions/upokedb_unit`](../../../missions/upokedb_missions/upokedb_unit).
It launches a small MOOS community, runs `uPokeDB` against the live DB, and
lets `pMissionEval` grade the variables posted by `uPokeDB`.

## Current Matrix

- `numeric_direct_pass` Posts one numeric value through explicit `--host/--port` arguments and expects the DB value to match exactly.
- `string_forced_pass` Posts a numeric-looking value with `:=` and expects it to remain available as the exact string payload.
- `multiple_mixed_pass` Posts numeric and string variables in one `uPokeDB` invocation and expects both to be present.
- `overwrite_existing_pass` Posts the same variable twice and expects the later value to replace the earlier one.
- `cache_config_pass` Uses `--cache` with a generated mission-file `ProcessConfig = uPokeDB` block and expects cached pokes to publish.
- `mission_file_connection_pass` Connects from `targ_shoreside.moos` instead of explicit host/port and expects the poke to arrive.
- `quiet_mode_pass` Uses `--quiet` and expects the poke to succeed without relying on terminal output.
- `negative_double_pass` Posts a negative floating-point value and expects the sign and magnitude to survive.
- `boolean_true_pass` Posts a boolean-looking value and expects mission evaluation to treat it as true.
- `repeated_batch_pass` Sends two sequential `uPokeDB` commands and expects the second command to own the final value.
- `mixed_numeric_string_batch_pass` Posts a numeric and a forced string value together, checking both forms in one run.
- `cache_string_numeric_pass` Uses cached numeric and string pokes together to exercise cache parsing for both value forms.
- `host_alias_args_pass` Uses `host=` and `port=` aliases instead of `--host=` and `--port=` and expects the poke to arrive.
- `server_alias_args_pass` Uses `serverhost=` and `serverport=` aliases and expects the poke to arrive.
- `server_underscore_args_pass` Uses `server_host=` and `server_port=` aliases and expects the poke to arrive.
- `quiet_short_alias_pass` Uses the short `-q` quiet option and expects the poke to succeed.
- `cache_short_alias_pass` Uses the short `-c` cache option and expects configured cached pokes to publish.
- `zero_double_pass` Posts numeric zero and expects the value to be treated as an intentional double rather than missing data.
- `scientific_literal_pass` Posts a scientific-notation numeric literal and expects the DB value to match.
- `large_double_pass` Posts a large floating-point value and expects the magnitude to survive.
- `cache_forced_string_pass` Uses cached `:=` syntax for a numeric-looking string and expects leading zeroes to survive.
- `cache_boolean_pass` Uses cached boolean-looking data and expects mission evaluation to see a true value.
- `utc_macro_pass` Posts `@UTC` and expects uPokeDB to replace it with a positive MOOS time value.
- `moostime_macro_pass` Posts `@MOOSTIME` and expects uPokeDB to replace it with a positive MOOS time value.
- `forced_utc_string_pass` Posts `@UTC` with forced-string syntax and expects the macro text to remain literal.
- `duplicate_same_invocation_pass` Posts the same variable twice in one command and expects the later value to win.
- `cache_duplicate_first_wins_pass` Defines the same cached poke twice and captures the current first-value result.
- `cache_cli_mixed_pass` Combines cached pokes with a command-line poke and expects both sources to publish.

## Run

```sh
./zlaunch.sh --jobs=4 --port_base=15000 --max_time=30 10
```

Run one inspectable case:

```sh
./zlaunch.sh --case=cache_config_pass --port_base=15000 --max_time=30 10
```

## Launcher Contract

Every path, including `--jobs=1` and single-case runs, uses an isolated copy of
the stem. The launcher requires Bash 5.1 or newer and uses rolling refill for
`--jobs`. Cases receive stride-30 port blocks with MOOSDB at `+0` and a
reserved shoreside pShare slot at `+15`; this mission does not launch pShare.

The harness writes each case's explicit `pMissionEval` conditions into the
copied `case_eval.moos`, starts the stem through its `zlaunch.sh`, and invokes
the case-specific `uPokeDB` command after MOOSDB startup. pMissionEval remains
the sole owner of ordinary pass/fail grading. The previous `EXPECTED=pass`,
result-token, absent-token, and numeric-threshold comparisons were removed
because they duplicated the same pMissionEval conditions.

The copied stem must produce exactly one row with exactly one
`grade=pass|fail`. Results are aggregated once in declared case order.
Preparation, stimulus, launch, malformed-result, missing-result, and teardown
failures use runner-owned failure rows. Ordinary pMissionEval failures remain
mission-owned. `uPokeDB` stimulus commands are bounded by the existing
`--max_time` infrastructure ceiling so a missing MOOSDB cannot hang a worker.

`--keep_workdirs` retains the invocation under `.harness_runs` for generated
target, evaluator config, cache fixture, result, and `.alog` inspection.

## Migration Validation

The legacy batch-wave runner passed three complete two-job matrices in 65.16,
66.18, and 66.58 seconds, for a 65.97-second mean. Its clean serial baseline
passed 28/28 in 121.64 seconds. One separate serial attempt interrupted by an
external `ktm` after twenty passing rows was discarded from all statistics.

The migrated rolling runner passed three complete matrices in 84, 85, and 82
seconds, for an 83.67-second mean. The isolated serial matrix passed 28/28 in
163 seconds. A final checkout-state rolling run also passed 28/28 in 82
seconds after the bounded-stimulus failure path was added. The increase comes
from using the standard `xlaunch.sh` lifecycle,
including its two-second shutdown interval, which the legacy harness bypassed.
The case conditions and `uPokeDB` command contracts are unchanged.
