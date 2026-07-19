# H01-pechovar_unit

Live `pEchoVar` harness for validating runtime echo and EFlipper publication
behavior against a deterministic single-community MOOSDB mission.

The harness copies the mission stem per case, assigns isolated port ranges,
writes case-specific `pEchoVar`, `uTimerScript`, and `pMissionEval` blocks, and
uses rolling `--jobs` scheduling. Ordinary scalar and string echoes are graded
directly by pMissionEval. Targeted `.alog` checks remain only where current
state cannot express the contract: exact serialized EFlipper payloads, absence
of a publication, and exact publication counts. Those checks stop at the first
`MISSION_EVALUATED=true` timestamp, so shutdown activity is outside the test
window.

Logging defaults to the six-variable grading allowlist. Use `--log=full` for
the pre-migration diagnostic logger; the option applies to every selected case.

The evaluation-ready event occurs at mission time 2.0. The previous 1.5 value
occasionally allowed pMissionEval to grade immediately before pEchoVar
published a value; no source event, expected value, mapping, or case assertion
changed.

```sh
./zlaunch.sh --jobs=4 --port_base=17600 10
./zlaunch.sh --case=flip_filter_suppresses_pass --port_base=17600 10
```

After the high-confidence coverage review, the current 34-case matrix passed
34/34 with four jobs in 54 seconds. Mutations removing latest-only behavior,
adding an unwanted mapped component, changing an empty value, making an alias
valid, and swapping dual-flipper destinations were each rejected by the new
count or variable-bound exact-publication checks.

## Cases

