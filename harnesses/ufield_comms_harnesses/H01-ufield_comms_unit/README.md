# H01 uField Comms Unit Harness

This harness exercises the core `uFldNodeBroker`, `uFldShoreBroker`, and
`uFldNodeComms` process interaction with a headless shoreside community and two
vehicle communities.

The current evaluated surface is broker registration, direct `NODE_MESSAGE`
delivery, mediated-message routing, `ACK_MESSAGE` delivery, destination resolution, bidirectional
`NODE_REPORT_<VNAME>` distribution, communication pulse posting, range
blocking, critical-range override, group filtering, stealth, enhanced acoustic
range, runtime updates, message/report rate limiting, drop percentage, invalid
message/ack rejection, `stale_time=-1`, `max_msg_length=0`, and
`NODE_REPORT_UNC` sharing.

Run one case:

```sh
./zlaunch.sh --case=baseline_broker_comms_pass --port_base=4000
```

Run the group:

```sh
./zlaunch.sh --jobs=2 --port_base=4000
```

## Current Matrix

- `baseline_broker_comms_pass` Confirms both vehicles register through the node and shore brokers, exchange node reports, and produce a visible comms pulse.
- `critical_range_override_pass` Sets a small nominal comms range but a critical range large enough to still allow report exchange.
- `shared_node_reports_pass` Enables `shared_node_reports` and requires `NODE_REPORT_UNC` in addition to the bidirectional vehicle reports.
- `pulse_disabled_pass` Disables node-report pulse rendering while preserving report delivery.
- `range_block_pass` Sets both comms and critical ranges below the vehicle separation and requires report delivery to stay blocked.
- `zero_range_block_pass` Sets `comms_range=0` and `critical_range=0`, requiring all range-gated reports, direct messages, acks, and pulses to stay blocked.
- `drop_all_reports_pass` Uses `drop_percentage=100` to deterministically block all inter-vehicle report delivery.
- `far_range_block_pass` Moves the vehicles far apart under finite range and requires report delivery to stay blocked.
- `no_limit_far_pass` Keeps the far geometry but uses `comms_range=nolimit` and requires bidirectional report delivery.
- `max_msg_length_block_pass` Sets `max_msg_length` below the scripted payload and requires direct message delivery to be blocked while ack delivery remains available.
- `dest_specific_message_pass` Sends the direct message with `dest_node=ben` and requires delivery through the named-destination path.
- `dest_group_message_pass` Sends the direct message with `dest_group=red` and requires delivery through group destination resolution.
- `ack_requested_mediated_pass` Sends a message with `ack=true` and requires `uFldNodeComms` to route it as `MEDIATED_MESSAGE_BEN`.
- `double_val_message_pass` Sends a direct message with `double_val` instead of `string_val` and requires the numeric payload to be preserved.
- `unknown_dest_block_pass` Sends a valid message to an unknown destination and requires it to be ignored while normal reports and acks still flow.
- `ghost_source_block_pass` Sends a valid message from an unknown source node and requires it to be ignored while normal reports and acks still flow.
- `invalid_ack_block_pass` Sends an ack missing its required id and requires `ACK_MESSAGE_abe` to stay absent while direct messages and reports still flow.
- `msg_group_filter_pass` Splits the vehicles into different groups and enables `msg_groups`, requiring messages to be blocked while reports still flow.
- `report_group_filter_pass` Splits the vehicles into different groups and enables `groups`, requiring reports to be blocked while messages still flow.
- `stealth_range_block_pass` Applies stealth to `abe`, requiring outbound traffic from `abe` to be blocked while reverse traffic still flows.
- `earange_extend_pass` Gives `ben` extended receive range, requiring `abe` to reach `ben` while the reverse direction remains blocked.
- `stale_message_block_pass` Publishes stale node-report timestamps and requires messages and acks to be blocked while report fanout still reflects recently received records.
- `min_msg_interval_filter_pass` Sends two fast direct messages with a long `min_msg_interval`, requiring the first delivery to pass and the follow-up payload to be suppressed.
- `runtime_range_extend_pass` Starts below communication range, posts `UNC_COMMS_RANGE=200`, and requires delivery to recover at runtime.
- `runtime_stealth_block_pass` Starts with deliverable traffic, posts `UNC_STEALTH` for `abe`, and requires later outbound traffic from `abe` to be blocked while reverse acks still flow.
- `runtime_earange_extend_pass` Starts below communication range, posts `UNC_EARANGE` for `ben`, and requires later `abe` to `ben` traffic to recover asymmetrically.
- `runtime_drop_all_pass` Starts with deliverable traffic, posts `UNC_DROP_PCT=100`, and requires later direct messages and acks to be dropped.
- `runtime_pulse_disabled_pass` Starts with visible report pulses, posts `UNC_VIEW_NODE_RPT_PULSES=false`, and requires later report delivery to continue without later pulse output.
- `runtime_shared_reports_pass` Starts with shared reports disabled, posts `UNC_SHARED_NODE_REPORTS=true`, and requires `NODE_REPORT_UNC` to appear.
- `shared_node_reports_numeric_pass` Enables shared reports with a numeric interval and requires `NODE_REPORT_UNC` through the interval-based config path.
- `min_rpt_interval_filter_pass` Sets a long `min_rpt_interval` and requires first report delivery to pass while repeated report posts are rate-limited.
- `stale_time_unlimited_pass` Publishes old report timestamps with `stale_time=-1` and requires reports, direct messages, and acks to remain deliverable.
- `max_msg_length_unlimited_pass` Sets `max_msg_length=0` and requires a deliberately long direct message payload to be delivered.
- `earrange_alias_extend_pass` Uses the accepted `earrange` spelling alias and requires the same asymmetric enhanced acoustic range outcome as `earange`.
- `critical_group_override_pass` Splits vehicle groups while setting a large critical range, requiring report delivery to bypass group filtering while range-gated messages remain blocked.
