# NSPATCH Notes

This harness patches an isolated copy of the moving stem in
[`/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/obmgr_missions/obmgr_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/obmgr_missions/obmgr_motion)
for every selected case. The source stem is never patched in place.

The patch flow is:
- copy the complete stem into the case work directory
- start from the copied `meta_shoreside.moos` and `meta_vehicle.moos`
- replace whole `ProcessConfig = ...` blocks with `nspatch`
- write generated overlays beside the copied stems as `meta_shoreside.moosx`
  and `meta_vehicle.moosx`
- let `launch.sh` consume those overlays when generating the final `targ_*.moos` and `targ_*.bhv`

Single-case, serial, and rolling runs all use this same copy-and-patch path.
Cleanup is scoped to the unique harness-owned invocation root, and
`--keep_workdirs` retains that root for inspection.
