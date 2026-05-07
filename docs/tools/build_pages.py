#!/usr/bin/env python3
"""Generate the static GitHub Pages site for the CI/CD harness catalog."""

from __future__ import annotations

import re
import json
import shlex
import sys
from dataclasses import dataclass
from html import escape
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPO_ROOT = Path(__file__).resolve().parents[2]
HARNESS_DIR = REPO_ROOT / "harnesses"
CPP_TEST_DIR = REPO_ROOT / "tests" / "cpp"
SCRIPTS_DIR = REPO_ROOT / "scripts"
CTEST_COVERAGE_DATA = ROOT / "data" / "ctest_coverage.json"
REPO_BLOB = "https://github.com/cbenjamin23/moos-ivp-cicd-testing/blob/main"
REPO_ACTIONS = "https://github.com/cbenjamin23/moos-ivp-cicd-testing/actions/workflows/build_extend.yml"
ASSET_VERSION = "20260501-1"
CASE_ARRAY_RE = re.compile(r'^\s*([A-Z0-9_]*CASES)\s*=\s*\(\s*(.*?)^\s*\)\s*$', re.MULTILINE | re.DOTALL)
ARRAY_REF_RE = re.compile(r"^\$\{([A-Z][A-Z0-9_]*)\[@\]\}$")
CASE_NAME_RE = re.compile(r"\b[A-Za-z0-9_]+_(?:pass|fail|absent)\b")
VISUAL_NOTES = {
    "cmgr-unit": "This unit harness uses map-style explanatory GIFs instead of pMarineViewer captures because the ownship geometry is intentionally simple and the verdict comes from MOOS publications such as contact alerts, reports, and filter messages.",
    "obmgr-unit": "This unit harness uses map-style explanatory GIFs instead of pMarineViewer captures because the ownship is stationary and the verdict comes from MOOS publications such as obstacle acceptance, distance reports, hull generation, and filter messages.",
    "pid-unit": "This unit harness uses variable-level PID evidence instead of pMarineViewer captures because it intentionally checks pMarinePIDV22 input/output behavior before adding vehicle dynamics.",
    "pid-motion": "This motion harness uses PID-focused generated visuals instead of raw pMarineViewer captures because the verdict depends on bridged navigation, actuator output, and closed-loop vehicle response.",
    "colregs-classification": "This classification harness uses alog-backed explanatory GIFs instead of raw pMarineViewer captures because the vehicles do move, but the verdict comes from COLREGS publications such as mode, index, summary, and range reports.",
    "colregs-thresholds": "This threshold harness uses alog-backed overlay GIFs instead of raw pMarineViewer captures because the cases differ by small geometry changes and the verdict comes from boundary-specific COLREGS publications.",
}


@dataclass(frozen=True)
class Harness:
    slug: str
    title: str
    family: str
    path: str
    mission: str
    summary: str
    proof: str
    gifs: tuple[tuple[str, str, str], tuple[str, str, str]]
    run: str
    notes: tuple[str, ...]


@dataclass(frozen=True)
class Family:
    name: str
    label: str
    summary: str
    slugs: tuple[str, ...]


@dataclass(frozen=True)
class CTestArea:
    key: str
    label: str
    target_count: int
    source_count: int
    case_count: int
    description: str
    coverage_note: str
    labels: tuple[str, ...]
    path: str


