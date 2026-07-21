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

- `static_shadow_pass` Runs the default eastbound target at speed `1.8` with unrestricted `pwt_outer_dist=0`; passes at 45 seconds with positive relevance, contact and vehicle headings in `[70,110]`, Abe's speed in `[1,2]`, Ben's speed at least `1`, and no warning or error.
- `west_shadow_pass` Commands Ben to heading `270`, exercising westbound contact tracking; passes at 80 seconds when Shadow reports contact heading in `[245,310]`, Ben is in the same heading band, relevance is positive, both vehicles are moving, and no warning or error occurred.
- `north_shadow_pass` Commands Ben to heading `20`, exercising contact tracking near north without crossing zero; passes at 55 seconds when Shadow and Ben headings are at most `35`, relevance is positive, both vehicles are moving, and no warning or error occurred.
- `heading_wrap_pass` Commands Ben to heading `350`, exercising Shadow's contact heading across the `0/360` boundary; passes at 60 seconds when Shadow and Ben headings are at least `330`, Abe's heading is at most `40`, relevance is positive, all motion bounds hold, and no warning or error occurred.
- `turn_north_shadow_pass` Changes Ben's commanded heading from `90` to `20` at 45 seconds, exercising Shadow after a live target turn; passes at 95 seconds when Shadow and Ben headings are at most `55`, Abe's heading is at most `70`, relevance is positive, all three speed floors hold, and no warning or error occurred.
- `slow_contact_speed_pass` Sets Ben's commanded speed to `0.8`, exercising slow-contact speed matching; passes when Shadow reports contact speed in `[0.55,1.05]`, Abe moves in `[0.45,1.15]`, Ben moves in `[0.5,1.05]`, Abe remains eastbound, and no warning or error occurred.
- `fast_contact_speed_pass` Sets Ben's commanded speed to `2.6`, exercising fast-contact speed matching; passes when Shadow reports contact speed in `[2,2.8]`, Abe moves at least `1.3`, Ben moves in `[1.7,2.8]`, Abe remains eastbound, and no warning or error occurred.
- `stationary_contact_pass` Sets Ben's commanded speed to `0`, exercising a stationary contact; passes when relevance remains positive, Shadow reports contact speed at most `0.2`, both vehicle speeds are at most `0.35`, and no warning or error occurred.
- `pwt_outer_inactive_pass` Sets `pwt_outer_dist=20`, below the starting contact range, exercising relevance gating; passes when `SHADOW_RELEVANCE=0`, Abe's speed is at most `0.35`, Ben keeps moving at least `1`, and no warning or error occurred.
- `runtime_pwt_outer_on_pass` Starts with `pwt_outer_dist=20` and raises it to `120` through `SHADOW_UPDATES` at 12 seconds, exercising runtime activation of relevance; passes with the default positive-relevance, eastbound heading, speed, and warning-free motion grade.
- `runtime_pwt_outer_off_pass` Starts with unrestricted relevance and lowers `pwt_outer_dist` to `5` at 38 seconds, exercising runtime deactivation; passes at 45 seconds when relevance is zero, Abe is stopped, Ben is still moving, and no warning or error occurred.
- `runtime_bad_update_recover_warn_pass` Starts inactive at `pwt_outer_dist=20`, rejects runtime `-5`, then accepts `120`; ordered evaluation requires the exact `param_error` and warning, exact success for `120`, then positive relevance with Shadow, Abe, and Ben moving before the deadline.
- `runtime_missing_contact_recover_warn_pass` Accepts `contact=ghost`, observes its missing-contact warning, then accepts `contact=ben` and `pwt_outer_dist=120`; ordered evaluation requires all three exact update values before positive relevance and moving Shadow/Abe/Ben evidence.
- `cnflag_range_macro_pass` Configures `cnflag=<120 SHADOW_RANGE_NEAR=$[RANGE]`, exercising inherited range-macro expansion; passes when relevance is positive, `SHADOW_RANGE_NEAR` is greater than zero and at most `120`, and no warning or error occurred.
- `missing_contact_warn_pass` Sets `contact=ghost` with `on_no_contact_ok=true`; the live case passes on a pre-deadline warning with no behavior error, and direct CTest requires the exact `shadow_contact: ghost x/y/heading/speed info not found` payload.
- `missing_contact_fail` Sets the same absent contact with `on_no_contact_ok=false`; the harness passes on a pre-deadline behavior error, and direct CTest requires the same exact missing-contact payload on `BHV_ERROR` rather than `BHV_WARNING`.
- `bad_pwt_outer_dist_fail` Sets `pwt_outer_dist=-1`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires Shadow to reject this exact value.
- `bad_heading_peakwidth_fail` Sets `heading_peakwidth=-5`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires Shadow to reject this exact value.
- `bad_heading_basewidth_fail` Sets `heading_basewidth=-5`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires Shadow to reject this exact value.
- `bad_speed_peakwidth_fail` Sets `speed_peakwidth=-0.1`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires Shadow to reject this exact value.
- `bad_speed_basewidth_fail` Sets `speed_basewidth=-1`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires Shadow to reject this exact value.
- `bad_decay_fail` Sets `decay=bad`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires the inherited parser to reject this exact value.
- `bad_decay_order_fail` Sets `decay=60,30`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires rejection when decay start exceeds decay end.
- `bad_extrapolate_fail` Sets `extrapolate=maybe`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires the inherited boolean parser to reject it.
- `bad_on_no_contact_ok_fail` Sets `on_no_contact_ok=maybe`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires the inherited boolean parser to reject it.
- `bad_time_on_leg_fail` Sets inherited `time_on_leg=-1`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires rejection of the negative duration.
- `bad_cnflag_fail` Sets inherited `cnflag=soon SHADOW_RANGE_NEAR=$[RANGE]`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires the contact-flag parser to reject the unsupported tag.
- `bad_post_per_contact_info_fail` Sets inherited `post_per_contact_info=maybe`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires the boolean parser to reject it.

