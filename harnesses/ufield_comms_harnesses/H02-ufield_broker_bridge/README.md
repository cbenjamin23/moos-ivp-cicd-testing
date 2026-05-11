# H02 uField Broker Bridge Harness

This harness exercises broker-level setup for `uFldNodeBroker` and
`uFldShoreBroker` using the shared `ufield_comms_unit` mission stem.

The evaluated surface is node broker ping publication, shore broker ack
publication, node-count registration, shoreside qbridge expansion, shoreside
custom bridge expansion, node-side user bridge registration, `NODE_PSHARE_VARS`
publication, keyword mismatch status propagation, shoreside bridge token
expansion, mediated node-message bridge selection, vnode shore discovery
publication, shoreside and node-side auto-bridge suppression, and default
alias/default vnode-port handling.

Run one case:

```sh
./zlaunch.sh --case=broker_handshake_pass --port_base=4000
```

Run the group:

```sh
./zlaunch.sh --jobs=2 --port_base=4000
```

## Current Matrix

- `broker_handshake_pass` Requires both vehicle brokers to ping shoreside, receive `status=ok` broker acks, publish `NODE_PSHARE_VARS`, and register as two shoreside nodes.
- `shore_qbridge_expansion_pass` Requires `uFldShoreBroker` to expand qbridge variables into per-node pShare output requests for both vehicles.
- `shore_custom_bridge_pass` Adds a custom `bridge = src=DEPLOY_$V, alias=DEPLOY` and requires expanded `DEPLOY_ABE` and `DEPLOY_BEN` pShare output requests.
- `node_custom_bridge_pass` Adds a custom node-side bridge and requires both node brokers to publish the custom pShare command and advertise the alias in `NODE_PSHARE_VARS`.
- `keyword_mismatch_status_pass` Requires a shoreside keyword mismatch to appear in broker ack payloads while registration still reaches both nodes.
- `shore_bridge_tokens_pass` Adds `$v` and `$N` shoreside bridge sources and requires lowercase-community and node-index pShare output requests for both vehicles.
- `node_mediated_bridge_pass` Replaces the unmediated node-message bridge with the mediated bridge and requires mediated pShare commands while suppressing the unmediated command.
- `shore_try_vnode_pass` Adds a vnode route and requires both the outbound `TRY_SHORE_HOST` pShare route and the local shore-host advertisement.
- `shore_minimal_autobridge_pass` Disables optional shoreside auto bridges and requires the core qbridge variables without realmcast, appcast, or mission-hash bridge advertisement.
- `node_minimal_autobridge_pass` Disables optional node broker auto bridges and requires only the core node aliases without pShare export of `NODE_PSHARE_VARS`.
- `shore_default_alias_pass` Adds a shoreside bridge with no explicit alias and requires the source variable to be used as the destination alias.
- `shore_try_vnode_default_port_pass` Adds a vnode route without a port and requires `uFldShoreBroker` to publish the default `9200` pShare route.
