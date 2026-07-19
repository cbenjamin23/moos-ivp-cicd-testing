# H01-ufld_scope_unit

This is an intentionally non-visual unit harness. `--gui` is unsupported
because the stem has no `pMarineViewer` configuration.

Live `uFldScope` harness for appcast table construction from scoped MOOS variables.

Each mission records its normal pMissionEval result. Because `uFldScope` exposes
the rendered table only in its AppCast report, the harness supplements that
result by checking the final `uFldScope` AppCast snapshot for the documented
table contract.

```sh
./zlaunch.sh --jobs=4 --port_base=5000 10
./zlaunch.sh --case=appcast_table_pass --port_base=5000 10
```

## Cases

- `appcast_table_pass` Scopes `MODE` from `NODE_REPORT`, `avg_spd` from `UPC_SPEED_REPORT`, and `trip_dist` from `UPC_ODOMETRY_REPORT` under key `alpha`; passes when the final `uFldScope` AppCast contains `alpha`, `survey`, `2.5`, and `12`.
- `alias_headers_pass` Renames the three scoped columns to `nav_mode`, `speed`, and `trip`; passes when all three tokens appear in the final AppCast and the original `trip_dist` header does not.
- `update_replaces_same_key_pass` Posts `MODE=survey` and then `MODE=return` for key `alpha`; passes when the final AppCast contains `return` and no longer contains `survey`.
- `multi_vehicle_rows_pass` Posts Alpha in `survey` mode and Bravo in `station` mode under the same `NAME` key definition; passes when the final AppCast contains `alpha`, `bravo`, and `station`.
- `invalid_scope_ignored_pass` Configures an incomplete `scope=var=NODE_REPORT,key=NAME` line before a valid `MODE` scope; passes when the final AppCast contains the `MODE` token from the valid definition.
- `missing_field_blank_cell_pass` Posts a keyed Alpha node report without the scoped `MODE` field; passes when the final AppCast contains the `alpha` row key and `MODE` header but not the baseline value `survey`.
- `missing_key_blank_row_pass` Posts `MODE=survey` without the scoped `NAME` key; passes when the final AppCast contains `MODE` and `survey` but does not contain `alpha`.
- `later_missing_field_replaces_value_pass` Posts `MODE=survey` for Alpha and then a second Alpha report with no `MODE` field; passes when the final AppCast still contains `alpha` but no longer contains `survey`.

Logging is minimal by default. It retains only asynchronous `APPCAST` evidence
because the harness grades the latest `uFldScope` table after the mission-owned
verdict passes. Use `--log=full` for the complete matrix, or combine it with
`--case=NAME` for one fully logged diagnostic case.
