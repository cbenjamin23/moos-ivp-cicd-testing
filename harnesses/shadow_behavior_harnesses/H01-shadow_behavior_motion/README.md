# H01-shadow_behavior_motion

Logging is minimal by default in both communities. Use `--log=full` for the
whole matrix or with `--case=NAME` for one diagnostic case.

Patch-driven matrix for
[`missions/shadow_behavior_missions/shadow_behavior_motion`](../../../missions/shadow_behavior_missions/shadow_behavior_motion).

This harness focuses on `BHV_Shadow` as the behavior under test. The stem uses
two simulated vehicles: `abe` owns `BHV_Shadow` and shadows `ben`, while `ben`
owns waypoint transits that create controlled contact headings and speeds.
Grading is mission-owned through `pMissionEval` and uses Shadow's contact
telemetry plus bridged NAV speed/heading from both vehicles.

## Cases

- `startup_no_warning_pass` Leaves Shadow inactive until `contact=ben` at 30 seconds and `SHADOW=true` at 32 seconds, exercising startup after contact data has arrived; passes at 45 seconds with positive relevance, eastbound contact and vehicle headings, all three speeds above their lower bounds, and no behavior warning or error.
- `static_shadow_pass` Runs the default eastbound target at speed `1.8` with unrestricted `pwt_outer_dist=0`; passes at 45 seconds with positive relevance, contact and vehicle headings in `[70,110]`, Abe's speed in `[1,2]`, Ben's speed at least `1`, and no warning or error.
- `west_shadow_pass` Commands Ben to heading `270`, exercising westbound contact tracking; passes at 80 seconds when Shadow reports contact heading in `[245,310]`, Ben is in the same heading band, relevance is positive, both vehicles are moving, and no warning or error occurred.
- `north_shadow_pass` Commands Ben to heading `20`, exercising contact tracking near north without crossing zero; passes at 55 seconds when Shadow and Ben headings are at most `35`, relevance is positive, both vehicles are moving, and no warning or error occurred.
- `heading_wrap_pass` Commands Ben to heading `350`, exercising Shadow's contact heading across the `0/360` boundary; passes at 60 seconds when Shadow and Ben headings are at least `330`, Abe's heading is at most `40`, relevance is positive, all motion bounds hold, and no warning or error occurred.
- `turn_north_shadow_pass` Changes Ben's commanded heading from `90` to `20` at 45 seconds, exercising Shadow after a live target turn; passes at 95 seconds when Shadow and Ben headings are at most `55`, Abe's heading is at most `70`, relevance is positive, all three speed floors hold, and no warning or error occurred.
- `slow_contact_speed_pass` Sets Ben's commanded speed to `0.8`, exercising slow-contact speed matching; passes when Shadow reports contact speed in `[0.55,1.05]`, Abe moves in `[0.45,1.15]`, Ben moves in `[0.5,1.05]`, Abe remains eastbound, and no warning or error occurred.
- `fast_contact_speed_pass` Sets Ben's commanded speed to `2.6`, exercising fast-contact speed matching; passes when Shadow reports contact speed in `[2,2.8]`, Abe moves at least `1.3`, Ben moves in `[1.7,2.8]`, Abe remains eastbound, and no warning or error occurred.
- `stationary_contact_pass` Sets Ben's commanded speed to `0`, exercising a stationary contact; passes when relevance remains positive, Shadow reports contact speed at most `0.2`, both vehicle speeds are at most `0.35`, and no warning or error occurred.
- `pwt_outer_active_pass` Sets `pwt_outer_dist=120`, exercising the outer-distance relevance setting during the default eastbound run; passes with the default positive-relevance, heading, speed, and warning-free motion grade.
- `pwt_outer_edge_pass` Sets `pwt_outer_dist=100`, exercising the configured outer-distance boundary; passes at 36 seconds when relevance is positive, Shadow's contact speed and Ben's speed are at least `1`, Abe's speed is at least `0.8`, and no warning or error occurred.
- `pwt_outer_inactive_pass` Sets `pwt_outer_dist=20`, below the starting contact range, exercising relevance gating; passes when `SHADOW_RELEVANCE=0`, Abe's speed is at most `0.35`, Ben keeps moving at least `1`, and no warning or error occurred.
- `runtime_pwt_outer_on_pass` Starts with `pwt_outer_dist=20` and raises it to `120` through `SHADOW_UPDATES` at 12 seconds, exercising runtime activation of relevance; passes with the default positive-relevance, eastbound heading, speed, and warning-free motion grade.
- `runtime_pwt_outer_off_pass` Starts with unrestricted relevance and lowers `pwt_outer_dist` to `5` at 38 seconds, exercising runtime deactivation; passes at 45 seconds when relevance is zero, Abe is stopped, Ben is still moving, and no warning or error occurred.
- `runtime_bad_update_recover_warn_pass` Posts invalid `pwt_outer_dist=-5` at 11 seconds and valid `pwt_outer_dist=120` at 38 seconds, exercising recovery after a rejected update; passes at 50 seconds with any warning observed, no behavior error, positive relevance, moving contact, and Abe moving at least `0.8`.
- `runtime_missing_contact_recover_warn_pass` Changes the contact to `ghost` at 10 seconds, restores `contact=ben` at 30 seconds, and sets `pwt_outer_dist=120` at 38 seconds, exercising recovery from a missing runtime contact; passes at 50 seconds with any warning observed, no behavior error, positive relevance, moving contact, and Abe moving at least `0.8`.
- `heading_widths_pass` Sets `heading_peakwidth=8` and `heading_basewidth=80`, exercising nondefault heading objective widths; passes with the default positive-relevance, eastbound heading, speed, and warning-free motion grade.
- `speed_width_aliases_pass` Configures `hdg_peakwidth=25`, `hdg_basewidth=170`, `spd_peakwidth=0.3`, and `spd_basewidth=1.5`, exercising the accepted width aliases; passes with the default positive-relevance, eastbound heading, speed, and warning-free motion grade.
- `no_extrapolate_pass` Sets `extrapolate=false`, exercising shadowing from the latest contact report without projection; passes with the default positive-relevance, eastbound heading, speed, and warning-free motion grade.
- `cnflag_range_macro_pass` Configures `cnflag=<120 SHADOW_RANGE_NEAR=$[RANGE]`, exercising inherited range-macro expansion; passes when relevance is positive, `SHADOW_RANGE_NEAR` is greater than zero and at most `120`, and no warning or error occurred.
- `missing_contact_warn_pass` Sets `contact=ghost` with `on_no_contact_ok=true`, exercising warning-only handling of an unavailable contact; passes at 20 seconds when any `BHV_WARNING` has been observed and no `BHV_ERROR` has been observed.
- `missing_contact_fail` Sets `contact=ghost` with `on_no_contact_ok=false`, exercising the error path for an unavailable required contact; the harness passes by 20 seconds when any `BHV_ERROR` publication is observed.
- `missing_contact_param_fail` Omits the required `contact` setting, exercising configuration rejection; the harness currently passes when any `ABE_IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_pwt_outer_dist_fail` Sets `pwt_outer_dist=-1`, exercising rejection of a negative outer relevance distance; the harness currently passes when any `ABE_IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_heading_peakwidth_fail` Sets `heading_peakwidth=-5`, exercising rejection of a negative heading peak width; the harness currently passes when any `ABE_IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_heading_basewidth_fail` Sets `heading_basewidth=-5`, exercising rejection of a negative heading base width; the harness currently passes when any `ABE_IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_speed_peakwidth_fail` Sets `speed_peakwidth=-0.1`, exercising rejection of a negative speed peak width; the harness currently passes when any `ABE_IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_speed_basewidth_fail` Sets `speed_basewidth=-1`, exercising rejection of a negative speed base width; the harness currently passes when any `ABE_IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_decay_fail` Sets `decay=bad`, exercising rejection of malformed contact extrapolation decay; the harness currently passes when any `ABE_IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_decay_order_fail` Sets `decay=60,30`, exercising rejection of a decay start greater than its end; the harness currently passes when any `ABE_IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_extrapolate_fail` Sets `extrapolate=maybe`, exercising rejection of a non-boolean extrapolation setting; the harness currently passes when any `ABE_IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_on_no_contact_ok_fail` Sets `on_no_contact_ok=maybe`, exercising rejection of a non-boolean missing-contact policy; the harness currently passes when any `ABE_IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_time_on_leg_fail` Sets inherited `time_on_leg=-1`, exercising rejection of a negative leg duration; the harness currently passes when any `ABE_IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_cnflag_fail` Sets malformed inherited `cnflag=soon SHADOW_RANGE_NEAR=$[RANGE]`, exercising contact-flag parser rejection; the harness currently passes when any `ABE_IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.
- `bad_post_per_contact_info_fail` Sets inherited `post_per_contact_info=maybe`, exercising rejection of a non-boolean publication setting; the harness currently passes when any `ABE_IVPHELM_STATE` publication sets its `HELM_MALCONFIG` latch.

## Running

From this harness directory:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=static_shadow_pass 10
./zlaunch.sh --case=west_shadow_pass --gui 1
./zlaunch.sh --case=turn_north_shadow_pass --gui 1
./zlaunch.sh --case=pwt_outer_inactive_pass --gui 1
./zlaunch.sh --case=runtime_pwt_outer_on_pass --gui 1
./zlaunch.sh --jobs=2 --port_base=33000 10
```

The paired mission `zlaunch.sh` remains a thin single-scenario wrapper. Run
named cases and multi-case jobs from this harness directory.

The full matrix currently has 35 cases. Normal expected-pass cases require
`BHV_WARNING_SEEN=false`; explicitly named `*_warn_pass` cases require the
expected warning and `BHV_ERROR_SEEN=false`. Serial and rolling modes both use
isolated temp mission copies and deterministic two-vehicle port blocks; keep
`--port_base` separated from any other live harness on the same machine.

Source-audit note: `BHV_Shadow` accepts inherited bearing-line configuration
parameters, but it does not call the inherited bearing-line publisher. This
harness therefore does not include a bearing-line output case for Shadow.

Latest validation:

- July 16, 2026
- generated-file matrix: `35/35` cases completed with isolated copies and distinct three-community port blocks
- rolling matrices: three valid `35/35` passes with `--jobs=4`
- serial matrix: `35/35` passes with `--jobs=1`
- warp: `10`