HARNESSES: tuple[Harness, ...] = (
    Harness(
        slug="cmgr-unit",
        title="H01 Contact Manager Unit",
        family="Contact Manager",
        path="harnesses/cmgr_harnesses/H01-cmgr_unit",
        mission="missions/cmgr_missions/cmgr_unit",
        summary="Contact-manager mode checks for detection, absence, alerts, reports, filters, and multi-contact outputs.",
        proof="Directly checks detection, absence, report, alert filter, runtime request, closest-contact, and multi-contact publication paths.",
        gifs=(
            ("Runtime Alert Path", "runtime_alert_add_pass", "cmgr-unit-runtime-alert.gif"),
            ("CPA-only Detection", "detect_cpa_only_pass", "cmgr-unit-cpa-detection.gif"),
        ),
        run="./zlaunch.sh --case=detect_baseline_pass 10",
        notes=(
            "This harness reads like a mission-level unit test suite: one stem mission, many focused contact-manager assertions.",
            "The important evidence is mostly variable-level: detection, absence, runtime requests, filters, and report fields.",
        ),
    ),
    Harness(
        slug="cmgr-motion",
        title="H02 Contact Manager Motion",
        family="Contact Manager",
        path="harnesses/cmgr_harnesses/H02-cmgr_motion",
        mission="missions/cmgr_missions/cmgr_motion",
        summary="Moving integration companion proving contact-manager alerts can drive safe collision-avoidance transit.",
        proof="Grades arrival, detection, contact range, encounters, near misses, and collisions over short moving encounters.",
        gifs=(
            ("Baseline Crossing Pass", "baseline_crossing_pass", "cmgr-motion-baseline-crossing.gif"),
            ("Head-on Pass", "head_on_pass", "cmgr-motion-head-on-pass.gif"),
            ("Two Contact Pass", "two_contact_pass", "cmgr-motion-two-contact-pmarineviewer.gif"),
        ),
        run="./zlaunch.sh --case=baseline_crossing_pass --gui 10",
        notes=(
            "This harness connects contact-manager output to a moving collision-avoidance outcome.",
            "Expected-fail cases are included to prove the gate catches missing or late avoidance evidence.",
        ),
    ),
    Harness(
        slug="obmgr-unit",
        title="H01 Obstacle Manager Unit",
        family="Obstacle Manager",
        path="harnesses/obmgr_harnesses/H01-obmgr_unit",
        mission="missions/obmgr_missions/obmgr_unit",
        summary="Obstacle-manager mode checks for alerts, given obstacles, point clusters, hulls, and lifecycle branches.",
        proof="Checks accepted/rejected obstacles, general alerts, distance reports, point-cluster hull generation, lasso, placeholder, disable, enable, and expunge paths.",
        gifs=(
            ("Given Obstacle Alert", "given_baseline_pass", "obmgr-unit-given-baseline.gif"),
            ("Point Cluster Hull", "points_cluster_dist_pass", "obmgr-unit-point-cluster.gif"),
        ),
        run="./zlaunch.sh --case=given_baseline_pass 10",
        notes=(
            "This harness covers obstacle-manager message paths before testing full moving obstacle avoidance.",
            "Point-cluster cases are useful for understanding how raw obstacle observations become usable obstacle geometry.",
        ),
    ),
    Harness(
        slug="obmgr-motion",
        title="H02 Obstacle Manager Motion",
        family="Obstacle Manager",
        path="harnesses/obmgr_harnesses/H02-obmgr_motion",
        mission="missions/obmgr_missions/obmgr_motion",
        summary="Moving integration suite for pObstacleMgr alert output feeding BHV_AvoidObstacleV24.",
        proof="Grades clean arrival, obstacle encounters, near misses, collisions, and expected fail modes in a short corridor.",
        gifs=(
            ("Baseline Center Pass", "baseline_center_pass", "obmgr-motion-baseline-center.gif"),
            ("Offset Clear Pass", "offset_clear_pass", "obmgr-motion-offset-clear.gif"),
            ("Two Sequential Fail", "two_sequential_fail", "obmgr-motion-two-sequential.gif"),
        ),
        run="./zlaunch.sh --case=baseline_center_pass --gui 10",
        notes=(
            "This harness checks whether obstacle-manager alerts lead to clean short-corridor transit.",
            "Expected-fail cases capture conditions where alerts are missing, too difficult, or not actionable.",
        ),
    ),
    Harness(
        slug="pid-unit",
        title="H01 Marine PID Unit",
        family="Marine PID",
        path="harnesses/pid_harnesses/H01-pid_unit",
        mission="missions/pid_missions/pid_unit",
        summary="Headless pMarinePIDV22 unit matrix covering heading, speed, depth, override, stale-input, yaw, and debug-output branches.",
        proof="Checks suffixed rudder, thrust, elevator, control-state, stale-input, yaw-source, and debug publications from scripted MOOS mail.",
        gifs=(
            ("Heading Wrap", "heading_wrap_pass", "pid-unit-heading-wrap.gif"),
            ("Manual Override", "manual_override_zero_pass", "pid-unit-manual-override.gif"),
        ),
        run="./zlaunch.sh --case=rudder_starboard_pass 10",
        notes=(
            "This harness keeps the PID app isolated from simulation and helm behavior so actuator output can be checked directly.",
            "The speed cases intentionally distinguish `speed_factor` mapping, runtime speed-factor updates, PID speed control, and thrust clipping.",
        ),
    ),
    Harness(
        slug="pid-motion",
        title="H02 Marine PID Motion",
        family="Marine PID",
        path="harnesses/pid_harnesses/H02-pid_motion",
        mission="missions/pid_missions/pid_motion",
        summary="Closed-loop pMarinePIDV22 motion matrix covering transit, turn recovery, speed-PID mode, depth response, and authority failures.",
        proof="Checks bridged arrival, corridor position, speed, heading, depth, and actuator evidence from pMarinePIDV22 driving uSimMarineV22.",
        gifs=(
            ("Hard Turn Recovery", "hard_turn_recover_pass", "pid-motion-hard-turn-recover.gif"),
            ("Depth Step Response", "depth_step_pass", "pid-motion-depth-step.gif"),
        ),
        run="./zlaunch.sh --case=baseline_transit_pass --gui 10",
        notes=(
            "This harness adds pHelmIvP and uSimMarineV22 so PID output is judged as vehicle motion instead of only as actuator publications.",
            "Expected-fail cases intentionally hold manual override or cap thrust too low to prove the motion gate catches lost authority.",
        ),
    ),
    Harness(
        slug="constant-depth-motion",
        title="H01 Constant Depth Motion",
        family="Depth Behavior",
        path="harnesses/depth_behavior_harnesses/H01-constant_depth_motion",
        mission="missions/depth_behavior_missions/depth_behavior_motion",
        summary="BHV_ConstantDepth motion matrix covering held depth, surfacing, negative-depth clipping, shape parameters, runtime updates, malformed update preservation, finite-duration completion, malformed config, missing depth mail/domain, and actuator authority failures.",
        proof="Checks bridged NAV_DEPTH, DEPTH_MISMATCH, behavior error evidence, and elevator/depth-control signals from pHelmIvP driving pMarinePIDV22 and uSimMarineV22.",
        gifs=(),
        run="./zlaunch.sh --case=constant_depth_hold_pass --gui 10",
        notes=(
            "This harness keeps the horizontal waypoint leg as background motion so BHV_ConstantDepth owns the vertical-control verdict.",
            "Expected-fail cases prove the evaluator catches invalid configuration, missing depth inputs, missing depth domain, and disabled vertical-control authority.",
        ),
    ),
    Harness(
        slug="goto-depth-motion",
        title="H02 GoToDepth Motion",
        family="Depth Behavior",
        path="harnesses/depth_behavior_harnesses/H02-goto_depth_motion",
        mission="missions/depth_behavior_missions/depth_behavior_motion",
        summary="BHV_GoToDepth motion matrix covering multi-level sequences, repeat behavior and exhaustion, vertical target crossings, zero-delta arrivals, single-level targets, malformed update preservation, unsupported perpetual config, invalid sequences, negative depths, malformed repeat values, missing depth mail, and missing depth domain.",
        proof="Checks bridged NAV_DEPTH, behavior-owned arrival counters, depth-band evidence, and configuration/error evidence from the simulated UUV stack.",
        gifs=(),
        run="./zlaunch.sh --case=goto_depth_sequence_pass --gui 10",
        notes=(
            "Finite sequence cases grade arrival and depth evidence instead of treating post-completion behavior quieting as the primary signal.",
            "Expected-fail cases isolate invalid sequence inputs and missing vertical-state conditions.",
        ),
    ),
    Harness(
        slug="periodic-surface-motion",
        title="H03 Periodic Surface Motion",
        family="Depth Behavior",
        path="harnesses/depth_behavior_harnesses/H03-periodic_surface_motion",
        mission="missions/depth_behavior_missions/depth_behavior_motion",
        summary="BHV_PeriodicSurface motion matrix covering surfacing cycles, status variables, wait windows, timeout reset behavior, mark-variable resets, acomms extensions, ascent-grade branches, current-speed mode, invalid period/ascent/status/acomms settings, missing nav mail, and missing depth domain.",
        proof="Checks surfaced-depth evidence, timeout/reset evidence, behavior status variables, vertical actuator evidence, and mission-owned error outcomes.",
        gifs=(),
        run="./zlaunch.sh --case=periodic_surface_pass --gui 10",
        notes=(
            "Vehicle-side overlays own the behavior timing inputs, including acomms extension, so the evaluated app receives the same mail as it would in a live vehicle community.",
            "Expected-fail cases distinguish malformed behavior parameters from missing navigation or domain support.",
        ),
    ),
    Harness(
        slug="max-depth-motion",
        title="H04 Max Depth Motion",
        family="Depth Behavior",
        path="harnesses/depth_behavior_harnesses/H04-max_depth_motion",
        mission="missions/depth_behavior_missions/depth_behavior_motion",
        summary="BHV_MaxDepth motion matrix covering max-depth guarding, zero-depth clamps, negative command clipping, tight and zero tolerance, unconstrained shallow commands, malformed max-depth/tolerance settings, unsupported ascent-field config, missing depth mail, and missing depth domain.",
        proof="Checks bridged NAV_DEPTH limits, slack/mismatch telemetry, behavior error evidence, and elevator response under commanded over-depth pressure.",
        gifs=(),
        run="./zlaunch.sh --case=max_depth_guard_pass --gui 10",
        notes=(
            "This harness grades the guard behavior while another behavior creates pressure to go deeper than the configured limit.",
            "Expected-fail cases prove both bad configuration and missing vertical state are caught by the mission evaluator.",
        ),
    ),
    Harness(
        slug="min-altitude-motion",
        title="H05 Min Altitude Motion",
        family="Depth Behavior",
        path="harnesses/depth_behavior_harnesses/H05-min_altitude_motion",
        mission="missions/depth_behavior_missions/depth_behavior_motion",
        summary="BHV_MinAltitude motion matrix covering bottom-clearance guarding, shallow-bottom response, unconstrained deep-bottom behavior, clearance boundary behavior, zero-minimum behavior, zero altitude failures, invalid configuration, missing nav inputs, and noncritical missing-altitude handling.",
        proof="Checks bridged NAV_DEPTH and NAV_ALTITUDE, min-altitude clearance bands, behavior error evidence, and elevator response under simulated bottom-clearance pressure.",
        gifs=(),
        run="./zlaunch.sh --case=min_altitude_guard_pass --gui 10",
        notes=(
            "This harness isolates bottom-clearance behavior from the other depth behaviors while reusing the same simulated UUV stack.",
            "The noncritical missing-altitude case documents the configured warning-only mode separately from critical guard failures.",
        ),
    ),
    Harness(
        slug="colregs-classification",
        title="H01 COLREGS Classification",
        family="COLREGS",
        path="harnesses/colregs_harnesses/H01-colregs_classification",
        mission="missions/colregs_missions/colregs_unit",
        summary="Canonical two-vessel COLREGS classification suite for BHV_AvdColregsV22.",
        proof="Checks expected mode/index selection across head-on, crossing, overtaking, and overtaken encounter geometry.",
        gifs=(
            ("Head-on Classification", "head_on_colregs_pass", "colregs-classification-head-on.gif"),
            ("Crossing Give-way", "crossing_starboard_giveway_pass", "colregs-classification-crossing.gif"),
        ),
        run="./zlaunch.sh --case=head_on_colregs_pass --gui 10",
        notes=(
            "This is the first COLREGS layer: select the correct rule family before judging maneuver quality.",
            "Cases cover canonical head-on, crossing, overtaking, and overtaken-vessel geometry.",
        ),
    ),
    Harness(
        slug="colregs-thresholds",
        title="H02 COLREGS Thresholds",
        family="COLREGS",
        path="harnesses/colregs_harnesses/H02-colregs_thresholds",
        mission="missions/colregs_missions/colregs_unit",
        summary="Geometry-edge suite for classification thresholds and near-boundary COLREGS behavior.",
        proof="Uses below, edge, above, mirrored, and family-grouped probes to catch subtle threshold regressions.",
        gifs=(
            ("Give-way Bow Distance", "giveway_bowdist_edge_pass", "colregs-thresholds-giveway-bowdist.gif"),
            ("Overtaking Threshold", "overtaking_thresh_edge_pass", "colregs-thresholds-overtaking-triplet.gif"),
        ),
        run="./zlaunch.sh --group=headon --gui 10",
        notes=(
            "This harness owns near-boundary geometry so H01 can stay focused on stable canonical cases.",
            "The below, edge, and above naming pattern makes threshold regressions easier to localize.",
        ),
    ),
    Harness(
        slug="colregs-execution",
        title="H03 COLREGS Execution",
        family="COLREGS",
        path="harnesses/colregs_harnesses/H03-colregs_execution",
        mission="missions/colregs_missions/colregs_unit",
        summary="Execution-quality layer for COLREGS cases after classification is known to be correct.",
        proof="Runs through completion and checks realized CPA bands, final side relationship, collisions, and loose wall-time envelopes.",
        gifs=(
            ("Head-on Resolution", "head_on_execution_pass", "colregs-execution-head-on.gif"),
            ("Overtaken Midrange", "overtaken_starboard_standon_midrange_execution_pass", "colregs-execution-overtaken-midrange.gif"),
            ("Crossing Give-way", "crossing_starboard_giveway_execution_pass", "colregs-execution-crossing-giveway.gif"),
        ),
        run="./zlaunch.sh --case=head_on_execution_pass --gui 10",
        notes=(
            "This layer assumes classification is correct and then checks whether the completed maneuver is acceptable.",
            "The key evidence is realized closest approach, relative side outcome, timing envelope, and zero collisions.",
        ),
    ),
    Harness(
        slug="colregs-parameters",
        title="H04 COLREGS Parameters",
        family="COLREGS",
        path="harnesses/colregs_harnesses/H04-colregs_parameters",
        mission="missions/colregs_missions/colregs_unit",
        summary="Parameter-regression suite varying one BHV_AvdColregsV22 knob at a time.",
        proof="Keeps trusted geometry fixed while proving selected tuning changes remain stable or change in one expected way.",
        gifs=(
            ("Bow Distance Threshold", "giveway_bow_dist_high_pass", "colregs-parameters-bow-distance.gif"),
            ("Turn Radius Branch", "turn_radius_high_pass", "colregs-parameters-turn-radius.gif"),
        ),
        run="./zlaunch.sh --group=min_util_cpa_dist --gui 10",
        notes=(
            "This harness changes behavior parameters while keeping the underlying geometry controlled.",
            "The goal is to make tuning changes safe by proving expected stability or expected change.",
        ),
    ),
    Harness(
        slug="collision-behavior-motion",
        title="H01 Avoid-Collision Behavior",
        family="Behavior Correctness",
        path="harnesses/collision_behavior_harnesses/H01-collision_behavior_motion",
        mission="missions/collision_behavior_missions/collision_behavior_motion",
        summary="Moving correctness harness for BHV_AvoidCollision independent of app-level contact-manager assertions.",
        proof="Checks alert request, contact info, per-contact outputs, behavior lifecycle, resolution, arrival, and collision-free transit.",
        gifs=(
            ("Head-on Resolve", "head_on_resolve_pass", "collision-behavior-head-on-resolve.gif"),
            ("No Alert Request Fail", "no_alert_request_fail", "collision-behavior-no-alert-fail.gif"),
            ("Baseline Resolve", "default_resolve_pass", "collision-behavior-default-resolve.gif"),
        ),
        run="./zlaunch.sh --case=head_on_resolve_pass --gui 10",
        notes=(
            "This harness isolates the avoid-collision behavior contract from contact-manager implementation details.",
            "A case can fail even without a collision when the required behavior lifecycle or resolution evidence is missing.",
        ),
    ),
    Harness(
        slug="obstacle-behavior-motion",
        title="H01 Avoid-Obstacle Behavior",
        family="Behavior Correctness",
        path="harnesses/obstacle_behavior_harnesses/H01-obstacle_behavior_motion",
        mission="missions/obstacle_behavior_missions/obstacle_behavior_motion",
        summary="Moving correctness harness for BHV_AvoidObstacleV24 with deterministic obstacle-manager input.",
        proof="Checks auto-request, obstacle alert receipt, behavior-owned range/CPA flags, resolution, arrival, encounters, and expected fail modes.",
        gifs=(
            ("Default Auto Request", "default_auto_request_pass", "obstacle-behavior-default-auto-request.gif"),
            ("Sequential Obstacle Alerts", "two_obstacles_clean_pass", "obstacle-behavior-two-obstacles.gif"),
            ("Avoid Disabled Fail", "avoid_disabled_fail", "obstacle-behavior-avoid-disabled.gif"),
        ),
        run="./zlaunch.sh --case=two_obstacles_clean_pass --gui 10",
        notes=(
            "This harness isolates the avoid-obstacle behavior contract while keeping obstacle-manager input deterministic.",
            "Helper-flag cases check behavior-specific observability in addition to mission outcome.",
        ),
    ),
    Harness(
        slug="opregion-safety",
        title="H01 OpRegion Safety",
        family="Behavior Correctness",
        path="harnesses/opregion_harnesses/H01-opregion_safety",
        mission="missions/opregion_missions/opregion_motion",
        summary="Moving safety harness for BHV_OpRegionV24 save regions, halt regions, timing gates, reset, and runtime region updates.",
        proof="Checks nominal containment, save recovery, generated buffers, halt breaches, entry/exit timing gates, reset handling, max-time failure, and dynamic region enforcement.",
        gifs=(
            ("Save Recovery", "save_recover_pass", "opregion-safety-save-recover.gif"),
            ("Dynamic Region Update", "dynamic_region_update_pass", "opregion-safety-dynamic-update.gif"),
            ("Entry Gate", "entry_gate_start_outside_pass", "opregion-safety-entry-gate.gif"),
        ),
        run="./zlaunch.sh --case=save_recover_pass --gui 10",
        notes=(
            "This harness isolates the operating-region safety contract around save/halt envelopes and timing gates.",
            "Expected-fail cases prove that true halt, max-time, or configured save-buffer breaches are caught by the mission verdict.",
        ),
    ),
    Harness(
        slug="waypoint-behavior-motion",
        title="H01 Waypoint Behavior",
        family="Behavior Correctness",
        path="harnesses/waypoint_behavior_harnesses/H01-waypoint_behavior_motion",
        mission="missions/waypoint_behavior_missions/waypoint_behavior_motion",
        summary="Moving correctness harness for BHV_Waypoint route traversal, runtime point updates, cycle flags, capture modes, and malformed inputs.",
        proof="Checks completion, waypoint-hit flags, end flags, repeat/cycle handling, dynamic route updates, capture/slip/line modes, status variables, and expected invalid-input failures.",
        gifs=(
            ("Multi-point Sequence", "multi_point_sequence_pass", "waypoint-behavior-multipoint.gif"),
            ("Dynamic Route Update", "xpoints_update_pass", "waypoint-behavior-dynamic-update.gif"),
            ("Repeat Forever Cycle", "repeat_forever_cycle_pass", "waypoint-behavior-repeat-forever.gif"),
        ),
        run="./zlaunch.sh --case=multi_point_sequence_pass --gui 10",
        notes=(
            "This harness keeps the vehicle on a short route while checking behavior-owned waypoint completion evidence.",
            "Runtime update cases prove that the behavior can accept, reject, or preserve traversal state across point updates.",
        ),
    ),
    Harness(
        slug="loiter-behavior-motion",
        title="H01 Loiter Behavior",
        family="Behavior Correctness",
        path="harnesses/loiter_behavior_harnesses/H01-loiter_behavior_motion",
        mission="missions/loiter_behavior_missions/loiter_behavior_motion",
        summary="Moving correctness harness for BHV_Loiter acquisition, polygon variants, center updates, speed modes, reports, and invalid inputs.",
        proof="Checks loiter mode, acquisition state, polygon distance/ETA, bearing accumulation, suffixed outputs, runtime polygon updates, and expected malformed-polygon failures.",
        gifs=(
            ("Radial Clockwise", "radial_clockwise_pass", "loiter-behavior-radial.gif"),
            ("Runtime Radius Expand", "mod_poly_rad_expand_pass", "loiter-behavior-radius-expand.gif"),
            ("Center Assign Pair", "center_assign_pair_pass", "loiter-behavior-center-assign-pair.gif"),
        ),
        run="./zlaunch.sh --case=radial_clockwise_pass --gui 10",
        notes=(
            "This harness checks both stable loiter acquisition and the output signals that make loiter state observable in CI.",
            "Dynamic center and polygon cases cover runtime geometry changes without copying the stem mission.",
        ),
    ),
    Harness(
        slug="stationkeep-behavior-motion",
        title="H01 StationKeep Behavior",
        family="Behavior Correctness",
        path="harnesses/stationkeep_behavior_harnesses/H01-stationkeep_behavior_motion",
        mission="missions/stationkeep_behavior_missions/stationkeep_behavior_motion",
        summary="Moving correctness harness for BHV_StationKeep station points, radii, hibernation, runtime retargeting, speed settings, warnings, and invalid inputs.",
        proof="Checks station distance, mode transitions, radius/speed updates, hibernation seek/settle behavior, warning recovery, visual hints, and expected parameter failures.",
        gifs=(
            ("Static Station", "static_station_pass", "stationkeep-behavior-static.gif"),
            ("Hibernation Settle", "hibernation_settle_pass", "stationkeep-behavior-hibernation.gif"),
            ("Runtime Radius Expand", "radius_update_expand_pass", "stationkeep-behavior-radius-expand.gif"),
        ),
        run="./zlaunch.sh --case=static_station_pass --gui 10",
        notes=(
            "This harness grades the station-keeping behavior through mode and distance outputs rather than only final vehicle position.",
            "Runtime retargeting and hibernation cases make the behavior's state transitions explicit for regression testing.",
        ),
    ),
    Harness(
        slug="trail-behavior-motion",
        title="H01 Trail Behavior",
        family="Behavior Correctness",
        path="harnesses/trail_behavior_harnesses/H01-trail_behavior_motion",
        mission="missions/trail_behavior_missions/trail_behavior_motion",
        summary="Two-vehicle moving correctness harness for BHV_Trail relative/absolute trailing geometry, runtime updates, pursuit relevance, and missing-contact paths.",
        proof="Checks trail distance, relevance, range, pursuit state, update handling, extrapolation, missing-contact warnings/failures, and malformed parameter rejection.",
        gifs=(
            ("Relative Port Trail", "relative_port_pass", "trail-behavior-relative-port.gif"),
            ("Lead Turn Angle Update", "lead_turn_angle_update_pass", "trail-behavior-lead-turn-angle-update.gif"),
            ("Runtime Range Extend", "runtime_range_extend_pass", "trail-behavior-runtime-range-extend.gif"),
        ),
        run="./zlaunch.sh --case=static_trail_pass --gui 10",
        notes=(
            "This harness uses a target vehicle and a chaser so trailing geometry is tested against moving contact state.",
            "Relevance and runtime-update cases prove the behavior can turn pursuit on/off and reframe the desired trail point.",
        ),
    ),
    Harness(
        slug="convoy-behavior-motion",
        title="H01 Convoy Behavior",
        family="Behavior Correctness",
        path="harnesses/convoy_behavior_harnesses/H01-convoy_behavior_motion",
        mission="missions/convoy_behavior_missions/convoy_behavior_motion",
        summary="Two-vehicle moving correctness harness for BHV_Convoy mark queues, following geometry, speed branches, runtime updates, warnings, and missing-contact failures.",
        proof="Checks queue length, track length, max range, average spacing, chaser/target speed aliases, visual breadcrumbs, geometry recovery, runtime updates, and expected parameter failures.",
        gifs=(
            ("Static Convoy", "static_convoy_pass", "convoy-behavior-static.gif"),
            ("Angled Entry", "angled_entry_pass", "convoy-behavior-angled-entry.gif"),
            ("Lead S-turn", "lead_s_turn_pass", "convoy-behavior-lead-s-turn.gif"),
        ),
        run="./zlaunch.sh --case=static_convoy_pass --gui 10",
        notes=(
            "This harness uses a leader/follower pair so queue and speed-branch outputs can be graded during real motion.",
            "Geometry-entry cases check recovery into convoy following from less tidy starting conditions.",
        ),
    ),
    Harness(
        slug="cutrange-behavior-motion",
        title="H01 CutRange Behavior",
        family="Behavior Correctness",
        path="harnesses/cutrange_behavior_harnesses/H01-cutrange_behavior_motion",
        mission="missions/cutrange_behavior_missions/cutrange_behavior_motion",
        summary="Two-vehicle moving correctness harness for BHV_CutRange pursuit, give-up behavior, relevance, runtime updates, and malformed inputs.",
        proof="Checks range closure, pursue/give-up flags, target geometry branches, relevance gates, warning recovery, runtime updates, and expected configuration failures.",
        gifs=(
            ("Head-on Closure", "headon_cutrange_pass", "cutrange-behavior-headon.gif"),
            ("Crossing Closure", "crossing_cutrange_pass", "cutrange-behavior-crossing.gif"),
            ("S-turn Target", "s_turn_target_pass", "cutrange-behavior-s-turn.gif"),
        ),
        run="./zlaunch.sh --case=static_cutrange_pass --gui 10",
        notes=(
            "This harness uses a chaser/target pair so range reduction is graded against live contact geometry instead of a static input.",
            "Runtime update and give-up cases make the behavior's pursuit state explicit enough for CI to catch subtle regressions.",
        ),
    ),
    Harness(
        slug="fixedturn-behavior-motion",
        title="H01 FixedTurn Behavior",
        family="Behavior Correctness",
        path="harnesses/fixedturn_behavior_harnesses/H01-fixedturn_behavior_motion",
        mission="missions/fixedturn_behavior_missions/fixedturn_behavior_motion",
        summary="Moving correctness harness for BHV_FixedTurn direction, scheduled turns, runtime updates, timeout completion, and invalid turn configuration.",
        proof="Checks fixed-turn completion, final heading bands, turn time/distance, scheduled-turn flags, runtime update recovery, and behavior error state.",
        gifs=(
            ("Starboard 90", "starboard_90_pass", "fixedturn-behavior-starboard-90.gif"),
            ("Port 360", "port_360_pass", "fixedturn-behavior-port-360.gif"),
            ("Turn Spec Sequence", "turn_spec_sequence_pass", "fixedturn-behavior-turn-spec-sequence.gif"),
        ),
        run="./zlaunch.sh --case=starboard_90_pass --gui 10",
        notes=(
            "This harness starts with a short approach leg and then activates the fixed-turn behavior so turn completion can be measured from mission-owned outputs.",
            "The scheduled-turn and runtime-update cases exercise configuration paths that are hard to prove from one static mission.",
        ),
    ),
    Harness(
        slug="shadow-behavior-motion",
        title="H01 Shadow Behavior",
        family="Behavior Correctness",
        path="harnesses/shadow_behavior_harnesses/H01-shadow_behavior_motion",
        mission="missions/shadow_behavior_missions/shadow_behavior_motion",
        summary="Two-vehicle moving correctness harness for BHV_Shadow contact telemetry, relevance gates, speed/heading following, runtime updates, and malformed inputs.",
        proof="Checks shadow contact heading/speed output, active relevance, stopped relevance, runtime recovery, warning-only paths, contact flags, and expected parameter failures.",
        gifs=(
            ("Static Shadow", "static_shadow_pass", "shadow-behavior-static.gif"),
            ("Turn North Shadow", "turn_north_shadow_pass", "shadow-behavior-turn-north.gif"),
            ("Runtime PWT On", "runtime_pwt_outer_on_pass", "shadow-behavior-runtime-pwt-on.gif"),
        ),
        run="./zlaunch.sh --case=static_shadow_pass --gui 10",
        notes=(
            "This harness uses one target and one chaser so shadowing can be graded against changing contact heading and speed.",
            "Relevance and missing-contact cases distinguish clean inactive behavior from warning and error paths.",
        ),
    ),
    Harness(
        slug="performance-obstacle-gauntlet",
        title="P01 Obstacle Gauntlet",
        family="Performance",
        path="harnesses/performance_harnesses/P01-obstacle_gauntlet",
        mission="missions/performance_missions/P01-obstacle_gauntlet",
        summary="Deterministic and randomized obstacle-field performance harness.",
        proof="Checks loop completion, zero collisions, bounded near misses/encounters, wall time, timeout, and warning-free logs.",
        gifs=(
            ("Baseline Field", "baseline_field_pass", "performance-obstacle-gauntlet-baseline.gif"),
            ("Dense Field", "dense_field_pass", "performance-obstacle-gauntlet-dense.gif"),
            ("Endurance Random", "endurance_random_pass", "performance-obstacle-gauntlet-endurance.gif"),
        ),
        run="./zlaunch.sh --case=baseline_field_pass --gui 10",
        notes=(
            "This harness moves beyond single encounters into repeated obstacle-field traversal.",
            "Shell-side checks add wall-time and warning-log constraints around mission-owned verdicts.",
        ),
    ),
    Harness(
        slug="performance-colregs-joust",
        title="P02 COLREGS Joust",
        family="Performance",
        path="harnesses/performance_harnesses/P02-colregs_joust",
        mission="missions/performance_missions/P02-colregs_joust",
        summary="Two- and three-vehicle COLREGS pressure harness focused on sustained safe spacing.",
        proof="Checks mission completion, zero collisions, closest-range envelopes, near misses, wall time, and warning-free logs.",
        gifs=(
            ("Baseline Joust", "baseline_colregs_pass", "performance-colregs-joust-baseline.gif"),
            ("Dense Joust", "dense_colregs_pass", "performance-colregs-joust-dense.gif"),
        ),
        run="./zlaunch.sh --case=dense_colregs_pass --gui 10",
        notes=(
            "This harness tests sustained COLREGS pressure rather than a single isolated encounter.",
            "Closest-range telemetry is the primary spacing signal for this mission family.",
        ),
    ),
    Harness(
        slug="performance-traffic-ring",
        title="P03 COLREGS Traffic Ring",
        family="Performance",
        path="harnesses/performance_harnesses/P03-colregs_traffic_ring",
        mission="missions/performance_missions/P03-colregs_traffic_ring",
        summary="Seeded five-vehicle traffic-ring harness with repeated reassignment pressure.",
        proof="Checks zero collisions, assignment activity floors, fixed runtime windows, warning-free logs, and reproducible seeded random behavior.",
        gifs=(
            ("Baseline Traffic Ring", "baseline_circle_pass", "performance-traffic-ring-baseline.gif"),
            ("Mixed-Speed Traffic Ring", "mixed_speed_circle_pass", "performance-traffic-ring-mixed-speed.gif"),
            ("Non-Cooperative Traffic Ring", "noncoop_circle_pass", "performance-traffic-ring-noncoop.gif"),
        ),
        run="./zlaunch.sh --case=baseline_circle_pass --gui 10",
        notes=(
            "This harness uses seeded randomization so traffic pressure is repeatable enough for CI.",
            "Assignment activity and zero-collision checks are the primary health signals.",
        ),
    ),
)


