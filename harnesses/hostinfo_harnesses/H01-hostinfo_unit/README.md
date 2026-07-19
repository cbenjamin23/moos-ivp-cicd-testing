# H01-hostinfo_unit

Patch-driven harness for
[`missions/hostinfo_missions/hostinfo_unit`](../../../missions/hostinfo_missions/hostinfo_unit).
It isolates `pHostInfo` host and route reporting in a single shoreside
community.

## Current Matrix

- `hostinfo_forced_ip_pass`
  Starts `pHostInfo` with `default_hostip_force=127.0.0.7`; passes when
  `PHI_HOST_IP` equals that address and `PHI_HOST_PORT_DB` equals the case's
  generated MOOSDB port.
- `hostinfo_forced_alt_ip_pass`
  Changes `default_hostip_force` to `10.1.2.3`; passes when `PHI_HOST_IP`
  equals the alternate address and `PHI_HOST_PORT_DB` equals the generated
  MOOSDB port.
- `hostinfo_pshare_route_pass`
  Posts `PSHARE_INPUT_SUMMARY=127.0.0.1:11111:udp`; passes only when
  `PHI_HOST_INFO` exactly includes `port_udp=11111` and
  `pshare_iroutes=127.0.0.1:11111` with the expected community, host, DB port,
  empty alternate hosts, and time warp.
- `hostinfo_pshare_multi_route_pass`
  Posts UDP routes `127.0.0.1:11111` and `127.0.0.2:11112`; passes only when
  `PHI_HOST_INFO` exactly contains the normalized ampersand-separated route
  list and the expected community, host, DB port, and time warp fields.
- `hostinfo_invalid_pshare_route_pass`
  Posts invalid `PSHARE_INPUT_SUMMARY=not_a_route`; passes only when the exact
  `PHI_HOST_INFO` payload omits all pShare fields while retaining the expected
  community, host, DB port, alternate-host, and time-warp fields.
- `hostinfo_pshare_mixed_routes_pass`
  Posts `127.0.0.1:11111:udp,not_a_route`; passes only when the exact host-info
  payload retains the valid route and `port_udp=11111` while omitting the
  invalid token.
- `hostinfo_pshare_nonudp_route_pass`
  Posts `127.0.0.1:11111:multicast_18`; passes only when the exact host-info
  payload normalizes the route to `pshare_iroutes=multicast_18` with no UDP
  port field and retains the expected host metadata.

## Run

```sh
./zlaunch.sh --jobs=3 --port_base=11000 --max_time=40 10
```

`--jobs` uses rolling scheduling. Every selected case runs from its own mission
copy and port block; add `--keep_workdirs` to retain those copies for target,
sidecar, result, and log inspection.

Run one inspectable case:

```sh
./zlaunch.sh --case=hostinfo_pshare_route_pass --port_base=11000 --max_time=40 10
```

Logging is minimal by default. Use `--log=full` for the complete matrix, or
combine it with `--case=NAME` for a fully logged diagnostic case.
