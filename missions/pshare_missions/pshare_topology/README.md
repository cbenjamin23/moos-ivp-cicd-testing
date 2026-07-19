# pShare Topology Mission

Logging is minimal by default in all four communities and launches no
`pLogger`. Add `--log=full` to either launcher to restore all four original
loggers.

This mission runs four small local MOOS communities: a shoreside evaluator,
two independent sender peers, and one relay/listener peer. It is used by the
H02 pShare topology harness for route behavior that needs more than one sender
or more than one receiver.

Typical commands:

```sh
./launch.sh --just_make --nogui 10
./zlaunch.sh --max_time=65 --nogui 10
```

`zlaunch.sh` forwards explicit MOOSDB and pShare ports for shore, alpha,
bravo, and relay to the shared xlaunch lifecycle. It validates all four target
files in generation mode, requires exactly one complete pMissionEval result
row after a live run, and applies repository-scoped cleanup as a standalone
backstop.
