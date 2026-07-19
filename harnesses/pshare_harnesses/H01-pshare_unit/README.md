# H01 pShare Unit Harness

Logging is minimal by default in both communities. Use `--log=full` for the
whole matrix or with `--case=NAME` for one diagnostic case. The CLI-output
case keeps separate minimal and full ANTLER patches so its extra pShare
arguments never reintroduce `pLogger` in minimal mode.

This harness runs a pair of local MOOS communities and verifies that `pShare`
routes controlled mail from the peer community into the shoreside community.
The mission owns the pass/fail grade; the harness only varies the pShare
configuration and launch ports.

## Cases

- `pshare_direct_route_pass`
  Routes `ROUTE_TEST=alpha` by unicast to destination `ROUTE_TEST_RX`; passes
  when shoreside receives `ROUTE_TEST_RX=alpha`.
- `pshare_rename_route_pass`
  Routes `ROUTE_TEST=bravo` to renamed destination `ROUTE_RENAMED`; passes when
  shoreside receives `ROUTE_RENAMED=bravo`.
- `pshare_max_shares_one_pass`
  Configures `max_shares=1` and posts `LIMITED_SRC=first` then `second` four
  seconds later; passes when `LIMITED_RX` remains `first` at evaluation.
- `pshare_wildcard_route_pass`
  Routes `src_name=WILD_*` and posts `WILD_SRC=wildcard`; passes when shoreside
  receives the same variable name and value.
- `pshare_duration_expiry_pass`
  Configures `duration=3`, posts `DURATION_SRC=first` at two seconds and
  `second` at eight, then evaluates at eleven; passes when `DURATION_RX`
  remains `first` after route expiry.
- `pshare_shorthand_route_pass`
  Configures shorthand route `SHORT_SRC->SHORT_RX` and posts value `short`;
  passes when shoreside receives `SHORT_RX=short`.
- `pshare_multi_dest_route_pass`
  Uses one shorthand route list to fan `MULTI_SRC=fanout` to `MULTI_A` and
  `MULTI_B`; passes when both destination variables equal `fanout`.
- `pshare_frequency_throttle_pass`
  Configures `frequency=0.25` Hz and posts `FREQ_SRC=first` then `second` one
  second later; passes when `FREQ_RX` retains `first`.
- `pshare_cli_output_route_pass`
  Launches pShare with command-line route `-o=CLI_SRC->CLI_RX` and posts
  `CLI_SRC=cli_route`; passes when shoreside receives `CLI_RX=cli_route`.
- `pshare_wildcard_source_app_pass`
  Routes `APPWILD_*` only from `uTimerScript` and has that app post
  `APPWILD_SRC=appwild`; passes when shoreside receives the matching mail.
- `pshare_multicast_alias_pass`
  Routes `MCAST_SRC` to `MCAST_RX` over `multicast_17`; passes when shoreside
  receives `MCAST_RX=multicast` on the alias-derived multicast input.
- `pshare_wildcard_prefix_rename_pass`
  Routes `PFX_*` with destination prefix `RENAMED_` and posts
  `PFX_ONE=prefixed`; passes when shoreside receives
  `RENAMED_PFX_ONE=prefixed`.
- `pshare_wildcard_caret_rename_pass`
  Routes `CARET_*` with `dest_name=^` and posts `CARET_DELTA=caret`; passes
  when shoreside receives only the matched suffix as `DELTA=caret`.

Typical commands:

```sh
./zlaunch.sh --jobs=2 --port_base=11000 --max_time=25 10
./zlaunch.sh --case=pshare_wildcard_route_pass --port_base=11000 --max_time=25 10
```

Every selected case runs from its own mission copy and four-port block. The
shoreside and peer communities receive distinct MOOSDB and pShare ports.
`--jobs=N` uses rolling scheduling, starting the next pending case whenever an
active case finishes. Use `--keep_workdirs` to retain generated targets,
sidecars, logs, and result rows for inspection.

Latest validation:

- July 15, 2026
- three `13/13` rolling passes with `--jobs=3 --max_time=25 10`
- one `13/13` isolated serial pass with `--jobs=1 --max_time=25 10`
- retained inspection confirmed 26 distinct-port targets, 24 intended
  sidecars, one mission-owned result row per case, and no remaining listeners
