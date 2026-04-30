#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
CPP_TEST_ROOT = REPO_ROOT / "tests" / "cpp"
ROOT_CMAKE = CPP_TEST_ROOT / "CMakeLists.txt"

SECTION_NAMES = {
    "SOURCES",
    "LIBS",
    "LABELS",
    "SUITE_LABELS",
    "INCLUDES",
    "DEFINES",
    "ENVIRONMENT",
    "WORKING_DIRECTORY",
    "DISCOVERY_MODE",
}


@dataclass(frozen=True)
class CheckFailure:
    label: str
    detail: str

    def format(self) -> str:
        return f"{self.label}: {self.detail}"


@dataclass(frozen=True)
class CppTestTarget:
    file: Path
    target: str
    sections: dict[str, list[str]]

    def values(self, section: str) -> list[str]:
        return self.sections.get(section, [])


def strip_line_comment(line: str) -> str:
    in_bracket = False
    i = 0
    while i < len(line):
        if line.startswith("[=[", i):
            in_bracket = True
            i += 3
            continue
        if in_bracket and line.startswith("]=]", i):
            in_bracket = False
            i += 3
            continue
        if line[i] == "#" and not in_bracket:
            return line[:i]
        i += 1
    return line


def extract_call(text: str, start: int) -> tuple[str, int]:
    open_index = text.find("(", start)
    if open_index == -1:
        raise ValueError("missing opening parenthesis")

    depth = 0
    for idx in range(open_index, len(text)):
        char = text[idx]
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                return text[open_index + 1 : idx], idx + 1

    raise ValueError("missing closing parenthesis")


def parse_add_moos_cpp_tests(cmake_path: Path) -> list[CppTestTarget]:
    text = cmake_path.read_text(encoding="utf-8")
    targets: list[CppTestTarget] = []
    offset = 0

    while True:
        match = re.search(r"\badd_moos_cpp_test\s*\(", text[offset:])
        if match is None:
            return targets

        call_start = offset + match.start()
        body, call_end = extract_call(text, call_start)
        tokens: list[str] = []
        for raw_line in body.splitlines():
            line = strip_line_comment(raw_line).strip()
            if not line:
                continue
            tokens.extend(line.split())

        if not tokens:
            raise ValueError(f"empty add_moos_cpp_test call in {cmake_path}")

        target = tokens[0]
        sections: dict[str, list[str]] = {}
        current: str | None = None
        for token in tokens[1:]:
            if token in SECTION_NAMES:
                current = token
                sections.setdefault(current, [])
            elif current is not None:
                sections[current].append(token)

        targets.append(CppTestTarget(cmake_path, target, sections))
        offset = call_end


def all_targets() -> list[CppTestTarget]:
    targets: list[CppTestTarget] = []
    for cmake_path in sorted(CPP_TEST_ROOT.rglob("CMakeLists.txt")):
        targets.extend(parse_add_moos_cpp_tests(cmake_path))
    return targets


def relative(path: Path) -> str:
    try:
        return path.relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def literal_source_path(target: CppTestTarget, source: str) -> Path | None:
    if "$" in source or not source.endswith(".cpp"):
        return None
    source_path = Path(source)
    if source_path.is_absolute():
        return source_path
    return target.file.parent / source_path


def normalize_source_lib_dir(lib_name: str) -> str:
    return lib_name.removeprefix("lib_").replace("-", "_")


def canonical_area_label(area: str) -> str:
    return area.replace("_colregs", "-colregs").replace("_marine", "-marine")


def cpp_area_labels() -> dict[str, str]:
    return {
        path.name: canonical_area_label(path.name)
        for path in CPP_TEST_ROOT.iterdir()
        if path.is_dir() and path.name != "support"
    }