FAMILIES: tuple[Family, ...] = (
    Family(
        name="pContactMgrV20",
        label="Contact Management",
        summary="Detects and reports relevant contacts so downstream avoidance behaviors can react to them.",
        slugs=("cmgr-unit", "cmgr-motion"),
    ),
    Family(
        name="pObstacleMgr",
        label="Obstacle Management",
        summary="Accepts obstacle inputs, builds obstacle representations, and publishes alerts used by obstacle avoidance.",
        slugs=("obmgr-unit", "obmgr-motion"),
    ),
    Family(
        name="pMarinePIDV22",
        label="Marine PID",
        summary="Converts desired heading, speed, and depth mail into direct actuator commands under override, stale-input, and yaw-source constraints.",
        slugs=("pid-unit", "pid-motion"),
    ),
    Family(
        name="BHV_ConstantDepth",
        label="Depth Behavior",
        summary="Checks held depth, surfacing, runtime updates, bad config, missing depth mail, and actuator authority. H01 in shared depth behavior group.",
        slugs=("constant-depth-motion",),
    ),
    Family(
        name="BHV_GoToDepth",
        label="Depth Behavior",
        summary="Checks depth sequences, repeats, arrivals, crossings, invalid inputs, and missing depth state. H02, not H01, in shared depth behavior group.",
        slugs=("goto-depth-motion",),
    ),
    Family(
        name="BHV_PeriodicSurface",
        label="Depth Behavior",
        summary="Checks surfacing cycles, waits, status variables, acomms extensions, ascent modes, and bad timing inputs. H03, not H01, in shared depth behavior group.",
        slugs=("periodic-surface-motion",),
    ),
    Family(
        name="BHV_MaxDepth",
        label="Depth Behavior",
        summary="Checks max-depth guarding, clamps, tolerances, slack telemetry, bad config, and missing depth state. H04, not H01, in shared depth behavior group.",
        slugs=("max-depth-motion",),
    ),
    Family(
        name="BHV_MinAltitude",
        label="Depth Behavior",
        summary="Checks bottom-clearance guarding, shallow-bottom response, boundary behavior, bad config, and missing altitude inputs. H05, not H01, in shared depth behavior group.",
        slugs=("min-altitude-motion",),
    ),
    Family(
        name="BHV_AvdColregsV22",
        label="COLREGS Behavior",
        summary="Exercises COLREGS classification, threshold edges, execution quality, parameter sensitivity, and multi-vehicle stress cases.",
        slugs=(
            "colregs-classification",
            "colregs-thresholds",
            "colregs-execution",
            "colregs-parameters",
            "performance-colregs-joust",
            "performance-traffic-ring",
        ),
    ),
    Family(
        name="BHV_AvoidCollision",
        label="Collision Avoidance Behavior",
        summary="Owns contact-alert requests, behavior lifecycle, and clean resolution of collision-avoidance encounters.",
        slugs=("collision-behavior-motion",),
    ),
    Family(
        name="BHV_AvoidObstacleV24",
        label="Obstacle Avoidance Behavior",
        summary="Checks obstacle-avoidance lifecycle, helper flags, clean transit outcomes, and longer obstacle-field stress cases.",
        slugs=(
            "obstacle-behavior-motion",
            "performance-obstacle-gauntlet",
        ),
    ),
    Family(
        name="BHV_OpRegionV24",
        label="Operating Region Behavior",
        summary="Checks save and halt envelopes, entry/exit timing gates, reset behavior, and runtime region updates.",
        slugs=("opregion-safety",),
    ),
    Family(
        name="BHV_Waypoint",
        label="Waypoint Behavior",
        summary="Checks route traversal, capture modes, cycle flags, runtime point updates, status outputs, and invalid route inputs.",
        slugs=("waypoint-behavior-motion",),
    ),
    Family(
        name="BHV_Loiter",
        label="Loiter Behavior",
        summary="Checks loiter acquisition, polygon forms, center updates, speed modes, status reports, and invalid polygon inputs.",
        slugs=("loiter-behavior-motion",),
    ),
    Family(
        name="BHV_StationKeep",
        label="Station-Keeping Behavior",
        summary="Checks station-point holding, hibernation, radii, speed updates, warning recovery, and invalid station-keeping inputs.",
        slugs=("stationkeep-behavior-motion",),
    ),
    Family(
        name="BHV_Trail",
        label="Trail Behavior",
        summary="Checks moving target trailing geometry, pursuit relevance, runtime trail updates, and missing-contact handling.",
        slugs=("trail-behavior-motion",),
    ),
    Family(
        name="BHV_Convoy",
        label="Convoy Behavior",
        summary="Checks mark queues, leader/follower geometry, speed branches, runtime updates, visual breadcrumbs, and missing-contact handling.",
        slugs=("convoy-behavior-motion",),
    ),
    Family(
        name="BHV_CutRange",
        label="Cut-Range Behavior",
        summary="Checks pursuit relevance, range reduction, give-up behavior, target geometry branches, runtime updates, and malformed inputs.",
        slugs=("cutrange-behavior-motion",),
    ),
    Family(
        name="BHV_FixedTurn",
        label="Fixed-Turn Behavior",
        summary="Checks fixed-turn direction, scheduled turn specifications, runtime updates, timeout completion, and invalid turn configuration.",
        slugs=("fixedturn-behavior-motion",),
    ),
    Family(
        name="BHV_Shadow",
        label="Shadow Behavior",
        summary="Checks contact speed/heading following, relevance gates, warning recovery, runtime updates, and malformed contact/configuration inputs.",
        slugs=("shadow-behavior-motion",),
    ),
)


