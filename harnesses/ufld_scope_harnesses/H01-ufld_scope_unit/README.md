# H01-ufld_scope_unit

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

- `appcast_table_pass` Verifies scoped node, speed, and odometry fields appear in the appcast table.
- `alias_headers_pass` Verifies configured field aliases are used as appcast table headers.
- `update_replaces_same_key_pass` Verifies a later posting for the same key replaces the displayed row value.
- `multi_vehicle_rows_pass` Verifies separate key values produce separate appcast rows.
- `invalid_scope_ignored_pass` Verifies an invalid scope line is ignored while a later valid scope still builds the table.
- `missing_field_blank_cell_pass` Verifies a missing scoped field leaves an empty cell while preserving the keyed row.
- `missing_key_blank_row_pass` Verifies a missing key value produces a blank-key row rather than a named vehicle row.
- `later_missing_field_replaces_value_pass` Verifies a later same-key posting with a missing field replaces an earlier populated value with a blank cell.