def check_static(moos_src: Path | None = None) -> list[CheckFailure]:
    failures: list[CheckFailure] = []
    targets = all_targets()
    target_names = [target.target for target in targets]
    area_labels = cpp_area_labels()
    area_label_values = set(area_labels.values())

    duplicate_targets = sorted({name for name in target_names if target_names.count(name) > 1})
    for target_name in duplicate_targets:
        failures.append(CheckFailure("duplicate C++ test target", target_name))

    for target in targets:
        target_file = relative(target.file)
        if not re.fullmatch(r"test_[a-z0-9_]+", target.target):
            failures.append(CheckFailure("C++ test target name", f"{target_file}: {target.target}"))
        if not target.values("SOURCES"):
            failures.append(CheckFailure("C++ test target sources", f"{target_file}: {target.target}"))
        if not target.values("LABELS"):
            failures.append(CheckFailure("C++ test target labels", f"{target_file}: {target.target}"))

        top_dir = target.file.relative_to(CPP_TEST_ROOT).parts[0]
        expected_label = area_labels[top_dir]
        if target.file != ROOT_CMAKE and expected_label not in target.values("LABELS"):
            failures.append(
                CheckFailure(
                    "C++ test library label",
                    f"{target_file}: {target.target} missing {expected_label}",
                )
            )
        for label in target.values("LABELS"):
            if label in area_label_values and label != expected_label:
                failures.append(
                    CheckFailure(
                        "C++ test area label leakage",
                        f"{target_file}: {target.target} has {label}; expected only {expected_label}",
                    )
                )

        for suite_label in target.values("SUITE_LABELS"):
            if "$" in suite_label:
                continue
            if "=" not in suite_label:
                failures.append(
                    CheckFailure(
                        "C++ test suite label format",
                        f"{target_file}: {target.target}: {suite_label}",
                    )
                )

        for source in target.values("SOURCES"):
            source_path = literal_source_path(target, source)
            if source_path is not None and not source_path.is_file():
                failures.append(
                    CheckFailure(
                        "C++ test source",
                        f"{target_file}: {target.target} references missing {source}",
                    )
                )

    referenced_local_sources = {
        source_path.resolve()
        for target in targets
        for source in target.values("SOURCES")
        if (source_path := literal_source_path(target, source)) is not None
        and source_path.resolve().is_relative_to(CPP_TEST_ROOT)
    }
    local_test_sources = {path.resolve() for path in CPP_TEST_ROOT.rglob("*Test.cpp")}
    for source_path in sorted(local_test_sources - referenced_local_sources):
        failures.append(CheckFailure("unregistered C++ test source", relative(source_path)))

    for test_dir in sorted({path.parent for path in CPP_TEST_ROOT.rglob("*Test.cpp")}):
        cmake_path = test_dir / "CMakeLists.txt"
        if not cmake_path.is_file():
            failures.append(CheckFailure("C++ test CMake", f"missing {relative(cmake_path)}"))
        elif "add_moos_cpp_test" not in cmake_path.read_text(encoding="utf-8"):
            failures.append(CheckFailure("C++ test CMake", f"no add_moos_cpp_test in {relative(cmake_path)}"))

    for cmake_path in sorted(CPP_TEST_ROOT.rglob("CMakeLists.txt")):
        text = cmake_path.read_text(encoding="utf-8")
        if re.search(r"\.(?:dylib|so)(?:\b|[\"')])", text):
            failures.append(CheckFailure("platform-specific C++ test link path", relative(cmake_path)))
        if cmake_path != ROOT_CMAKE and re.search(r"\b(?:add_test|gtest_add_tests|gtest_discover_tests|set_tests_properties)\s*\(", text):
            failures.append(CheckFailure("manual CTest plumbing", relative(cmake_path)))

    if moos_src is not None:
        source_libs = sorted(path.name for path in moos_src.iterdir() if path.is_dir() and path.name.startswith("lib_"))
        test_library_dirs = sorted(
            path.name for path in CPP_TEST_ROOT.iterdir() if path.is_dir() and path.name != "support"
        )
        source_lib_test_dirs = {normalize_source_lib_dir(lib_name) for lib_name in source_libs}
        missing = sorted(source_lib_test_dirs - set(test_library_dirs))
        extra = sorted(set(test_library_dirs) - source_lib_test_dirs)
        for name in missing:
            failures.append(CheckFailure("MOOS-IvP lib without C++ test bucket", name))
        for name in extra:
            failures.append(CheckFailure("C++ test bucket without MOOS-IvP lib", name))

    return failures


def labels_for_ctest(ctest_test: dict[str, object]) -> list[str]:
    for prop in ctest_test.get("properties", []):
        if prop.get("name") == "LABELS":
            value = prop.get("value", [])
            return value if isinstance(value, list) else [str(value)]
    return []


def def_source_for_ctest(ctest_test: dict[str, object]) -> Path | None:
    for prop in ctest_test.get("properties", []):
        if prop.get("name") == "DEF_SOURCE_LINE":
            value = str(prop.get("value", ""))
            source = value.rsplit(":", 1)[0]
            return Path(source)
    return None


def is_not_built_placeholder(ctest_test: dict[str, object]) -> bool:
    return str(ctest_test.get("name", "")).endswith("_NOT_BUILT")


