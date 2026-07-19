# H02 pShare Topology Harness

Logging is minimal by default in all four communities. Use `--log=full` for
the whole matrix or with `--case=NAME` for one diagnostic case. The four cases
that replace alpha or relay ANTLER blocks keep separate minimal and full
patches so minimal mode never reintroduces `pLogger`.

This harness verifies pShare behavior that needs more than one sender or more
than one receiver. It runs a four-community local topology: one shoreside
evaluator, two sender peers, and one relay/listener peer.

## Cases

- `pshare_topology_fanin_pass`
  Routes `ALPHA_SRC=alpha` and `BRAVO_SRC=bravo` from separate communities to
  `FANIN_ALPHA` and `FANIN_BRAVO` on one shoreside input; passes when both
  destination values arrive.
- `pshare_topology_competing_update_pass`
  Routes alpha and bravo into the same `COMPETE_RX` destination at 20 and 24
  seconds; passes when the later bravo update is the final value.
- `pshare_topology_multi_input_port_pass`
  Configures two separate shoreside `Input` entries, sends `WL_ALLOW=allowed`
  to the primary port and `WL_BLOCK=blocked` to the second port, and passes when
  both values arrive.
- `pshare_topology_input_route_list_pass`
  Configures the same two shoreside ports in one ampersand-separated input
  route list; passes when values `input_alpha` and `input_bravo` sent to the two
  ports both arrive.
- `pshare_topology_multicast_multi_listener_pass`
  Sends `MCAST_SRC=multicast` over `multicast_33` to shoreside and relay;
  passes when shoreside receives `MCAST_TO_SHORE=multicast` and the relay's
  evaluator returns `RELAY_SEEN=true`.
- `pshare_topology_dynamic_input_route_pass`
  Adds the relay-port unicast input through shoreside `PSHARE_CMD` at 18
  seconds, then sends `DYN_INPUT_RX=dynamic` to that port at 28 seconds;
  passes when the value arrives.
- `pshare_topology_dynamic_multicast_input_pass`
  Adds `multicast_44` through shoreside `PSHARE_CMD` at 18 seconds, then sends
  `DYN_MCAST_RX=dynamic_mcast` on that channel at 28 seconds; passes when the
  value arrives.
- `pshare_topology_custom_multicast_base_pass`
  Configures sender and receiver with multicast address `224.1.1.12`, a custom
  base port, and channel `multicast_7`; passes when
  `CUSTOM_MCAST_RX=custom_mcast` arrives.
- `pshare_topology_unicast_relay_branch_pass`
  Defines two unicast outputs from `DUAL_SRC`, one to shoreside and one to
  relay; passes when shoreside gets `DUAL_SHORE=branch` and the relay evaluator
  returns `UNICAST_RELAY_SEEN=true`.
- `pshare_topology_route_list_branch_pass`
  Uses one ampersand-separated output route list from `LIST_SRC` to shoreside
  and relay; passes when shoreside gets `LIST_BRANCH=route_list` and the relay
  evaluator returns `LIST_RELAY_SEEN=true`.
- `pshare_topology_alias_cmd_route_pass`
  Launches alpha pShare as Antler alias `ShareAlpha`, posts an output route on
  `SHAREALPHA_CMD`, then posts `ALIAS_SRC=alias`; passes when shoreside receives
  `ALIAS_RX=alias`.
- `pshare_topology_runtime_route_pass`
  Posts a unicast output route on alpha's `PSHARE_CMD` at 18 seconds and
  `RUNTIME_SRC=runtime` at 22 seconds; passes when shoreside receives
  `RUNTIME_RX=runtime`.

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
