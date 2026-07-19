#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


REPO_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO_ROOT))

from scripts import check_cpp_tests, ci_cpp_test_targets, ci_harness_case_count, generate_context_graph, harness_targets  # noqa: E402

DOCS_BUILD_PATH = REPO_ROOT / "docs" / "tools" / "build_pages.py"
HARNESS_LOCK_MARKERS = (
    "LOCK_DIR=",
    "HAVE_LOCK",
    ".harness_runs.lock",
    ".performance_harness.lock",
    "another harness run appears active",
    "another performance harness is already active",
)


@dataclass(frozen=True)
class CheckFailure:
    label: str
    detail: str

    def format(self) -> str:
        return f"{self.label}: {self.detail}"


def load_docs_harness_paths() -> set[str]:
    module_name = "docs_build_pages"
    spec = importlib.util.spec_from_file_location(module_name, DOCS_BUILD_PATH)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load {DOCS_BUILD_PATH}")

    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    spec.loader.exec_module(module)
    return {str(harness.path) for harness in module.HARNESSES}


def harness_gui_contract_issues(zlaunch_text: str) -> list[str]:
    if "--gui)" not in zlaunch_text:
        return []

    issues: list[str] = []
    if "--gui) DISPLAY_ARGS=(--gui) ;;" not in zlaunch_text:
        issues.append("--gui must forward an explicit --gui argument to the stem mission")
    if '"${DISPLAY_ARGS[0]}" = --gui' not in zlaunch_text:
        issues.append("--gui must require one explicit --case")
    return issues


def mission_launch_args_contract_issues(zlaunch_text: str) -> list[str]:
    if "set -u" not in zlaunch_text:
        return []

    assignment = re.search(
        r"(?ms)^\s*launch_args=\((.*?)\)\s*$",
        zlaunch_text,
    )
    if assignment is None:
        return []
    if re.search(r'"\$\{[A-Za-z_][A-Za-z0-9_]*\[@\]\}"', assignment.group(1)):
        return [
            "launch_args must append optional arrays only after a nonempty length guard "
            "for Bash 3.2 set -u compatibility"
        ]
    return []


def harness_lock_contract_issues(shell_text: str) -> list[str]:
    return [marker for marker in HARNESS_LOCK_MARKERS if marker in shell_text]


def check_harness_targets(targets: list[dict[str, object]]) -> list[CheckFailure]:
    failures: list[CheckFailure] = []

    try:
        harness_targets.validate_targets(targets)
    except ValueError as err:
        failures.append(CheckFailure("target metadata", str(err)))
        return failures

    for target in targets:
        key = str(target["key"])
        harness_path = REPO_ROOT / str(target["path"])
        zlaunch_path = harness_path / "zlaunch.sh"
        readme_path = harness_path / "README.md"

        if not readme_path.is_file():
            failures.append(CheckFailure(key, f"missing README.md at {readme_path}"))
        if not zlaunch_path.is_file():
            failures.append(CheckFailure(key, f"missing zlaunch.sh at {zlaunch_path}"))
            continue
        if not zlaunch_path.stat().st_mode & 0o111:
            failures.append(CheckFailure(key, f"zlaunch.sh is not executable: {zlaunch_path}"))

        zlaunch_text = zlaunch_path.read_text(encoding="utf-8", errors="replace")
        for issue in harness_gui_contract_issues(zlaunch_text):
            failures.append(CheckFailure(key, issue))

        try:
            case_count = ci_harness_case_count.derive_case_count(zlaunch_path)
        except ValueError as err:
            failures.append(CheckFailure(key, str(err)))
            continue

        if case_count <= 0:
            failures.append(CheckFailure(key, "default case count must be greater than zero"))

    return failures


def check_mission_wrapper_contracts(_targets: list[dict[str, object]]) -> list[CheckFailure]:
    failures: list[CheckFailure] = []
    mission_root = REPO_ROOT / "missions"
    for zlaunch_path in sorted(mission_root.glob("*_missions/*/zlaunch.sh")):
        zlaunch_text = zlaunch_path.read_text(encoding="utf-8", errors="replace")
        label = zlaunch_path.relative_to(REPO_ROOT).as_posix()
        for issue in mission_launch_args_contract_issues(zlaunch_text):
            failures.append(CheckFailure(label, issue))
    return failures


def check_harness_shell_contracts(_targets: list[dict[str, object]]) -> list[CheckFailure]:
    failures: list[CheckFailure] = []
    harness_root = REPO_ROOT / "harnesses"
    for shell_path in sorted(harness_root.rglob("*.sh")):
        shell_text = shell_path.read_text(encoding="utf-8", errors="replace")
        markers = harness_lock_contract_issues(shell_text)
        if markers:
            label = shell_path.relative_to(REPO_ROOT).as_posix()
            failures.append(
                CheckFailure(
                    label,
                    "harness-wide filesystem lock markers are forbidden: "
                    + ", ".join(markers),
                )
            )
    return failures