def check_build(build_dir: Path, allow_not_built: bool = False) -> list[CheckFailure]:
    failures: list[CheckFailure] = []
    ctest_n = subprocess.run(
        ["ctest", "--test-dir", str(build_dir), "-N"],
        cwd=REPO_ROOT,
        check=False,
        text=True,
        capture_output=True,
    )
    if ctest_n.returncode != 0:
        return [CheckFailure("CTest discovery", f"ctest -N failed with exit {ctest_n.returncode}")]
    if "NOT_BUILT" in ctest_n.stdout and not allow_not_built:
        failures.append(CheckFailure("CTest discovery", "stale NOT_BUILT placeholder present"))

    ctest_json = subprocess.run(
        ["ctest", "--test-dir", str(build_dir), "--show-only=json-v1"],
        cwd=REPO_ROOT,
        check=False,
        text=True,
        capture_output=True,
    )
    if ctest_json.returncode != 0:
        return failures + [
            CheckFailure("CTest registry", f"ctest --show-only=json-v1 failed with exit {ctest_json.returncode}")
        ]

    try:
        registry = json.loads(ctest_json.stdout)
    except json.JSONDecodeError as err:
        return failures + [CheckFailure("CTest registry", f"invalid JSON: {err}")]

    ctest_tests = registry.get("tests", [])
    if not ctest_tests:
        failures.append(CheckFailure("CTest registry", "no tests discovered"))
    active_ctest_tests = [
        test
        for test in ctest_tests
        if not (allow_not_built and is_not_built_placeholder(test))
    ]

    static_targets = {
        target.target
        for target in all_targets()
        if (REPO_ROOT / "bin" / target.target).exists()
    }
    not_built_targets = {
        str(test["name"]).removesuffix("_NOT_BUILT")
        for test in ctest_tests
        if is_not_built_placeholder(test)
    }
    if allow_not_built:
        static_targets -= not_built_targets
    ctest_targets = {str(test["name"]).split(".", 1)[0] for test in active_ctest_tests}
    for target in sorted(static_targets - ctest_targets):
        failures.append(CheckFailure("CTest target missing from registry", target))
    for target in sorted(ctest_targets - static_targets):
        failures.append(CheckFailure("CTest target without add_moos_cpp_test", target))

    unlabelled = sorted(str(test["name"]) for test in active_ctest_tests if not labels_for_ctest(test))
    for test_name in unlabelled[:20]:
        failures.append(CheckFailure("unlabelled CTest", test_name))
    if len(unlabelled) > 20:
        failures.append(CheckFailure("unlabelled CTest", f"{len(unlabelled) - 20} additional unlabelled tests"))

    area_labels = cpp_area_labels()
    area_label_values = set(area_labels.values())
    for test in active_ctest_tests:
        source = def_source_for_ctest(test)
        if source is None:
            continue
        try:
            top_dir = source.relative_to(CPP_TEST_ROOT).parts[0]
        except ValueError:
            continue
        expected_label = area_labels.get(top_dir)
        if expected_label is None:
            continue
        if expected_label not in labels_for_ctest(test):
            failures.append(
                CheckFailure(
                    "CTest area label missing",
                    f"{test['name']} missing {expected_label}",
                )
            )
        for label in labels_for_ctest(test):
            if label in area_label_values and label != expected_label:
                failures.append(
                    CheckFailure(
                        "CTest area label leakage",
                        f"{test['name']} has {label}; expected only {expected_label}",
                    )
                )

    return failures


def print_failures(failures: list[CheckFailure]) -> None:
    for failure in failures:
        print(failure.format(), file=sys.stderr)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Check C++ test tree and CTest registry invariants.")
    parser.add_argument(
        "--build-dir",
        type=Path,
        help="Optional configured build directory for CTest registry checks.",
    )
    parser.add_argument(
        "--moos-src",
        type=Path,
        help="Optional MOOS-IvP ivp/src directory for lib_* test-bucket mapping checks.",
    )
    parser.add_argument(
        "--allow-not-built",
        action="store_true",
        help="Ignore generated *_NOT_BUILT placeholder tests during build registry checks.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    failures = check_static(args.moos_src)
    if args.build_dir is not None:
        failures.extend(check_build(args.build_dir, args.allow_not_built))

    if failures:
        print_failures(failures)
        return 1

    details = [f"{len(all_targets())} C++ test target(s)"]
    if args.build_dir is not None:
        details.append(f"CTest registry in {args.build_dir}")
    print("C++ test invariants passed for " + ", ".join(details) + ".")
    return 0


if __name__ == "__main__":
    sys.exit(main())
