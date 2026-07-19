# pShare Unit Mission

Logging is minimal by default in both communities and launches no `pLogger`.
Add `--log=full` to either launcher to restore both original loggers.

This mission runs two small MOOS communities on local ports. The peer community
publishes controlled mail through `pShare`; the shoreside community grades what
arrives through its receiving `pShare` instance.

Typical commands:

```sh
./launch.sh --just_make --nogui 10
./zlaunch.sh --max_time=25 --nogui 10
```
