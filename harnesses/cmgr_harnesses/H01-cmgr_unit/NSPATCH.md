# NSPatch Notes For H01-cmgr_tests

This harness uses `nspatch` to turn one stem mission into a matrix of short
contact-manager tests without duplicating the whole mission directory.

## Stem And Overlays

The one shared stem mission is:

- `missions/cmgr_missions/cmgr_unit/meta_vehicle.moos`
- `missions/cmgr_missions/cmgr_unit/meta_shoreside.moos`

For each harness case, `nspatch` may generate temporary overlay files:

- `missions/cmgr_missions/cmgr_unit/meta_vehicle.moosx`
- `missions/cmgr_missions/cmgr_unit/meta_shoreside.moosx`
- `missions/cmgr_missions/cmgr_unit/meta_vehicle.bhvx`

The mission launchers already use `nsplug -x`, so if those `*.moosx` or
`*.bhvx` files exist they are folded into the generated `targ_*` files for that
case.

Important architecture point:

- the harness copies the stem into one isolated directory per case
- it composes the optional full-logging restoration and case patches there
- then it calls the copied stem mission's `zlaunch.sh`

The copied stem launcher is told that its logging sidecars are already
prepared, so it does not remove or rebuild the harness-composed overlays.

## What Gets Patched

Typical patch targets in this harness:

- `ProcessConfig = pContactMgrV20`
  For threshold cases, alert counts, closest-contact behavior, or contact aging.
- `ProcessConfig = uTimerScript`
  For spoof timing, spoof geometry, number of spoofed contacts, or evaluation
  checkpoint timing.
- `ProcessConfig = pMissionEval`
  For case-specific grading rules such as detection, non-detection, closest
  contact, count/list checks, range-band checks, or stale-contact checks.

In simple terms:

- vehicle-side patches usually change what contact manager sees
- shoreside patches usually change when the case is graded and what gets
  written to `results.txt`

## How The Consolidated Design Works

The stem mission itself is a small single-contact scenario. Multi-contact cases
are not a separate stem anymore. They are created by vehicle-side patches that
replace the default spoof block with a two-contact `uTimerScript` setup and,
when needed, a matching `pContactMgrV20` configuration.

That means:

- one stem mission
- one harness
- both single-contact and multi-contact cases

The consolidation works because the multi-contact differences are value and
logic changes, not a fundamentally different launcher topology.

## Harness Flow

For each case:

1. Copy the stem into an isolated case directory and run its `clean.sh`.
2. In full mode, apply the stem-local full-logging patches.
3. Apply the selected case patches after the logging patches so case-specific
   blocks retain final precedence.
4. Run the mission through the copied stem's `zlaunch.sh` path.
5. Read the mission-local `results.txt`.
6. Normalize the mission-owned result into the harness result row.

## Cleanup And Safety

The harness treats generated overlay files as part of each isolated case
workspace. `clean.sh` removes `meta_*.moosx` and `meta_*.bhvx` before patching.

That means:

- one case should not leak patches into the next case
- parallel workers do not write to the checked-in stem directory
- the harness lock prevents two live invocations from sharing its results and
  invocation-root state

## Adding A New Case

Typical pattern:

1. Decide whether the case changes:
   - mission parameters only
   - mission evaluation only
   - or both
2. Add a new `*.xmoos` patch file in this directory.
3. Update `zlaunch.sh` case mapping:
   - case name
   - expected result
   - shoreside patch path
   - vehicle patch path if needed
4. Add the case to the `ALL_CASES=(...)` array if it should run in the full
   matrix.
5. Update `README.md` so the case intent is documented in plain language.

Practical rule of thumb:

- if you are changing values, spoof geometry, spoof count, or evaluation logic,
  stay in this harness
- only split into a new stem if the mission topology itself really changes
  enough that the patches stop being readable