TEST_STYLE: dict[str, str] = {
    "cmgr-unit": "Simple correctness tests for pContactMgrV20 output behavior. These cases keep the mission controlled and verify that contact alerts, reports, filters, runtime requests, and multi-contact selections appear only when expected.",
    "cmgr-motion": "Moving correctness tests for contact-management evidence. These cases check whether contact-manager alerts arrive early enough for downstream avoidance to produce a safe transit, while expected-fail cases prove the gate catches missing or late avoidance evidence.",
    "obmgr-unit": "Simple correctness tests for pObstacleMgr output behavior. These cases keep vehicle dynamics minimal and verify accepted obstacles, rejected obstacles, alert messages, point-cluster hulls, distance reports, and obstacle lifecycle messages.",
    "obmgr-motion": "Moving correctness tests for obstacle-manager alert output. These cases check whether obstacle alerts are actionable enough to support clean obstacle-avoidance transit, and whether intentionally bad setups fail for the expected reason.",
    "pid-unit": "Simple correctness tests for pMarinePIDV22 output behavior. These cases keep the mission to one MOOS community and verify rudder, thrust, elevator, override, stale-input, yaw-source, and debug publication paths.",
    "pid-motion": "Moving correctness tests for pMarinePIDV22 closed-loop behavior. These cases add pHelmIvP and uSimMarineV22 and verify that PID output drives arrival, turn recovery, speed-PID transit, depth response, and expected authority failures.",
    "constant-depth-motion": "Moving correctness tests for BHV_ConstantDepth. These cases verify held depths, surfacing, negative-depth clipping, shape parameters, runtime updates, malformed update preservation, finite-duration completion, malformed config, missing inputs/domain, and disabled depth-control authority.",
    "goto-depth-motion": "Moving correctness tests for BHV_GoToDepth. These cases verify multi-level sequences, repeated cycles and exhaustion, vertical target crossings, zero-delta arrivals, single-level targets, malformed update preservation, unsupported perpetual config, invalid sequences, malformed repeat values, missing inputs, and missing domain support.",
    "periodic-surface-motion": "Moving correctness tests for BHV_PeriodicSurface. These cases verify periodic surfacing, status variables, wait windows, timeout reset behavior, mark-variable resets, acomms extension, ascent profiles, current-speed mode, malformed status/ascent parameters, and missing inputs/domain.",
    "max-depth-motion": "Moving correctness tests for BHV_MaxDepth. These cases verify max-depth guarding, zero-depth clamps, negative command clipping, tight and zero tolerances, unconstrained shallow commands, invalid configuration, unsupported ascent-field config, missing depth inputs, and missing depth domain support.",
    "min-altitude-motion": "Moving correctness tests for BHV_MinAltitude. These cases verify bottom-clearance guarding, shallow-bottom response, unconstrained deep-bottom behavior, clearance boundary behavior, zero-minimum behavior, zero altitude failures, invalid configuration, missing nav inputs, and noncritical warning mode.",
    "colregs-classification": "Classification correctness tests for BHV_AvdColregsV22. These cases verify that canonical two-vessel geometries enter the expected COLREGS mode before execution quality is evaluated elsewhere.",
    "colregs-thresholds": "Boundary correctness tests for COLREGS classification. These cases probe below, edge, above, and mirrored geometries to catch regressions near rule-transition thresholds.",
    "colregs-execution": "Execution correctness tests for BHV_AvdColregsV22 after classification is known. These cases run encounters through completion and grade the realized maneuver using CPA, side outcome, timing, and collision evidence.",
    "colregs-parameters": "Parameter-regression tests for BHV_AvdColregsV22. These cases vary one tuning knob at a time while holding trusted geometry steady, verifying expected stability or one deliberate behavioral change.",
    "collision-behavior-motion": "Behavior correctness tests for BHV_AvoidCollision. These cases isolate the behavior contract: alert requests, contact information, lifecycle evidence, per-contact outputs, resolution signals, and collision-free transit.",
    "obstacle-behavior-motion": "Behavior correctness tests for BHV_AvoidObstacleV24. These cases isolate the behavior-owned contract for obstacle alert requests, helper flags, lifecycle evidence, resolution signals, and clean transit outcomes.",
    "opregion-safety": "Behavior correctness tests for BHV_OpRegionV24. These cases isolate the operating-region safety contract: save-region warnings, halt-region failures, generated buffers, entry/exit timing gates, reset handling, and runtime polygon updates.",
    "waypoint-behavior-motion": "Behavior correctness tests for BHV_Waypoint. These cases verify route completion, waypoint-hit and end flags, repeat/cycle behavior, runtime point updates, capture modes, status variables, and invalid route handling.",
    "loiter-behavior-motion": "Behavior correctness tests for BHV_Loiter. These cases verify stable loiter acquisition, polygon input forms, clockwise selection, center updates, speed modes, reports, and malformed polygon/update handling.",
    "stationkeep-behavior-motion": "Behavior correctness tests for BHV_StationKeep. These cases verify station arrival, hold/swing modes, hibernation, radius and speed updates, retargeting, warning recovery, and invalid parameter handling.",
    "trail-behavior-motion": "Behavior correctness tests for BHV_Trail. These cases verify relative and absolute trailing geometry, pursuit relevance, runtime trail updates, extrapolation choices, missing-contact behavior, and malformed parameter handling.",
    "convoy-behavior-motion": "Behavior correctness tests for BHV_Convoy. These cases verify convoy mark queues, following geometry, speed-control branches, runtime updates, warning paths, geometry recovery, and invalid parameter handling.",
    "cutrange-behavior-motion": "Behavior correctness tests for BHV_CutRange. These cases verify range closure, target geometry variants, pursuit and give-up flags, relevance gates, runtime updates, warning recovery, and invalid parameter handling.",
    "fixedturn-behavior-motion": "Behavior correctness tests for BHV_FixedTurn. These cases verify fixed-turn direction, scheduled turn specifications, inherited and fixed speed branches, timeout completion, runtime updates, and invalid turn configuration.",
    "shadow-behavior-motion": "Behavior correctness tests for BHV_Shadow. These cases verify contact heading and speed following, relevance gating, runtime recovery, warning paths, contact flags, and invalid parameter handling.",
    "performance-obstacle-gauntlet": "Performance and endurance tests for obstacle avoidance. These cases exercise repeated obstacle-field traversal and grade completion, collision safety, warning cleanliness, and wall-time behavior.",
    "performance-colregs-joust": "Performance tests for sustained COLREGS pressure. These cases run repeated multi-vehicle interaction and grade safe spacing, completion, warning cleanliness, and runtime bounds.",
    "performance-traffic-ring": "Seeded traffic-stress tests for COLREGS behavior. These cases replay five-vehicle traffic pressure and grade assignment activity, fixed runtime windows, zero collisions, and warning-free logs.",
}


STEM_CONTEXT: dict[str, str] = {
    "cmgr-unit": "The stem mission is a small single-ownship setup in the lower-band Forrest 19 / MIT_SP frame. `alpha` is the ownship; contacts such as `bravo` or `intruder` are synthetic targets introduced by individual cases so pContactMgrV20 can be checked without a full behavior stack.",
    "cmgr-motion": "The stem mission places `alpha` on a short transit with one or more synthetic contacts. The harness keeps the geometry compact and grades whether contact-manager output gives the avoidance behavior enough evidence to arrive safely.",
    "obmgr-unit": "The stem mission keeps the ownship context simple and introduces given obstacles or point clusters through the case inputs. That isolates pObstacleMgr message behavior before any moving obstacle-avoidance outcome is judged.",
    "obmgr-motion": "The stem mission sends one ownship through a short corridor with obstacle layouts that vary by case. The harness grades whether obstacle-manager alerts support arrival without collisions, near misses, or unresolved encounters.",
    "pid-unit": "The stem mission runs one shoreside MOOS community with scripted desired/nav mail and pMarinePIDV22 using suffixed outputs. It deliberately avoids a simulated vehicle so the cases can grade the PID app's direct actuator contract.",
    "pid-motion": "The stem mission runs one simulated vehicle and one shoreside evaluation community. The vehicle uses pHelmIvP, pMarinePIDV22, and uSimMarineV22; the shoreside evaluator grades bridged nav and actuator evidence.",
    "constant-depth-motion": "The shared depth stem mission runs one simulated UUV and one shoreside evaluation community. The vehicle uses pHelmIvP, pMarinePIDV22, and uSimMarineV22; the horizontal waypoint keeps the vehicle underway while BHV_ConstantDepth owns the vertical-control verdict.",
    "goto-depth-motion": "The shared depth stem mission runs one simulated UUV and one shoreside evaluation community. The vehicle uses pHelmIvP, pMarinePIDV22, and uSimMarineV22; the horizontal waypoint keeps the vehicle underway while BHV_GoToDepth owns the depth-sequence verdict.",
    "periodic-surface-motion": "The shared depth stem mission runs one simulated UUV and one shoreside evaluation community. The vehicle uses pHelmIvP, pMarinePIDV22, and uSimMarineV22; the horizontal waypoint keeps the vehicle underway while BHV_PeriodicSurface owns the surfacing-cycle verdict.",
    "max-depth-motion": "The shared depth stem mission runs one simulated UUV and one shoreside evaluation community. The vehicle uses pHelmIvP, pMarinePIDV22, and uSimMarineV22; the horizontal waypoint keeps the vehicle underway while BHV_MaxDepth owns the over-depth guard verdict.",
    "min-altitude-motion": "The shared depth stem mission runs one simulated UUV and one shoreside evaluation community. The vehicle uses pHelmIvP, pMarinePIDV22, and uSimMarineV22; the horizontal waypoint keeps the vehicle underway while BHV_MinAltitude owns the bottom-clearance verdict.",
    "colregs-classification": "The shared COLREGS stem mission uses two vessels, `ben` and `abe`; `ben` is the evaluated ownship. Case overlays vary the encounter geometry for head-on, crossing, overtaking, and overtaken situations.",
    "colregs-thresholds": "The shared COLREGS stem mission uses two vessels, `ben` and `abe`; `ben` is the evaluated ownship. Threshold cases reuse the same stem while moving the geometry just below, at, or above classification boundaries.",
    "colregs-execution": "The shared COLREGS stem mission uses two vessels, `ben` and `abe`; `ben` is the evaluated ownship. Execution cases reuse trusted classification geometry and then judge the maneuver that actually unfolds.",
    "colregs-parameters": "The shared COLREGS stem mission uses two vessels, `ben` and `abe`; `ben` is the evaluated ownship. Parameter cases keep the encounter setup stable while changing one behavior setting at a time.",
    "collision-behavior-motion": "The stem mission puts one ownship in a compact moving encounter with a synthetic contact. Unlike the app-level contact-manager harnesses, this stem focuses on what BHV_AvoidCollision requests, publishes, and resolves.",
    "obstacle-behavior-motion": "The stem mission runs one ownship through deterministic obstacle layouts with obstacle-manager input held stable. The cases focus on what BHV_AvoidObstacleV24 requests, flags, and resolves.",
    "opregion-safety": "The stem mission keeps one ownship on a compact transit through configured operating-region envelopes. Case overlays reshape save/halt regions or timing parameters so the behavior's safety flags and breach handling can be graded directly.",
    "waypoint-behavior-motion": "The stem mission keeps one simulated vehicle on a short waypoint corridor. Case overlays change route shape, capture settings, runtime point updates, or status variables while the mission grades behavior-owned completion evidence.",
    "loiter-behavior-motion": "The stem mission starts one simulated vehicle near a compact loiter area. Case overlays vary polygon geometry, acquisition conditions, center updates, and speed behavior while grading loiter-owned status outputs.",
    "stationkeep-behavior-motion": "The stem mission starts one simulated vehicle near a station point. Case overlays change the station geometry, hibernation settings, runtime retargeting, and speed/radius settings while grading mode and distance outputs.",
    "trail-behavior-motion": "The stem mission uses two simulated vehicles: `abe` runs BHV_Trail while `ben` provides the moving contact track. Case overlays vary trailing geometry, relevance settings, runtime updates, and missing-contact behavior.",
    "convoy-behavior-motion": "The stem mission uses two simulated vehicles: `abe` runs BHV_Convoy while `ben` creates the breadcrumb trail. Case overlays vary mark queues, following geometry, speed branches, and runtime updates.",
    "cutrange-behavior-motion": "The stem mission uses two simulated vehicles: `abe` runs BHV_CutRange and tries to reduce range to `ben`. Case overlays vary target route geometry, relevance distances, give-up conditions, runtime updates, and warning/error paths.",
    "fixedturn-behavior-motion": "The stem mission runs one simulated vehicle through a short approach leg before activating BHV_FixedTurn. Case overlays vary turn direction, fixed-turn magnitude, scheduled turn specifications, speed selection, and timeout behavior.",
    "shadow-behavior-motion": "The stem mission uses two simulated vehicles: `abe` runs BHV_Shadow while `ben` provides controlled contact headings and speeds. Case overlays vary target motion, relevance distance, runtime updates, and missing-contact behavior.",
    "performance-obstacle-gauntlet": "The stem mission is a repeatable obstacle-field loop. Individual cases change field density or duration so the same avoidance stack can be tested under baseline, dense, and longer-running pressure.",
    "performance-colregs-joust": "The stem mission creates sustained two- or three-vehicle COLREGS encounters. Cases adjust traffic density and duration to test whether safe spacing holds beyond a single isolated encounter.",
    "performance-traffic-ring": "The stem mission keeps five vehicles moving through a seeded traffic-ring assignment pattern. The seed makes the pressure replayable enough for CI while still exercising repeated COLREGS interactions.",
}


def repo_url(path: str) -> str:
    return f"{REPO_BLOB}/{path}"


def ctest_targets():
    if str(SCRIPTS_DIR) not in sys.path:
        sys.path.insert(0, str(SCRIPTS_DIR))
    import check_cpp_tests

    return check_cpp_tests.all_targets()


def ctest_coverage_data() -> dict[str, object]:
    if not CTEST_COVERAGE_DATA.exists():
        return {}
    with CTEST_COVERAGE_DATA.open(encoding="utf-8") as handle:
        data = json.load(handle)
    families = data.get("families", {})
    if not isinstance(families, dict):
        return {}
    return families


def ctest_coverage_note(area: str, coverage_data: dict[str, object]) -> str:
    raw = coverage_data.get(area)
    if not isinstance(raw, dict):
        return "Coverage: not measured."
    line_percent = raw.get("line_percent")
    source_files = raw.get("source_files")
    if line_percent is None or not source_files:
        return "Coverage: not measured."
    return f"Coverage: {line_percent:.2f}% line direct-compiled."


def count_gtest_cases(path: Path) -> int:
    count = 0
    for source in path.rglob("*Test.cpp"):
        text = source.read_text(encoding="utf-8", errors="ignore")
        count += len(re.findall(r"^\s*TEST(?:_F)?\s*\(", text, re.MULTILINE))
    return count


def ctest_area_label(area: str) -> str:
    explicit = {
        "alogavg": "ALog Average",
        "alogbin": "ALog Bin",
        "alogcat": "ALog Cat",
        "alogcd": "ALog Collision Detect",
        "alogcheck": "ALog Check",
        "alogclip": "ALog Clip",
        "alogeplot": "ALog EPlot",
        "alogeval": "ALog Eval",
        "aloggrep": "ALog Grep",
        "aloghelm": "ALog Helm",
        "alogiter": "ALog Iter",
        "alogload": "ALog Load",
        "alogmhash": "ALog Mission Hash",
        "alogpare": "ALog Pare",
        "alogpick": "ALog Pick",
        "alogrm": "ALog Remove",
        "alogscan": "ALog Scan",
        "alogsort": "ALog Sort",
        "alogsplit": "ALog Split",
        "alogtest": "ALog Test",
        "alogtm": "ALog Time Mark",
        "apputil": "App Utilities",
        "behaviors": "Behavior Core",
        "behaviors_colregs": "COLREGS Behaviors",
        "behaviors_marine": "Marine Behaviors",
        "bhvutil": "Behavior Utilities",
        "contacts": "Contact Records",
        "dep_behaviors": "Deprecated Behaviors",
        "encounters": "Encounters",
        "evalutil": "Evaluation Utilities",
        "gen_moos_app": "MOOS App Generator",
        "genutil": "General Utilities",
        "geodaid": "Geodesy Aids",
        "helmivp": "Helm IvP",
        "ipfview": "IPF View",
        "isay": "iSay",
        "uhelmscope": "uHelmScope",
        "ivpbuild": "IvP Build",
        "ivpcore": "IvP Core",
        "ivpsolve": "IvP Solve",
        "logutils": "Log Utilities",
        "marine_pid": "Marine PID",
        "marineview": "Marine Viewer",
        "mbutil": "MB Utilities",
        "manifest": "Manifest",
        "nspatch": "NSPatch",
        "nsplug": "NSPlug",
        "obstacles": "Obstacles",
        "pechovar": "pEchoVar",
        "pickpos": "Pick Position",
        "pluck": "Pluck",
        "polar": "Polar Geometry",
        "prealm": "pRealm",
        "projfield": "Project Field",
        "realm": "Realm Core",
        "survey": "Survey Utilities",
        "tagrep": "Tag Replace",
        "turngeo": "Turn Geometry",
        "ucommand": "Command Utilities",
        "ufield": "Field Utilities",
        "ufldbeaconrangesensor": "uFldBeaconRangeSensor",
        "ufldcollisiondetect": "uFldCollisionDetect",
        "uflddelve": "uFldDelve",
        "uplotviewer": "uPlotViewer",
        "utimerscript": "uTimerScript",
        "zaicview": "ZAIC View",
    }
    if area in explicit:
        return explicit[area]
    return area.replace("_", " ").title()


