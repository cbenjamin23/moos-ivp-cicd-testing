# P02-colregs_joust

Initial performance harness for the COLREGS joust stem.

Current case set:
- `baseline_colregs_pass`
- `dense_colregs_pass`
- `endurance_colregs_pass`

The harness checks:
- mission `grade`
- selected reported metrics against `performance_baselines.tsv`
- disallowed planner/runtime warning text in the newest vehicle `.alog`

For these joust cases, the main pressure metric is
`closest_range_ever`, derived from `UCD_CLOSEST_RANGE_EVER`. That is more
useful for this stem than `ENCOUNTER_TOTAL`, which may stay low or zero even
when the vehicles repeatedly operate inside a meaningful contact envelope.

The current case split is:
- `baseline_colregs_pass`
  - mild two-vehicle deterministic crossing
- `dense_colregs_pass`
  - denser three-vehicle continuous joust
- `endurance_colregs_pass`
  - longer-running three-vehicle joust with the same zero-collision and
    zero-warning requirements

Each case uses a checked-in baseline band from `performance_baselines.tsv`
rather than exact-value matching.

Typical runs:

```bash
./zlaunch.sh 10
./zlaunch.sh --jobs=2 10
./zlaunch.sh --case=baseline_colregs_pass 10
./zlaunch.sh --case=dense_colregs_pass 10
./zlaunch.sh --case=endurance_colregs_pass 10
```

Wave-mode notes:
- The harness supports `--jobs=<n>` with the same wave-batch model used by the
  other parallelized harnesses in this repo.
- Each live case gets its own temp mission copy plus its own MOOSDB and pShare
  port block, and the harness waits once between waves with `ktm`.
- Use `--keep_workdirs` if you want to inspect the isolated temp mission copies
  after a parallel run.
