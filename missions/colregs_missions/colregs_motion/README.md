# colregs_motion

This stem mission is the first moving COLREGS correctness baseline. It is
derived from `missions-auto/08-colavd-lanes` and currently runs a small
lane-style encounter set under auto-deploy and headless self-evaluation.

This stem is intentionally broader than `colregs_unit`. The unit stem stays
focused on a canonical two-vessel encounter, while this motion stem keeps the
multi-vessel lane geometry that makes it useful as the family’s first moving
integration baseline.

The mission evaluates after a short fixed runtime window and writes
`results.txt`.

Pass contract:
- `MISSION_DONE = true`
- `COLLISION_TOTAL = 0`
- `MISSION_TIMEOUT = false`
- `UCD_CLOSEST_RANGE_EVER > 0`
- `UCD_CLOSEST_RANGE_EVER <= 40`

The lane-style motion stem is evaluated on the closest detected contact range
ever reported by `uFldCollisionDetect`. That is more stable for this baseline
than requiring `ENCOUNTER_TOTAL`, which can stay at zero even when the
vehicles clearly operate inside a meaningful contact envelope.

Typical runs:

```bash
./launch.sh --just_make --nogui 10
./zlaunch.sh 10
```
