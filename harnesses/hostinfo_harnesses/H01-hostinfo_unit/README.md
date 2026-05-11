# H01-hostinfo_unit

Patch-driven harness for
[`missions/hostinfo_missions/hostinfo_unit`](../../../missions/hostinfo_missions/hostinfo_unit).
It isolates `pHostInfo` host and route reporting in a single shoreside
community.

## Current Matrix

- `hostinfo_forced_ip_pass` Baseline forced host IP case; expects `PHI_HOST_IP=127.0.0.7` and the generated MOOSDB port.
- `hostinfo_forced_alt_ip_pass` Overrides the forced host IP and expects the alternate address and MOOSDB port to be reported.
- `hostinfo_pshare_route_pass` Adds one UDP pShare route and exact-checks the resulting `PHI_HOST_INFO` route payload.
- `hostinfo_pshare_multi_route_pass` Adds two UDP pShare routes and exact-checks both normalized route entries in `PHI_HOST_INFO`.
- `hostinfo_invalid_pshare_route_pass` Adds an invalid pShare route and expects it to be omitted from the host-info payload.
- `hostinfo_pshare_mixed_routes_pass` Mixes valid and invalid route specs and expects only the valid UDP route to survive normalization.
- `hostinfo_pshare_nonudp_route_pass` Adds a non-UDP pShare route token and expects that token to be preserved in the route list.

## Run

```sh
./zlaunch.sh --jobs=3 --port_base=11000 --max_time=40 10
```

Run one inspectable case:

```sh
./zlaunch.sh --case=hostinfo_pshare_route_pass --port_base=11000 --max_time=40 10
```
