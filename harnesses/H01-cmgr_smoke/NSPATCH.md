# NSPatch Notes For H01-cmgr_smoke

This harness uses `nspatch` to turn one stem mission into a matrix of short
contact-manager tests without duplicating the whole mission directory.

## Stem And Overlays

The stem mission is:

- `missions/cmgr_smoke/meta_vehicle.moos`
- `missions/cmgr_smoke/meta_shoreside.moos`

For each harness case, `nspatch` may generate temporary overlay files:

- `missions/cmgr_smoke/meta_vehicle.moosx`
- `missions/cmgr_smoke/meta_shoreside.moosx`
- `missions/cmgr_smoke/meta_vehicle.bhvx`

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
  For threshold cases such as `alert_range`, `cpa_range`, or contact aging.
- `ProcessConfig = uTimerScript`
  For spoof timing, spoof geometry, or the evaluation checkpoint timing.
- `ProcessConfig = pMissionEval`
  For case-specific grading rules, such as detection, non-detection, closest
  contact, or range band checks.

In simple terms:

- vehicle-side patches usually change what contact manager sees
- shoreside patches usually change when the case is graded and what gets
  written to `results.txt`

## Harness Flow

For each case:

1. Remove any active `*.moosx` / `*.bhvx` overlay files from the stem mission.
2. Apply the selected case patches with `nspatch`.
3. Run the mission through the harness-controlled `xlaunch.sh` path.
4. Read the mission-local `results.txt`.
5. Compare the observed mission result against the harness expectation.

## Cleanup And Safety

The harness backs up any pre-existing `meta_*.moosx` and `meta_*.bhvx` files
before starting. On exit it restores those originals and removes the temporary
case-generated overlays.

That means the harness is intended to be non-destructive:

- one case should not leak patches into the next case
- running the harness should not permanently overwrite existing overlay files
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

- if you are changing only values or evaluation logic, stay in this harness
- if you need a different mission shape or more actors, create a new stem
  mission and a new harness

## Design Guideline

Keep this harness focused on one stem topology. Use `nspatch` for value and
logic changes. If a new test needs a substantially different topology, add a
new stem mission and a new harness rather than forcing everything into this one.
