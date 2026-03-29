# colregs_unit

This stem mission is the first COLREGS correctness baseline. It is derived from
`missions-auto/11-colavd` and currently exercises a simple two-vessel
encounter with `BHV_AvdColregsV22`.

This stem is intentionally the canonical two-vessel version of the family.
`colregs_motion` is the broader four-vessel lane scenario derived from
`08-colavd-lanes`, so the two stems are meant to stay different rather than be
collapsed into one mission.

The mission auto-deploys, self-evaluates, and writes `results.txt`.

Pass contract:
- `COLLISION_TOTAL = 0`
- `MISSION_TIMEOUT = false`
- `ENCOUNTER_TOTAL >= 1`

Typical runs:

```bash
./launch.sh --just_make --nogui 10
./zlaunch.sh 10
```
