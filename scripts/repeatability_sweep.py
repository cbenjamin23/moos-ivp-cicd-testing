#!/usr/bin/env python3

from __future__ import annotations

import argparse
import datetime as dt
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time


REPO_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO_ROOT))

from scripts.ci_harness_case_count import derive_case_order


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run repeated harness sweeps and summarize per-case stability."
    )
    parser.add_argument(
        "--harness",
        required=True,
        help="Absolute path to the harness directory containing zlaunch.sh",
    )
    parser.add_argument(
        "--runs",
        type=int,
        default=5,
        help="Number of repeated harness runs to execute (default 5)",
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=3,
        help="Jobs value passed to zlaunch.sh (default 3)",
    )
    parser.add_argument(
        "--time_warp",
        type=int,
        default=10,
        help="Time warp passed to zlaunch.sh (default 10)",
    )
    parser.add_argument(
        "--max_time",
        type=int,
        default=None,
        help="Optional max_time override passed to zlaunch.sh",
    )
    parser.add_argument(
        "--out",
        default=None,
        help="Optional output root directory; defaults to repo time_benchmarking/",
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


def row_result(row: dict[str, str]) -> str:
    if row.get("grade") == "pass":
        return "pass"
    return "fail"


def extract_case_order(zlaunch_path: Path) -> list[str]:
    try:
        return derive_case_order(zlaunch_path)
    except ValueError as err:
        raise RuntimeError(f"Could not extract ALL_CASES from {zlaunch_path}") from err


def fmt_float_range(values: list[float]) -> str:
    if not values:
        return "-"
    return f"{min(values):.3f}-{max(values):.3f}"


def set_string(values: set[str]) -> str:
    if not values:
        return "-"
    return ", ".join(sorted(values))


def main() -> int:
    args = parse_args()
    harness = Path(args.harness).resolve()
    zlaunch = harness / "zlaunch.sh"
    results_file = harness / "results.txt"

    if not harness.is_dir() or not zlaunch.is_file():
        print(f"repeatability_sweep.py: bad harness path: {harness}", file=sys.stderr)
        return 1

    repo_dir = harness.parents[2]
    out_root = Path(args.out).resolve() if args.out else repo_dir / "time_benchmarking"
    stamp = dt.datetime.now().strftime("%Y-%m-%d_%H%M%S")
    out_dir = out_root / f"{harness.name}_repeatability_{stamp}"
    raw_dir = out_dir / "raw"
    raw_dir.mkdir(parents=True, exist_ok=True)

    case_order = extract_case_order(zlaunch)

    run_meta: list[dict[str, object]] = []
    case_stats: dict[str, dict[str, object]] = {}
    for case in case_order:
        case_stats[case] = {
            "seen": 0,
            "success": 0,
            "results": set(),
            "mission_grades": set(),
            "cn_ports": set(),
            "cn_fores": set(),
            "cn_crosseds": set(),
            "modes": set(),
            "ixs": set(),
            "cpa_values": [],
            "wall_values": [],
            "missing_runs": [],
        }

    verdict_failures: list[str] = []

    for run_num in range(1, args.runs + 1):
        subprocess.run(
            ["ktm"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )

        cmd = ["./zlaunch.sh", f"--jobs={args.jobs}"]
        if args.max_time is not None:
            cmd.append(f"--max_time={args.max_time}")
        cmd.append(str(args.time_warp))

        log_path = raw_dir / f"run{run_num:02d}.log"
        start = time.monotonic()
        with log_path.open("w", encoding="utf-8") as handle:
            proc = subprocess.run(
                cmd,
                cwd=harness,
                stdout=handle,
                stderr=subprocess.STDOUT,
                check=False,
            )
        elapsed = time.monotonic() - start

        run_results_path = raw_dir / f"run{run_num:02d}.results.txt"
        if results_file.exists():
            shutil.copyfile(results_file, run_results_path)
        else:
            run_results_path.write_text("", encoding="utf-8")

        rows: list[dict[str, str]] = []
        raw_lines = [
            line.strip()
            for line in run_results_path.read_text(encoding="utf-8").splitlines()
            if line.strip()
        ]
        for line in raw_lines:
            rows.append(parse_results_line(line))

        seen_cases: set[str] = set()
        for row in rows:
            case = row.get("case", "")
            if case == "" or case not in case_stats:
                continue
            seen_cases.add(case)
            stats = case_stats[case]
            stats["seen"] = int(stats["seen"]) + 1
            result = row_result(row)
            cast_results = stats["results"]
            assert isinstance(cast_results, set)
            cast_results.add(result)
            grade = row.get("grade")
            if grade not in (None, ""):
                cast_grades = stats["mission_grades"]
                assert isinstance(cast_grades, set)
                cast_grades.add(grade)
            if result == "pass":
                stats["success"] = int(stats["success"]) + 1

            for key, stat_key in (
                ("cn_port", "cn_ports"),
                ("cn_fore", "cn_fores"),
                ("cn_crossed", "cn_crosseds"),
                ("colregs_mode", "modes"),
                ("colregs_ix", "ixs"),
            ):
                value = row.get(key)
                if value not in (None, ""):
                    cast_set = stats[stat_key]
                    assert isinstance(cast_set, set)
                    cast_set.add(value)

            for key, stat_key in (
                ("closest_range_ever", "cpa_values"),
                ("wall_time", "wall_values"),
            ):
                value = row.get(key)
                if value in (None, ""):
                    continue
                try:
                    num = float(value)
                except ValueError:
                    continue
                cast_list = stats[stat_key]
                assert isinstance(cast_list, list)
                cast_list.append(num)

        for case in case_order:
            if case not in seen_cases:
                cast_missing = case_stats[case]["missing_runs"]
                assert isinstance(cast_missing, list)
                cast_missing.append(run_num)

        run_meta.append(
            {
                "run": run_num,
                "elapsed": elapsed,
                "rc": proc.returncode,
                "parsed_cases": len(rows),
            }
        )

        if proc.returncode != 0:
            verdict_failures.append(f"run {run_num} exited with rc={proc.returncode}")

    for case in case_order:
        stats = case_stats[case]
        if int(stats["seen"]) != args.runs:
            verdict_failures.append(
                f"{case} appeared in {stats['seen']}/{args.runs} runs"
            )
        if int(stats["success"]) != args.runs:
            verdict_failures.append(
                f"{case} was successful in {stats['success']}/{args.runs} runs"
            )

    summary_path = out_dir / "README.md"
    timestamp = dt.datetime.now().strftime("%B %d, %Y %H:%M:%S %Z")
    lines: list[str] = [
        "# Repeatability Sweep",
        "",
        f"Date: {timestamp}",
        "",
        "Harness:",
        "",
        f"- `{harness}`",
        "",
        "Settings:",
        "",
        f"- runs: `{args.runs}`",
        f"- jobs: `{args.jobs}`",
        f"- warp: `{args.time_warp}`",
    ]
    if args.max_time is not None:
        lines.append(f"- max_time override: `{args.max_time}`")
    lines.extend(
        [
            "",
            "Verdict:",
            "",
            f"- `{'fail' if verdict_failures else 'pass'}`",
            "",
            "Per-run wall clock:",
            "",
        ]
    )

    for item in run_meta:
        lines.append(
            "- `run={run}`: `{elapsed:.2f}s`, `rc={rc}`, `parsed_cases={parsed}`".format(
                run=item["run"],
                elapsed=float(item["elapsed"]),
                rc=item["rc"],
                parsed=item["parsed_cases"],
            )
        )

    lines.extend(
        [
            "",
            "Per-case summary:",
            "",
            "| case | pass/runs | result values | mission_grade values | CPA range | wall range | cn_port values | cn_fore values | cn_crossed values | mode values | ix values |",
            "| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |",
        ]
    )

    for case in case_order:
        stats = case_stats[case]
        lines.append(
            "| {case} | {success}/{runs} | {results} | {mission_grades} | {cpa} | {wall} | {cn_ports} | {cn_fores} | {cn_crosseds} | {modes} | {ixs} |".format(
                case=case,
                success=stats["success"],
                runs=args.runs,
                results=set_string(stats["results"]),  # type: ignore[arg-type]
                mission_grades=set_string(stats["mission_grades"]),  # type: ignore[arg-type]
                cpa=fmt_float_range(stats["cpa_values"]),  # type: ignore[arg-type]
                wall=fmt_float_range(stats["wall_values"]),  # type: ignore[arg-type]
                cn_ports=set_string(stats["cn_ports"]),  # type: ignore[arg-type]
                cn_fores=set_string(stats["cn_fores"]),  # type: ignore[arg-type]
                cn_crosseds=set_string(stats["cn_crosseds"]),  # type: ignore[arg-type]
                modes=set_string(stats["modes"]),  # type: ignore[arg-type]
                ixs=set_string(stats["ixs"]),  # type: ignore[arg-type]
            )
        )

    missing_rows = [
        (case, stats["missing_runs"])
        for case, stats in case_stats.items()
        if stats["missing_runs"]
    ]
    if missing_rows:
        lines.extend(["", "Missing-case detail:", ""])
        for case, missing_runs in missing_rows:
            lines.append(f"- `{case}` missing on run(s): `{missing_runs}`")

    if verdict_failures:
        lines.extend(["", "Failures:", ""])
        for item in verdict_failures:
            lines.append(f"- {item}")

    lines.extend(
        [
            "",
            "Saved artifacts:",
            "",
            f"- summary: `{summary_path}`",
            f"- raw logs/results: `{raw_dir}`",
            "",
        ]
    )

    summary_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote {summary_path}")
    return 1 if verdict_failures else 0


if __name__ == "__main__":
    sys.exit(main())
