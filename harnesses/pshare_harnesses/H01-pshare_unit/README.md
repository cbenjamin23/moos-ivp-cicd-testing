# H01 pShare Unit Harness

This harness runs a pair of local MOOS communities and verifies that `pShare`
routes controlled mail from the peer community into the shoreside community.
The mission owns the pass/fail grade; the harness only varies the pShare
configuration and launch ports.

## Cases

- `pshare_direct_route_pass` Shares one variable directly and expects the receiver to grade the routed value.
- `pshare_rename_route_pass` Shares one variable under a different destination name and expects only the renamed destination to be graded.
- `pshare_max_shares_one_pass` Configures `max_shares=1`, publishes two values, and expects the receiver to retain the first routed value.
- `pshare_wildcard_route_pass` Routes a wildcard source pattern and expects the matching sender mail to arrive under its original variable name.
- `pshare_duration_expiry_pass` Configures a short route duration and expects mail after expiry not to replace the first routed value.
- `pshare_shorthand_route_pass` Uses pShare shorthand output syntax and expects the renamed destination mail to arrive.
- `pshare_multi_dest_route_pass` Fans one source variable out to two destination names and expects both receiver variables to arrive.
- `pshare_frequency_throttle_pass` Configures a low share frequency, publishes two quick updates, and expects the receiver to retain the first value.
- `pshare_cli_output_route_pass` Starts pShare with a command-line `-o` route and expects the routed destination mail to arrive.
- `pshare_wildcard_source_app_pass` Routes a wildcard variable pattern qualified by source app and expects the matching timer-script mail to arrive.
- `pshare_multicast_alias_pass` Routes mail through a `multicast_N` channel alias and expects the receiver to grade the multicast delivery.
- `pshare_wildcard_prefix_rename_pass` Routes a wildcard source pattern with a destination prefix and expects the original variable name appended to that prefix.
- `pshare_wildcard_caret_rename_pass` Routes a single-star wildcard source pattern with `dest_name=^` and expects only the wildcard-matched substring as the receiver variable.

Typical commands:

```sh
./zlaunch.sh --jobs=2 --port_base=11000 --max_time=25 10
./zlaunch.sh --case=pshare_wildcard_route_pass --port_base=11000 --max_time=25 10
```
