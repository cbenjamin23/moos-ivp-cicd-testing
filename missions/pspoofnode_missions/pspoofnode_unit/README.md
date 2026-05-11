# pspoofnode_unit

Headless single-community stem for exercising `pSpoofNode` as an app-under-test.
The harness writes case-specific spoof configuration and runtime `SPOOF` /
`SPOOF_CANCEL` mail, then validates readiness plus structured `NODE_REPORT`
payloads from the generated alog.

Typical harness run:

```sh
../../../harnesses/pspoofnode_harnesses/H01-pspoofnode_unit/zlaunch.sh --jobs=4 --port_base=16000 10
```

Run one inspectable case:

```sh
../../../harnesses/pspoofnode_harnesses/H01-pspoofnode_unit/zlaunch.sh --case=config_static_report_pass --port_base=16000 10
```
