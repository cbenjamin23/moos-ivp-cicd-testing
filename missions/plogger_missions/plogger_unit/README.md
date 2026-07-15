# pLogger Unit Mission

This mission publishes controlled MOOS mail and uses `pLogger` to create log
artifacts. `pMissionEval` owns the live mission grade; the harness additionally
checks the generated logger files after shutdown. The mission wrapper forwards
the requested ports, mission label, display mode, time warp, and maximum time,
requires exactly one valid pMissionEval result row, and applies scoped cleanup
when run directly.

Typical commands:

```sh
./launch.sh --just_make --nogui 10
./zlaunch.sh --max_time=20 --nogui 10
```
