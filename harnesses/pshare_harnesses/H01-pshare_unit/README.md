# H01 pShare Unit Harness

Logging is minimal by default in both communities. Use `--log=full` for the
whole matrix or with `--case=NAME` for one diagnostic case. The CLI-output and
source-app cases keep separate minimal and full ANTLER patches so their extra
process configuration never reintroduces `pLogger` in minimal mode.

This harness runs a pair of local MOOS communities and verifies that `pShare`
routes controlled mail from the peer community into the shoreside community.
The mission owns the pass/fail grade; the harness only varies the pShare
configuration and launch ports.

## Cases

- `pshare_direct_route_pass`
  Routes `ROUTE_TEST=alpha` by unicast to destination `ROUTE_TEST_RX`; passes
  when shoreside receives `ROUTE_TEST_RX=alpha`.
- `pshare_rename_route_pass`
  Sends `ROUTE_TEST=bravo` through explicit `dest_name=ROUTE_RENAMED`, testing
  that pShare replaces rather than duplicates the source name; passes at the
  routed completion event when `ROUTE_RENAMED=bravo` and shoreside has received
  no `ROUTE_TEST` mail.
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
  Sends `SHORT_SRC=short` through shorthand route `SHORT_SRC->SHORT_RX`, testing
  its rename semantics; passes at the routed completion event when
  `SHORT_RX=short` and shoreside has received no `SHORT_SRC` mail.
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
  Has alias `uTimerBlocked` post `APPWILD_BLOCKED` before `uTimerScript` posts
  `APPWILD_ALLOWED` and `APPWILD_DONE` through
  `src_name=APPWILD_*:uTimerScript`; passes on `APPWILD_DONE` when the allowed
  mail arrived and no blocked-source mail did.
- `pshare_multicast_alias_pass`
  Routes `MCAST_SRC` to `MCAST_RX` over `multicast_17`; passes when shoreside
  receives `MCAST_RX=multicast` on the alias-derived multicast input.
- `pshare_wildcard_prefix_rename_pass`
  Sends `PFX_ONE=prefixed` through wildcard destination prefix `RENAMED_`,
  testing prefix construction without source-name duplication; passes at the
  routed completion event when `RENAMED_PFX_ONE=prefixed` and shoreside has
  received no `PFX_ONE` mail.
- `pshare_wildcard_caret_rename_pass`
  Sends `CARET_DELTA=caret` through `CARET_*` with `dest_name=^`, testing that
  the wildcard-matched suffix becomes the complete destination name; passes at
  the routed completion event when `DELTA=caret` and shoreside has received no
  `CARET_DELTA` mail.

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

- July 20, 2026
- `13/13` rolling passes in both minimal and full logging modes with
  `--jobs=3 --max_time=25 10`
- deliberate original-name injection and source-filter removal each produced a
  mission-owned failure while the expected destination mail still arrived
- retained full logs showed `APPWILD_BLOCKED` only in the sender community,
  while `APPWILD_ALLOWED` and `APPWILD_DONE` reached shoreside
