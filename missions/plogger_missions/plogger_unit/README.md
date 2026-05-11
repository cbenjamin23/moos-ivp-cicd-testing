# pLogger Unit Mission

This mission publishes controlled MOOS mail and uses `pLogger` to create log
artifacts. `pMissionEval` owns the live mission grade; the harness additionally
checks the generated `.alog` contents after shutdown.

Typical commands:

```sh
./launch.sh --just_make --nogui 10
./zlaunch.sh --max_time=20 --nogui 10
```
