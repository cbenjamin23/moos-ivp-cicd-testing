# H04-max_depth_motion

Patch-driven moving correctness harness for `BHV_MaxDepth`.

The harness reuses
`missions/depth_behavior_missions/depth_behavior_motion`. Cases command unsafe
deeper targets while max-depth constraints should keep the UUV at or above the
allowed depth boundary.

## Current Matrix

- `max_depth_guard_pass` Commands an unsafe deeper target while `BHV_MaxDepth` constrains the vehicle near the guard depth.
- `max_depth_zero_guard_pass` Commands a dive while the guard clamps the allowed depth to the surface boundary.
- `max_depth_negative_clip_pass` Supplies a negative guard depth and verifies clipping to the surface boundary.
- `max_depth_tight_tolerance_pass` Uses a tight tolerance and verifies the vehicle remains near the constrained depth.
- `max_depth_basewidth_alias_pass` Uses the `basewidth` parameter alias and verifies it constrains the over-depth command like `tolerance`.
- `max_depth_no_slack_var_pass` Omits the optional slack telemetry variable and verifies the guard still constrains maximum depth.
- `max_depth_unconstrained_shallow_pass` Commands a target shallower than the max-depth limit and verifies the guard does not interfere.
- `max_depth_zero_tolerance_pass` Uses exact zero tolerance and verifies the max-depth guard remains stable at the boundary.
- `max_depth_bad_config_fail` Supplies malformed max-depth config and expects the normal good-case verdict to fail.
- `max_depth_bad_tolerance_fail` Supplies malformed tolerance config and expects the normal good-case verdict to fail.
- `max_depth_bad_basewidth_fail` Supplies malformed basewidth config and expects the normal good-case verdict to fail.
- `max_depth_bad_slack_var_fail` Supplies a whitespace-bearing depth-slack variable name and expects the normal good-case verdict to fail.
- `max_depth_bad_ascent_field_fail` Supplies unsupported ascent fields from legacy mission style and expects helm configuration failure.
- `max_depth_missing_nav_depth_fail` Removes usable ownship depth mail and expects the max-depth verdict to fail.
- `max_depth_domain_missing_fail` Removes the helm depth domain and expects the max-depth verdict to fail.

## Running

```bash
./zlaunch.sh --case=max_depth_guard_pass 10
./zlaunch.sh --jobs=3 --port_base=44000 10
./zlaunch.sh --just_make 10
```

The wrapper supports `--jobs` and `--port_base` for isolated grouped local and
CI runs.
