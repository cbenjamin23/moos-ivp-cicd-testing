# legrun_behavior_motion

Single-vehicle moving stem for `BHV_LegRun` correctness tests. The vehicle runs
a two-leg pattern with port and starboard turns, while shoreside evaluation
grades behavior-owned leg, turn, mid-leg, and mid-turn flags.

Typical runs:

```sh
./launch.sh --nogui 10
./zlaunch.sh --max_time=70 10
```

Logging is minimal by default. Add `--log=full` to either launcher to restore
the stem's original shoreside and vehicle `pLogger` configuration.

The harness in
`harnesses/legrun_behavior_harnesses/H01-legrun_behavior_motion` applies case
overlays to this stem with `nspatch`.
