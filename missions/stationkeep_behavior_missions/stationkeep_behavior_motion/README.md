# stationkeep_behavior_motion

Logging is minimal by default in both communities and launches no `pLogger`.
Add `--log=full` to either launcher to restore both original loggers.

Deterministic one-vehicle motion stem for `BHV_StationKeep`.

The vehicle starts west of a station point centered at `(12,-121)`. The default
behavior drives toward that point and then lets `BHV_StationKeep` manage the
inner/outer station-keeping rings. The stem grades on behavior-owned outputs
rather than only on final vehicle position.

## Default Flow

- `pAutoPoke` deploys the vehicle and initializes station-keep status
  variables.
- `uTimerScript` raises `TEST_EVAL_READY=true` after the default settle window.
- `pMissionEval` records one `results.txt` row with distance, mode, warning,
  error, and behavior-settings columns.

## Default Pass Conditions

- `TEST_EVAL_READY = true`
- `DIST_TO_STATION <= 8`
- `BHV_ERROR_SEEN = false`

The default case does not require a particular `PSKEEP_MODE`. Upstream
`BHV_StationKeep` may continue reporting `SEEKING_STATION` while the vehicle is
already close enough to the station point, even when hibernation is disabled.

## Important Outputs

- `DIST_TO_STATION`
- `PSKEEP_MODE`
- `BHV_SETTINGS`
- `BHV_WARNING`
- `BHV_ERROR`
- `STATIONKEEP_UPDATES`

## Running

From this mission directory:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=static_station_pass 10
./zlaunch.sh --case=hibernation_seek_pass --gui 1
```

The wrapper forwards named cases and wave-mode options to the paired harness.

## Paired Harness

- [`harnesses/stationkeep_behavior_harnesses/H01-stationkeep_behavior_motion`](../../../harnesses/stationkeep_behavior_harnesses/H01-stationkeep_behavior_motion)
