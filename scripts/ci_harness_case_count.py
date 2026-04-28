#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path
import re
import shlex
import sys


ARRAY_ASSIGNMENT_RE = re.compile(r"\s*([A-Z][A-Z0-9_]*)=\(\s*(.*)$")
ARRAY_REF_RE = re.compile(r"^\$\{([A-Z][A-Z0-9_]*)\[@\]\}$")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Derive the default case count from a harness zlaunch.sh file."
    )
    parser.add_argument("zlaunch", help="Path to the harness zlaunch.sh")
    return parser.parse_args()


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


def derive_case_order(zlaunch_path: Path) -> list[str]:
    arrays: dict[str, list[str]] = {}
    lines = zlaunch_path.read_text(encoding="utf-8", errors="replace").splitlines()
    idx = 0
    while idx < len(lines):
        line = lines[idx]
        match = ARRAY_ASSIGNMENT_RE.match(line)
        if match:
            name, first_value = match.groups()
            values, consumed = parse_array_values(first_value, lines[idx + 1 :])
            arrays[name] = values
            idx += consumed + 1
            continue

        idx += 1

    if "ALL_CASES" not in arrays:
        raise ValueError(f"No ALL_CASES assignment found in {zlaunch_path}")

    def expand_array(name: str, stack: tuple[str, ...] = ()) -> list[str]:
        if name in stack:
            raise ValueError(f"Cyclic case array reference in {zlaunch_path}: {' -> '.join(stack + (name,))}")
        expanded: list[str] = []
        for token in arrays.get(name, []):
            ref = ARRAY_REF_RE.match(token)
            if ref and ref.group(1) in arrays:
                expanded.extend(expand_array(ref.group(1), stack + (name,)))
            else:
                expanded.append(token)
        return expanded

    return [case for case in expand_array("ALL_CASES") if case]


def derive_case_count(zlaunch_path: Path) -> int:
    return len(derive_case_order(zlaunch_path))


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
