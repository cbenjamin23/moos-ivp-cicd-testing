# H05-colregs_motion

Initial moving correctness harness for the COLREGS motion stem.

This harness is currently a provisional broader integration case. It is not
part of the planned `H01-H04` unit-oriented correctness expansion built on the
shared `colregs_unit` stem. It now occupies the `H05` slot so `H01-H04`
remain reserved for the shared unit-stem correctness family.

Current case set:
- `lanes_colregs_pass`

Typical runs:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=lanes_colregs_pass 10
```
