#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys


ASSIGNMENT_RE = re.compile(r'\s*([A-Z][A-Z0-9_]*)="([^"]*)"\s*$')
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


def derive_case_count(zlaunch_path: Path) -> int:
    variables: dict[str, str] = {}
    case_sets: list[str] = []

    for line in zlaunch_path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = ASSIGNMENT_RE.match(line)
        if not match:
            continue

        name, value = match.groups()
        expanded = expand_vars(value, variables)
        variables[name] = expanded
        if name == "CASES":
            case_sets.append(expanded)

    if not case_sets:
        raise ValueError(f"No CASES assignment found in {zlaunch_path}")

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
