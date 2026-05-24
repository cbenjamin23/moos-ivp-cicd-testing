# H03 uField Route Resilience

This harness reuses the `ufield_comms_unit` stem to test route recovery paths in
`uFldNodeBroker`, `uFldShoreBroker`, and pShare rather than static bridge
configuration. Recovery cases require the base two-node communications mission
to pass; expected-negative cases require mission-owned blocked-route evidence
and still report `grade=pass` when that evidence is observed. All cases add
alog checks for the specific route mechanism under test.

Typical runs:

```bash
./zlaunch.sh --jobs=2 --port_base=4000
./zlaunch.sh --case=shore_vnode_discovery_recover_pass --port_base=4000 --gui 10
```

## Cases

- `dead_first_tryhost_blocks_fail` Places a dead route in the effective first `try_shore_host` slot; the case is expected to fail while proving both broker ping routes were emitted and no valid shore ack was received.
- `secondary_dead_tryhost_ignored_pass` Places a live route in the effective first slot and a dead secondary route; both vehicles must establish key `0` broker acks and complete direct message/report delivery.
- `startup_invalid_bad_port_valid_route_pass` Adds a startup `try_shore_host` with a nonnumeric port beside a valid route; the invalid startup route must not create a pShare command.
- `startup_invalid_bad_param_valid_route_pass` Adds a startup `try_shore_host` with the wrong parameter name beside a valid route; the invalid startup line must not create a pShare command.
- `startup_ip_route_recover_pass` Uses a startup `try_shore_host` with `127.0.0.1` instead of `localhost`; both node brokers must recover through the numeric loopback route.
- `runtime_tryhost_recover_pass` Removes startup shore routing and injects a valid runtime `TRY_SHORE_HOST`; both node brokers must add the route, handshake on key `0`, and finish normal communications.
- `runtime_dead_then_valid_blocks_fail` Sends a runtime dead route before a valid route; the case is expected to fail and proves the later valid route does not recover this dead-first ordering.
- `runtime_valid_then_dead_stays_pass` Sends a valid runtime route before a dead runtime route; the original key `0` route must remain sufficient for delivery.
- `runtime_duplicate_tryhost_ignored_pass` Sends the same valid runtime route twice after removing startup routing; duplicate runtime route mail must not create a key `1` pShare route.
- `startup_duplicate_runtime_ignored_pass` Keeps startup routing and repeats the same route through runtime mail; the duplicate must be ignored while the startup route remains healthy.
- `delayed_runtime_tryhost_recover_pass` Delays the first valid runtime route until mid-scenario; both vehicles must still recover before the direct message and report checks complete.
- `late_runtime_tryhost_blocks_delivery_fail` Delays the first valid runtime route until after key traffic has already been missed; broker acks may recover, but direct/report delivery must remain failed.
- `runtime_one_node_missing_route_fail` Gives only `abe` a runtime shore route; the case is expected to fail because `ben` never receives a valid broker ack.
- `runtime_staggered_nodes_recover_pass` Gives `abe` and `ben` valid runtime routes at different times; both must recover before the mission traffic window closes.
- `invalid_runtime_tryhost_recover_pass` Sends an invalid runtime `TRY_SHORE_HOST` before a valid one; the invalid payload must not create a pShare command, and the later valid route must recover the mission.
- `invalid_runtime_bad_port_recover_pass` Sends a runtime route with a nonnumeric port before a valid one; the bad port must not create a pShare command.
- `invalid_runtime_bad_param_recover_pass` Sends a runtime route with the wrong parameter name before a valid one; the bad parameter must not create a pShare command.
- `runtime_ip_route_recover_pass` Uses `127.0.0.1` instead of `localhost` in the runtime route; the node brokers must accept the numeric loopback route and recover.
- `shore_vnode_discovery_recover_pass` Removes vehicle startup routing and has the shore broker publish `TRY_SHORE_HOST` to both vehicle pShare listeners via `try_vnode`; both vehicles must consume the discovery route and complete the mission.
- `shore_vnode_one_node_only_fail` Publishes shore discovery to only one vehicle pShare listener; the case is expected to fail because the other node never recovers.
- `shore_vnode_extra_dead_ignored_pass` Publishes valid discovery routes plus one dead vnode route; both live vehicles must still recover.
- `shore_vnode_default_port_no_listener_fail` Uses a no-port vnode route, which defaults to `127.0.0.1:9200`; the case is expected to fail because no vehicle listener is present there.
- `shore_vnode_invalid_host_ignored_pass` Adds a `try_vnode` route with an invalid host beside valid vnode routes; the invalid host must not create a pShare command.
- `shore_vnode_bad_port_ignored_pass` Adds a `try_vnode` route with a nonnumeric port beside valid vnode routes; the invalid port must not create a pShare command.
- `shore_vnode_duplicate_ignored_pass` Adds a duplicate valid vnode route beside the two required vehicle routes; the duplicate warning must not prevent both live vehicles from recovering.

## Evaluation

`pMissionEval` owns the mission grade. Recovery cases require two brokered
nodes, direct message delivery, ack delivery, cross-node report delivery, pulse
evidence, and no shared-report leakage. Expected-negative cases replace those
criteria with the corresponding blocked-route evidence and include an
`expected=<scenario>` result field. The harness supplements the mission grade
with targeted alog metrics: exact pShare command routes, broker ack keys,
runtime `TRY_SHORE_HOST` mail, and absence of pShare output for invalid route
strings.
