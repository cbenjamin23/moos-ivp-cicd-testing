#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_TARGETS_PATH = REPO_ROOT / "config" / "harness_targets.json"
REQUIRED_FIELDS = ("key", "path", "harness", "artifact", "families")


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
    raw_targets: str,
) -> list[dict[str, object]]:
    if dispatch_mode == "family_run":
        selected = [target for target in targets if family in target["families"]]
        if not selected:
            raise ValueError(f"No live harnesses are configured for family '{family}'.")
        return selected

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


def write_github_output(selected: list[dict[str, object]], output_path: Path | None) -> None:
    keys = ",".join(str(target["key"]) for target in selected)
    matrix = json.dumps({"include": selected}, separators=(",", ":"))
    lines = [f"matrix={matrix}", f"selected_keys={keys}"]

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
        profile = f" [{target['perf_profile']}]" if target.get("perf_profile") else ""
        print(f"{target['key']}: {families} -> {target['path']}{profile}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Manage CI harness target metadata.")
    parser.add_argument(
        "--targets-file",
        default=str(DEFAULT_TARGETS_PATH),
        help="Path to harness target JSON metadata.",
    )

    subparsers = parser.add_subparsers(dest="command", required=True)

    select_parser = subparsers.add_parser("select", help="Select targets for a dispatch.")
    select_parser.add_argument("--mode", required=True, choices=("family_run", "correctness_subset", "just_make_subset"))
    select_parser.add_argument("--family", default="")
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
            selected = select_targets(targets, args.mode, args.family, args.targets)
            write_github_output(
                selected,
                Path(args.github_output) if args.github_output else None,
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
