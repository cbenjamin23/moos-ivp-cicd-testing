# H01-upokedb_unit

Patch-free harness for
[`missions/upokedb_missions/upokedb_unit`](../../../missions/upokedb_missions/upokedb_unit).
It launches a small MOOS community, runs `uPokeDB` against the live DB, and
lets `pMissionEval` grade the variables posted by `uPokeDB`.

## Current Matrix

- `numeric_direct_pass`: Runs `uPokeDB --quiet --host=localhost --port=<case-port>` with `POKE_NUM=42.5`, testing the ordinary long-form quiet, connection, and numeric-assignment path; passes when `POKE_NUM=42.5`.
- `string_forced_pass`: Posts `POKE_STR:=0012`, testing forced-string parsing of a numeric-looking token; passes when pMissionEval reports the exact four-character value `0012`.
- `overwrite_existing_pass`: Posts `POKE_OVER:=old`, waits 0.5 seconds, then posts `POKE_OVER:=new` in a second invocation, testing replacement across commands; passes when the final value is `new`.
- `cache_config_pass`: Generates a `ProcessConfig=uPokeDB` block containing cached `POKE_CACHE_NUM=88` and runs with `--cache`, testing numeric cache-file execution; passes when `POKE_CACHE_NUM=88`.
- `mission_file_connection_pass`: Passes `targ_shoreside.moos` instead of host/port flags and posts `POKE_FILE:=from_file`, testing connection discovery from a mission file; passes when `POKE_FILE=from_file`.
- `negative_double_pass`: Posts `POKE_NEG=-13.25`, testing signed floating-point parsing; passes when `POKE_NEG=-13.25`.
- `boolean_true_pass`: Posts `POKE_BOOL=true` with numeric assignment syntax, testing the boolean-looking value accepted by the evaluator; passes when `POKE_BOOL=true`.
- `mixed_numeric_string_batch_pass`: Posts `POKE_MIXED_NUM=31` and `POKE_MIXED_STR:=031` together, testing numeric parsing alongside preservation of a leading-zero string; passes when pMissionEval reports `31` and the exact three-character value `031`.
- `cache_string_numeric_pass`: Caches `POKE_CACHE_NUM=77` and `POKE_CACHE_STR:=seventy_seven` in one config block, testing mixed cache value forms; passes when both exact values publish.
- `host_alias_args_pass`: Connects with `host=localhost port=<case-port>` and posts `POKE_ALIAS:=alias_host`, testing the unprefixed host/port aliases; passes when `POKE_ALIAS=alias_host`.
- `server_alias_args_pass`: Connects with `serverhost=localhost serverport=<case-port>` and posts `POKE_SERVER_ALIAS:=server_alias`, testing the compact server aliases; passes when the exact value publishes.
- `server_underscore_args_pass`: Connects with `server_host=localhost server_port=<case-port>` and posts `POKE_SERVER_UNDERSCORE:=server_under`, testing the underscored server aliases; passes when the exact value publishes.
- `quiet_short_alias_pass`: Uses `-q` with explicit host/port flags and posts `POKE_QSHORT:=quiet_short`, testing the short quiet alias; passes when `POKE_QSHORT=quiet_short`.
- `cache_short_alias_pass`: Runs the generated cache file with `-c -q` and cached `POKE_CACHE_SHORT:=cache_short`, testing both short options; passes when the exact value publishes.
- `zero_double_pass`: Posts `POKE_ZERO=0`, testing that numeric zero is a present value rather than missing mail; passes when `POKE_ZERO=0`.
- `scientific_literal_pass`: Posts `POKE_SCI=1e-3`, testing scientific-notation input; passes when the evaluator matches `POKE_SCI=1e-3`.
- `large_double_pass`: Posts `POKE_BIG=123456.75`, testing preservation of a large fractional value; passes when `POKE_BIG=123456.75`.
- `cache_forced_string_pass`: Caches `POKE_CACHE_FORCED:=0007`, testing forced-string syntax inside a `poke=` config entry; passes when pMissionEval reports the exact four-character value `0007`.
- `cache_boolean_pass`: Caches `POKE_CACHE_BOOL=true`, testing boolean-looking cache data; passes when `POKE_CACHE_BOOL=true`.
- `time_macros_alias_pass`: Posts `TIME_BEFORE=@MOOSTIME`, `POKE_UTC=@UTC`, `POKE_MOOSTIME=@MOOSTIME`, and `TIME_AFTER=@MOOSTIME` in one invocation, testing the source-defined `@UTC` alias and both macro spellings together; passes when all four values fall in the broad `10^9` to `10^11` MOOS-time range, rejecting unchanged tokens and arbitrary small constants without depending on run timing.
- `forced_utc_string_pass`: Posts `POKE_UTC_STRING:=@UTC`, testing that forced-string syntax suppresses macro expansion; passes when the value remains the literal `@UTC`.
- `duplicate_same_invocation_pass`: Supplies `POKE_DUP:=first POKE_DUP:=second` in one command, testing duplicate command-line arguments; passes when the later value `second` wins.
- `cache_cli_mixed_pass`: Caches `POKE_CACHE_MIX:=from_cache` while adding command-line `POKE_CLI_MIX:=from_cli` to the same `--cache` invocation, testing mixed stimulus sources; passes when both exact values publish.

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
the sole owner of ordinary pass/fail grading. Three numeric-looking forced
strings additionally require the exact pMissionEval report field because
`LogicCondition` deliberately coerces string/number comparisons and therefore
cannot distinguish `031` from numeric `31`. The previous general-purpose
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

The harness defaults to `--log=minimal`, which omits `pLogger` because logs
are not grading inputs. Use `--log=full` for the complete matrix, or combine
it with `--case=<name>` for one fully logged case.

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

After the high-confidence coverage review consolidated redundant cases and
strengthened the time-macro and forced-string checks, the current 23-case
matrix passed 23/23 with two jobs in 57 seconds. Mutations replacing `@UTC`
with `1`, changing forced string `:=031` to numeric `=031`, and withholding the
second overwrite value were each rejected by the intended evaluator layer.