## Direct CTest coverage

`BHVShadowTest` now isolates contracts that the simulator matrix could not:
range 119 is relevant for `pwt_outer_dist=120` while the exact 120-meter
boundary is not; `heading_peakwidth=8` and `heading_basewidth=80` change the
objective at 130 and 170 degrees; canonical and short width names produce the
same utility at every course/speed sample; and missing-contact policy selects
the exact warning or error payload. The duplicate startup case, two broad
outer-distance cases, and two motion-only width cases were removed in favor of
those direct checks. `no_extrapolate_pass` was also removed because the current
upstream extrapolation block is compiled out; `ACOL-UP-001` records that
upstream contract rather than pretending the live case distinguishes it.

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

The full matrix currently has 28 cases. Normal expected-pass cases require
`BHV_WARNING_SEEN=false`; explicitly named `*_warn_pass` cases require the
expected warning and `BHV_ERROR_SEEN=false`. Serial and rolling modes both use
isolated temp mission copies and deterministic two-vehicle port blocks; keep
`--port_base` separated from any other live harness on the same machine.

Source-audit note: `BHV_Shadow` accepts inherited bearing-line configuration
parameters, but it does not call the inherited bearing-line publisher. This
harness therefore does not include a bearing-line output case for Shadow.

Latest validation:

- July 20, 2026
- generated-file matrix: `28/28` cases completed with isolated copies and distinct three-community port blocks
- minimal-logging matrix: `28/28` cases passed
- full-logging matrix: `28/28` cases passed
- focused `BHVShadowTest`: `8/8` direct cases passed
- focused `behaviors-marine` family: `218/218` CTests passed
- six deliberate mutations failed for the intended reason: wrong recovery
  distance, omitted ghost phase, repaired malformed distance, shifted relevance
  boundary, restored default objective widths, and selecting warning policy
  where an error was required
- warp: `10`