def ctest_area_description(area: str) -> str:
    explicit = {
        "alogavg": "ALog averaging handlers, time-window parsing, numeric aggregation, and malformed log-input edges.",
        "alogbin": "ALog binning handlers, variable bucketing, interval boundaries, and report formatting.",
        "alogcat": "ALog concatenation handlers, file ordering, header handling, and timestamp normalization.",
        "alogcd": "ALog collision-detection reporting over contact and CPA-related log records.",
        "alogcheck": "ALog check condition parsing, log evaluation, pass/fail reporting, and malformed specification handling.",
        "alogclip": "ALog clipping handlers, inclusive time windows, header preservation, preserved variables, and CLI-facing prechecks.",
        "alogeplot": "ALog expression plotting helpers, variable extraction, source filtering, and expression/report behavior.",
        "alogeval": "ALog evaluation helpers for expression parsing, time windows, condition handling, and result summaries.",
        "aloggrep": "ALog grep handlers, variable/source filters, time gates, format options, and malformed selector handling.",
        "aloghelm": "ALog helm report extraction, helm summary parsing, iteration handling, and report formatting.",
        "alogiter": "ALog iteration handlers, timestamp bucketing, iteration boundaries, and summary output.",
        "alogload": "ALog load-report helpers, per-app load parsing, threshold handling, and output summaries.",
        "alogmhash": "ALog mission-hash reporting, hash extraction, missing-field behavior, and report formatting.",
        "alogpare": "ALog pare engine behavior for variable/source filtering, retained records, and file output.",
        "alogpick": "ALog pick handlers for field selection, output formatting, and malformed selector behavior.",
        "alogrm": "ALog removal filters, variable/source exclusion, retained headers, and filter edge cases.",
        "alogscan": "ALog scan handlers, variable discovery, source summaries, and scan-output formatting.",
        "alogsort": "ALog sort handlers, timestamp ordering, stable header behavior, and malformed-line handling.",
        "alogsplit": "ALog split handlers, split-key parsing, output naming, batch behavior, and file prechecks.",
        "alogtest": "ALog test condition parsing, truth evaluation, time-window checks, and result reporting.",
        "alogtm": "ALog time-mark grep handlers, mark selection, time extraction, and malformed marker handling.",
        "apputil": "AppCast, InfoCast, and display-formatting helpers used by app-facing utilities.",
        "behaviors": "Core IvP behavior state, contact behavior state, and lifecycle records.",
        "behaviors_colregs": "COLREGS behavior support logic, velocity filtering, AOF helpers, and behavior contracts.",
        "behaviors_marine": "Marine behavior implementations such as waypoint, loiter, station keeping, collision avoidance, and obstacle avoidance.",
        "bhvutil": "Behavior utility engines, AOF helpers, refineries, waypoint/loiter helpers, and fixed-turn support.",
        "contacts": "Node record and rider parsing, serialization, and contact-report behavior.",
        "dep_behaviors": "Deprecated behavior and obstacle-model code kept under regression coverage.",
        "encounters": "CPA encounter events and alog-facing encounter records.",
        "evalutil": "Mission-evaluation helpers, logic-test sequences, and variable-check utilities.",
        "gen_moos_app": "MOOS app generator source transformations, file planning, naming rules, and generated text checks.",
        "genutil": "General-purpose string, file, pipe, and utility helpers.",
        "geodaid": "Contact-ledger and geodesy-aid logic used by contact-aware apps.",
        "geometry": "Geometry primitives, parsers, CPA utilities, shape lifecycles, grids, paths, and polygon generation.",
        "helmivp": "Helm behavior-loading and core helm state helpers.",
        "ipfview": "IPF viewer parsing and model support.",
        "isay": "iSay utterance parsing, voice validation, priority queue ordering, and audio-command-adjacent helper behavior.",
        "uhelmscope": "uHelmScope behavior records, iteration-block parsing, behavior-list summaries, decision-variable tracking, and mode formatting.",
        "ivpbuild": "AOFs, ZAICs, PDMaps, reflector internals, and IvP function construction.",
        "ivpcore": "Core IvP objects, domains, boxes, functions, and priority data structures.",
        "ivpsolve": "IvP problem-solving and encoded-function behavior.",
        "logic": "Logic conditions, info buffers, conditional ledgers, and related parsing helpers.",
        "logutils": "ALog parsing, log files, plot data, app logs, and analysis-plot utilities.",
        "manifest": "Manifest file parsing, population, and manifest-set behavior.",
        "marine_pid": "Marine PID engines and pMarinePIDV22 app-level control logic.",
        "marineview": "Marine viewer shapes, GUI-adjacent models, image handling, and viewer support classes.",
        "mbutil": "MOOS-IvP utility objects, formatting, timers, macros, hashes, JSON helpers, and node messages.",
        "nspatch": "NSPatch patch-file parsing, named-block replacement, line patch behavior, and conflict/edge semantics.",
        "nsplug": "NSPlug expansion helpers, macro handling, conditional expansion, and template-processing behavior.",
        "obstacles": "Obstacle parsing, representation, and obstacle-manager support objects.",
        "pechovar": "pEchoVar EFlipper parsing, component mapping, separator handling, and filter suppression behavior.",
        "pickpos": "Pick-position parsing, geometry selection, output formatting, and invalid-input handling.",
        "pluck": "Pluck handlers for extracting selected fields, formatting output, and rejecting malformed requests.",
        "polar": "Polar plotting and simulator-facing polar helpers.",
        "prealm": "pRealm PipeWay parsing, display flag handling, expiry math, content descriptors, and equality semantics.",
        "projfield": "Field projection helpers, input parsing, projection geometry, and output formatting.",
        "realm": "RealmCast and WatchCast core records and channel logic.",
        "survey": "Survey pattern generation and survey-update helpers.",
        "tagrep": "Tag replacement handlers, tag parsing, replacement rules, and malformed template behavior.",
        "turngeo": "Turn-generation geometry and leg-run turn helpers.",
        "ucommand": "Command item, command folio, command summary, and command-routing helpers.",
        "ufield": "Field utilities, host records, pShare-facing records, and field communication helpers.",
        "ufldbeaconrangesensor": "uFldBeaconRangeSensor beacon-buoy state, frequency parsing, range settings, drawing hints, counters, and spec formatting.",
        "ufldcollisiondetect": "uFldCollisionDetect CPA monitor state, NODE_REPORT admission, group filters, range thresholds, contact density, and CPA event emission.",
        "uflddelve": "uFldDelve transmission records, message counters, latest-source tracking, rate-window math, and pruning boundaries.",
        "uplotviewer": "uPlotViewer partition records, histogram bucket boundaries, running averages, range filtering, and color/name helpers.",
        "utimerscript": "uTimerScript random variables, random pairs, event parsing, scheduling helpers, math macro expansion, and block-app handling.",
        "zaicview": "ZAIC viewer models and GUI-adjacent support code.",
    }
    if area in explicit:
        return explicit[area]
    return f"Source-level checks for the {ctest_area_label(area)} component group."


def ctest_area_rows(targets=None) -> list[CTestArea]:
    targets = list(ctest_targets() if targets is None else targets)
    coverage_data = ctest_coverage_data()
    area_targets: dict[str, list[object]] = {}
    for target in targets:
        area = target.file.relative_to(CPP_TEST_DIR).parts[0]
        area_targets.setdefault(area, []).append(target)

    areas: list[CTestArea] = []
    for area, grouped_targets in area_targets.items():
        area_path = CPP_TEST_DIR / area
        labels: set[str] = set()
        for target in grouped_targets:
            labels.update(target.values("LABELS"))
        areas.append(
            CTestArea(
                key=area,
                label=ctest_area_label(area),
                target_count=len(grouped_targets),
                source_count=sum(1 for _ in area_path.rglob("*Test.cpp")),
                case_count=count_gtest_cases(area_path),
                description=ctest_area_description(area),
                coverage_note=ctest_coverage_note(area, coverage_data),
                labels=tuple(sorted(label for label in labels if label)),
                path=f"tests/cpp/{area}",
            )
        )
    return sorted(areas, key=lambda item: (-item.target_count, item.label))


def ctest_total_target_count(targets=None) -> int:
    return len(list(ctest_targets() if targets is None else targets))


def ctest_total_source_count() -> int:
    return sum(1 for _ in CPP_TEST_DIR.rglob("*Test.cpp"))


def ctest_total_case_count() -> int:
    return count_gtest_cases(CPP_TEST_DIR)


def ctest_area_count(targets=None) -> int:
    return len(ctest_area_rows(targets))


def inline_code(text: str) -> str:
    parts = re.split(r"(`[^`]+`)", text)
    rendered: list[str] = []
    for part in parts:
        if part.startswith("`") and part.endswith("`"):
            rendered.append(f"<code>{escape(part[1:-1])}</code>")
        else:
            rendered.append(escape(part))
    return "".join(rendered)


def page_shell(title: str, body: str, prefix: str = "") -> str:
    return f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>{escape(title)} | MOOS-IvP CI/CD</title>
  <link rel="stylesheet" href="{prefix}styles.css?v={ASSET_VERSION}">
  <script src="{prefix}theme.js?v={ASSET_VERSION}"></script>
</head>
<body>
  <header class="site-header">
    <a class="brand" href="{prefix}index.html">
      <span class="brand-mark">CI</span>
      <span>MOOS-IvP Harnesses</span>
    </a>
    <div class="header-actions">
      <nav>
        <a href="{prefix}index.html#families">Harnesses</a>
        <a href="{prefix}ctest-coverage.html">CTest Coverage</a>
        <a href="{prefix}user-guide.html">Run Tests</a>
        <a href="{prefix}technical.html">Technical</a>
      </nav>
      <label class="theme-switch" title="Toggle Frost theme">
        <input type="checkbox" data-theme-toggle aria-label="Frost theme">
        <span class="theme-switch-track" aria-hidden="true"></span>
      </label>
    </div>
  </header>
  {body}
  <footer class="site-footer">
    <span>Overview site for the MOOS-IvP CI/CD test workspace.</span>
    <span>Source mission and harness links are included for readers who want implementation detail.</span>
  </footer>
