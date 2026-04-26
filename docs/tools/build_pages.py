#!/usr/bin/env python3
"""Generate the static GitHub Pages site for the CI/CD harness catalog."""

from __future__ import annotations

import re
from dataclasses import dataclass
from html import escape
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPO_ROOT = Path(__file__).resolve().parents[2]
HARNESS_DIR = REPO_ROOT / "harnesses"
REPO_BLOB = "https://github.com/cbenjamin23/moos-ivp-cicd-testing/blob/main"
CASE_LIST_RE = re.compile(r'^\s*([A-Z0-9_]*CASES)\s*=\s*(["\'])(.*)\2\s*$')


@dataclass(frozen=True)
class Harness:
    slug: str
    title: str
    family: str
    path: str
    mission: str
    summary: str
    proof: str
    cases: tuple[str, ...]
    gifs: tuple[tuple[str, str, str], tuple[str, str, str]]
    run: str
    notes: tuple[str, ...]


@dataclass(frozen=True)
class Family:
    name: str
    label: str
    summary: str
    slugs: tuple[str, ...]


HARNESSES: tuple[Harness, ...] = (
    Harness(
        slug="cmgr-unit",
        title="H01 Contact Manager Unit",
        family="Contact Manager",
        path="harnesses/cmgr_harnesses/H01-cmgr_unit",
        mission="missions/cmgr_missions/cmgr_unit",
        summary="Contact-manager mode checks for detection, absence, alerts, reports, filters, and multi-contact outputs.",
        proof="Directly checks detection, absence, report, alert filter, runtime request, closest-contact, and multi-contact publication paths.",
        cases=(
            "detect_baseline_pass",
            "detect_cpa_only_pass",
            "runtime_alert_add_pass",
            "filter_match_type_absent_pass",
            "closest_contact_pass",
            "count_two_pass",
        ),
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
        cases=(
            "baseline_crossing_pass",
            "head_on_pass",
            "two_contact_pass",
            "avoid_disabled_fail",
            "fast_intruder_fail",
        ),
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
        cases=(
            "given_baseline_pass",
            "given_general_alert_pass",
            "points_cluster_dist_pass",
            "lasso_cluster_pass",
            "enable_obstacle_pass",
            "expunge_obstacle_pass",
        ),
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
        cases=(
            "baseline_center_pass",
            "offset_clear_pass",
            "tight_alert_pass",
            "two_sequential_fail",
            "wide_center_fail",
            "avoid_disabled_fail",
        ),
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
        slug="colregs-classification",
        title="H01 COLREGS Classification",
        family="COLREGS",
        path="harnesses/colregs_harnesses/H01-colregs_classification",
        mission="missions/colregs_missions/colregs_unit",
        summary="Canonical two-vessel COLREGS classification suite for BHV_AvdColregsV22.",
        proof="Checks expected mode/index selection across head-on, crossing, overtaking, and overtaken encounter geometry.",
        cases=(
            "head_on_colregs_pass",
            "crossing_starboard_giveway_pass",
            "crossing_port_standon_pass",
            "overtaking_starboard_pass",
            "overtaken_port_standon_pass",
        ),
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
        cases=(
            "headon threshold group",
            "giveway threshold group",
            "standon threshold group",
            "overtaking threshold group",
            "overtaken threshold group",
        ),
        gifs=(
            ("Below/Edge/Above Triplet", "group=headon", "colregs-thresholds-headon-triplet.gif"),
            ("Mirrored Boundary", "group=standon", "colregs-thresholds-mirror-boundary.gif"),
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
        cases=(
            "head_on_execution_pass",
            "crossing_starboard_giveway_execution_pass",
            "crossing_port_standon_execution_pass",
            "overtaking_starboard_execution_pass",
            "overtaken_starboard_standon_midrange_execution_pass",
        ),
        gifs=(
            ("Head-on Resolution", "head_on_execution_pass", "colregs-execution-head-on.gif"),
            ("Overtaken Midrange", "overtaken_starboard_standon_midrange_execution_pass", "colregs-execution-overtaken-midrange.gif"),
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
        cases=(
            "min_util_cpa_dist",
            "headon_only",
            "giveway_bow_dist",
            "velocity_filter",
            "turn_radius",
            "use_refinery",
        ),
        gifs=(
            ("Default vs High CPA", "min_util_cpa_default/high", "colregs-parameters-cpa-compare.gif"),
            ("Head-on Only Toggle", "headon_only_true/false", "colregs-parameters-headon-only.gif"),
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
        cases=(
            "default_resolve_pass",
            "post_per_contact_info_pass",
            "behavior_filter_absent_pass",
            "head_on_resolve_pass",
            "no_alert_request_fail",
        ),
        gifs=(
            ("Head-on Resolve", "head_on_resolve_pass", "collision-behavior-head-on-resolve.gif"),
            ("No Alert Request Fail", "no_alert_request_fail", "collision-behavior-no-alert-fail.gif"),
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
        cases=(
            "default_auto_request_pass",
            "rng_flag_pass",
            "cpa_flag_pass",
            "offlane_no_engagement_pass",
            "two_obstacles_clean_pass",
            "avoid_disabled_fail",
        ),
        gifs=(
            ("Two Obstacles Clean", "two_obstacles_clean_pass", "obstacle-behavior-two-obstacles.gif"),
            ("Avoid Disabled Fail", "avoid_disabled_fail", "obstacle-behavior-avoid-disabled.gif"),
        ),
        run="./zlaunch.sh --case=two_obstacles_clean_pass --gui 10",
        notes=(
            "This harness isolates the avoid-obstacle behavior contract while keeping obstacle-manager input deterministic.",
            "Helper-flag cases check behavior-specific observability in addition to mission outcome.",
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
        cases=(
            "baseline_field_pass",
            "dense_field_pass",
            "endurance_random_pass",
        ),
        gifs=(
            ("Baseline Field", "baseline_field_pass", "performance-obstacle-gauntlet-baseline.gif"),
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
        cases=(
            "baseline_colregs_pass",
            "dense_colregs_pass",
            "endurance_colregs_pass",
        ),
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
        cases=(
            "baseline_circle_pass",
            "mixed_speed_circle_pass",
            "endurance_circle_pass",
        ),
        gifs=(
            ("Baseline Traffic Ring", "baseline_circle_pass", "performance-traffic-ring-baseline.gif"),
            ("Mixed-Speed Traffic Ring", "mixed_speed_circle_pass", "performance-traffic-ring-mixed-speed.gif"),
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
)


TEST_STYLE: dict[str, str] = {
    "cmgr-unit": "Simple correctness tests for pContactMgrV20 output behavior. These cases keep the mission controlled and verify that contact alerts, reports, filters, runtime requests, and multi-contact selections appear only when expected.",
    "cmgr-motion": "Moving correctness tests for contact-management evidence. These cases check whether contact-manager alerts arrive early enough for downstream avoidance to produce a safe transit, while expected-fail cases prove the gate catches missing or late avoidance evidence.",
    "obmgr-unit": "Simple correctness tests for pObstacleMgr output behavior. These cases keep vehicle dynamics minimal and verify accepted obstacles, rejected obstacles, alert messages, point-cluster hulls, distance reports, and obstacle lifecycle messages.",
    "obmgr-motion": "Moving correctness tests for obstacle-manager alert output. These cases check whether obstacle alerts are actionable enough to support clean obstacle-avoidance transit, and whether intentionally bad setups fail for the expected reason.",
    "colregs-classification": "Classification correctness tests for BHV_AvdColregsV22. These cases verify that canonical two-vessel geometries enter the expected COLREGS mode before execution quality is evaluated elsewhere.",
    "colregs-thresholds": "Boundary correctness tests for COLREGS classification. These cases probe below, edge, above, and mirrored geometries to catch regressions near rule-transition thresholds.",
    "colregs-execution": "Execution correctness tests for BHV_AvdColregsV22 after classification is known. These cases run encounters through completion and grade the realized maneuver using CPA, side outcome, timing, and collision evidence.",
    "colregs-parameters": "Parameter-regression tests for BHV_AvdColregsV22. These cases vary one tuning knob at a time while holding trusted geometry steady, verifying expected stability or one deliberate behavioral change.",
    "collision-behavior-motion": "Behavior correctness tests for BHV_AvoidCollision. These cases isolate the behavior contract: alert requests, contact information, lifecycle evidence, per-contact outputs, resolution signals, and collision-free transit.",
    "obstacle-behavior-motion": "Behavior correctness tests for BHV_AvoidObstacleV24. These cases isolate the behavior-owned contract for obstacle alert requests, helper flags, lifecycle evidence, resolution signals, and clean transit outcomes.",
    "performance-obstacle-gauntlet": "Performance and endurance tests for obstacle avoidance. These cases exercise repeated obstacle-field traversal and grade completion, collision safety, warning cleanliness, and wall-time behavior.",
    "performance-colregs-joust": "Performance tests for sustained COLREGS pressure. These cases run repeated multi-vehicle interaction and grade safe spacing, completion, warning cleanliness, and runtime bounds.",
    "performance-traffic-ring": "Seeded traffic-stress tests for COLREGS behavior. These cases replay five-vehicle traffic pressure and grade assignment activity, fixed runtime windows, zero collisions, and warning-free logs.",
}


STEM_CONTEXT: dict[str, str] = {
    "cmgr-unit": "The stem mission is a small single-ownship setup in the lower-band Forrest 19 / MIT_SP frame. `alpha` is the ownship; contacts such as `bravo` or `intruder` are synthetic targets introduced by individual cases so pContactMgrV20 can be checked without a full behavior stack.",
    "cmgr-motion": "The stem mission places `alpha` on a short transit with one or more synthetic contacts. The harness keeps the geometry compact and grades whether contact-manager output gives the avoidance behavior enough evidence to arrive safely.",
    "obmgr-unit": "The stem mission keeps the ownship context simple and introduces given obstacles or point clusters through the case inputs. That isolates pObstacleMgr message behavior before any moving obstacle-avoidance outcome is judged.",
    "obmgr-motion": "The stem mission sends one ownship through a short corridor with case-specific obstacle layouts. The harness grades whether obstacle-manager alerts support arrival without collisions, near misses, or unresolved encounters.",
    "colregs-classification": "The shared COLREGS stem mission uses two vessels, `ben` and `abe`; `ben` is the evaluated ownship. Case overlays vary the encounter geometry for head-on, crossing, overtaking, and overtaken situations.",
    "colregs-thresholds": "The shared COLREGS stem mission uses two vessels, `ben` and `abe`; `ben` is the evaluated ownship. Threshold cases reuse the same stem while moving the geometry just below, at, or above classification boundaries.",
    "colregs-execution": "The shared COLREGS stem mission uses two vessels, `ben` and `abe`; `ben` is the evaluated ownship. Execution cases reuse trusted classification geometry and then judge the maneuver that actually unfolds.",
    "colregs-parameters": "The shared COLREGS stem mission uses two vessels, `ben` and `abe`; `ben` is the evaluated ownship. Parameter cases keep the encounter setup stable while changing one behavior setting at a time.",
    "collision-behavior-motion": "The stem mission puts one ownship in a compact moving encounter with a synthetic contact. Unlike the app-level contact-manager harnesses, this stem focuses on what BHV_AvoidCollision requests, publishes, and resolves.",
    "obstacle-behavior-motion": "The stem mission runs one ownship through deterministic obstacle layouts with obstacle-manager input held stable. The cases focus on what BHV_AvoidObstacleV24 requests, flags, and resolves.",
    "performance-obstacle-gauntlet": "The stem mission is a repeatable obstacle-field loop. Individual cases change field density or duration so the same avoidance stack can be tested under baseline, dense, and longer-running pressure.",
    "performance-colregs-joust": "The stem mission creates sustained two- or three-vehicle COLREGS encounters. Cases adjust traffic density and duration to test whether safe spacing holds beyond a single isolated encounter.",
    "performance-traffic-ring": "The stem mission keeps five vehicles moving through a seeded traffic-ring assignment pattern. The seed makes the pressure replayable enough for CI while still exercising repeated COLREGS interactions.",
}


def repo_url(path: str) -> str:
    return f"{REPO_BLOB}/{path}"


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
  <link rel="stylesheet" href="{prefix}styles.css">
</head>
<body>
  <header class="site-header">
    <a class="brand" href="{prefix}index.html">
      <span class="brand-mark">CI</span>
      <span>MOOS-IvP Harnesses</span>
    </a>
    <nav>
      <a href="{prefix}index.html#families">Harnesses</a>
      <a href="{prefix}index.html#workflow">How it works</a>
      <a href="{prefix}technical.html">Technical</a>
    </nav>
  </header>
  {body}
  <footer class="site-footer">
    <span>Overview site for the MOOS-IvP CI/CD test workspace.</span>
    <span>Source mission and harness links are included for readers who want implementation detail.</span>
  </footer>
</body>
</html>
"""


def case_chips(cases: tuple[str, ...]) -> str:
    return "\n".join(f"<li><code>{escape(case)}</code></li>" for case in cases)


def humanize_case(case_name: str) -> str:
    clean = case_name.replace("_", " ").replace("-", " ")
    clean = re.sub(r"\bpass\b", "expected pass", clean)
    clean = re.sub(r"\bfail\b", "expected fail", clean)
    clean = re.sub(r"\babsent\b", "absence check", clean)
    return f"Covers {clean}."


def normalize_description(text: str) -> str:
    text = text.replace("`", "")
    text = re.sub(r"\bpatched mission\b", "case", text, flags=re.IGNORECASE)
    text = re.sub(r"\bpatched\b", "case-specific", text, flags=re.IGNORECASE)
    text = re.sub(r"\s+", " ", text).strip()
    text = text.strip(" :-")
    if not text:
        return ""
    if len(text) > 165:
        text = text[:162].rsplit(" ", 1)[0] + "..."
    return text


def extract_case_docs(h: Harness) -> list[tuple[str, str]]:
    readme = ROOT.parent / h.path / "README.md"
    if not readme.exists():
        return [(case, humanize_case(case)) for case in h.cases]

    lines = readme.read_text(encoding="utf-8", errors="ignore").splitlines()
    found: list[tuple[str, str]] = []
    seen: set[str] = set()
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

        desc = normalize_description(" ".join(parts)) or humanize_case(case_name)
        found.append((case_name, desc))
        seen.add(case_name)

    if not found:
        found = [(case, humanize_case(case)) for case in h.cases]
    return found


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
    description = descriptions.get(case_name, humanize_case(case_name))
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
        seen: set[str] = set()
        for line in zlaunch.read_text().splitlines():
            match = CASE_LIST_RE.match(line)
            if not match:
                continue
            value = match.group(3)
            if "$" in value:
                continue
            seen.update(value.split())
        counts[zlaunch.parent.name] = len(seen)
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
        (str(total_case_count()), "Cases", "Harness case matrices across the catalog."),
        (str(harness_count()), "Harnesses", "Case matrices covering app-level units, motion checks, behavior checks, and performance gates."),
        (str(app_behavior_target_count()), "Apps and Behaviors", "Ten MOOS apps and IvP behavior targets, plus their related stress missions."),
        ("3", "Evidence Layers", "Simple mode validation, moving mission outcomes, and longer stress/endurance runs."),
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


def render_index() -> str:
    harness_by_slug = {h.slug: h for h in HARNESSES}
    family_sections = "\n".join(family_panel(family, harness_by_slug) for family in FAMILIES)

    body = f"""
  <main>
    <section class="hero">
      <div class="hero-copy">
        <p class="eyebrow">MOOS-IvP CI/CD Pipeline</p>
        <h1>Regression harnesses for MOOS-IvP Apps and Behaviors.</h1>
        <p class="lede">This site introduces a CI/CD workspace that packages MOOS-IvP missions into repeatable gates for contact management, obstacle management, COLREGS, and avoidance behavior changes.</p>
        <div class="hero-actions">
          <a class="button primary" href="#families">Browse catalog</a>
          <a class="button secondary" href="technical.html">Technical overview</a>
        </div>
      </div>
      <div class="hero-panel">
        <div class="mission-visual" aria-hidden="true">
          <svg class="trail-svg" viewBox="0 0 360 560" role="img">
            <path class="trail trail-a" d="M48 455 L302 112" />
            <path class="trail trail-b" d="M42 132 L318 250" />
            <path class="trail trail-c" d="M90 520 L260 316" />
          </svg>
          <div class="boat boat-a"><span></span></div>
          <div class="boat boat-b"><span></span></div>
          <div class="boat boat-c"><span></span></div>
        </div>
        <div class="hero-process">
          <h2>Process</h2>
          <p>Stem missions define the baseline MOOS community and behavior setup. Harness cases apply small overlays, run through the harness launch wrapper, and grade MOOS-visible evidence from the resulting mission.</p>
        </div>
      </div>
    </section>

    <section class="content-section stats-section">
      <div class="section-heading">
        <p class="eyebrow">Repository Snapshot</p>
        <h2>Current pipeline at a glance</h2>
      </div>
      <div class="stats-grid">
        {render_stats()}
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
      <p class="eyebrow">How It Works</p>
      <h2>From scenario to CI verdict</h2>
      <ol>
        <li>Stem missions define reusable MOOS community layouts, behavior configs, and baseline geometry.</li>
        <li>Harness overlays use small patches so each run isolates one scenario or regression risk.</li>
        <li>The harness <code>zlaunch.sh</code> wrapper selects the case and then delegates to the shared <code>xlaunch.sh</code> path, keeping local and CI runs aligned.</li>
        <li><code>pMissionEval</code> reports and shell-side checks reduce the run to compact CI result lines.</li>
      </ol>
      <a class="text-link" href="technical.html">Read the in-depth technical explanation</a>
    </section>
  </main>
"""
    return page_shell("Home", body)


def render_harness(h: Harness) -> str:
    descriptions = dict(extract_case_docs(h))
    gifs = "\n".join(gif_card(g, descriptions, "../") for g in h.gifs)
    test_style = TEST_STYLE.get(h.slug, h.proof)
    stem_context = STEM_CONTEXT.get(h.slug, "")
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

    <section class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Examples</p>
        <h2>Representative runs</h2>
      </div>
      <div class="gif-grid">{gifs}
      </div>
    </section>

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
      <h1>How the CI/CD pipeline is organized.</h1>
      <p class="lede">The repository treats MOOS-IvP stem missions as executable regression tests. It keeps reusable mission stems separate from case overlays, then runs those cases in CI with a shared launch and grading pattern.</p>
    </section>

    <section class="technical-grid">
      <article class="technical-card">
        <h2>Core Idea</h2>
        <p>A mission stem defines the common world: MOOS communities, simulator apps, behavior files, launch scripts, and baseline geometry. Harness cases apply small patches over that stem so one scenario can change without copying the whole mission.</p>
      </article>
      <article class="technical-card">
        <h2>Launch Model</h2>
        <p>Harness <code>zlaunch.sh</code> wrappers own case selection, patch application, result collection, and port isolation. They delegate the actual mission startup to shared <code>xlaunch.sh</code>, which keeps local and CI launch behavior aligned.</p>
      </article>
      <article class="technical-card">
        <h2>Verdict Model</h2>
        <p><code>pMissionEval</code> owns the mission-visible verdict where possible. Shell-side checks add constraints that are easier to evaluate outside MOOS, such as wall-time ranges, result completeness, and warning-log scans.</p>
      </article>
    </section>

    <section class="content-section">
      <div class="section-heading">
        <p class="eyebrow">Pipeline Layers</p>
        <h2>Why there are multiple harness types</h2>
      </div>
      <div class="explain-stack">
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
        <h2>Apps and behaviors covered</h2>
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
        <p class="eyebrow">CI Execution</p>
        <h2>How GitHub Actions uses the repo</h2>
      </div>
      <div class="technical-grid">
        <article class="technical-card">
          <h3>Build</h3>
          <p>The workflow checks out this repo, builds MOOS-IvP and the local workspace apps, then uploads the built tools as an artifact for later jobs.</p>
        </article>
        <article class="technical-card">
          <h3>Make Checks</h3>
          <p>Selected active harnesses run in <code>--just_make</code> mode to verify patched mission generation before the heavier runtime gates execute.</p>
        </article>
        <article class="technical-card">
          <h3>Runtime Gates</h3>
          <p>Correctness and performance jobs run full harnesses, upload result files, summarize the result lines, and fail the workflow if the expected matrix does not complete cleanly.</p>
        </article>
      </div>
    </section>

    <section class="workflow">
      <p class="eyebrow">Source Orientation</p>
      <h2>Where to look in the repository</h2>
      <ol>
        <li><code>missions/</code> contains reusable stem missions.</li>
        <li><code>harnesses/</code> contains case overlays, harness wrappers, and per-harness documentation.</li>
        <li><code>scripts/</code> contains helper scripts for summaries, sweeps, and benchmarking.</li>
        <li><code>.github/workflows/</code> contains the CI and Pages workflows.</li>
      </ol>
      <a class="text-link" href="{repo_url('README.md')}">Open the repository README</a>
    </section>
  </main>
"""
    return page_shell("Technical Overview", body)


def write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


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
    write(ROOT / "technical.html", render_technical())
    for harness in HARNESSES:
        write(ROOT / "harnesses" / f"{harness.slug}.html", render_harness(harness))
    write(ROOT / "assets" / "gifs" / "README.md", render_gif_manifest())
    write(ROOT / ".nojekyll", "")


if __name__ == "__main__":
    main()
