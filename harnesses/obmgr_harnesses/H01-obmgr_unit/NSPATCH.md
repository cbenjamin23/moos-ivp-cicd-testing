# NSPatch Notes For H01-obmgr_unit

This harness uses `nspatch` to turn one obstacle-manager stem mission into a
matrix of short `pObstacleMgr` tests without copying the whole mission.

## Stem And Overlays

The shared stem mission is:

- `missions/obmgr_missions/obmgr_unit/meta_vehicle.moos`
- `missions/obmgr_missions/obmgr_unit/meta_shoreside.moos`

For each harness case, `nspatch` may generate:

- `missions/obmgr_missions/obmgr_unit/meta_vehicle.moosx`
- `missions/obmgr_missions/obmgr_unit/meta_shoreside.moosx`
- `missions/obmgr_missions/obmgr_unit/meta_vehicle.bhvx`

The launchers already use `nsplug -x`, so if these overlay files exist they are
folded into the generated `targ_*` files for that case.

## What Gets Patched

Typical patch targets in this harness:

- `ProcessConfig = pObstacleMgr`
  For alert ranges, general-alert configuration, lasso mode, ignore range,
  point aging, duration limits, or enable/disable/expunge variables.
- `ProcessConfig = uTimerScript`
  For obstacle injection timing and the actual obstacle or point messages sent
  into `pObstacleMgr`.
- `ProcessConfig = pMissionEval`
  For case-specific grading rules such as accepted/not accepted, alerted/not
  alerted, resolved obstacle, distance-band checks, or filter-message checks.
- `ProcessConfig = uFldNodeBroker`
  Only when a case needs an extra bridged variable on shoreside.

In simple terms:

- vehicle-side patches usually change what obstacle manager sees
- shoreside patches usually change when the case is graded and what gets
  written to `results.txt`

## Harness Flow

For each case:

1. Run `clean.sh` and scoped harness teardown in the stem mission directory.
2. Apply the selected case patches with `nspatch`.
3. Run the mission through the harness-controlled `xlaunch.sh` path.
4. Read the mission-local `results.txt`.
5. Compare the observed mission result against the harness expectation.

For the two general-alert name cases, the harness also checks that the final
results line contains the exact expected `GEN_OBSTACLE_ALERT` token. This is a
small exception to the normal pattern because `pMissionEval` cannot directly
compare strings that contain their own `=` assignments. The filter-message
cases now grade on mission-side confirmation flags instead.

## Cleanup And Safety

The harness treats the stem overlay files as normal mission workspace files.
`clean.sh` already removes `meta_*.moosx` and `meta_*.bhvx`, so the harness
relies on `clean.sh` before each case and one final `clean.sh` on exit.

That means:

- one case should not leak patches into the next case
- the harness assumes this repo is being used in the normal harness-driven way
- do not run two live harness invocations in parallel against this same stem
  mission directory, because they will race on the temporary overlay files

## Adding A New Case

Typical pattern:

1. Decide whether the case changes:
   - obstacle inputs only
   - evaluation only
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
