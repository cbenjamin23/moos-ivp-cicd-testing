#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_TARGETS_PATH = REPO_ROOT / "config" / "harness_targets.json"
REQUIRED_FIELDS = ("key", "path", "harness", "artifact", "families")
FULL_MODES = ("correctness",)
SPECIFIC_HARNESSES_MODES = ("specific_harnesses", "correctness_subset")


def load_targets(path: Path = DEFAULT_TARGETS_PATH) -> list[dict[str, object]]:
    with path.open(encoding="utf-8") as stream:
        targets = json.load(stream)

    if not isinstance(targets, list):
        raise ValueError(f"{path} must contain a JSON list")

    return targets


def validate_targets(targets: list[dict[str, object]], repo_root: Path = REPO_ROOT) -> None:
    seen_keys: set[str] = set()

    for idx, target in enumerate(targets, start=1):
        if not isinstance(target, dict):
            raise ValueError(f"target #{idx} must be an object")

        missing = [field for field in REQUIRED_FIELDS if field not in target]
        if missing:
            raise ValueError(f"target #{idx} is missing: {', '.join(missing)}")

        for field in ("key", "path", "harness", "artifact"):
            if not isinstance(target[field], str) or not target[field]:
                raise ValueError(f"target #{idx} field '{field}' must be a non-empty string")

        families = target["families"]
        if (
            not isinstance(families, list)
            or not families
            or any(not isinstance(item, str) or not item for item in families)
        ):
            raise ValueError(f"target #{idx} field 'families' must be a non-empty string list")

        perf_profile = target.get("perf_profile")
        if perf_profile is not None and not isinstance(perf_profile, str):
            raise ValueError(f"target #{idx} field 'perf_profile' must be a string")

        full_modes = target.get("full_modes", [])
        if (
            not isinstance(full_modes, list)
            or any(mode not in FULL_MODES for mode in full_modes)
        ):
            raise ValueError(
                f"target #{idx} field 'full_modes' must use only: {', '.join(FULL_MODES)}"
            )

        key = str(target["key"])
        if key in seen_keys:
            raise ValueError(f"duplicate target key: {key}")
        seen_keys.add(key)

        harness_path = repo_root / str(target["path"])
        if not harness_path.is_dir():
            raise ValueError(f"target {key} path does not exist: {target['path']}")
        if not (harness_path / "zlaunch.sh").is_file():
            raise ValueError(f"target {key} has no zlaunch.sh: {target['path']}")


def select_targets(
    targets: list[dict[str, object]],
    dispatch_mode: str,
    family: str,
    raw_families: str,
    raw_targets: str,
) -> list[dict[str, object]]:
    if dispatch_mode == "none":
        return []

    if dispatch_mode == "correctness_subset":
        dispatch_mode = "specific_harnesses"

    if dispatch_mode == "family_run":
        selected = [target for target in targets if family in target["families"]]
        if not selected:
            raise ValueError(f"No live harnesses are configured for family '{family}'.")
        return selected

    if dispatch_mode == "batch_family_run":
        families = [item.strip() for item in raw_families.split(",") if item.strip()]
        if not families:
            raise ValueError("No families were selected.")
        known_families = {
            str(family_name)
            for target in targets
            for family_name in target["families"]
        }
        invalid = [family_name for family_name in families if family_name not in known_families]
        if invalid:
            raise ValueError(
                "Unknown family/families: "
                + ", ".join(invalid)
                + "\nValid families: "
                + ", ".join(sorted(known_families))
            )

        selected = [
            target
            for target in targets
            if any(family_name in target["families"] for family_name in families)
        ]
        if not selected:
            raise ValueError("No live harnesses are configured for the selected families.")
        return selected

    if dispatch_mode != "specific_harnesses":
        raise ValueError(f"Unknown dispatch mode: {dispatch_mode}")

    wanted = [item.strip() for item in raw_targets.split(",") if item.strip()]
    target_map = {str(target["key"]): target for target in targets}
    invalid = [item for item in wanted if item not in target_map]
    if invalid:
        valid_keys = ", ".join(sorted(target_map))
        raise ValueError(
            "Unknown target key(s): "
            + ", ".join(invalid)
            + "\nValid keys: "
            + valid_keys
        )

    selected = [target_map[item] for item in wanted]
    if not selected:
        raise ValueError("No valid targets were selected.")
    return selected


