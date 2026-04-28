# H01-cutrange_behavior_motion

Patch-driven matrix for
[`/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/cutrange_behavior_missions/cutrange_behavior_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/cutrange_behavior_missions/cutrange_behavior_motion).

This harness focuses on `BHV_CutRange` as the behavior under test. The stem uses
two simulated vehicles: `abe` owns `BHV_CutRange` and tries to reduce range to
`ben`, while `ben` owns simple waypoint or station-keep behaviors that create
controlled contact geometry. Grading is mission-owned through `pMissionEval` and
uses `pursueflag`/`giveupflag`, `uFldCollisionDetect` closest-range telemetry,
bridged NAV speed from both vehicles, and behavior warning/error mail.

## Cases

- `startup_no_warning_pass` Verifies the default CutRange behavior enters pursuit shortly after deployment without behavior warning or error.
- `static_cutrange_pass` Baseline eastbound closure where the chaser reduces target range below the default threshold.
- `crossing_cutrange_pass` Starts the target on a northbound crossing leg and verifies CutRange still produces meaningful closure.
- `headon_cutrange_pass` Starts the target head-on and grades the rapid approach under opposing headings.
- `angled_entry_pass` Starts both vehicles at offset positions/headings and verifies recovery into an active closure maneuver.
- `right_turn_target_pass` Sends the target through an east-to-north dogleg and only evaluates after the target is on the northbound leg.
- `s_turn_target_pass` Sends the target through a multi-leg S-shaped route and only evaluates after the target is on the southbound middle leg.
- `slow_target_pass` Slows the target and requires close approach while confirming the target is in the slower speed regime.
- `fast_target_pass` Speeds the target up and gives the chaser longer to demonstrate range reduction without warnings.
- `stationary_target_pass` Holds the target nearly stationary and verifies the chaser can close on a low-speed contact.
- `short_time_on_leg_pass` Uses a short `time_on_leg` horizon and verifies closure remains valid.
- `long_time_on_leg_pass` Uses a long `time_on_leg` horizon and verifies closure remains valid.
- `low_patience_pass` Uses low `patience` weighting while still reducing target range.
- `high_patience_pass` Uses high `patience` weighting with a wider `max_patience` while still reducing target range.
- `max_patience_clamp_pass` Sets `patience` higher than `max_patience` and verifies the behavior clamps cleanly.
- `max_patience_100_pass` Sets both `patience` and `max_patience` to 100 and verifies the old 85 cap can be overridden cleanly.
- `pwt_outer_active_pass` Raises `pwt_outer_dist` and verifies active CutRange closure remains valid.
- `pwt_inner_zero_relevance_pass` Sets the inner range above the starting contact range and verifies zero-relevance stopped behavior without warnings.
- `pwt_equal_zero_relevance_pass` Sets equal inner/outer relevance distances and verifies the zero-total-range case stays clean and inactive.
- `giveup_start_far_pass` Starts the target beyond `giveup_range` and verifies CutRange never enters pursuit or posts giveup from a cold start.
- `giveup_dist_alias_pass` Uses the `giveup_dist` alias accepted by `BHV_CutRange` and verifies it behaves like `giveup_range`.
- `giveup_hysteresis_pass` Starts pursuit near the one-meter giveup buffer and verifies the behavior remains in pursuit without posting giveup, allowing a small range tolerance for startup timing.
- `runtime_pwt_on_pass` Starts relevance gated off, then lowers the inner/outer distances through `CUTRANGE_UPDATES` and verifies recovered closure.
- `runtime_pwt_off_pass` Starts active, then raises inner/outer distances through `CUTRANGE_UPDATES` and verifies relevance shuts off cleanly.
- `runtime_patience_update_pass` Starts with low `patience`, updates it at runtime, and verifies the behavior continues to close range.
- `runtime_giveup_update_pass` Lowers `giveup_range` at runtime and requires the behavior to post the configured giveup flag and stop pursuing.
- `runtime_bad_update_recover_warn_pass` Sends one rejected runtime update, then valid recovery updates, and requires warning-only recovery with range closure.
- `no_extrapolate_pass` Verifies normal closure when contact extrapolation is disabled.
- `missing_contact_warn_pass` Uses a missing contact with `on_no_contact_ok=true` and requires warning-only behavior with no behavior error.
- `missing_contact_fail` Uses a missing contact with `on_no_contact_ok=false` and expects the mission grade to fail.
- `missing_contact_param_fail` Omits the required `contact` setting and expects the mission grade to fail.
- `bad_pwt_outer_dist_fail` Negative `pwt_outer_dist` should be rejected and the mission should fail.
- `bad_pwt_inner_dist_fail` Rejects an inner relevance distance greater than the outer distance.
- `bad_giveup_range_fail` Negative `giveup_range` should be rejected and the mission should fail.
- `bad_patience_fail` Negative `patience` should be rejected and the mission should fail.
- `bad_max_patience_fail` Out-of-range `max_patience` should be rejected and the mission should fail.
- `bad_time_on_leg_fail` Negative inherited `time_on_leg` should be rejected and the mission should fail.
- `bad_decay_fail` Malformed inherited `decay` input should be rejected and the mission should fail.
- `bad_legacy_util_param_fail` Confirms legacy AOF-only utility parameters are not accepted as top-level `BHV_CutRange` config.

## Running

From this harness directory:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=static_cutrange_pass 10
./zlaunch.sh --case=headon_cutrange_pass --gui 1
./zlaunch.sh --case=right_turn_target_pass --gui 1
./zlaunch.sh --case=s_turn_target_pass --gui 1
./zlaunch.sh --case=runtime_giveup_update_pass --gui 1
./zlaunch.sh --jobs=4 --port_base=25000 10
```

From the paired mission directory, named cases are forwarded to this harness:

```bash
./zlaunch.sh --case=runtime_pwt_on_pass --gui 1
```

The full matrix currently has 39 cases. Normal expected-pass cases clear the
known first-active contact warm-up warning before evaluation, then require no
settled chaser-side behavior warnings or errors; explicitly named `*_warn_pass`
cases require the expected warning and `BHV_ERROR_SEEN=false`. Wave mode uses
isolated temp mission copies and deterministic two-vehicle port blocks. Each
case uses `case_base + 0..2` for MOOSDB and `case_base + 10..12` for pShare
inside a 30-port stride; keep `--port_base` separated from any other live
harness on the same machine.

The dogleg and S-turn geometry cases use evaluator gates on `BEN_NAV_X`,
`BEN_NAV_Y`, and `BEN_NAV_HEADING`, then grade current range at that moment.
That keeps them from passing solely because the vehicles happened to get close
before the target reached the turn geometry under test.

Latest validation:

- April 26, 2026
- full matrix: `39/39` expected outcomes matched
- warp: `10`
- command: `./zlaunch.sh --jobs=4 --port_base=25000 10`
