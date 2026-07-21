# H01-trail_behavior_motion

Logging is minimal by default in both communities. Use `--log=full` for the
whole matrix or with `--case=NAME` for one diagnostic case.

Patch-driven matrix for
[`missions/trail_behavior_missions/trail_behavior_motion`](../../../missions/trail_behavior_missions/trail_behavior_motion).

This harness focuses on `BHV_Trail` as the behavior under test. The stem uses
two simulated vehicles: `abe` owns `BHV_Trail` and chases `ben`, while `ben`
owns a simple waypoint leg. Grading is mission-owned through `pMissionEval` and
uses behavior outputs such as:

- `TRAIL_DISTANCE`
- `TRAIL_RELEVANCE`
- `TRAIL_RANGE`
- `PURSUIT`
- `REGION`
- `BHV_WARNING`
- `BHV_ERROR`

## Cases

- `static_trail_pass` Configures a relative `180`-degree trail point `45` meters behind Ben with `radius=8` and `nm_radius=18`; passes at 45 seconds when `TRAIL_DISTANCE<=20`, `PURSUIT=1`, relevance is positive, and no `BHV_ERROR` occurred.
- `lead_right_turn_pass` Replaces Ben's straight leg with points `(55,-80):(55,-35):(125,-35)`, exercising pursuit through a right-angle target turn; passes at 80 seconds when Ben has reached `x>=50,y>=-45`, `TRAIL_DISTANCE<=45`, pursuit and relevance are active, and no error occurred.
- `pwt_outer_inactive_pass` Sets `pwt_outer_dist=30`, below the observed contact range; passes when `TRAIL_RANGE>30`, `PURSUIT=0`, relevance is zero, and no error occurred.
- `runtime_relevance_off_pass` Posts `pwt_outer_dist=5` at 12 seconds, exercising runtime relevance shutdown; passes at 35 seconds when `TRAIL_RANGE>5`, `PURSUIT=0`, relevance is zero, and no error occurred.
- `runtime_bad_update_recover_pass` Rejects `trail_range=-5` before accepting the distinct recovery value `70`; ordered evaluation requires the exact `param_error`, a warning, exact success for `70`, then `TRAIL_DISTANCE<=40`, active pursuit, and positive relevance before the deadline.
- `idle_post_distance_pass` Sets `post_trail_dist_on_idle=true` and turns `TRAIL=false` at 25 seconds, exercising distance publication while idle; passes at 35 seconds when `PURSUIT=0`, `TRAIL_DISTANCE<999`, and no error occurred.
- `idle_no_post_distance_pass` Sets `post_trail_dist_on_idle=false`, turns `TRAIL=false` at 25 seconds, and writes sentinel `TRAIL_DISTANCE=999` at 28 seconds; passes at 35 seconds when the sentinel remains `999`, `PURSUIT=0`, and no error occurred.
- `auto_alert_request_pass` Starts a spawn-templated Trail with an updates variable and `no_alert_request=false`; the live case requires a `BCM_ALERT_REQUEST` before active pursuit, while direct CTest requires the complete request payload and proves each suppression gate.
- `missing_contact_warn_pass` Sets `contact=ghost` with `on_no_contact_ok=true`; the live case leads on a pre-deadline warning with no behavior error, and direct CTest requires exact `trail_contact: ghost x/y/heading/speed info not found` warning text.
- `missing_contact_fail` Sets the same absent contact with `on_no_contact_ok=false`; the live case leads on a pre-deadline behavior error, and direct CTest requires the exact ghost-contact message on `BHV_ERROR`.
- `missing_contact_param_fail` Omits `contact` with `on_no_contact_ok=false`; the live case leads on a pre-deadline behavior error, and direct CTest requires exact `trail_contact: contact IDX not set.` text.
- `bad_trail_angle_type_fail` Sets `trail_angle_type=diagonal`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires rejection of this exact type.
- `bad_trail_angle_fail` Sets `trail_angle=west`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires rejection of this exact nonnumeric angle.
- `bad_trail_range_fail` Sets `trail_range=-1`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires rejection of this exact negative range.
- `bad_mod_trail_range_pct_fail` Sets `mod_trail_range_pct=0`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires rejection of this nonpositive percentage.
- `bad_radius_fail` Sets `radius=-1`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires rejection of this exact radius.
- `bad_nm_radius_fail` Sets `nm_radius=-1`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires rejection of this exact near-mode radius.
- `bad_pwt_outer_dist_fail` Sets `pwt_outer_dist=-1`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires rejection of this exact relevance distance.
- `bad_decay_fail` Sets `decay=60,30`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires rejection when decay start exceeds decay end.
- `bad_time_on_leg_fail` Sets inherited `time_on_leg=-1`; the harness requires armed, pre-deadline `MALCONFIG`, while direct CTest requires rejection of this exact negative duration.

## Direct CTest coverage

`BHVTrailTest` holds contact and ownship state fixed so configuration can be
graded at the layer where its effect is exact. It compares absolute west,
relative port, relative starboard, and repeated runtime-style angle changes by
their generated trail-point coordinates; compares every static and modifier
range fixture by its effective distance, including the 10-meter floor; and
requires the precise `Inside radius`, `Inside nm_radius`, and `Outside
nm_radius` boundary states. Separate tests prove the strict outer relevance
boundary, Trail's own `extrapolate=false` zero-speed branch, complete alert
request and suppression gates, exact missing-contact diagnostics, and rejection
of every malformed live fixture.

The 22 removed missions checked only broad eventual pursuit around a point the
implementation itself chose. The retained static and right-turn cases continue
to cover live pursuit, while direct tests now fail when the claimed angle,
range, radius, relevance, extrapolation, or alert-request setting is ignored.

## Running

From this harness directory:

```bash
./zlaunch.sh 10
./zlaunch.sh --case=static_trail_pass 10
./zlaunch.sh --case=pwt_outer_inactive_pass --gui 1
./zlaunch.sh --case=lead_right_turn_pass --gui 1
./zlaunch.sh --case=runtime_bad_update_recover_pass --gui 1
./zlaunch.sh --case=auto_alert_request_pass --gui 1
./zlaunch.sh --case=idle_post_distance_pass --gui 1
./zlaunch.sh --jobs=4 --port_base=15000 10
```

The paired mission launcher remains a single-scenario launcher. It accepts the
mission modifier and all six MOOSDB/pShare ports directly:

```bash
./zlaunch.sh --mmod=static_trail_pass --shore_mport=15000 \
  --veh1_mport=15001 --veh2_mport=15002 --shore_pshare=15010 \
  --veh1_pshare=15011 --veh2_pshare=15012 10
```

The full matrix currently has 20 cases. Serial and rolling modes both use one
isolated mission copy and one deterministic two-vehicle port block per case.
The harness requires Bash 5.1 or newer for rolling scheduling and prevents a
second invocation from starting while one is active. Do not overlap it with
other MOOS harnesses on the same machine.

## Latest validation

- July 20, 2026
- generated-file matrix: `20/20` cases completed
- minimal-logging matrix: `20/20` cases passed
- full-logging matrix: `20/20` cases passed
- focused `BHVTrailTest`: `12/12` direct cases passed
- focused `behaviors-marine` family: `226/226` CTests passed
- nine deliberate mutations failed at their intended evidence: wrong angle,
  ignored additive range, shifted radius boundary, shifted relevance boundary,
  wrong extrapolation branch, unintended alert request, wrong missing-contact
  policy, wrong recovery value, and repaired malformed range
- warp: `10`
