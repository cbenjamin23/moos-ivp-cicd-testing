# Parallelization Idea

This note captures the current thinking on running CI mission cases in
parallel. The goal is to reduce wall-clock CI time without making the harnesses
flaky or hard to reason about.

## Short Version

Parallel execution is a good idea, but ports alone are not enough.

The current missions and harnesses are logically isolated per case, but they
are not yet filesystem-isolated per case. Right now each harness patches and
runs one shared stem mission directory in place, so two live cases would race
even if they used different MOOSDB and pShare ports.

## What Already Helps

- missions are already headless and self-terminating
- missions already write machine-readable `results.txt`
- harnesses already think in terms of one case at a time
- launchers already understand explicit MOOSDB / pShare ports

## What Currently Blocks True Parallelism

Each live case currently shares:

- one stem mission directory
- one set of temporary overlay files such as `meta_*.moosx`
- one set of generated `targ_*` files
- one mission-local `results.txt`
- one set of field-init files such as `vpositions.txt` and `vnames.txt`
- one log/output area

The current harnesses also:

- call mission-local `clean.sh`
- use `ktm`
- assume they are the only live mission using that stem directory

So if two harness runs overlap today, they can interfere through:

- patch files
- generated target files
- results files
- cleanup
- global process teardown

## Safer Parallel Design

The safer design is:

1. For each case, create a temporary working directory.
2. Copy or stage the stem mission into that temp directory.
3. Apply `nspatch` inside that temp directory.
4. Allocate a unique port block for that case.
5. Run the mission from the temp directory.
6. Read the temp-directory `results.txt`.
7. Aggregate harness output in the harness directory.
8. Remove the temp directory when the case is done.

This gives each case:

- private overlays
- private generated targets
- private field-init files
- private logs
- private results
- private ports

At that point, a `--jobs=<N>` harness option becomes realistic.

## Port Allocation Idea

One simple approach is to allocate a small fixed port block per case.

Example:

- base shore MOOS port: `9000`
- base vehicle MOOS port: `9001`
- base shore pShare port: `9200`
- base vehicle pShare port: `9201`

For worker index `k`, compute:

- shore MOOS port = `9000 + 10*k`
- vehicle MOOS port = `9001 + 10*k`
- shore pShare port = `9200 + 10*k`
- vehicle pShare port = `9201 + 10*k`

The exact offsets do not matter much as long as:

- each live case gets its own block
- the block is deterministic
- the block is large enough for future mission growth

## Global Cleanup Concern

`ktm` is a major blocker for parallelism in the current harnesses because it is
effectively global. A parallel-safe harness should not use `ktm` for ordinary
per-case cleanup.

Instead, each worker should:

- track only the processes it launched
- stop only those processes
- avoid killing sibling workers

## Rough Estimate Of Max Parallel Jobs On One Computer

There is no single fixed number. The practical limit depends on:

- CPU cores and sustained CPU availability
- memory pressure
- disk/logging overhead
- whether MOOS apps are mostly idle or actively publishing a lot
- how expensive launch/teardown is relative to mission runtime

The right way to estimate the limit is empirical.

### Practical Method

1. Start with serial timing for a full harness run.
2. Run with `--jobs=2` and measure:
   - total wall-clock time
   - whether any cases become flaky
   - CPU usage
   - memory pressure
3. Repeat with `--jobs=3`, `--jobs=4`, and so on.
4. Stop increasing when one of these happens:
   - wall-clock improvement becomes small
   - case flakiness increases
   - CPU stays saturated
   - memory pressure becomes high
   - launch failures or port/process errors appear

### Rule Of Thumb

For these current missions, the limit is likely to be lower than "number of
CPU cores" would suggest, because:

- launch and teardown overhead matters a lot
- logging and filesystem churn are non-trivial
- MOOS process counts multiply quickly per live case

So the useful parallelism ceiling may be something like:

- low single digits on a laptop
- somewhat higher on a desktop or CI runner

But the real answer should come from timing curves, not guesswork.

## Suggested Future Implementation Order

1. Refactor one harness to use per-case temp workdirs.
2. Add deterministic per-case port allocation.
3. Remove `ktm` from ordinary per-case cleanup.
4. Add `--jobs=<N>` with a small worker pool.
5. Measure scaling and document the safe default.

## Current Conclusion

Parallel case execution looks worthwhile, but only after per-case filesystem
isolation is added. Without that isolation, parallel ports alone would not make
the current harnesses safe.

