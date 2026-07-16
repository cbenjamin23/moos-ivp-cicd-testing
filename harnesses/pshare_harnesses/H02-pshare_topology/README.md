# H02 pShare Topology Harness

This harness verifies pShare behavior that needs more than one sender or more
than one receiver. It runs a four-community local topology: one shoreside
evaluator, two sender peers, and one relay/listener peer.

## Cases

- `pshare_topology_fanin_pass` Routes independent mail from two senders into one receiver and expects both values at the evaluator.
- `pshare_topology_competing_update_pass` Routes two senders into the same destination variable and expects the later sender update to win.
- `pshare_topology_multi_input_port_pass` Sends one update to the shore primary input and another to a second shore input port, proving one community can listen on multiple pShare ports.
- `pshare_topology_input_route_list_pass` Configures two receiver ports in one input route list and expects mail sent to both ports to arrive at the evaluator.
- `pshare_topology_multicast_multi_listener_pass` Sends one multicast route to both the evaluator and relay, then expects the relay to forward proof of receipt back to the evaluator.
- `pshare_topology_dynamic_input_route_pass` Adds a receiver input port through `PSHARE_CMD` at runtime and expects later mail sent to that new port to arrive.
- `pshare_topology_dynamic_multicast_input_pass` Adds a multicast receiver route through `PSHARE_CMD` at runtime and expects later multicast mail to arrive.
- `pshare_topology_custom_multicast_base_pass` Sends multicast mail over a non-default multicast base port and address, proving both sender and receiver honor the custom channel mapping.
- `pshare_topology_unicast_relay_branch_pass` Sends one source update over two unicast routes, proving both direct shore delivery and relay receipt through a relay proof flag.
- `pshare_topology_route_list_branch_pass` Sends one source update through a single route-list output that targets both shore and relay, proving route-list branching across communities.
- `pshare_topology_alias_cmd_route_pass` Launches alpha pShare under an Antler alias and installs a route through the alias-specific command variable.
- `pshare_topology_runtime_route_pass` Installs a route through `PSHARE_CMD` at runtime and expects later mail to arrive through the new route.

Typical commands:

```sh
./zlaunch.sh --jobs=2 --port_base=9000 --max_time=65 10
./zlaunch.sh --case=pshare_topology_fanin_pass --port_base=9000 --max_time=65 10
```

The launcher requires Bash 5.1 or newer for rolling scheduling. Every case,
including serial runs, executes in its own mission copy and stride-50 port
block. The four MOOSDB ports use offsets 0 through 3 and the four pShare ports
use offsets 10 through 13. pMissionEval owns every topology verdict; the
harness only applies the explicit community patches, validates one mission
row, aggregates results in case order, and performs scoped cleanup.