def check_docs_catalog(targets: list[dict[str, object]]) -> list[CheckFailure]:
    target_paths = {str(target["path"]) for target in targets}
    docs_paths = load_docs_harness_paths()
    failures: list[CheckFailure] = []

    missing_from_docs = sorted(target_paths - docs_paths)
    missing_from_targets = sorted(docs_paths - target_paths)

    for path in missing_from_docs:
        failures.append(CheckFailure("docs catalog", f"registered harness missing from docs: {path}"))
    for path in missing_from_targets:
        failures.append(CheckFailure("docs catalog", f"docs harness missing from target registry: {path}"))

    return failures


def check_docs_generated_clean() -> list[CheckFailure]:
    before = snapshot_tree(REPO_ROOT / "docs")

    subprocess.run(
        [sys.executable, str(DOCS_BUILD_PATH)],
        cwd=REPO_ROOT,
        check=True,
    )

    after = snapshot_tree(REPO_ROOT / "docs")
    changed = sorted(path for path in set(before) | set(after) if before.get(path) != after.get(path))
    if changed:
        return [
            CheckFailure(
                "docs generated output",
                "docs/tools/build_pages.py changed docs: " + ", ".join(changed[:8]),
            )
        ]
    return []


def check_context_graph_generated_clean(_targets: list[dict[str, object]]) -> list[CheckFailure]:
    try:
        graph = generate_context_graph.build_graph()
    except Exception as err:  # noqa: BLE001 - report graph generation failures as invariant failures.
        return [CheckFailure("context graph generation", str(err))]

    expected = {
        generate_context_graph.DEFAULT_JSON_PATH: json.dumps(graph, indent=2, sort_keys=True) + "\n",
        generate_context_graph.DEFAULT_MD_PATH: generate_context_graph.render_markdown(graph),
    }
    failures: list[CheckFailure] = []
    for path, content in expected.items():
        if not path.is_file():
            failures.append(CheckFailure("context graph output", f"missing generated file: {path}"))
            continue
        if path.read_text(encoding="utf-8") != content:
            failures.append(CheckFailure("context graph output", f"out of date: {path}"))
    return failures


def check_cpp_test_tree(_targets: list[dict[str, object]]) -> list[CheckFailure]:
    failures = [
        CheckFailure(failure.label, failure.detail)
        for failure in check_cpp_tests.check_static()
    ]
    for issue in ci_cpp_test_targets.family_name_issues(
        list(ci_cpp_test_targets.CPP_FAMILIES)
    ):
        failures.append(CheckFailure("CTest family selector naming", issue))
    declared_labels = check_cpp_tests.declared_labels()
    for family in sorted(set(ci_cpp_test_targets.CPP_FAMILIES) - declared_labels):
        failures.append(
            CheckFailure(
                "CTest family selector",
                f"{family} is not declared by CMake LABELS or SUITE_LABELS",
            )
        )
    return failures


def snapshot_tree(root: Path) -> dict[str, str]:
    snapshot: dict[str, str] = {}
    for path in sorted(item for item in root.rglob("*") if item.is_file()):
        rel = path.relative_to(REPO_ROOT).as_posix()
        snapshot[rel] = hashlib.sha256(path.read_bytes()).hexdigest()
    return snapshot


def print_failures(failures: Iterable[CheckFailure]) -> None:
    for failure in failures:
        print(failure.format(), file=sys.stderr)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Check static repository invariants.")
    parser.add_argument(
        "--skip-docs-diff",
        action="store_true",
        help="Skip regenerating docs and checking for a dirty docs diff.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    failures: list[CheckFailure] = []

    try:
        targets = harness_targets.load_targets()
    except (OSError, ValueError) as err:
        print(CheckFailure("target metadata", str(err)).format(), file=sys.stderr)
        return 1

    check_steps = [
        check_harness_targets,
        check_harness_shell_contracts,
        check_mission_wrapper_contracts,
        check_docs_catalog,
        check_context_graph_generated_clean,
        check_cpp_test_tree,
    ]

    for check in check_steps:
        try:
            failures.extend(check(targets))
        except Exception as err:  # noqa: BLE001 - report all invariant checker failures uniformly.
            failures.append(CheckFailure(check.__name__, str(err)))

    if not args.skip_docs_diff:
        try:
            failures.extend(check_docs_generated_clean())
        except subprocess.CalledProcessError as err:
            failures.append(CheckFailure("docs generation", f"command failed with exit {err.returncode}"))

    if failures:
        print_failures(failures)
        return 1

    print(f"Repository invariants passed for {len(targets)} harness target(s).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
