#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path
import sys


REQUIRED_KEYS = ("case", "expected", "actual", "status")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Summarize a harness results.txt file for GitHub Actions."
    )
    parser.add_argument("--results", required=True, help="Path to results.txt")
    parser.add_argument("--harness", required=True, help="Harness label for the summary")
    parser.add_argument(
        "--expected-count",
        type=int,
        default=None,
        help="Require this many result lines.",
    )
    parser.add_argument(
        "--require-status",
        default="ok",
        help="Require every case to have this status value.",
    )
    parser.add_argument(
        "--summary-path",
        default=None,
        help="Optional path to append markdown summary output.",
    )
    return parser.parse_args()


def parse_results_line(line: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for token in line.split():
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        fields[key] = value
    return fields


def build_summary(
    harness: str,
    results_path: Path,
    rows: list[dict[str, str]],
    issues: list[str],
) -> str:
    lines = [
        f"### {harness}",
        "",
        f"- Results file: `{results_path}`",
        f"- Parsed cases: `{len(rows)}`",
    ]

    if issues:
        lines.append(f"- Verdict: `fail` ({len(issues)} issue(s))")
    else:
        lines.append("- Verdict: `pass`")

    lines.extend(
        [
            "",
            "| case | expected | actual | status | grade |",
            "| --- | --- | --- | --- | --- |",
        ]
    )

    if rows:
        for row in rows:
            lines.append(
                "| {case} | {expected} | {actual} | {status} | {grade} |".format(
                    case=row.get("case", "?"),
                    expected=row.get("expected", "?"),
                    actual=row.get("actual", "?"),
                    status=row.get("status", "?"),
                    grade=row.get("grade", "?"),
                )
            )
    else:
        lines.append("| _none_ | - | - | - | - |")

    if issues:
        lines.extend(["", "#### Issues", ""])
        for issue in issues:
            lines.append(f"- {issue}")

    lines.append("")
    return "\n".join(lines)


def main() -> int:
    args = parse_args()
    results_path = Path(args.results)
    issues: list[str] = []
    rows: list[dict[str, str]] = []

    if not results_path.exists():
        issues.append(f"Results file was not found at `{results_path}`.")
    else:
        raw_lines = [line.strip() for line in results_path.read_text().splitlines() if line.strip()]
        if not raw_lines:
            issues.append(f"Results file `{results_path}` was empty.")
        else:
            for index, line in enumerate(raw_lines, start=1):
                row = parse_results_line(line)
                missing_keys = [key for key in REQUIRED_KEYS if key not in row]
                if missing_keys:
                    issues.append(
                        f"Line {index} was missing required field(s): {', '.join(missing_keys)}."
                    )
                rows.append(row)

    if args.expected_count is not None and len(rows) != args.expected_count:
        issues.append(
            f"Expected {args.expected_count} case line(s) but parsed {len(rows)}."
        )

    for row in rows:
        case_name = row.get("case", "<missing>")
        expected = row.get("expected")
        actual = row.get("actual")
        status = row.get("status")

        if expected is not None and actual is not None and actual != expected:
            issues.append(
                f"Case `{case_name}` expected `{expected}` but got `{actual}`."
            )
        if status is not None and status != args.require_status:
            issues.append(
                f"Case `{case_name}` had status `{status}` instead of `{args.require_status}`."
            )

    summary = build_summary(args.harness, results_path, rows, issues)
    print(summary)

    if args.summary_path:
        summary_path = Path(args.summary_path)
        with summary_path.open("a", encoding="utf-8") as handle:
            handle.write(summary)
            if not summary.endswith("\n"):
                handle.write("\n")

    return 1 if issues else 0


if __name__ == "__main__":
    sys.exit(main())
