# H01-shadow_behavior_motion

Patch-driven matrix for
[`/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/shadow_behavior_missions/shadow_behavior_motion`](/Users/charlesbenjamin/moos-ivp-cicd-testing/missions/shadow_behavior_missions/shadow_behavior_motion).

This harness focuses on `BHV_Shadow` as the behavior under test. The stem uses
two simulated vehicles: `abe` owns `BHV_Shadow` and shadows `ben`, while `ben`
owns waypoint transits that create controlled contact headings and speeds.
Grading is mission-owned through `pMissionEval` and uses Shadow's contact
telemetry plus bridged NAV speed/heading from both vehicles.

## Cases

- `startup_no_warning_pass` Delays Shadow activation until contact data is available and verifies the default run remains warning-free.
- `static_shadow_pass` Baseline eastbound shadowing with active relevance and matched chaser/contact heading.
- `west_shadow_pass` Forces a westbound contact leg and verifies Shadow reports westbound contact heading without warnings.
- `north_shadow_pass` Forces a north-northeast contact leg without heading wrap and verifies Shadow reports the same contact heading family.
- `heading_wrap_pass` Sends the target onto a northwest-ish leg and verifies Shadow reports a high heading across the 360-degree wrap boundary.
- `turn_north_shadow_pass` Sends the target through an east-to-north dogleg and grades the chaser after the contact turn.
- `slow_contact_speed_pass` Lowers the target waypoint speed and verifies the chaser follows the slower speed regime.
- `fast_contact_speed_pass` Raises the target waypoint speed and verifies the chaser responds with a faster speed regime.
- `stationary_contact_pass` Holds the contact nearly stationary and verifies Shadow reports near-zero contact speed without warning.
- `pwt_outer_active_pass` Sets `pwt_outer_dist` high enough that contact relevance remains active.
- `pwt_outer_edge_pass` Sets `pwt_outer_dist` at the contact-range edge and verifies relevance remains active.
- `pwt_outer_inactive_pass` Sets `pwt_outer_dist` below contact range and verifies zero relevance and stopped chaser motion.
- `runtime_pwt_outer_on_pass` Starts with relevance gated off, then raises `pwt_outer_dist` through `SHADOW_UPDATES` and verifies active shadowing.
- `runtime_pwt_outer_off_pass` Starts active, then lowers `pwt_outer_dist` through `SHADOW_UPDATES` and verifies relevance shuts off.
- `runtime_bad_update_recover_warn_pass` Sends one rejected runtime update followed by a valid update and requires both the expected warning and recovered active relevance.
- `runtime_missing_contact_recover_warn_pass` Temporarily switches Shadow to a missing contact, then restores `contact=ben` and requires warning plus recovered active relevance.
- `heading_widths_pass` Uses custom `heading_peakwidth` and `heading_basewidth` tuning while preserving clean shadowing.
- `speed_width_aliases_pass` Uses accepted `hdg_*` and `spd_*` alias parameters while preserving clean shadowing.
- `no_extrapolate_pass` Verifies clean shadowing with contact extrapolation disabled.
- `cnflag_range_macro_pass` Exercises inherited contact-flag handling by posting a range macro when the target is within 120 meters and requiring the bridged value.
- `missing_contact_warn_pass` Uses a missing contact with `on_no_contact_ok=true` and requires warning-only behavior with no behavior error.
- `missing_contact_fail` Uses a missing contact with `on_no_contact_ok=false` and expects the mission to fail.
- `missing_contact_param_fail` Omits the required `contact` setting and expects the mission to fail.
- `bad_pwt_outer_dist_fail` Negative `pwt_outer_dist` should be rejected and the mission should fail.
- `bad_heading_peakwidth_fail` Negative `heading_peakwidth` should be rejected and the mission should fail.
- `bad_heading_basewidth_fail` Negative `heading_basewidth` should be rejected and the mission should fail.
- `bad_speed_peakwidth_fail` Negative `speed_peakwidth` should be rejected and the mission should fail.
- `bad_speed_basewidth_fail` Negative `speed_basewidth` should be rejected and the mission should fail.
- `bad_decay_fail` Malformed `decay` input should be rejected and the mission should fail.
- `bad_decay_order_fail` Rejects `decay` ranges whose start is greater than the end.
- `bad_extrapolate_fail` Non-boolean `extrapolate` input should be rejected and the mission should fail.
- `bad_on_no_contact_ok_fail` Non-boolean `on_no_contact_ok` input should be rejected and the mission should fail.
- `bad_time_on_leg_fail` Negative inherited `time_on_leg` input should be rejected and the mission should fail.
- `bad_cnflag_fail` Rejects a malformed inherited contact-flag tag.
- `bad_post_per_contact_info_fail` Non-boolean `post_per_contact_info` input should be rejected and the mission should fail.

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

From the paired mission directory, named cases are forwarded to this harness:

```bash
./zlaunch.sh --case=runtime_pwt_outer_off_pass --gui 1
```

The full matrix currently has 35 cases. Normal expected-pass cases require
`BHV_WARNING_SEEN=false`; explicitly named `*_warn_pass` cases require the
expected warning and `BHV_ERROR_SEEN=false`. Wave mode uses isolated temp mission
copies and deterministic two-vehicle port blocks; keep `--port_base` separated
from any other live harness on the same machine.

Source-audit note: `BHV_Shadow` accepts inherited bearing-line configuration
parameters, but it does not call the inherited bearing-line publisher. This
harness therefore does not include a bearing-line output case for Shadow.

Latest validation:

- April 27, 2026
- generated-file matrix: `35/35` cases completed with `--just_make --jobs=2 --port_base=15000`
- wave matrix: `35/35` expected outcomes matched with `--jobs=2 --port_base=15000`
- warp: `10`
