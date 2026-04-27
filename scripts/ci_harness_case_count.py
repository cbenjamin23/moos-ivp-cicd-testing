#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path
import re
import shlex
import sys


ASSIGNMENT_RE = re.compile(r'\s*([A-Z][A-Z0-9_]*)="([^"]*)"\s*$')
ARRAY_ASSIGNMENT_RE = re.compile(r"\s*([A-Z][A-Z0-9_]*)=\(\s*(.*)$")
VAR_RE = re.compile(r"\$\{([A-Z][A-Z0-9_]*)\}|\$([A-Z][A-Z0-9_]*)")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Derive the default case count from a harness zlaunch.sh file."
    )
    parser.add_argument("zlaunch", help="Path to the harness zlaunch.sh")
    return parser.parse_args()


def expand_vars(value: str, variables: dict[str, str]) -> str:
    def repl(match: re.Match[str]) -> str:
        name = match.group(1) or match.group(2)
        return variables.get(name, "")

    prev = None
    expanded = value
    while expanded != prev:
        prev = expanded
        expanded = VAR_RE.sub(repl, expanded)
    return expanded


def parse_array_values(first_value: str, remaining_lines: list[str]) -> tuple[list[str], int]:
    chunks: list[str] = []
    consumed = 0

    line = first_value
    while True:
        if ")" in line:
            chunks.append(line.split(")", 1)[0])
            break
        chunks.append(line)
        if consumed >= len(remaining_lines):
            break
        line = remaining_lines[consumed]
        consumed += 1

    text = "\n".join(chunks)
    return shlex.split(text, comments=True, posix=True), consumed


def derive_case_count(zlaunch_path: Path) -> int:
    variables: dict[str, str] = {}
    case_sets: list[str] = []
    fallback_case_sets: list[str] = []

    lines = zlaunch_path.read_text(encoding="utf-8", errors="replace").splitlines()
    idx = 0
    while idx < len(lines):
        line = lines[idx]
        match = ASSIGNMENT_RE.match(line)
        if match:
            name, value = match.groups()
            expanded = expand_vars(value, variables)
            variables[name] = expanded
            if name == "CASES":
                case_sets.append(expanded)
            idx += 1
            continue

        match = ARRAY_ASSIGNMENT_RE.match(line)
        if match:
            name, first_value = match.groups()
            values, consumed = parse_array_values(first_value, lines[idx + 1 :])
            expanded = expand_vars(" ".join(values), variables)
            variables[name] = expanded
            if name == "CASES":
                case_sets.append(expanded)
            elif name == "ALL_CASES":
                fallback_case_sets.append(expanded)
            idx += consumed + 1
            continue

        idx += 1

    if not case_sets:
        case_sets = fallback_case_sets

    if not case_sets:
        raise ValueError(f"No CASES or ALL_CASES assignment found in {zlaunch_path}")

    return max(len(case_set.split()) for case_set in case_sets)


def main() -> int:
    args = parse_args()
    zlaunch_path = Path(args.zlaunch)

    if not zlaunch_path.exists():
        print(f"zlaunch file not found: {zlaunch_path}", file=sys.stderr)
        return 1

    try:
        print(derive_case_count(zlaunch_path))
    except ValueError as err:
        print(str(err), file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
