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

Logging defaults to the six grid/reset variables needed by these artifact
checks. Use `--log=full` for the original nine-variable diagnostic logger.

## Cases

- `initial_grid_publish_pass`: Configures a 20-by-20 grid with 10-meter cells, variable `x:0`, and label `psg`; passes when `VIEW_GRID` history contains the polygon, cell size, cell variable, and label tokens.
- `node_local_delta_pass`: Posts `NODE_REPORT_LOCAL` at `(5,5)`, testing local-report accumulation in cell 0; passes when `VIEW_GRID_DELTA` contains label prefix `psg@` and an `x,1` increment.
- `node_global_delta_pass`: Posts global `NODE_REPORT` at `(15,5)`, testing the global report subscription; passes when `VIEW_GRID_DELTA` contains `psg@` and an `x,1` increment.
- `boundary_origin_delta_pass`: Posts a local report exactly at grid origin `(0,0)`, testing inclusive boundary containment; passes when a `psg@` delta with `x,1` is published.
- `outside_grid_no_delta_pass`: Posts a local report at `(50,50)`, testing positive-side rejection outside the polygon; passes when no `VIEW_GRID_DELTA` is published before evaluation.
- `negative_outside_no_delta_pass`: Posts a local report at `(-1,-1)`, testing negative-side rejection just beyond the origin; passes when no `VIEW_GRID_DELTA` is published.
- `full_grid_report_pass`: Sets `report_deltas=false` and posts at `(5,5)`, testing full-grid updates instead of delta mail; passes when `VIEW_GRID` contains a cell record and exact `cell=0:x:1`.
- `custom_var_label_pass`: Sets `grid_var_name=SEARCH_GRID` and `grid_label=custom_psg`, then posts at `(5,5)`; passes when base mail uses `SEARCH_GRID`, delta mail uses `SEARCH_GRID_DELTA` with `custom_psg@` and `x,1`, and `VIEW_GRID_DELTA` is absent.
- `lowercase_var_name_pass`: Configures lowercase `grid_var_name=search_grid` and label `lower_psg`, testing output-variable normalization; passes when uppercase `SEARCH_GRID`/`SEARCH_GRID_DELTA` mail appears and `VIEW_GRID_DELTA` does not.
- `custom_full_grid_var_pass`: Combines lowercase `search_grid` with `report_deltas=false`, testing custom naming in full-grid mode; passes when `SEARCH_GRID` contains `cell=0:x:1` and no `VIEW_GRID` mail is published.
- `invalid_report_deltas_default_delta_pass`: Configures invalid `report_deltas=maybe`, testing fallback to the default delta mode; passes when `VIEW_GRID_DELTA` contains `psg@` and `x,1`.
- `reset_grid_clears_pass`: In full-grid mode, increments cell 0 and then posts `PSG_RESET_GRID=true`; passes when `cell=0:x:1` existed before reset and is absent more than 0.2 seconds afterward.
- `reset_false_payload_clears_pass`: Repeats the reset sequence with `PSG_RESET_GRID=false`, testing trigger-on-mail rather than boolean value; passes with the same pre-reset `x:1` presence and post-reset absence.
- `two_cell_delta_pass`: Posts simultaneous local reports at `(5,5)` and `(15,5)`, intended to exercise two distinct cells; passes when delta history contains `psg@` and at least one `x,1` token.
- `repeated_cell_delta_pass`: Runs at AppTick 1 and posts two same-time reports at `(5,5)`, testing same-cycle accumulation; passes when one delta contains `x,2`.
- `sequential_delta_cleared_pass`: Posts to cell 0 at times 0.4 and 1.0, testing that published delta state is cleared between cycles; passes when at least two `x,1` delta tokens appear and no `x,2` token appears.
- `match_community_allows_pass`: Sets `match_name=shoreside` and posts a shoreside local report; passes when an `x,1` delta is published.
- `match_list_allows_pass`: Sets `match_name={vehicle,shoreside}`, testing brace/comma list parsing; passes when the listed shoreside report produces an `x,1` delta.
- `match_community_blocks_pass`: Sets `match_name=vehicle` and posts from shoreside, testing nonmatching-community rejection; passes when no delta is published.
- `ignore_other_allows_pass`: Sets `ignore_name=vehicle` and posts from shoreside, testing that an unrelated ignore value does not block mail; passes when an `x,1` delta is published.
- `ignore_community_blocks_pass`: Sets `ignore_name=shoreside`, testing direct community suppression; passes when no delta is published.
- `ignore_list_blocks_pass`: Sets `ignore_name={vehicle,shoreside}`, testing list-form suppression; passes when no delta is published for shoreside mail.
- `malformed_report_absent_pass`: Posts local `NAME=bad,X=5` without Y or remaining node-report fields, testing malformed-report rejection; passes when no delta is published.
