# convoy_behavior_motion

Deterministic two-vehicle motion stem for `BHV_Convoy`.

`abe` runs `BHV_Convoy` as the chaser. `ben` is a moving target on a short
eastbound waypoint leg. The convoy behavior follows `ben`'s breadcrumb queue,
with default mark spacing of 5 meters and a maximum mark queue length of 80
meters.

The stem grades on behavior-owned and mission-observed outputs:

- `QLEN`
- `TLEN`
- `MXRNG`
- `AVG2`
- `AVG5`
- `WPTX`
- `WPTY`
- `ABE_NAV_SPEED`
- `BEN_NAV_SPEED`
- `ABE_NAV_X`
- `ABE_NAV_Y`
- `BEN_NAV_X`
- `BEN_NAV_Y`
- `BHV_WARNING`
- `BHV_ERROR`

Default pass conditions are:

- `TEST_EVAL_READY = true`
- `QLEN >= 2`
- `TLEN >= 10`
- `TLEN <= 95`
- `MXRNG = 80`
- `AVG2 >= 1`
- `AVG5 >= 1`
- `BHV_ERROR_SEEN = false`

Geometry-entry cases use a later evaluation gate and additionally require the
chaser to have converged back near the target's eastbound line:

- `TEST_EVAL_READY = true` at 65 seconds
- `QLEN >= 2`
- `TLEN >= 10`
- `TLEN <= 95`
- `MXRNG = 80`
- `AVG2 >= 1`
- `ABE_NAV_SPEED >= 0.5`
- `BEN_NAV_SPEED >= 0.9`
- `ABE_NAV_Y >= -102`
- `ABE_NAV_Y <= -58`
- `BHV_ERROR_SEEN = false`

Run from this directory:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=static_convoy_pass 10
./zlaunch.sh --case=fine_mark_spacing_pass --gui 1
./zlaunch.sh --case=angled_entry_pass --gui 1
./zlaunch.sh --case=fast_follower_pass --gui 1
./zlaunch.sh --jobs=4 10
```

Logging is minimal by default. Add `--log=full` to restore the original
shoreside and two-vehicle `pLogger` configuration for diagnostic runs.

Named cases are implemented in the paired harness:

[`harnesses/convoy_behavior_harnesses/H01-convoy_behavior_motion`](../../../harnesses/convoy_behavior_harnesses/H01-convoy_behavior_motion)
