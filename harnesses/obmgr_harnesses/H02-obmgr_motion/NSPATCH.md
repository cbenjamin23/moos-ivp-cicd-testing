# NSPATCH Notes

This harness patches the moving stem in
[`/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/obmgr_missions/obmgr_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/obmgr_missions/obmgr_motion)
in place before each run.

The patch flow is:
- start from `meta_shoreside.moos` and `meta_vehicle.moos`
- replace whole `ProcessConfig = ...` blocks with `nspatch`
- write generated overlays as `meta_shoreside.moosx` and `meta_vehicle.moosx`
- let `launch.sh` consume those overlays when generating the final `targ_*.moos` and `targ_*.bhv`

The harness clears generated overlays before each case and removes them again in
cleanup. It assumes the mission directory is owned by the harness workflow.