</body>
</html>
"""


def phrase_colregs_case(case_name: str) -> str:
    name = case_name.removesuffix("_pass")
    name = name.removesuffix("_execution")
    name = name.replace("_execution", "")

    mirror = " Mirrored geometry checks the same branch on the opposite side." if "_mirror" in name else ""
    spacing = ""
    if "_far" in name or "_range_far" in name:
        spacing = " with a longer-range setup"
    elif "_close" in name:
        spacing = " with a tighter setup"
    elif "_small_gap" in name:
        spacing = " with a smaller lateral gap"
    elif "_large_gap" in name:
        spacing = " with a larger lateral gap"
    elif "_midrange" in name:
        spacing = " with a midrange live-encounter setup"

    if name.startswith("head_on_cpa_fallback"):
        return f"Near-head-on geometry expected to fall back to CPA rather than true head-on classification{spacing}."
    if name.startswith("head_on_port_offset"):
        return "Offset head-on encounter on the port side; expected to remain in the head-on family."
    if name.startswith("head_on_starboard_offset"):
        return "Offset head-on encounter on the starboard side; expected to remain in the head-on family."
    if name.startswith("head_on"):
        return f"Head-on encounter used to check the head-on COLREGS branch{spacing}."

    if name.startswith("crossing_starboard_giveway"):
        return f"Starboard crossing where ownship should take the give-way role{spacing}."
    if name.startswith("crossing_port_standon_close_unsure_bow"):
        return "Tight port-side crossing expected to enter the stand-on unsure-bow branch before resolving."
    if name.startswith("crossing_port_standon_unsure_bow"):
        return "Port-side crossing expected to enter the stand-on unsure-bow branch before resolving."
    if name.startswith("crossing_port_standon_stern"):
        return "Port-side crossing expected to settle into the stand-on stern branch."
    if name.startswith("crossing_port_standon_unsure"):
        return "Port-side crossing expected to enter the generic stand-on unsure branch."
    if name.startswith("crossing_port_standon"):
        return f"Port-side crossing where ownship should stand on{spacing}."

    if name.startswith("overtaking_starboard"):
        side = "mirrored passing side" if "_mirror" in name else "starboard passing side"
        return f"Ownship overtakes on the {side}{spacing}; classification should remain in the overtaking family.{mirror}"
    if name.startswith("overtaken_port"):
        return f"Ownship is being overtaken on its port side{spacing}; the stand-on overtaken role should be preserved."
    if name.startswith("overtaken_starboard"):
        return f"Ownship is being overtaken on its starboard side{spacing}; the stand-on overtaken role should be preserved."

    return ""


def phrase_threshold_case(case_name: str) -> str:
    name = case_name.removesuffix("_pass")
    mirror = " mirrored" if name.endswith("_mirror") or "_mirror_" in name else ""
    base = name.replace("_mirror", "")

    side = ""
    if "_below" in base:
        side = "below"
    elif "_edge" in base:
        side = "at the calibrated edge of"
    elif "_above" in base:
        side = "above"

    if base.startswith("head_on_thresh"):
        return f"Probes {side} the head-on entry cutoff{mirror}; expected classification should land on the calibrated side of head-on vs CPA."
    if base.startswith("giveway_bowdist"):
        return f"Probes {side} the give-way bow-distance split{mirror}; expected classification should stay stern-side or flip bow-side as calibrated."
    if base.startswith("giveway_turngap"):
        return f"Probes {side} the give-way turn-gap split{mirror}; expected classification should follow the stock turn-gap branch."
    if base.startswith("standon_unsurebow"):
        return f"Probes {side} the stand-on unsure-bow transition; expected classification should remain on the calibrated side of unsure-bow vs bow."
    if base.startswith("standon_band350_unsurebow_alt"):
        return "Alternate band-350 stand-on geometry expected to remain in the unsure-bow branch."
    if base.startswith("standon_band350_unsurebow"):
        return "Band-350 stand-on geometry expected to remain in the unsure-bow branch."
    if base.startswith("standon_band350_bow"):
        return "Band-350 stand-on geometry expected to resolve to the bow branch."
    if base.startswith("standon_band315_unsure_bow"):
        return "Band-315 stand-on geometry expected to enter the unsure-bow branch."
    if base.startswith("standon_band315_unsure"):
        return "Band-315 stand-on geometry expected to enter the generic unsure branch."
    if base.startswith("standon_band315_bow"):
        return "Band-315 stand-on geometry expected to resolve to the bow branch."
    if base.startswith("standon_band270_stern"):
        return "Band-270 stand-on geometry expected to resolve to the stern branch."
    if base.startswith("standon_southwest_unsurebow"):
        return "Southwest stand-on probe expected to enter the unsure-bow branch."
    if base.startswith("standon_southwest_unsure"):
        return "Southwest stand-on probe expected to enter the generic unsure branch."
    if base.startswith("standon_southwest_stern"):
        return "Southwest stand-on probe expected to resolve to the stern branch."
    if base.startswith("standon_ot_inextremis_range"):
        return f"Probes {side} the overtaken-vessel in-extremis range cutoff; expected classification should follow the calibrated range branch."
    if base.startswith("standon_ot_inextremis_cpa"):
        return f"Probes {side} the overtaken-vessel in-extremis CPA cutoff; expected classification should follow the calibrated CPA branch."
    if base.startswith("standon_inextremis_range"):
        return f"Probes {side} the stand-on in-extremis range cutoff; expected classification should follow the calibrated range branch."
    if base.startswith("standon_inextremis_cpa"):
        return f"Probes {side} the stand-on in-extremis CPA cutoff; expected classification should follow the calibrated CPA branch."
    if base.startswith("overtaking_thresh"):
        return f"Probes {side} the overtaking classification threshold{mirror}; expected classification should stay on the calibrated side of overtaking vs fallback."
    if base.startswith("overtaken_thresh"):
        return f"Probes {side} the overtaken-vessel classification threshold{mirror}; expected classification should stay on the calibrated side of stand-on overtaken vs fallback."
    if base.startswith("outer_dist"):
        return f"Probes {side} the COLREGS outer-distance relevance gate; expected behavior should activate only on the calibrated side."
    return ""


def phrase_parameter_case(case_name: str) -> str:
    name = case_name.removesuffix("_pass")
    if name.startswith("min_util_cpa_"):
        level = name.removeprefix("min_util_cpa_")
        return f"Runs fixed stand-on geometry with {level} `min_util_cpa_dist`; expected classification should reflect the CPA-distance setting."
    if name.startswith("headon_only_"):
        state = name.removeprefix("headon_only_")
        return f"Runs crossing geometry with `headon_only={state}` to verify non-head-on encounters are either admitted or demoted as expected."
    if name.startswith("giveway_bow_dist_"):
        level = name.removeprefix("giveway_bow_dist_")
        return f"Runs fixed give-way bow-distance geometry with the {level} threshold setting."
    if name.startswith("max_util_cpa_"):
        level = name.removeprefix("max_util_cpa_")
        return f"Runs fixed stand-on geometry with {level} `max_util_cpa_dist`; expected classification should reflect the widened CPA envelope."
    if name.startswith("velocity_filter_"):
        state = name.removeprefix("velocity_filter_")
        return f"Runs fixed stand-on geometry with velocity filtering {state}; expected classification should show whether the filter changes effective CPA evaluation."
    if name.startswith("eval_tol_"):
        level = name.removeprefix("eval_tol_")
        return f"Runs fixed overtaking geometry with {level} `eval_tol`; expected execution quality should remain in the configured CPA band."
    if name.startswith("pwt_outer_dist_"):
        level = name.removeprefix("pwt_outer_dist_")
        return f"Runs fixed range-gate geometry with {level} `pwt_outer_dist`; expected relevance should admit or suppress COLREGS behavior accordingly."
    if name.startswith("pwt_inner_dist_"):
        level = name.removeprefix("pwt_inner_dist_")
        return f"Runs fixed stand-on geometry with {level} `pwt_inner_dist`; expected posted priority weight should follow the configured inner-distance band."
    if name.startswith("pwt_grade_"):
        grade = name.removeprefix("pwt_grade_")
        return f"Runs fixed relevance geometry with `pwt_grade={grade}` and checks the expected priority-weight curve."
    if name.startswith("pts_port_turns_ok_"):
        state = name.removeprefix("pts_port_turns_ok_")
        return f"Runs fixed give-way geometry with `pts_port_turns_ok={state}` and checks the early side relationship."
    if name.startswith("turn_radius_"):
        level = name.removeprefix("turn_radius_")
        return f"Runs fixed give-way turn-gap geometry with {level} `turn_radius`; expected classification should reflect the turn-radius branch."
    if name.startswith("check_plateaus_"):
        state = name.removeprefix("check_plateaus_")
        return f"Runs CPA fallback geometry with `check_plateaus={state}` and checks whether refinery diagnostics are posted."
    if name.startswith("completed_dist_"):
        level = name.removeprefix("completed_dist_")
        return f"Runs CPA fallback geometry with {level} `completed_dist` and checks whether the encounter has completed at the shared checkpoint."
    if name.startswith("refinery_"):
        state = name.removeprefix("refinery_")
        return f"Runs CPA fallback geometry with refinery {state}; expected classification should remain CPA while diagnostics follow the refinery setting."
    return ""


def phrase_performance_case(slug: str, case_name: str) -> str:
    if slug == "performance-obstacle-gauntlet":
        return {
            "baseline_field_pass": "Baseline obstacle-field traversal; expected to complete the loop with zero collisions and clean warning logs.",
            "dense_field_pass": "Denser obstacle-field traversal; expected to preserve clean completion under heavier obstacle pressure.",
            "endurance_random_pass": "Longer seeded obstacle-field run; expected to preserve completion and safety over an extended randomized field.",
        }.get(case_name, "")
    if slug == "performance-colregs-joust":
        return {
            "baseline_colregs_pass": "Baseline sustained COLREGS joust; expected to complete with zero collisions and safe closest-range telemetry.",
            "dense_colregs_pass": "Denser COLREGS joust; expected to preserve safe spacing under added multi-vehicle pressure.",
            "endurance_colregs_pass": "Longer COLREGS joust; expected to preserve warning-free, collision-free behavior over an extended run.",
        }.get(case_name, "")
    if slug == "performance-traffic-ring":
        return {
            "baseline_circle_pass": "Baseline seeded five-vehicle traffic ring; expected to maintain assignment activity with zero collisions.",
            "mixed_speed_circle_pass": "Traffic ring with mixed vehicle speeds; expected to keep assignment pressure active while remaining collision-free.",
            "endurance_circle_pass": "Longer traffic-ring run; expected to sustain repeated assignments over the endurance window with zero collisions.",
            "noncoop_circle_pass": "Traffic ring with one non-cooperative vehicle; expected to remain collision-free while `eve` runs with avoidance disabled.",
        }.get(case_name, "")
    return ""


def describe_case(slug: str, case_name: str) -> str:
    if slug == "colregs-thresholds":
        desc = phrase_threshold_case(case_name)
    elif slug == "colregs-parameters":
        desc = phrase_parameter_case(case_name)
    elif slug in {"colregs-classification", "colregs-execution"}:
        desc = phrase_colregs_case(case_name)
        if slug == "colregs-execution" and desc:
            desc = desc.replace("classification", "realized maneuver").replace("classify", "complete")
    elif slug.startswith("performance-"):
        desc = phrase_performance_case(slug, case_name)
    else:
        desc = ""

    if desc:
        return desc

    clean = case_name.replace("_", " ").replace("-", " ")
    clean = re.sub(r"\bpass\b", "expected pass", clean)
    clean = re.sub(r"\bfail\b", "expected fail", clean)
    clean = re.sub(r"\babsent\b", "absence check", clean)
    return f"Covers {clean}."


def normalize_description(text: str, max_len: int | None = 165) -> str:
    text = text.replace("`", "")
    text = re.sub(r"\bpatched mission\b", "case", text, flags=re.IGNORECASE)
    text = re.sub(r"\bis patched to\b", "is configured in this case to", text, flags=re.IGNORECASE)
    text = re.sub(r"\bis patched with\b", "is configured in this case with", text, flags=re.IGNORECASE)
    text = re.sub(r"\bis patched so\b", "is configured in this case so", text, flags=re.IGNORECASE)
    text = re.sub(r"\bthe behavior patch sets\b", "this case sets", text, flags=re.IGNORECASE)
    text = re.sub(r"\bthe behavior patch provides\b", "this case provides", text, flags=re.IGNORECASE)
    text = re.sub(r"\bpatched\b", "configured for this case", text, flags=re.IGNORECASE)
    text = re.sub(r"\s+", " ", text).strip()
    text = text.strip(" :-")
    if not text:
        return ""
    if max_len is not None and len(text) > max_len:
        text = text[: max_len - 3].rsplit(" ", 1)[0] + "..."
    return text


def zlaunch_case_order(h: Harness) -> list[str]:
    zlaunch = ROOT.parent / h.path / "zlaunch.sh"
    if not zlaunch.exists():
        return []

    text = zlaunch.read_text(encoding="utf-8", errors="ignore")
    return zlaunch_all_cases(text)


def zlaunch_all_cases(text: str) -> list[str]:
    arrays: dict[str, list[str]] = {}
    for match in CASE_ARRAY_RE.finditer(text):
        name, body = match.groups()
        try:
            arrays[name] = shlex.split(body, comments=True, posix=True)
        except ValueError:
            arrays[name] = CASE_NAME_RE.findall(body)

    if "ALL_CASES" not in arrays:
        return []

    def expand_array(name: str, stack: tuple[str, ...] = ()) -> list[str]:
        if name in stack:
            return []
        expanded: list[str] = []
        for token in arrays.get(name, []):
            ref = ARRAY_REF_RE.match(token)
            if ref and ref.group(1) in arrays:
                expanded.extend(expand_array(ref.group(1), stack + (name,)))
            elif CASE_NAME_RE.search(token):
                expanded.append(token)
        return expanded

    names: list[str] = []
    seen: set[str] = set()
    for token in expand_array("ALL_CASES"):
        if token not in seen:
            names.append(token)
            seen.add(token)
    return names


def zlaunch_case_names(h: Harness) -> set[str]:
    return set(zlaunch_case_order(h))


def extract_case_docs(h: Harness, max_len: int | None = None) -> list[tuple[str, str]]:
    generated_description_slugs: set[str] = set()
    ordered_cases = zlaunch_case_order(h)
    if h.slug in generated_description_slugs:
        return [(case, describe_case(h.slug, case)) for case in ordered_cases]

    readme = ROOT.parent / h.path / "README.md"
    if not readme.exists():
        return [(case, describe_case(h.slug, case)) for case in ordered_cases]

    lines = readme.read_text(encoding="utf-8", errors="ignore").splitlines()
    found: dict[str, str] = {}
    seen: set[str] = set()
    actual_cases = set(ordered_cases)
    case_line = re.compile(r"^\s*-\s+`([^`]+)`\s*:?\s*(.*)$")

    for i, line in enumerate(lines):
        match = case_line.match(line)
        if not match:
            continue

        case_name = match.group(1).strip()
        if case_name in seen:
            continue
        if not re.search(r"(_pass|_fail|_absent|group=| threshold group|parameter)", case_name):
            continue
        if case_name not in actual_cases:
            continue

        parts = [match.group(2).strip()]
        for follow in lines[i + 1:]:
            stripped = follow.strip()
            if not stripped:
                continue
            if stripped.startswith("```") or stripped.startswith("#") or stripped.startswith("- `") or re.match(r"^\s*-\s+\S", follow):
                break
            if follow.startswith(" ") or follow.startswith("\t"):
                parts.append(stripped)
                continue
            break

        desc = normalize_description(" ".join(parts), max_len=max_len)
        if desc:
            found[case_name] = desc
            seen.add(case_name)

    if not found:
        return [(case, describe_case(h.slug, case)) for case in ordered_cases]

    return [(case, found.get(case, describe_case(h.slug, case))) for case in ordered_cases]


def case_rows(h: Harness) -> str:
    return "\n".join(
        f"""
        <article class="case-row">
          <code>{escape(case_name)}</code>
          <p>{escape(description)}</p>
        </article>
"""
        for case_name, description in extract_case_docs(h)
    )


def gif_card(gif: tuple[str, str, str], descriptions: dict[str, str], prefix: str) -> str:
    title, case_name, filename = gif
    asset = f"{prefix}assets/gifs/{filename}"
    description = descriptions.get(case_name, describe_case("", case_name))
    return f"""
      <article class="gif-card">
        <div class="gif-frame" data-file="{escape(filename)}">
          <img src="{asset}" alt="{escape(title)} GIF" onerror="this.style.display='none'">
        </div>
        <div class="gif-meta">
          <h3>{escape(title)}</h3>
          <p>Description: {escape(description)}</p>
        </div>
      </article>
"""


def harness_card(h: Harness) -> str:
    return f"""
      <a class="harness-card" href="harnesses/{h.slug}.html">
        <span class="eyebrow">{escape(h.family)}</span>
        <h3>{escape(h.title)}</h3>
        <p>{escape(h.summary)}</p>
      </a>
"""


def family_panel(family: Family, harness_by_slug: dict[str, Harness]) -> str:
    cards = "\n".join(harness_card(harness_by_slug[slug]) for slug in family.slugs)
    count = len(family.slugs)
    noun = "harness" if count == 1 else "harnesses"
    return f"""
      <details class="family-panel">
        <summary>
          <span class="family-label">{escape(family.label)}</span>
          <span class="family-name">{escape(family.name)}</span>
          <span class="family-summary">{escape(family.summary)}</span>
          <span class="family-count">{count} {noun}</span>
        </summary>
        <div class="family-harnesses">
          {cards}
        </div>
      </details>
"""


def harness_case_counts() -> dict[str, int]:
    counts: dict[str, int] = {}
    for zlaunch in sorted(HARNESS_DIR.glob("*/*/zlaunch.sh")):
        text = zlaunch.read_text()
        all_cases = zlaunch_all_cases(text)
        if not all_cases:
            raise ValueError(f"No ALL_CASES entries found in {zlaunch}")
        counts[zlaunch.parent.name] = len(all_cases)
    return counts


def harness_count() -> int:
    return len(harness_case_counts())


def app_behavior_target_count() -> int:
    roots = {
        zlaunch.parents[1].name
        for zlaunch in HARNESS_DIR.glob("*/*/zlaunch.sh")
        if zlaunch.parents[1].name != "performance_harnesses"
    }
    return len(roots)


def total_case_count() -> int:
    return sum(harness_case_counts().values())


def render_stats() -> str:
    stats = (
        (str(ctest_total_case_count()), "CTest Cases", "Source-level checks for component behavior that does not need a mission scenario."),
        (str(ctest_area_count()), "Libraries & Components", "Source-level MOOS-IvP areas covered by GoogleTest/CTest."),
        (str(total_case_count()), "Harness Cases", "Mission-run cases with expected outcomes and generated result summaries."),
        (str(app_behavior_target_count()), "Apps & Behaviors", "MOOS apps and IvP behaviors covered by mission harness cases."),
    )
    return "\n".join(
        f"""
      <article class="stat-card">
        <strong>{escape(value)}</strong>
        <span>{escape(label)}</span>
        <p>{escape(text)}</p>
      </article>
