# psearchgrid_unit

Headless single-community stem for exercising `pSearchGrid` as an
app-under-test. The harness writes case-specific grid configuration and
`NODE_REPORT` mail, then validates readiness plus structured grid payloads from
the generated alog.

Typical harness run:

```sh
../../../harnesses/psearchgrid_harnesses/H01-psearchgrid_unit/zlaunch.sh --jobs=4 --port_base=16400 10
```

Run one inspectable case:

```sh
../../../harnesses/psearchgrid_harnesses/H01-psearchgrid_unit/zlaunch.sh --case=node_local_delta_pass --port_base=16400 10
```