- `numeric_echo_pass`: Maps `ECHO_SRC_NUM -> ECHO_OUT_NUM` and posts numeric `42.5`, testing preservation of numeric mail; passes when pMissionEval observes `ECHO_OUT_NUM=42.5`.
- `string_echo_pass`: Maps `ECHO_SRC_STR -> ECHO_OUT_STR` and posts string `ready`, testing preservation of string mail without numeric coercion; passes when pMissionEval observes `ECHO_OUT_STR=ready`.
- `bool_switch_true_pass`: Maps `ECHO_SRC_BOOL !-> ECHO_OUT_BOOL` and posts lowercase `true`, testing boolean inversion with case preservation; passes when pMissionEval observes lowercase `false`.
- `bool_switch_mixed_case_pass`: Posts `True` through a `!->` mapping, testing the title-case boolean branch; passes when pMissionEval observes `ECHO_OUT_BOOL=False`.
- `bool_switch_upper_false_pass`: Posts uppercase `FALSE` through a `!->` mapping, testing the uppercase boolean branch; passes when pMissionEval observes `ECHO_OUT_BOOL=TRUE`.
- `bool_switch_numeric_passthrough_pass`: Posts numeric `7` through a `!->` mapping, testing that boolean inversion applies only to string mail; passes when pMissionEval observes unchanged numeric `ECHO_OUT_NUM=7`.
- `bool_switch_nonbool_passthrough_pass`: Posts the non-boolean string `maybe` through a `!->` mapping, testing string passthrough outside recognized boolean spellings; passes when pMissionEval observes `ECHO_OUT_BOOL=maybe`.
- `multi_target_echo_pass`: Maps one `ECHO_SRC_MULTI=alpha` publication to both `ECHO_OUT_A` and `ECHO_OUT_B`, testing one-to-many fan-out; passes when pMissionEval observes `alpha` on both targets.
- `acyclic_chain_echo_pass`: Configures `ECHO_SRC_STR -> ECHO_OUT_A -> ECHO_OUT_B` and posts `chain`, testing propagation through a two-edge acyclic graph; passes when both downstream variables equal `chain`.
- `duplicate_mapping_single_post_pass`: Declares `ECHO_SRC_STR -> ECHO_OUT_STR` twice and posts `dedupe`, testing duplicate-pair removal during configuration; passes when the value is correct and the pre-evaluation logs contain exactly one `ECHO_OUT_STR` publication.
- `default_repeated_source_posts_each_pass`: Leaves `echo_latest_only` false and posts `first` then `second` in one MOOS mail batch, testing release of every queued update; passes when the final value is `second` and the logs contain exactly two `ECHO_OUT_LATEST` publications.
- `latest_only_repeated_source_pass`: Sets `echo_latest_only=true` and posts `first` then `second` in the same MOOS mail batch, testing replacement of the older held message before release; passes when final `ECHO_OUT_LATEST=second` is published exactly once.
- `flip_component_pass`: Maps `x` and `y` from `x=10,y=20,type=redeploy` to `xcenter` and `ycenter` with `#` as the destination separator; passes when the pre-evaluation logs contain exact payload `xcenter=10#ycenter=20`.
- `flip_explicit_param_syntax_pass`: Configures `filter = type == redeploy` and `component = x -> xcenter` instead of shorthand entries, testing explicit EFlipper parameter syntax; passes when input `type=redeploy,x=18` produces `xcenter=18`.
- `flip_explicit_filter_suppresses_pass`: Uses the same explicit `filter =` and `component =` syntax but posts `type=loiter,x=18`, proving the explicit filter controls output rather than merely parsing; passes when no `FLIP_OUT` publication occurs.
- `flip_filter_match_pass`: Filters on `type == redeploy`, maps `x` and `y`, and posts a matching payload; passes when the logs contain exact `xcenter=11#ycenter=21` output.
- `flip_multiple_filters_match_pass`: Requires both `type == redeploy` and `mode == survey` before mapping `x` and `y`; passes when the matching input produces exact `xcenter=19,ycenter=29`.
- `flip_filter_case_insensitive_match_pass`: Configures lowercase `type == redeploy` but posts uppercase `TYPE=REDEPLOY`, testing case-insensitive filter keys and values; passes when the mapped output is exactly `xcenter=14,ycenter=24`.
- `flip_missing_filter_allows_pass`: Configures `type == redeploy` but posts only `x=12,y=22`, testing the rule that an absent filtered field does not reject the message; passes when exact `xcenter=12,ycenter=22` is published.
- `flip_filter_suppresses_pass`: Configures `type == redeploy` and posts mismatched `type=loiter`, testing single-filter rejection; passes when no `FLIP_OUT` publication occurs before evaluation.
- `flip_multiple_filters_suppress_pass`: Posts matching `type=redeploy` but mismatched `mode=loiter` against two filters, testing that any present mismatch rejects the message; passes when no `FLIP_OUT` publication occurs.
- `flip_custom_separator_pass`: Parses `mode=survey|x=5|y=-2` with source separator `|` and serializes mapped fields with destination separator `@`; passes when the logs contain exact `xpos=5@ypos=-2`.
- `flip_component_case_sensitive_pass`: Maps uppercase component `X` from input containing both `x=15` and `X=16`, testing case-sensitive component lookup; passes when `xcenter=16` appears and `xcenter=15` does not.
- `flip_unmapped_component_omitted_pass`: Maps only `x` from `x=8,z=99`, testing omission of an unmapped source component; passes when the complete `FLIP_OUT` value is exactly `xcenter=8`.
- `flip_no_mapped_fields_no_output_pass`: Configures a valid `x -> xcenter` component but posts only `y=33`, testing empty flip results when no source field is mapped; passes when no `FLIP_OUT` publication occurs.
- `flip_duplicate_component_repeats_pass`: Posts `x=10,x=11,y=20` through `x -> xcenter` and `y -> ycenter`, testing preservation and order of duplicate source components; passes when the exact payload is `xcenter=10,xcenter=11,ycenter=20`.
- `flip_empty_value_pass`: Maps source component `x` from the payload `x=`, testing preservation of an empty component value; passes when the complete `FLIP_OUT` value is exactly `xcenter=`.
- `cycle_guard_no_echo_pass`: Configures the two-node cycle `CYCLE_A -> CYCLE_B -> CYCLE_A` and posts `CYCLE_A=one`, testing startup cycle rejection; passes when no `CYCLE_B` publication occurs.
- `self_cycle_guard_no_echo_pass`: Configures `CYCLE_A -> CYCLE_A` and posts `CYCLE_A=self`, testing direct self-cycle rejection; passes when the logs contain exactly one `CYCLE_A` publication, the original source post only.
- `indirect_cycle_blocks_all_pass`: Adds a three-node cycle plus an unrelated valid `ECHO_SRC_STR -> ECHO_OUT_STR` mapping, testing the app-wide guard after any cycle is detected; passes when neither cyclic `CYCLE_B` nor unrelated `ECHO_OUT_STR` is published.
- `invalid_flip_no_output_pass`: Configures source and destination variables but no component mapping, testing rejection of an incomplete EFlipper; passes when source mail arrives without any `FLIP_OUT` publication.
- `invalid_flip_alias_no_output_pass`: Configures `FLIP_SRC` as both source and destination with an `x -> xcenter` mapping, testing source/destination alias rejection; passes when the logs contain exactly the one original `FLIP_SRC=x=17` post and no `FLIP_OUT` publication.
- `shared_source_dual_flip_pass`: Gives two named EFlippers the same `FLIP_SRC=x=31,y=41`, mapping `x` to `FLIP_OUT` and `y` to `ECHO_OUT_STR`; passes when the exact bound publications are `FLIP_OUT=xcenter=31` and `ECHO_OUT_STR=ypos=41`.
- `dual_flip_independent_pass`: Gives two named EFlippers separate sources and destinations, posting `x=44` to one and `word=bravo` to the other; passes when the exact bound publications are `FLIP_OUT=xcenter=44` and `ECHO_OUT_STR=word_out=bravo`.
