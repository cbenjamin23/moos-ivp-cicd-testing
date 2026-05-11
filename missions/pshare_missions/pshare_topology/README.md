# pShare Topology Mission

This mission runs four small local MOOS communities: a shoreside evaluator,
two independent sender peers, and one relay/listener peer. It is used by the
H02 pShare topology harness for route behavior that needs more than one sender
or more than one receiver.

Typical commands:

```sh
./launch.sh --just_make --nogui 10
./zlaunch.sh --max_time=65 --nogui 10
```
