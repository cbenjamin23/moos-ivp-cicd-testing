#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_TARGETS_PATH = REPO_ROOT / "config" / "moos_ivp_build_targets.json"
REQUIRED_FIELDS = ("key", "image", "compiler", "profiles", "families")
SUPPORTED_COMPILERS = ("gcc", "clang")
SUPPORTED_BUILD_PROFILES = ("full", "headless")


def load_targets(path: Path = DEFAULT_TARGETS_PATH) -> list[dict[str, object]]:
    with path.open(encoding="utf-8") as stream:
        targets = json.load(stream)

    if not isinstance(targets, list):
        raise ValueError(f"{path} must contain a JSON list")

    return targets


def validate_targets(targets: list[dict[str, object]]) -> None:
    seen_keys: set[str] = set()

    for idx, target in enumerate(targets, start=1):
        if not isinstance(target, dict):
            raise ValueError(f"target #{idx} must be an object")

        missing = [field for field in REQUIRED_FIELDS if field not in target]
        if missing:
            raise ValueError(f"target #{idx} is missing: {', '.join(missing)}")

        for field in ("key", "image", "compiler"):
            if not isinstance(target[field], str) or not target[field]:
                raise ValueError(f"target #{idx} field '{field}' must be a non-empty string")

        compiler = str(target["compiler"])
        if compiler not in SUPPORTED_COMPILERS:
            raise ValueError(
                f"target #{idx} compiler must be one of: {', '.join(SUPPORTED_COMPILERS)}"
            )

        profiles = target["profiles"]
        if (
            not isinstance(profiles, list)
            or not profiles
            or any(not isinstance(item, str) or item not in SUPPORTED_BUILD_PROFILES for item in profiles)
        ):
            raise ValueError(
                f"target #{idx} field 'profiles' must be a non-empty list containing: "
                + ", ".join(SUPPORTED_BUILD_PROFILES)
            )

        families = target["families"]
        if (
            not isinstance(families, list)
            or not families
            or any(not isinstance(item, str) or not item for item in families)
        ):
            raise ValueError(f"target #{idx} field 'families' must be a non-empty string list")

        key = str(target["key"])
        if key in seen_keys:
            raise ValueError(f"duplicate target key: {key}")
        seen_keys.add(key)


def comma_list(raw: str) -> list[str]:
    return [item.strip() for item in raw.split(",") if item.strip()]


def select_targets(
    targets: list[dict[str, object]],
    target_set: str,
    raw_targets: str,
    build_profile: str,
) -> list[dict[str, object]]:
    if build_profile not in SUPPORTED_BUILD_PROFILES:
        raise ValueError(
            "Unknown build profile: "
            + build_profile
            + "\nValid profiles: "
            + ", ".join(SUPPORTED_BUILD_PROFILES)
        )

    if target_set == "all":
        selected = targets

    elif target_set == "specific_targets":
        wanted = comma_list(raw_targets)
        target_map = {str(target["key"]): target for target in targets}
        invalid = [item for item in wanted if item not in target_map]
        if invalid:
            valid_keys = ", ".join(sorted(target_map))
            raise ValueError(
                "Unknown build target key(s): "
                + ", ".join(invalid)
                + "\nValid keys: "
                + valid_keys
            )
        if not wanted:
            raise ValueError("No build targets were selected.")
        selected = [target_map[item] for item in wanted]

    else:
        selected = [target for target in targets if target_set in target["families"]]
        if not selected:
            known_families = sorted(
                {
                    str(family)
                    for target in targets
                    for family in target["families"]
                }
            )
            raise ValueError(
                f"Unknown build target set: {target_set}\n"
                + "Valid sets: all, specific_targets, "
                + ", ".join(known_families)
            )

    incompatible = [
        str(target["key"])
        for target in selected
        if build_profile not in target["profiles"]
    ]
    if incompatible and target_set == "specific_targets":
        raise ValueError(
            f"Build profile '{build_profile}' is not supported by target(s): "
            + ", ".join(incompatible)
        )

    selected = [
        target
        for target in selected
        if build_profile in target["profiles"]
    ]
    if not selected:
        raise ValueError(
            f"No build targets in set '{target_set}' support build profile '{build_profile}'."
        )
    return selected


def matrix_entry(target: dict[str, object]) -> dict[str, str]:
    key = str(target["key"])
    return {
        "key": key,
        "image": str(target["image"]),
        "compiler": str(target["compiler"]),
        "artifact": f"moos-ivp-build-{key}",
    }


def matrix_for_targets(selected: list[dict[str, object]]) -> dict[str, list[dict[str, str]]]:
    return {"include": [matrix_entry(target) for target in selected]}


def write_github_output(selected: list[dict[str, object]], output_path: Path | None) -> None:
    lines = [
        "matrix=" + json.dumps(matrix_for_targets(selected), separators=(",", ":")),
        "selected_keys=" + ",".join(str(target["key"]) for target in selected),
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

    with summary_path.open("a", encoding="utf-8") as stream:
        stream.write("### Selected MOOS-IvP Build Targets\n\n")
        for target in selected:
            profiles = ", ".join(str(item) for item in target["profiles"])
            stream.write(
                f"- `{target['key']}`: `{target['image']}` with `{target['compiler']}` "
                f"(profiles: `{profiles}`)\n"
            )


def print_target_list(targets: list[dict[str, object]]) -> None:
    for target in targets:
        families = ", ".join(str(item) for item in target["families"])
        profiles = ", ".join(str(item) for item in target["profiles"])
        print(
            f"{target['key']}: {target['image']} / {target['compiler']} "
            f"[profiles: {profiles}; families: {families}]"
        )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Select MOOS-IvP build portability targets."
    )
    parser.add_argument(
        "--targets-file",
        default=str(DEFAULT_TARGETS_PATH),
        help="Path to MOOS-IvP build target JSON metadata.",
    )

    subparsers = parser.add_subparsers(dest="command", required=True)

    select_parser = subparsers.add_parser("select", help="Select targets for a build matrix.")
    select_parser.add_argument(
        "--target-set",
        default="modern",
        help="Target family to select, or all, or specific_targets.",
    )
    select_parser.add_argument(
        "--targets",
        default="",
        help="Comma-separated target keys when --target-set specific_targets is used.",
    )
    select_parser.add_argument(
        "--build-profile",
        default="full",
        help="Build profile to select compatible targets for.",
    )
    select_parser.add_argument(
        "--github-output",
        default="",
        help="Optional GITHUB_OUTPUT path for matrix output.",
    )
    select_parser.add_argument(
        "--summary-path",
        default="",
        help="Optional GitHub step summary path to append Markdown output.",
    )

    subparsers.add_parser("list", help="List configured build targets.")
    subparsers.add_parser("validate", help="Validate build target metadata.")

    return parser.parse_args()


def main() -> int:
    args = parse_args()
    targets_file = Path(args.targets_file)

    try:
        targets = load_targets(targets_file)
        validate_targets(targets)

        if args.command == "validate":
            print(f"Validated {len(targets)} MOOS-IvP build target(s).")
        elif args.command == "list":
            print_target_list(targets)
        elif args.command == "select":
            selected = select_targets(
                targets,
                target_set=args.target_set,
                raw_targets=args.targets,
                build_profile=args.build_profile,
            )
            write_github_output(
                selected,
                Path(args.github_output) if args.github_output else None,
            )
            write_github_summary(
                selected,
                Path(args.summary_path) if args.summary_path else None,
            )
        else:
            raise ValueError(f"Unknown command: {args.command}")
    except ValueError as err:
        print(err)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
