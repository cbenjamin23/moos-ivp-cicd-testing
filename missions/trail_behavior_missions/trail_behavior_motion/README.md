# trail_behavior_motion

Logging is minimal by default in both communities and launches no `pLogger`.
Add `--log=full` to either launcher to restore both original loggers.

Deterministic two-vehicle motion stem for `BHV_Trail`.

`abe` runs `BHV_Trail` as the chaser. `ben` is a moving target on a short
eastbound waypoint leg. The default case trails `ben` at a relative angle of
180 degrees and a 45 meter trailing range.

The stem grades on behavior-owned outputs:

- `TRAIL_DISTANCE`
- `TRAIL_RELEVANCE`
- `TRAIL_RANGE`
- `PURSUIT`
- `REGION`
- `BHV_WARNING`
- `BHV_ERROR`

Default pass conditions are:

- `TEST_EVAL_READY = true`
- `TRAIL_DISTANCE <= 20`
- `PURSUIT = 1`
- `TRAIL_RELEVANCE > 0`
- `BHV_ERROR_SEEN = false`

Run from this directory:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=static_trail_pass 10
./zlaunch.sh --case=relative_port_pass --gui 1
./zlaunch.sh --jobs=4 10
```

Named cases are implemented in the paired harness:

[`harnesses/trail_behavior_harnesses/H01-trail_behavior_motion`](../../../harnesses/trail_behavior_harnesses/H01-trail_behavior_motion)
