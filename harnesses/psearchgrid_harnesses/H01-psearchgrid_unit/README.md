# H01-psearchgrid_unit

Live `pSearchGrid` harness for grid and delta publication behavior.

```sh
./zlaunch.sh --jobs=4 --port_base=9000 10
./zlaunch.sh --case=node_local_delta_pass --port_base=9000 10
```

The launcher requires Bash 5.1 or newer, gives every selected case an
isolated mission copy and port block, and refills rolling `--jobs` slots as
cases finish. `pMissionEval` owns each case's variable-presence or
variable-absence verdict. The launcher supplements that verdict only where
the contract depends on structured grid payloads or publication history, and
it considers only evidence posted no later than `MISSION_EVALUATED=true`.

## Cases

- `initial_grid_publish_pass` Verifies the configured convex grid is published on the configured base grid variable.
- `node_local_delta_pass` Verifies a `NODE_REPORT_LOCAL` inside the grid produces a delta update.
- `node_global_delta_pass` Verifies a global `NODE_REPORT` inside the grid produces a delta update.
- `boundary_origin_delta_pass` Verifies a report exactly on the grid origin boundary still intersects the grid.
- `outside_grid_no_delta_pass` Verifies a report outside the grid does not produce a delta update.
- `negative_outside_no_delta_pass` Verifies a report just outside the negative side of the grid does not produce a delta update.
- `full_grid_report_pass` Verifies `report_deltas=false` publishes full grid state after a report.
- `custom_var_label_pass` Verifies custom grid variable and label settings affect the output channel and delta payload.
- `lowercase_var_name_pass` Verifies a lowercase configured grid variable is normalized to uppercase output mail.
- `custom_full_grid_var_pass` Verifies custom grid variable settings also apply to full-grid publication mode.
- `invalid_report_deltas_default_delta_pass` Verifies an invalid `report_deltas` value leaves the app in its default delta-reporting mode.
- `reset_grid_clears_pass` Verifies `PSG_RESET_GRID` clears a previously incremented full-grid cell value.
- `reset_false_payload_clears_pass` Verifies `PSG_RESET_GRID` clears the grid based on mail presence regardless of payload value.
- `two_cell_delta_pass` Verifies two simultaneous reports in different cells produce delta publication evidence.
- `repeated_cell_delta_pass` Verifies two same-cycle reports in one cell are accumulated into one two-count delta.
- `sequential_delta_cleared_pass` Verifies separated reports in one cell produce separate one-count deltas instead of retaining stale delta state.
- `match_community_allows_pass` Verifies a matching community filter permits incoming reports.
- `match_list_allows_pass` Verifies brace/comma-list match filters permit a listed community.
- `match_community_blocks_pass` Verifies a nonmatching community filter blocks incoming reports.
- `ignore_other_allows_pass` Verifies an ignore filter for another community still permits incoming reports.
- `ignore_community_blocks_pass` Verifies an ignored community filter blocks incoming reports.
- `ignore_list_blocks_pass` Verifies brace/comma-list ignore filters block a listed community.
- `malformed_report_absent_pass` Verifies malformed node reports do not produce delta updates.