"""
        for value, label, text in stats
    )


def load_manual_targets() -> list[dict[str, object]]:
    targets_path = REPO_ROOT / "config" / "harness_targets.json"
    with targets_path.open(encoding="utf-8") as stream:
        targets = json.load(stream)
    return targets


def manual_target_rows() -> str:
    rows = []
    for target in load_manual_targets():
        rows.append(
            f"""
          <tr>
            <th><code>{escape(str(target["key"]))}</code></th>
            <td>{escape(", ".join(str(item) for item in target["families"]))}</td>
            <td>{escape(str(target["harness"]))}</td>
          </tr>
"""
        )
    return "\n".join(rows)


def cpp_test_family_rows() -> str:
    if str(SCRIPTS_DIR) not in sys.path:
        sys.path.insert(0, str(SCRIPTS_DIR))
    import ci_cpp_test_targets

    rows = []
    for family in ci_cpp_test_targets.CPP_FAMILIES:
        rows.append(
            f"""
          <tr>
            <th><code>{escape(family)}</code></th>
            <td>{escape(ctest_area_label(family))}</td>
          </tr>
"""
        )
    return "\n".join(rows)


def ctest_area_table_rows(areas: list[CTestArea]) -> str:
    rows = []
    for area in areas:
        rows.append(
            f"""
            <tr class="ctest-summary-row">
              <th>
                <span class="component-key"><code>{escape(area.key)}</code></span>
                <span class="component-name">{escape(area.label)}</span>
              </th>
              <td>{area.target_count}</td>
              <td>{area.case_count}</td>
              <td class="ctest-description">{escape(area.description)} <span class="coverage-note">{escape(area.coverage_note)}</span></td>
            </tr>
"""
        )
    return "\n".join(rows)


def render_ctest_coverage() -> str:
    targets = ctest_targets()
    areas = ctest_area_rows(targets)
    body = f"""
  <main>
    <section class="page-hero page-hero--wide-lede">
      <a class="back-link" href="index.html">Back to overview</a>
      <p class="eyebrow">CTest Coverage</p>
      <h1>Fast source-level coverage for MOOS-IvP components.</h1>
      <p class="lede">These tests use GoogleTest and CTest to check MOOS-IvP component behavior that does not need a mission scenario: parsers, geometry, IvP functions, contact records, behavior helpers, app utility classes, and other source-level logic.</p>
      <div class="hero-actions">
        <a class="button primary" href="#buckets">Browse component families</a>
        <a class="button secondary" href="{repo_url('tests/cpp/README.md')}">CTest README</a>
      </div>
    </section>

    <section class="content-section stats-section">
      <div class="section-heading">
        <p class="eyebrow">Current Snapshot</p>
        <h2>CTest coverage at a glance</h2>
      </div>
      <div class="stats-grid stats-grid--three">
        <article class="stat-card">
          <strong>{ctest_total_case_count()}</strong>
          <span>Test Cases</span>
          <p>Individual GoogleTest checks covering expected and edge-case behavior.</p>
        </article>
        <article class="stat-card">
          <strong>{ctest_total_target_count(targets)}</strong>
          <span>CTest Targets</span>
          <p>Focused executables that group related component checks.</p>
        </article>
        <article class="stat-card">
          <strong>{len(areas)}</strong>
          <span>Component Families</span>
          <p>Groups such as behaviors, geometry, utilities, viewers, and IvP tools.</p>
        </article>
      </div>
    </section>

    <section id="buckets" class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Component Families</p>
        <h2>CTest coverage by MOOS-IvP component family</h2>
      </div>
      <div class="table-wrap">
        <table class="ctest-table">
          <thead>
            <tr>
              <th>Component family</th>
              <th>Targets</th>
              <th>Cases</th>
              <th>Description + Coverage %</th>
            </tr>
          </thead>
          <tbody>
            {ctest_area_table_rows(areas)}
          </tbody>
        </table>
      </div>
    </section>
  </main>
"""
    return page_shell("CTest Coverage", body)


def render_user_guide() -> str:
    body = f"""
  <main>
    <section class="page-hero">
      <a class="back-link" href="index.html">Back to overview</a>
      <p class="eyebrow">User Guide</p>
      <h1>Run regression checks from GitHub Actions.</h1>
      <p class="lede">Launch CTest coverage, mission harnesses, or both. CTest runs source-level component checks; mission harnesses run live MOOS scenarios.</p>
      <div class="hero-actions">
        <a class="button primary" href="{REPO_ACTIONS}">Open workflow</a>
        <a class="button secondary" href="#example-values">Example values</a>
        <a class="button secondary" href="#target-keys">Selectors</a>
      </div>
    </section>

    <section class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Before You Run</p>
        <h2>Plain-language workflow terms</h2>
        <p>The workflow form has two similar selection lanes. The CTest lane chooses source-level GoogleTest/CTest coverage; the harness lane chooses live MOOS mission scenarios.</p>
      </div>
      <div class="technical-grid">
        <article class="technical-card">
          <h3>Workflow</h3>
          <p>A workflow is a GitHub Actions page that runs CI. This guide uses the <code>Build And Check Active Harnesses</code> workflow.</p>
        </article>
        <article class="technical-card">
          <h3>CTest mode</h3>
          <p><code>cpp_test_mode</code> selects the CTest lane. Use it for source-level checks that do not need a mission scenario.</p>
        </article>
        <article class="technical-card">
          <h3>Harness mode</h3>
          <p><code>dispatch_mode</code> selects the mission-harness lane. Use it when behavior depends on live MOOS processes, ports, timing, or vehicle motion.</p>
        </article>
        <article class="technical-card">
          <h3>Family</h3>
          <p>Both lanes support one family or comma-separated family batches: <code>cpp_test_family</code>/<code>cpp_test_families</code> for CTest, and <code>family</code>/<code>families</code> for harnesses.</p>
        </article>
        <article class="technical-card">
          <h3>Exact harnesses</h3>
          <p>Harnesses can also run exact target keys through <code>targets</code>. CTest selection stays at the family level.</p>
        </article>
        <article class="technical-card">
          <h3>Selected workloads</h3>
          <p>The workflow can run CTest only, harnesses only, or both. It rejects only the empty case where both lanes are set to <code>none</code>.</p>
        </article>
      </div>
    </section>

    <section class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Normal Flow</p>
        <h2>Pick one lane, or run both</h2>
        <p>The workflow builds once, uploads that build as an artifact, then fans out the selected CTest and harness rows. Both lanes support one family or comma-separated family batches; harnesses also support exact target keys.</p>
      </div>
      <div class="technical-grid">
        <article class="technical-card">
          <h3>1. Open Actions</h3>
          <p>Use the <strong>Open workflow</strong> button at the top of this page, then click <code>Run workflow</code> in GitHub. Keep the branch on <code>main</code> unless you deliberately want to test another branch.</p>
        </article>
        <article class="technical-card">
          <h3>2. Choose CTest coverage</h3>
          <p>Use <code>cpp_test_mode: family_run</code> for one component family, or <code>batch_family_run</code> for a comma-separated batch.</p>
        </article>
        <article class="technical-card">
          <h3>3. Choose harness coverage</h3>
          <p>Use <code>dispatch_mode: none</code> to skip missions, or select <code>family_run</code>, <code>batch_family_run</code>, or <code>specific_harnesses</code>.</p>
        </article>
      </div>
    </section>

    <section class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Selection Modes</p>
        <h2>The two lanes use matching patterns</h2>
      </div>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Pattern</th>
              <th>CTest inputs</th>
              <th>Harness inputs</th>
            </tr>
          </thead>
          <tbody>
            <tr>
              <th>Skip this lane</th>
              <td><code>cpp_test_mode: none</code></td>
              <td><code>dispatch_mode: none</code></td>
            </tr>
            <tr>
              <th>One family</th>
              <td><code>cpp_test_mode: family_run</code><br><code>cpp_test_family: geometry</code></td>
              <td><code>dispatch_mode: family_run</code><br><code>family: waypoint_behavior</code></td>
            </tr>
            <tr>
              <th>Family batch</th>
              <td><code>cpp_test_mode: batch_family_run</code><br><code>cpp_test_families: geometry,ivpbuild,mbutil</code></td>
              <td><code>dispatch_mode: batch_family_run</code><br><code>families: convoy_behavior,cutrange_behavior,waypoint_behavior</code></td>
            </tr>
            <tr>
              <th>Exact harnesses</th>
              <td><span class="muted">Use one family or a family batch.</span></td>
              <td><code>dispatch_mode: specific_harnesses</code><br><code>targets: cutrange_h01,waypoint_h01</code></td>
            </tr>
          </tbody>
        </table>
      </div>
    </section>

    <section id="example-values" class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Inputs</p>
        <h2>Example values</h2>
        <p>These examples show the same selection shapes on each side of the workflow form. Leave unused lane-specific fields unchanged; only the mode decides whether that lane runs.</p>
      </div>
      <div class="explain-stack">
        <article>
          <h3>CTest one family</h3>
          <pre><code>dispatch_mode: none
cpp_test_mode: family_run
cpp_test_family: geometry
moos_ivp_ref: main</code></pre>
        </article>
        <article>
          <h3>CTest family batch</h3>
          <pre><code>dispatch_mode: none
cpp_test_mode: batch_family_run
cpp_test_families: geometry,ivpbuild,mbutil
moos_ivp_ref: main</code></pre>
        </article>
        <article>
          <h3>Harness family batch</h3>
          <pre><code>dispatch_mode: batch_family_run
families: convoy_behavior,cutrange_behavior,waypoint_behavior
cpp_test_mode: none
time_warp: 10
moos_ivp_ref: main</code></pre>
        </article>
        <article>
          <h3>Harness one family</h3>
          <pre><code>dispatch_mode: family_run
family: waypoint_behavior
cpp_test_mode: none
time_warp: 10
moos_ivp_ref: main</code></pre>
        </article>
        <article>
          <h3>Combined family run</h3>
          <pre><code>dispatch_mode: family_run
family: waypoint_behavior
cpp_test_mode: family_run
cpp_test_family: geometry
time_warp: 10
moos_ivp_ref: main</code></pre>
        </article>
        <article>
          <h3>Harness targets with CTest family</h3>
          <pre><code>dispatch_mode: specific_harnesses
targets: cutrange_h01,waypoint_h01
cpp_test_mode: family_run
cpp_test_family: geometry
time_warp: 10
moos_ivp_ref: main</code></pre>
        </article>
      </div>
      <p class="guide-note">GitHub's manual workflow form does not support dynamic checkboxes from repository metadata, so batch selection uses comma-separated family names and exact harness selection uses comma-separated target keys.</p>
    </section>

    <section id="target-keys" class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Selectors</p>
        <h2>Selector values</h2>
        <p>Use these exact values in the GitHub Actions form. CTest values go in <code>cpp_test_family</code> or <code>cpp_test_families</code>. Harness family values go in <code>family</code> or <code>families</code>. Harness target keys go in <code>targets</code>.</p>
      </div>
      <h3>CTest families</h3>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Selector value</th>
              <th>Coverage area</th>
            </tr>
          </thead>
          <tbody>
            {cpp_test_family_rows()}
          </tbody>
        </table>
      </div>
      <h3>Harness target keys</h3>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Selector value</th>
              <th>Harness family</th>
              <th>Harness</th>
            </tr>
          </thead>
          <tbody>
            {manual_target_rows()}
          </tbody>
        </table>
      </div>
      <p class="guide-note">There is no catch-all harness or CTest mode. Use family batches when you want a broad run, and keep the batch explicit so runtime stays predictable as coverage grows.</p>
    </section>

    <section class="workflow">
      <p class="eyebrow">Interpreting Results</p>
      <h2>What success means</h2>
      <div class="guide-figures">
        <figure class="guide-figure">
          <img src="assets/images/workflow-graph-ctest-runtime-example.png" alt="GitHub Actions workflow graph showing prepare, build, ctest geometry, and runtime harness jobs">
          <figcaption>Example run <a href="https://github.com/cbenjamin23/moos-ivp-cicd-testing/actions/runs/25176874485">25176874485</a> selected one CTest family and two runtime harness jobs. The graph shows one shared build feeding both selected lanes.</figcaption>
        </figure>
        <article class="run-summary-example">
          <h3>Reading a passing run</h3>
          <ul class="summary-meta">
            <li>CTest row: <code>ctest / geometry</code></li>
            <li>Runtime matrix: <code>2 jobs completed</code></li>
            <li>Requested MOOS-IvP ref: <code>main</code></li>
            <li>Workflow verdict: <code class="result-pass">green</code></li>
          </ul>
          <div class="table-wrap">
            <table>
              <thead>
                <tr>
                  <th>Stage</th>
                  <th>What it proves</th>
                </tr>
              </thead>
              <tbody>
                <tr>
                  <th><code>prepare</code></th>
                  <td>Inputs were valid and resolved into CTest and harness matrix rows.</td>
                </tr>
                <tr>
                  <th><code>build</code></th>
                  <td>MOOS-IvP and this repo built once, then the shared build artifact was published.</td>
                </tr>
                <tr>
                  <th><code>ctest / geometry</code></th>
                  <td>The selected CTest family passed and produced a JUnit report artifact.</td>
                </tr>
                <tr>
                  <th><code>runtime / ...</code></th>
                  <td>Each selected harness ran its case matrix and uploaded its result summary.</td>
                </tr>
              </tbody>
            </table>
          </div>
        </article>
      </div>
      <ol>
        <li><code>prepare</code> validates the workflow inputs and resolves CTest families and harness selections into matrix rows.</li>
        <li><code>build</code> compiles once and validates the CTest registry before either test lane runs.</li>
        <li>Each <code>ctest / &lt;family&gt;</code> row runs the selected CTest family and uploads JUnit XML.</li>
        <li>Each <code>runtime / &lt;key&gt; / &lt;harness&gt;</code> row runs one harness <code>zlaunch.sh</code> and uploads <code>results.txt</code>.</li>
        <li>A green run means every selected CTest row passed and every selected harness completed with its expected case matrix.</li>
      </ol>
      <div class="workflow-actions">
        <a class="text-link" href="https://github.com/cbenjamin23/moos-ivp-cicd-testing/actions/runs/25176874485">Open example run</a>
        <a class="text-link" href="technical.html">Read the technical architecture</a>
      </div>
    </section>
  </main>
