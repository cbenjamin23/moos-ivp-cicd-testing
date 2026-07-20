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

- `appcast_table_pass` Scopes `MODE`, aliased `avg_spd`, and `trip_dist` under key `alpha`; passes when the exact header is `VName,MODE,speed,trip_dist` and the sole row is `alpha,survey,2.5,12`.
- `alias_headers_pass` Renames the three scoped columns to `nav_mode`, `speed`, and `trip`; passes when those are the exact non-key headers and the sole row remains `alpha,survey,2.5,12`.
- `update_replaces_same_key_pass` Posts `MODE=survey` and then `MODE=return` for key `alpha`; passes when the sole row is `alpha,return` with blank speed and trip-distance cells.
- `multi_vehicle_rows_pass` Posts Alpha in `survey` mode and Bravo in `station` mode under the same `NAME` key definition; passes when the exact two rows are `alpha,survey` and `bravo,station`, each with blank speed and trip-distance cells.
- `invalid_scope_ignored_pass` Places incomplete `scope=var=NODE_REPORT,key=NAME` before a valid `MODE` scope; passes when the rendered table has exactly `VName,MODE` and the sole row `alpha,survey`, with no column from the invalid definition.
- `missing_field_blank_cell_pass` Posts a keyed Alpha node report without the scoped `MODE` field; passes when the exact four-column table contains one Alpha row with all three data cells blank.
- `missing_key_blank_row_pass` Posts `MODE=survey` without the scoped `NAME` key; passes when the exact `VName,MODE` table contains one row with a blank key and `survey` in the `MODE` cell.
- `later_missing_field_replaces_value_pass` Posts `MODE=survey` for Alpha and then a second Alpha report without `MODE`; passes when the exact four-column table retains the Alpha key but replaces all three data cells with blanks.

Logging is minimal by default. It retains only asynchronous `APPCAST` evidence
because the harness grades the latest `uFldScope` table after the mission-owned
verdict passes. Use `--log=full` for the complete matrix, or combine it with
`--case=NAME` for one fully logged diagnostic case.
