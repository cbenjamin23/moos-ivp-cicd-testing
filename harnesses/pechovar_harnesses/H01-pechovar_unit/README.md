# H01-pechovar_unit

Live `pEchoVar` harness for validating runtime echo and EFlipper publication
behavior against a deterministic single-community MOOSDB mission.

The harness copies the mission stem per case, assigns isolated port ranges,
writes case-specific `pEchoVar`, `uTimerScript`, and `pMissionEval` blocks, and
then grades the mission result plus targeted alog evidence for suppression and
latest-only cases.

```sh
./zlaunch.sh --jobs=4 --port_base=17600 10
./zlaunch.sh --case=flip_filter_suppresses_pass --port_base=17600 10
```

## Cases

- `numeric_echo_pass`: Numeric source mail is echoed to the configured target variable.
- `string_echo_pass`: String source mail is echoed without numeric coercion.
- `bool_switch_true_pass`: `!->` mapping flips lowercase `true` to `false`.
- `bool_switch_mixed_case_pass`: `!->` mapping preserves the documented mixed-case boolean flip behavior.
- `bool_switch_upper_false_pass`: `!->` mapping flips uppercase `FALSE` to uppercase `TRUE`.
- `bool_switch_numeric_passthrough_pass`: `!->` leaves numeric source mail unchanged because boolean inversion only applies to strings.
- `bool_switch_nonbool_passthrough_pass`: `!->` leaves non-boolean string payloads unchanged.
- `multi_target_echo_pass`: One source variable fans out to two independent target variables.
- `acyclic_chain_echo_pass`: A non-cyclic echo chain can propagate source mail through two configured mappings.
- `duplicate_mapping_single_post_pass`: Duplicate `Echo` entries for the same source-target pair produce a single target publication.
- `default_repeated_source_posts_each_pass`: Repeated source mail publishes each update when `echo_latest_only` is left at its default.
- `latest_only_repeated_source_pass`: `echo_latest_only=true` accepts repeated source mail and the latest value is the final echoed output.
- `flip_component_pass`: EFlipper component mapping builds a destination payload from selected fields.
- `flip_explicit_param_syntax_pass`: EFlipper accepts explicit `filter =` and `component =` configuration syntax.
- `flip_filter_match_pass`: Matching EFlipper filters allow the mapped destination payload through.
- `flip_multiple_filters_match_pass`: Multiple EFlipper filters all match and allow the mapped destination payload through.
- `flip_filter_case_insensitive_match_pass`: EFlipper filters match source field names and values case-insensitively.
- `flip_missing_filter_allows_pass`: A configured filter does not suppress output when the filtered field is absent from the source payload.
- `flip_filter_suppresses_pass`: A mismatched EFlipper filter suppresses the destination publication.
- `flip_multiple_filters_suppress_pass`: A single mismatch among multiple EFlipper filters suppresses the destination publication.
- `flip_custom_separator_pass`: EFlipper honors custom source and destination separators.
- `flip_component_case_sensitive_pass`: EFlipper component mappings remain case-sensitive even when filters are case-insensitive.
- `flip_unmapped_component_omitted_pass`: Unmapped source fields are omitted while mapped fields still publish.
- `flip_no_mapped_fields_no_output_pass`: Source mail with no mapped components produces no destination publication.
- `flip_duplicate_component_repeats_pass`: Duplicate source fields produce repeated mapped fields in the destination payload.
- `flip_empty_value_pass`: Empty source component values are preserved in the mapped destination payload.
- `cycle_guard_no_echo_pass`: A detected echo cycle prevents cyclic echo output from being posted.
- `self_cycle_guard_no_echo_pass`: A direct source-to-self echo is rejected by the cycle guard.
- `indirect_cycle_blocks_all_pass`: A detected indirect cycle prevents all echo processing in the app instance.
- `invalid_flip_no_output_pass`: An incomplete EFlipper receives source mail but posts no destination payload.
- `invalid_flip_alias_no_output_pass`: An EFlipper that aliases source and destination variables is rejected and posts no mapped payload.
- `shared_source_dual_flip_pass`: Two named EFlippers can consume the same source mail and publish separate mapped outputs.
- `dual_flip_independent_pass`: Two named EFlippers with separate source variables publish independent destination payloads.