## Benchmark Script Note

This should be implemented after parallel-safe harness execution exists.

Before that, a benchmark script would mostly measure the current serial
in-place harness design rather than the architecture we actually want.

Once parallel-safe harnessing exists, a small benchmark script is likely worth
adding to the repo so anyone can run it on their own machine. The script should
be simple and repeatable rather than overly clever.

Good first version:

- run a chosen harness at `--jobs=1,2,3,...`
- record wall-clock time
- record pass/fail stability
- optionally capture a few basic system stats
- report the best stable `jobs` value observed

That is preferable to rediscovering the method ad hoc every time.

## Collision Behavior Prototype Notes

A small prototype was attempted first on:

- `harnesses/collision_behavior_harnesses/H01-collision_behavior_motion`

Main findings:

- The stem itself needed hardening before parallelization work was meaningful.
  The original `uTimerScript` flow depended on one-shot deploy mail and could
  miss that mail at startup. Making the timer sequence launch-relative was a
  necessary stability fix even for serial harness runs.
- The reliable unit of isolation is the single-case mission invocation, not a
  long-lived worker process that reuses one mission directory repeatedly.
- Per-case temp workdirs and per-case port blocks still look like the right
  shape, but the orchestration layer should probably be built around child
  single-case runs rather than one shell process trying to manage many live
  case transitions directly.
- `xlaunch.sh` teardown is not strong enough by itself for repeated isolated
  case execution. Any future parallel harness will need explicit case-local
  cleanup logic in addition to unique ports.

Practical implication:

- When parallelization resumes, start from a harness that already has stable
  single-case `--case=<name>` execution and treat that child invocation as the
  building block for `--jobs=<N>`.

## Wave-Based Compromise

The first successful implementation in this repo is a simpler compromise:

- run a wave of up to `N` isolated cases in parallel
- each live case gets:
  - its own temp mission copy
  - its own patch sidecars
  - its own target files
  - its own `results.txt`
  - its own port block
- wait for the whole wave to finish
- call `ktm` once as a global batch barrier
- then start the next wave

This is not fully general parallelism, because `ktm` still makes it unsuitable
to overlap with unrelated MOOS missions. But it is much simpler and can still
reduce harness wall clock substantially.

Current measured example:

- `harnesses/collision_behavior_harnesses/H01-collision_behavior_motion`
  - serial: about `63.82` seconds wall clock at warp `10`
  - wave mode with `--jobs=2`: about `43.62` seconds wall clock at warp `10`
  - wave mode with `--jobs=6`: about `20.89` seconds wall clock at warp `10`

The same wave-based pattern now works on the app-level harnesses too:

- `harnesses/cmgr_harnesses/H01-cmgr_unit`
  - serial: about `239.45` seconds wall clock at warp `10`
  - wave mode with `--jobs=4`: about `110.56` seconds wall clock at warp `10`
- `harnesses/obmgr_harnesses/H01-obmgr_unit`
  - serial: about `138.31` seconds wall clock at warp `10`
  - wave mode with `--jobs=4`: about `59.65` seconds wall clock at warp `10`

The same pattern now works on one contact-motion harness as well:

- `harnesses/cmgr_harnesses/H02-cmgr_motion`
  - serial: about `97.15` seconds wall clock at warp `10`
  - wave mode with `--jobs=4`: about `45.46` seconds wall clock at warp `10`

And it now works on the obstacle motion harness too:

- `harnesses/obmgr_harnesses/H02-obmgr_motion`
  - serial: about `65.77` seconds wall clock at warp `10`
  - wave mode with `--jobs=4`: about `24.93` seconds wall clock at warp `10`

## Sleep-Tuning Findings

After the first wave-mode rollout, the fixed waits in the launch path were
tested directly.

Results:

- the per-wave `sleep 1` after `ktm` in the harnesses was unnecessary on the
  tested suites
- the `sleep 0.5` gap between vehicle launch and shoreside launch in the stem
  `launch.sh` files was also unnecessary on the tested suites
- the `sleep 2` in the shared `xlaunch.sh` was too conservative, but removing
  it entirely caused missing result lines on a motion harness
- reducing that shared wait to about `0.25s` remained stable in the tested
  runs and recovered most of the benefit

Practical implication:

- repo-local harness and stem sleeps can be removed where validated
- the shared launcher still needs a small bounded wait unless it is replaced
  with a real process-exit poll
