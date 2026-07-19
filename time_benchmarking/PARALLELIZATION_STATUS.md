# Parallelization Status

This note captures the current state of harness parallelization in this repo.
It is no longer just an idea file. Several harnesses now support a working
wave-based parallel mode.

## Current Model

The implemented compromise is wave-based parallelism:

1. Launch up to `N` isolated cases in parallel.
2. Give each live case:
   - its own temp mission copy
   - its own patch sidecars
   - its own generated targets
   - its own `results.txt`
   - its own port block
3. Wait for the whole wave to finish.
4. Run one `ktm` barrier.
5. Start the next wave.

This is simpler and more robust than a continuous worker pool. It is also why
the current `--jobs=<N>` mode should be treated as a CI batch feature, not as
something that can safely overlap with unrelated MOOS missions on the same
machine.

## Why Ports Alone Were Not Enough

The original blocker was shared mission state. Running two cases at once in the
same stem directory would race through:

- `meta_*.moosx` sidecars
- generated `targ_*` files
- mission-local `results.txt`
- field-init files such as `vpositions.txt` and `vnames.txt`
- cleanup paths such as `clean.sh`

The current wave-based model solves that by isolating the filesystem per live
case, not just the ports.

## Port Allocation

The current harnesses use deterministic per-case port blocks. The exact numbers
vary by slot, but the pattern is:

- shore MOOS port
- vehicle MOOS port
- shore pShare port
- vehicle pShare port

Each live case gets its own block, and the block moves with the case's temp
mission copy.

## Working Harnesses

Wave-based `--jobs=<N>` mode is currently in place on:

- `harnesses/collision_behavior_harnesses/H01-collision_behavior_motion`
- `harnesses/cmgr_harnesses/H01-cmgr_unit`
- `harnesses/cmgr_harnesses/H02-cmgr_motion`
- `harnesses/obmgr_harnesses/H01-obmgr_unit`
- `harnesses/obmgr_harnesses/H02-obmgr_motion`
- `harnesses/opregion_harnesses/H01-opregion_safety`
- `harnesses/waypoint_behavior_harnesses/H01-waypoint_behavior_motion`
- `harnesses/loiter_behavior_harnesses/H01-loiter_behavior_motion`
- `harnesses/stationkeep_behavior_harnesses/H01-stationkeep_behavior_motion`
- `harnesses/trail_behavior_harnesses/H01-trail_behavior_motion`

Other harnesses remain serial for now.

## Why This Is Not A Full Worker Pool Yet

This repo does not yet have true continuously reused workers.

The remaining limitation is teardown. Even with unique ports, `xlaunch.sh`
teardown and mission shutdown are not yet strong enough to trust immediate
slot reuse with no hard barrier. The current wave model avoids that problem by
using `ktm` once between waves instead of trying to recycle slots continuously.

So the current state is:

- good enough for faster CI batch runs
- not yet the final architecture for general parallel mission scheduling

## Sleep Tuning Findings

The current validated hot-path reductions are:

- wave-harness `sleep 1` after `ktm`: removed on active wave-enabled harnesses
- stem `launch.sh` vehicle-to-shoreside `sleep 0.5`: removed on active
  harnessed stems

Those repo-local reductions were kept because they validated cleanly.

## Shared `xlaunch.sh` Note

The shared launcher lives outside this repo at:

- `moos-ivp/scripts/xlaunch.sh` (outside this repository)

Its shutdown path still includes a post-kill wait. That wait exists because
mission processes may still be flushing `results.txt` and other outputs when
`uMayFinish` decides the mission is done.

A local experiment showed:

- reducing that wait can improve wall clock
- removing it entirely is unsafe

That experiment was reverted, and any future change there should be managed in
the separate `moos-ivp` repo, not here.

## Benchmarking

Measured timings and recommended `--jobs` values live under:

- `time_benchmarking/parallel_jobs_2026-03-20/README.md`

For future sweeps, invoke the harness directly at each `--jobs` value and record
the exact arguments and wall times with the results. No general benchmark
wrapper is retained.

## Practical Guidance

- Use wave mode only on harnesses that already support `--jobs=<N>`.
- Do not run multiple wave-mode harness benchmarks at the same time.
- Treat `ktm` as a batch barrier, not as a per-case cleanup tool.
- Keep app-level and motion-level correctness validation separate from
  performance timing conclusions.

## Next Step

The next real parallelization step, if needed later, would be a true worker
pool with:

- per-case temp workdirs
- per-case port blocks
- case-local cleanup that does not rely on global `ktm`
- immediate slot reuse after verified teardown

Until then, the current wave-based model is the stable middle ground.