def select_full_targets(
    targets: list[dict[str, object]],
    full_mode: str,
) -> list[dict[str, object]]:
    selected = [target for target in targets if full_mode in target.get("full_modes", [])]
    if not selected:
        raise ValueError(f"No targets are configured for full {full_mode}.")
    return selected


def matrix_json(selected: list[dict[str, object]]) -> str:
    return json.dumps({"include": selected}, separators=(",", ":"))


def write_github_output(
    selected: list[dict[str, object]],
    output_path: Path | None,
    runtime: list[dict[str, object]] | None = None,
) -> None:
    keys = ",".join(str(target["key"]) for target in selected)
    lines = [
        f"matrix={matrix_json(selected)}",
        f"runtime_matrix={matrix_json(runtime or [])}",
        f"selected_keys={keys}",
    ]

    if output_path is None:
        print("\n".join(lines))
        return

    with output_path.open("a", encoding="utf-8") as stream:
        stream.write("\n".join(lines))
        stream.write("\n")


def write_github_summary(selected: list[dict[str, object]], summary_path: Path | None) -> None:
    if summary_path is None:
        return

    by_family: dict[str, list[str]] = {}
    for target in selected:
        for family in target["families"]:
            by_family.setdefault(str(family), []).append(str(target["key"]))

    with summary_path.open("a", encoding="utf-8") as stream:
        stream.write("### Selected Harness Targets\n\n")
        if not selected:
            stream.write("- _none_\n")
            return
        for target in selected:
            stream.write(
                f"- `{target['key']}`: `{target['harness']}` at `{target['path']}`\n"
            )
        stream.write("\nFamilies covered:\n")
        for family in sorted(by_family):
            stream.write(f"- `{family}`: {', '.join(f'`{key}`' for key in by_family[family])}\n")


def print_target_list(targets: list[dict[str, object]]) -> None:
    for target in targets:
        families = ", ".join(str(item) for item in target["families"])
        flags: list[str] = []
        if target.get("perf_profile"):
            flags.append(str(target["perf_profile"]))
        if target.get("full_modes"):
            flags.append("full:" + ",".join(str(item) for item in target["full_modes"]))
        suffix = f" [{' | '.join(flags)}]" if flags else ""
        print(f"{target['key']}: {families} -> {target['path']}{suffix}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Manage CI harness target metadata.")
    parser.add_argument(
        "--targets-file",
        default=str(DEFAULT_TARGETS_PATH),
        help="Path to harness target JSON metadata.",
    )

    subparsers = parser.add_subparsers(dest="command", required=True)

    select_parser = subparsers.add_parser("select", help="Select targets for a dispatch.")
    select_parser.add_argument(
        "--mode",
        required=True,
        choices=(
            "none",
            "full",
            "family_run",
            "batch_family_run",
            "specific_harnesses",
            "correctness_subset",
        ),
    )
    select_parser.add_argument("--family", default="")
    select_parser.add_argument("--families", default="")
    select_parser.add_argument("--targets", default="")
    select_parser.add_argument("--github-output", default="")
    select_parser.add_argument("--github-summary", default="")

    subparsers.add_parser("list", help="List configured targets.")
    subparsers.add_parser("validate", help="Validate target metadata.")

    return parser.parse_args()


def main() -> int:
    args = parse_args()
    targets_file = Path(args.targets_file)

    try:
        targets = load_targets(targets_file)
        validate_targets(targets)

        if args.command == "validate":
            print(f"Validated {len(targets)} harness target(s).")
        elif args.command == "list":
            print_target_list(targets)
        elif args.command == "select":
            if args.mode == "none":
                selected = []
                runtime = []
            elif args.mode == "full":
                runtime = select_full_targets(targets, "correctness")
                selected = runtime
            else:
                selected = select_targets(
                    targets,
                    args.mode,
                    args.family,
                    args.families,
                    args.targets,
                )
                runtime = selected if args.mode in (
                    *SPECIFIC_HARNESSES_MODES,
                    "family_run",
                    "batch_family_run",
                ) else []
            write_github_output(
                selected,
                Path(args.github_output) if args.github_output else None,
                runtime,
            )
            write_github_summary(
                selected,
                Path(args.github_summary) if args.github_summary else None,
            )
    except (OSError, ValueError, json.JSONDecodeError) as err:
        print(str(err), file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
