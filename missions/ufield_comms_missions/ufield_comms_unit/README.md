# uField Comms Unit Mission

This headless mission runs one shoreside community plus two vehicle
communities, `abe` and `ben`, to exercise the uField broker/comms stack without
helm or vehicle simulation.

The vehicles run `uFldNodeBroker`, `pShare`, `pHostInfo`, and `uTimerScript`.
They publish controlled `NODE_REPORT_LOCAL`, `NODE_MESSAGE_LOCAL`, and
`ACK_MESSAGE_LOCAL` mail. The shoreside community runs `uFldShoreBroker`,
`uFldNodeComms`, `pShare`, and `pMissionEval`. The evaluated signals are broker
discovery (`UFSB_NODE_COUNT`), inter-node report fanout, node-message routing,
ack routing, and comms pulse publication.

Direct launches default to `--log=minimal`, which starts no pLogger. Use
`--log=full` to restore the pre-migration wildcard shore and vehicle loggers.
The H02/H03 harnesses apply their own narrow grading-log policy in minimal
mode because their route assertions intentionally inspect selected `.alog`
variables.

Typical run:

```sh
./zlaunch.sh --case=baseline_broker_comms_pass --port_base=4000 10
./zlaunch.sh --log=full --case=baseline_broker_comms_pass --port_base=4000 10
```

Manual inspection:

```sh
./zlaunch.sh --case=baseline_broker_comms_pass --gui --port_base=4000 10
```
