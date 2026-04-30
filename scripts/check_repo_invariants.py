#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


REPO_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO_ROOT))

from scripts import check_cpp_tests, ci_cpp_test_targets, ci_harness_case_count, harness_targets  # noqa: E402

WORKFLOW_PATH = REPO_ROOT / ".github" / "workflows" / "build_extend.yml"
DOCS_BUILD_PATH = REPO_ROOT / "docs" / "tools" / "build_pages.py"
EXPECTED_DISPATCH_MODES = {
    "none",
    "family_run",
    "batch_family_run",
    "specific_harnesses",
}
EXPECTED_CPP_TEST_MODES = {
    "none",
    "family_run",
    "batch_family_run",
}


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


def workflow_choice_block(input_name: str, workflow_text: str) -> list[str]:
    lines = workflow_text.splitlines()
    input_indent = None
    in_input = False
    in_options = False
    choices: list[str] = []

    for line in lines:
        stripped = line.strip()
        indent = len(line) - len(line.lstrip(" "))

        if stripped == f"{input_name}:":
            input_indent = indent
            in_input = True
            in_options = False
            continue

        if not in_input:
            continue

        if input_indent is not None and indent <= input_indent and stripped.endswith(":"):
            break

        if stripped == "options:":
            in_options = True
            continue

        if in_options:
            if stripped.startswith("- "):
                choices.append(stripped[2:].strip().strip('"'))
            elif stripped and not stripped.startswith("#"):
                in_options = False

    return choices


def workflow_default(input_name: str, workflow_text: str) -> str:
    lines = workflow_text.splitlines()
    input_indent = None
    in_input = False

    for line in lines:
        stripped = line.strip()
        indent = len(line) - len(line.lstrip(" "))

        if stripped == f"{input_name}:":
            input_indent = indent
            in_input = True
            continue

        if not in_input:
            continue

        if input_indent is not None and indent <= input_indent and stripped.endswith(":"):
            break

        if stripped.startswith("default:"):
            return stripped.split(":", 1)[1].strip().strip('"')

    return ""


def comma_list(raw: str) -> list[str]:
    return [item.strip() for item in raw.split(",") if item.strip()]


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

        try:
            case_count = ci_harness_case_count.derive_case_count(zlaunch_path)
        except ValueError as err:
            failures.append(CheckFailure(key, str(err)))
            continue

        if case_count <= 0:
            failures.append(CheckFailure(key, "default case count must be greater than zero"))

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


def check_workflow_inputs(targets: list[dict[str, object]]) -> list[CheckFailure]:
    workflow_text = WORKFLOW_PATH.read_text(encoding="utf-8")
    target_keys = {str(target["key"]) for target in targets}
    target_families = {
        str(family)
        for target in targets
        for family in target["families"]
    }
    failures: list[CheckFailure] = []

    dispatch_modes = set(workflow_choice_block("dispatch_mode", workflow_text))
    if dispatch_modes != EXPECTED_DISPATCH_MODES:
        failures.append(
            CheckFailure(
                "workflow dispatch modes",
                "expected "
                + ", ".join(sorted(EXPECTED_DISPATCH_MODES))
                + "; found "
                + ", ".join(sorted(dispatch_modes)),
            )
        )

    cpp_test_modes = set(workflow_choice_block("cpp_test_mode", workflow_text))
    if cpp_test_modes != EXPECTED_CPP_TEST_MODES:
        failures.append(
            CheckFailure(
                "workflow C++ test modes",
                "expected "
                + ", ".join(sorted(EXPECTED_CPP_TEST_MODES))
                + "; found "
                + ", ".join(sorted(cpp_test_modes)),
            )
        )

    cpp_test_families = set(workflow_choice_block("cpp_test_family", workflow_text))
    expected_cpp_test_families = set(ci_cpp_test_targets.CPP_FAMILIES)
    if cpp_test_families != expected_cpp_test_families:
        missing = sorted(expected_cpp_test_families - cpp_test_families)
        extra = sorted(cpp_test_families - expected_cpp_test_families)
        detail_parts = []
        if missing:
            detail_parts.append("missing: " + ", ".join(missing))
        if extra:
            detail_parts.append("extra: " + ", ".join(extra))
        failures.append(CheckFailure("workflow C++ test family choices", "; ".join(detail_parts)))

    workflow_families = set(workflow_choice_block("family", workflow_text))
    if workflow_families != target_families:
        missing = sorted(target_families - workflow_families)
        extra = sorted(workflow_families - target_families)
        detail_parts = []
        if missing:
            detail_parts.append("missing: " + ", ".join(missing))
        if extra:
            detail_parts.append("extra: " + ", ".join(extra))
        failures.append(CheckFailure("workflow family choices", "; ".join(detail_parts)))

    default_family = workflow_default("family", workflow_text)
    if default_family and default_family not in target_families:
        failures.append(CheckFailure("workflow family default", f"unknown family: {default_family}"))

    default_targets = comma_list(workflow_default("targets", workflow_text))
    unknown_targets = sorted(set(default_targets) - target_keys)
    if unknown_targets:
        failures.append(
            CheckFailure("workflow targets default", "unknown target(s): " + ", ".join(unknown_targets))
        )

    default_families = comma_list(workflow_default("families", workflow_text))
    unknown_families = sorted(set(default_families) - target_families)
    if unknown_families:
        failures.append(
            CheckFailure("workflow families default", "unknown family/families: " + ", ".join(unknown_families))
        )

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


def check_cpp_test_tree(_targets: list[dict[str, object]]) -> list[CheckFailure]:
    return [
        CheckFailure(failure.label, failure.detail)
        for failure in check_cpp_tests.check_static()
    ]


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
        check_docs_catalog,
        check_workflow_inputs,
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
