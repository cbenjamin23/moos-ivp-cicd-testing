# NSPatch Notes For H01-cmgr_tests

This harness uses `nspatch` to turn one stem mission into a matrix of short
contact-manager tests without duplicating the whole mission directory.

## Stem And Overlays

The one shared stem mission is:

- `missions/cmgr_tests/meta_vehicle.moos`
- `missions/cmgr_tests/meta_shoreside.moos`

For each harness case, `nspatch` may generate temporary overlay files:

- `missions/cmgr_tests/meta_vehicle.moosx`
- `missions/cmgr_tests/meta_shoreside.moosx`
- `missions/cmgr_tests/meta_vehicle.bhvx`

The mission launchers already use `nsplug -x`, so if those `*.moosx` or
`*.bhvx` files exist they are folded into the generated `targ_*` files for that
case.

Important architecture point:

- the harness does not call the stem mission's `zlaunch.sh`
- it patches the stem mission directly
- then it calls shared `xlaunch.sh`, which calls the stem mission `launch.sh`

That is why the harness can own patching, cleanup, and results parsing without
stacking two automation wrappers on top of each other.

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

1. Run `clean.sh` and `ktm` in the stem mission directory.
2. Apply the selected case patches with `nspatch`.
3. Run the mission through the harness-controlled `xlaunch.sh` path.
4. Read the mission-local `results.txt`.
5. Compare the observed mission result against the harness expectation.

## Cleanup And Safety

The harness treats the stem overlay files as part of the normal mission
workspace. `clean.sh` already removes `meta_*.moosx` and `meta_*.bhvx`, so the
harness relies on `clean.sh` before each case and one final `clean.sh` on exit.

That means:

- one case should not leak patches into the next case
- the harness assumes this repo is being used in the normal harness-driven way
- do not run two live harness invocations in parallel against this same stem
  mission directory, because they will race on the temporary overlay files

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
4. Add the case to the default `CASES=` list if it should run in the full
   matrix.
5. Update `README.md` so the case intent is documented in plain language.

Practical rule of thumb:

- if you are changing values, spoof geometry, spoof count, or evaluation logic,
  stay in this harness
- only split into a new stem if the mission topology itself really changes
  enough that the patches stop being readable