"""
    return page_shell("User Guide", body)


def render_index() -> str:
    harness_by_slug = {h.slug: h for h in HARNESSES}
    family_sections = "\n".join(family_panel(family, harness_by_slug) for family in FAMILIES)

    body = f"""
  <main>
    <section class="hero">
      <div class="hero-copy">
        <p class="eyebrow">MOOS-IvP CI/CD Pipeline</p>
        <h1>Regression tests for MOOS-IvP Apps, Behaviors, and Libraries</h1>
        <p class="lede">This workspace collects fast CTest coverage for source-level component behavior and mission harnesses for live MOOS app, behavior, and motion scenarios.</p>
        <div class="hero-actions">
          <a class="button primary" href="#families">Browse catalog</a>
          <a class="button secondary" href="ctest-coverage.html">CTest coverage</a>
          <a class="button secondary" href="user-guide.html">Run tests</a>
          <a class="button secondary" href="technical.html">Technical overview</a>
        </div>
      </div>
      <div class="hero-panel hero-panel--actions">
        <img class="actions-screenshot" src="assets/images/github-actions-runtime-harnesses.png" alt="GitHub Actions runtime harness matrix with passing jobs">
      </div>
    </section>

    <section class="content-section stats-section">
      <div class="section-heading">
        <p class="eyebrow">Repository Snapshot</p>
        <h2>Current coverage at a glance</h2>
      </div>
      <div class="stats-grid">
        {render_stats()}
      </div>
    </section>

    <section class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Coverage Layers</p>
        <h2>Two complementary ways to catch regressions</h2>
        <p>CTest coverage checks component behavior directly, while mission harnesses check live MOOS process behavior and motion outcomes.</p>
      </div>
      <div class="technical-grid">
        <article class="technical-card">
          <h3>CTest Coverage</h3>
          <p>Fast GoogleTest/CTest targets for MOOS-IvP components/libraries such as geometry, behaviors, contacts, utilities, viewers, and IvP construction.</p>
          <a class="text-link" href="ctest-coverage.html">Browse CTest coverage</a>
        </article>
        <article class="technical-card">
          <h3>Mission Harnesses</h3>
          <p>Patch-driven MOOS mission matrices that exercise app postings, behavior lifecycle, vehicle motion, timing, and multi-process outcomes.</p>
          <a class="text-link" href="#families">Browse harness catalog</a>
        </article>
        <article class="technical-card">
          <h3>Choosing A Layer</h3>
          <p>Use CTest for local C++ contracts. Use a harness when the behavior depends on live MOOS mail, helm state, simulator motion, or ports.</p>
          <a class="text-link" href="technical.html">Read the technical overview</a>
        </article>
      </div>
    </section>

    <section id="families" class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Harness Catalog</p>
        <h2>MOOS apps and IvP behaviors under test</h2>
        <p>Select an app or behavior family to see its harness pages.</p>
      </div>
      <div class="family-list">
        {family_sections}
      </div>
    </section>

    <section id="workflow" class="workflow">
      <p class="eyebrow">Mission Harness Flow</p>
      <h2>From scenario to mission result</h2>
      <ol>
        <li>Stem missions define reusable MOOS community layouts, behavior configs, and baseline geometry.</li>
        <li>Harness overlays use small patches so each run isolates one scenario or regression risk.</li>
        <li>The harness <code>zlaunch.sh</code> wrapper selects the case and then delegates to the shared <code>xlaunch.sh</code> path, keeping local and CI runs aligned.</li>
        <li><code>pMissionEval</code> reports and shell-side checks reduce the run to compact result lines.</li>
      </ol>
      <div class="workflow-actions">
        <a class="text-link" href="user-guide.html">Run harnesses manually</a>
        <a class="text-link" href="technical.html">Read the technical architecture</a>
      </div>
    </section>
  </main>
"""
    return page_shell("Home", body)


def render_harness(h: Harness) -> str:
    descriptions = dict(extract_case_docs(h, max_len=None))
    gifs = "\n".join(gif_card(g, descriptions, "../") for g in h.gifs)
    gif_grid_class = "gif-grid"
    if len(h.gifs) == 2:
        gif_grid_class += " gif-grid--two"
    examples_section = ""
    if h.gifs:
        examples_section = f"""
    <section class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Examples</p>
        <h2>Representative runs</h2>
      </div>
      {{visual_note}}
      <div class="{gif_grid_class}">{gifs}
      </div>
    </section>
"""
    test_style = TEST_STYLE.get(h.slug, h.proof)
    stem_context = STEM_CONTEXT.get(h.slug, "")
    visual_note_text = VISUAL_NOTES.get(h.slug, "")
    visual_note = ""
    if visual_note_text:
        visual_note = f"""
      <p class="visual-note">{escape(visual_note_text)} The visuals keep a pMarineViewer-like map frame while calling out the variable-level evidence that makes each case pass.</p>
"""
    stem_section = ""
    if stem_context:
        stem_section = f"""
    <section class="content-section compact-section">
      <article class="stem-card">
        <p class="eyebrow">Stem Mission</p>
        <h2>Baseline scenario context</h2>
        <p>{inline_code(stem_context)}</p>
      </article>
    </section>
"""
    body = f"""
  <main>
    <section class="page-hero">
      <a class="back-link" href="../index.html#families">Back to catalog</a>
      <p class="eyebrow">{escape(h.family)}</p>
      <h1>{escape(h.title)}</h1>
      <p class="lede">{escape(test_style)}</p>
    </section>

    {stem_section}

    {examples_section.format(visual_note=visual_note) if examples_section else ""}

    <section class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Case Matrix</p>
        <h2>All cases</h2>
      </div>
      <div class="case-matrix">
        {case_rows(h)}
      </div>
    </section>

    <section class="source-strip">
      <h2>Source links</h2>
      <a href="{repo_url(h.path + '/README.md')}">Harness README</a>
      <a href="{repo_url(h.mission + '/README.md')}">Mission README</a>
    </section>
  </main>
"""
    return page_shell(h.title, body, "../")


def render_technical() -> str:
    family_rows = "\n".join(
        f"""
          <tr>
            <th>{escape(family.name)}</th>
            <td>{escape(family.summary)}</td>
            <td>{len(family.slugs)}</td>
          </tr>
"""
        for family in FAMILIES
    )
    body = f"""
  <main>
    <section class="page-hero">
      <a class="back-link" href="index.html">Back to overview</a>
      <p class="eyebrow">Technical Overview</p>
      <h1>How the coverage workspace is organized.</h1>
      <p class="lede">The repository has two complementary coverage layers: CTest targets for source-level MOOS-IvP component behavior and mission harnesses for live MOOS app, behavior, and motion scenarios.</p>
    </section>

    <section class="content-section compact-section">
      <div class="section-heading">
        <p class="eyebrow">Core Terms</p>
        <h2>Vocabulary for the rest of the site</h2>
      </div>
      <dl class="term-list">
        <div class="term-item">
          <dt>CTest target</dt>
          <dd>A GoogleTest executable registered with CTest. Use it for component behavior that does not need a mission scenario.</dd>
        </div>
        <div class="term-item">
          <dt>CTest family</dt>
          <dd>A workflow selector for one source-level component group, such as <code>geometry</code>, <code>mbutil</code>, or <code>behaviors_marine</code>.</dd>
        </div>
        <div class="term-item">
          <dt>Mission stem</dt>
          <dd>The reusable base mission: MOOS communities, launch files, behavior templates, simulator setup, and map geometry shared by many checks.</dd>
        </div>
        <div class="term-item">
          <dt>Harness case</dt>
          <dd>One scenario layered on top of a stem. A case changes only the inputs needed to test a specific behavior, edge condition, or regression risk.</dd>
        </div>
      </dl>
    </section>

    <section class="technical-grid">
      <article class="technical-card">
        <h2>CTest Coverage</h2>
        <p><code>tests/cpp/</code> holds GoogleTest executables for source-level logic: parsers, geometry, IvP functions, contact records, behavior helpers, app utility classes, and other component contracts.</p>
      </article>
      <article class="technical-card">
        <h2>Mission Harnesses</h2>
        <p>Harnesses run live MOOS missions when the behavior under test depends on process startup, MOOSDB traffic, vehicle motion, mission timing, or interactions between apps and behaviors.</p>
      </article>
      <article class="technical-card">
        <h2>Separate Selectors</h2>
        <p>CTest families select source-level component groups. Harness families and target keys select live mission workloads.</p>
      </article>
    </section>

    <section class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Coverage Layers</p>
        <h2>Why the workspace uses more than one test style</h2>
      </div>
      <div class="explain-stack">
        <article>
          <h3>CTest component checks</h3>
          <p>Fast checks exercise library, app, behavior, and utility logic directly without launching a mission or allocating MOOS ports.</p>
        </article>
        <article>
          <h3>Unit-style mission checks</h3>
          <p>Small stationary or controlled missions verify app outputs directly: detection, absence, report values, alert requests, filters, and lifecycle messages.</p>
        </article>
        <article>
          <h3>Moving integration checks</h3>
          <p>Short corridor missions verify that app outputs lead to correct downstream behavior: safe arrival, avoidance engagement, zero collisions, and expected fail modes.</p>
        </article>
        <article>
          <h3>Behavior correctness checks</h3>
          <p>Behavior-focused missions isolate behavior-owned contracts, such as alert requests, helper flags, lifecycle state, and resolution evidence.</p>
        </article>
        <article>
          <h3>Performance and endurance checks</h3>
          <p>Longer missions apply sustained pressure and add timing, warning-log, and repeatability constraints around mission-owned verdicts.</p>
        </article>
      </div>
    </section>

    <section class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Families</p>
        <h2>Mission harness families</h2>
      </div>
      <div class="table-wrap">
        <table>
          <thead>
            <tr>
              <th>Area</th>
              <th>Role in the pipeline</th>
              <th>Harnesses</th>
            </tr>
          </thead>
          <tbody>
            {family_rows}
          </tbody>
        </table>
      </div>
    </section>

    <section class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Metadata</p>
        <h2>Selectors and invariant checks</h2>
      </div>
      <div class="technical-grid">
        <article class="technical-card">
          <h3>CTest registry labels</h3>
          <p><code>tests/cpp/</code> target definitions use labels for CTest registry metadata and suite-level reporting. The workflow exposes only the top-level family selectors.</p>
        </article>
        <article class="technical-card">
          <h3>CTest invariants</h3>
          <p><code>scripts/check_cpp_tests.py</code> checks that CTest registrations, labels, source references, discovery placeholders, and optional upstream bucket coverage stay consistent.</p>
        </article>
        <article class="technical-card">
          <h3>Harness targets</h3>
          <p><code>config/harness_targets.json</code> maps target keys to harness paths and families so grouped mission validation can select the right harness set.</p>
        </article>
      </div>
    </section>

    <section class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Guide Map</p>
        <h2>Use the right page for the task</h2>
      </div>
      <div class="technical-grid">
        <article class="technical-card">
          <h3>CTest Coverage</h3>
          <p>Review the current CTest target and case snapshot, grouped by MOOS-IvP component family.</p>
          <a class="text-link" href="ctest-coverage.html">Open CTest coverage</a>
        </article>
        <article class="technical-card">
          <h3>Harness Catalog</h3>
          <p>Browse the app and behavior mission harness families, then open individual harness pages for case coverage and source links.</p>
          <a class="text-link" href="index.html#families">Open catalog</a>
        </article>
        <article class="technical-card">
          <h3>Repository README</h3>
          <p>Use the README for repository setup notes, directory context, and links into the source tree.</p>
          <a class="text-link" href="{repo_url('README.md')}">Open README</a>
        </article>
      </div>
    </section>

    <section class="workflow">
      <p class="eyebrow">Source Orientation</p>
      <h2>Where to look in the repository</h2>
      <ol>
        <li><code>tests/cpp/</code> contains the CTest coverage layer.</li>
        <li><code>scripts/check_cpp_tests.py</code> contains the CTest invariant checker.</li>
        <li><code>missions/</code> contains reusable stem missions.</li>
        <li><code>harnesses/</code> contains case overlays, harness wrappers, and per-harness documentation.</li>
        <li><code>config/harness_targets.json</code> contains mission harness target metadata.</li>
        <li><code>docs/tools/build_pages.py</code> generates this website from repository metadata.</li>
      </ol>
      <div class="workflow-actions">
        <a class="text-link" href="{repo_url('scripts/check_cpp_tests.py')}">View the invariant checker</a>
      </div>
    </section>
  </main>
"""
    return page_shell("Technical Overview", body)


def write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    normalized = ""
    if content:
        normalized = "\n".join(line.rstrip() for line in content.splitlines()) + "\n"
    path.write_text(normalized, encoding="utf-8")


def render_gif_manifest() -> str:
    lines = [
        "# GIF Asset Manifest",
        "",
        "Place captured presentation clips in this directory using the filenames below.",
        "Each harness page already references these paths.",
        "",
        "Maintainer note: use the harness wrapper command for capture because it applies",
        "case overlays before delegating to shared `xlaunch.sh`. Calling `xlaunch.sh`",
        "directly is useful inside those wrappers, but it is not the case-selection",
        "entry point for the harness pages.",
        "",
        "Generated visual standard: app-level unit, classification, and variable-heavy motion pages may use 16:9,",
        "map-style explanatory GIFs when a raw `pMarineViewer` capture would not make",
        "the pass condition legible. Keep the look close to the PMV captures: dark chart",
        "background, simple ownship/contact/obstacle geometry, compact labels, and one",
        "small pass-condition callout. The callout should name the MOOS publication being",
        "graded, because that is the evidence the harness is actually testing.",
        "",
        "Generated unit visuals live in `docs/tools/render_unit_harness_gifs.py`.",
        "Generated COLREGS classification visuals live in",
        "`docs/tools/render_colregs_classification_gifs.py`.",
        "Generated COLREGS threshold overlay visuals live in",
        "`docs/tools/render_colregs_threshold_gifs.py`.",
        "Generated COLREGS parameter comparison visuals live in",
        "`docs/tools/render_colregs_parameter_gifs.py`.",
        "Generated PID motion visuals live in `docs/tools/render_pid_motion_gifs.py`.",
        "Headless harness runs remain the source of truth for the case behavior; the",
        "generated GIFs are documentation views of that same geometry and variable-level",
        "evidence.",
        "",
    ]
    for harness in HARNESSES:
        lines.extend(
            [
                f"## {harness.title}",
                "",
                f"Capture from: `{harness.path}`",
                f"Typical capture launch: `{harness.run}`",
                "",
            ]
        )
        for title, case_name, filename in harness.gifs:
            lines.append(f"- `{filename}` - {title}; representative case `{case_name}`")
        lines.append("")
    return "\n".join(lines)


def main() -> None:
    write(ROOT / "index.html", render_index())
    write(ROOT / "ctest-coverage.html", render_ctest_coverage())
    write(ROOT / "user-guide.html", render_user_guide())
    write(ROOT / "technical.html", render_technical())
    for harness in HARNESSES:
        write(ROOT / "harnesses" / f"{harness.slug}.html", render_harness(harness))
    write(ROOT / "assets" / "gifs" / "README.md", render_gif_manifest())
    write(ROOT / ".nojekyll", "")


if __name__ == "__main__":
    main()
