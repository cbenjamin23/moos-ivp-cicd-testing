# NSPatch Notes For H02-cmgr_multi

This harness uses `nspatch` to turn one multi-contact stem mission into a
matrix of short contact-manager tests without duplicating the whole mission
directory.

## Stem And Overlays

The stem mission is:

- `missions/cmgr_multi/meta_vehicle.moos`
- `missions/cmgr_multi/meta_shoreside.moos`

For each harness case, `nspatch` may generate temporary overlay files:

- `missions/cmgr_multi/meta_vehicle.moosx`
- `missions/cmgr_multi/meta_shoreside.moosx`
- `missions/cmgr_multi/meta_vehicle.bhvx`

The mission launchers already use `nsplug -x`, so if those overlay files exist
they are folded into the generated `targ_*` files for that case.

Important architecture point:

- the harness does not call the stem mission's `zlaunch.sh`
- it patches the stem mission directly
- then it calls shared `xlaunch.sh`, which calls the stem mission `launch.sh`

## What Gets Patched

Typical patch targets in this harness:

- `ProcessConfig = uTimerScript`
  For multi-contact geometry, delayed second contact arrival, or later
  evaluation checkpoints.
- `ProcessConfig = pContactMgrV20`
  For aging or stale-contact tests.
- `ProcessConfig = pMissionEval`
  For case-specific grading rules around count, list, closest-contact choice,
  and expiry behavior.

In simple terms:

- vehicle-side patches usually change which contacts are present and when
- shoreside patches usually change when the case is graded and what the mission
  considers a pass

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
   - contact geometry only
   - evaluation timing only
   - aging behavior only
   - or a combination of those
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

- if you are still on one-ownship, two-contact topology, stay in this harness
- if you need another mission shape again, add another stem and another harness
