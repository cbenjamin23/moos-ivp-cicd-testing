#!/usr/bin/env python3
"""Preview or regenerate documentation GIF crops from source recordings.

The GIFs in docs/assets/gifs are derivatives. To "uncrop" or reframe them,
run this against the original MP4 capture and choose a new normalized center
and zoom. Center coordinates are percentages across the source video frame.
"""

from __future__ import annotations

import argparse
import dataclasses
import json
import math
import subprocess
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]


@dataclasses.dataclass(frozen=True)
class Preset:
    source: Path
    output: Path
    crop_w: int
    crop_h: int
    x: int
    y: int
    start: float = 3.5
    duration: float = 8.0
    preview_time: float = 7.5
    fps: int = 8
    out_w: int = 960
    out_h: int = 540


PRESETS: dict[str, Preset] = {
    "obmgr-baseline": Preset(
        source=Path("/tmp/obmgr_visual_options/baseline_center_pass_screen.mp4"),
        output=REPO_ROOT / "docs/assets/gifs/obmgr-motion-baseline-center.gif",
        crop_w=980,
        crop_h=551,
        x=1700,
        y=690,
    ),
    "obmgr-offset": Preset(
        source=Path("/tmp/obmgr_visual_options/offset_clear_pass_screen.mp4"),
        output=REPO_ROOT / "docs/assets/gifs/obmgr-motion-offset-clear.gif",
        crop_w=980,
        crop_h=551,
        x=1751,
        y=585,
    ),
    "obmgr-two-seq": Preset(
        source=Path("/tmp/obmgr_twoseq_visible_capture/two_sequential_fail_screen.mp4"),
        output=REPO_ROOT / "docs/assets/gifs/obmgr-motion-two-sequential.gif",
        crop_w=980,
        crop_h=551,
        x=1712,
        y=705,
    ),
}


def run(cmd: list[str]) -> None:
    subprocess.run(cmd, check=True)


def capture(cmd: list[str]) -> str:
    return subprocess.check_output(cmd, text=True).strip()


def video_size(source: Path) -> tuple[int, int]:
    raw = capture(
        [
            "ffprobe",
            "-v",
            "error",
            "-select_streams",
            "v:0",
            "-show_entries",
            "stream=width,height",
            "-of",
            "json",
            str(source),
        ]
    )
    stream = json.loads(raw)["streams"][0]
    return int(stream["width"]), int(stream["height"])


def clamp(value: int, low: int, high: int) -> int:
    return max(low, min(value, high))


def crop_from_center(
    preset: Preset,
    source_w: int,
    source_h: int,
    source_center_x_pct: float | None,
    source_center_y_pct: float | None,
    frame_center_x_pct: float | None,
    frame_center_y_pct: float | None,
    zoom: float,
) -> tuple[int, int, int, int]:
    if zoom <= 0:
        raise ValueError("zoom must be greater than zero")

    current_cx = preset.x + preset.crop_w / 2
    current_cy = preset.y + preset.crop_h / 2

    if source_center_x_pct is not None and frame_center_x_pct is not None:
        raise ValueError("choose either source x-center or frame x-center, not both")
    if source_center_y_pct is not None and frame_center_y_pct is not None:
        raise ValueError("choose either source y-center or frame y-center, not both")

    if frame_center_x_pct is not None:
        cx = preset.x + preset.crop_w * frame_center_x_pct / 100
    elif source_center_x_pct is not None:
        cx = source_w * source_center_x_pct / 100
    else:
        cx = current_cx

    if frame_center_y_pct is not None:
        cy = preset.y + preset.crop_h * frame_center_y_pct / 100
    elif source_center_y_pct is not None:
        cy = source_h * source_center_y_pct / 100
    else:
        cy = current_cy

    crop_w = int(round(preset.crop_w / zoom))
    crop_h = int(round(preset.crop_h / zoom))

    # Preserve the final 16:9 frame exactly. The preset is close to 16:9, but
    # this keeps the output stable when zoom changes round to integer pixels.
    target_ratio = preset.out_w / preset.out_h
    crop_h = int(round(crop_w / target_ratio))

    crop_w = min(crop_w, source_w)
    crop_h = min(crop_h, source_h)

    x = clamp(int(round(cx - crop_w / 2)), 0, source_w - crop_w)
    y = clamp(int(round(cy - crop_h / 2)), 0, source_h - crop_h)
    return crop_w, crop_h, x, y


def filter_expr(crop_w: int, crop_h: int, x: int, y: int, preset: Preset) -> str:
    return (
        f"crop={crop_w}:{crop_h}:{x}:{y},"
        f"scale={preset.out_w}:{preset.out_h}:flags=lanczos,fps={preset.fps}"
    )


def print_preset(name: str, preset: Preset, source_w: int, source_h: int) -> None:
    cx = preset.x + preset.crop_w / 2
    cy = preset.y + preset.crop_h / 2
    print(f"{name}")
    print(f"  source: {preset.source}")
    print(f"  output: {preset.output}")
    print(f"  source_size: {source_w}x{source_h}")
    print(f"  crop: {preset.crop_w}:{preset.crop_h}:{preset.x}:{preset.y}")
    print(f"  center: x={cx / source_w * 100:.2f}, y={cy / source_h * 100:.2f}")
    print("  zoom: 1.00")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Preview or regenerate a docs GIF crop from its source MP4."
    )
    parser.add_argument("preset", nargs="?", choices=sorted(PRESETS))
    parser.add_argument("--list", action="store_true", help="list known presets")
    parser.add_argument(
        "--x-center",
        type=float,
        help="x position in the current GIF frame, 0-100, to make the new center",
    )
    parser.add_argument(
        "--y-center",
        type=float,
        help="y position in the current GIF frame, 0-100, to make the new center",
    )
    parser.add_argument(
        "--source-x-center",
        type=float,
        help="absolute x center in the full source recording, 0-100",
    )
    parser.add_argument(
        "--source-y-center",
        type=float,
        help="absolute y center in the full source recording, 0-100",
    )
    parser.add_argument(
        "--zoom",
        type=float,
        default=1.0,
        help="1.0 keeps current crop size; 1.3 zooms in; 0.8 uncrops",
    )
    parser.add_argument("--start", type=float, help="GIF start time in source seconds")
    parser.add_argument("--duration", type=float, help="GIF duration in seconds")
    parser.add_argument(
        "--time",
        type=float,
        help="source timestamp for preview still; defaults to preset preview time",
    )
    parser.add_argument("--preview", type=Path, help="write a preview PNG")
    parser.add_argument("--render", action="store_true", help="regenerate the GIF")
    parser.add_argument("--out", type=Path, help="override output GIF path")
    args = parser.parse_args()

    if args.list:
        for name, preset in PRESETS.items():
            if preset.source.exists():
                source_w, source_h = video_size(preset.source)
                print_preset(name, preset, source_w, source_h)
            else:
                print(f"{name}")
                print(f"  source: {preset.source} (missing)")
        return

    if args.preset is None:
        parser.error("choose a preset or pass --list")

    preset = PRESETS[args.preset]
    if not preset.source.exists():
        raise SystemExit(f"source recording not found: {preset.source}")

    source_w, source_h = video_size(preset.source)
    crop_w, crop_h, x, y = crop_from_center(
        preset,
        source_w,
        source_h,
        args.source_x_center,
        args.source_y_center,
        args.x_center,
        args.y_center,
        args.zoom,
    )
    filt = filter_expr(crop_w, crop_h, x, y, preset)
    print(f"crop={crop_w}:{crop_h}:{x}:{y}")
    print(f"filter={filt}")
    print(f"center=x={(x + crop_w / 2) / source_w * 100:.2f}, y={(y + crop_h / 2) / source_h * 100:.2f}")

    if args.preview:
        args.preview.parent.mkdir(parents=True, exist_ok=True)
        run(
            [
                "ffmpeg",
                "-hide_banner",
                "-loglevel",
                "error",
                "-y",
                "-ss",
                str(args.time if args.time is not None else preset.preview_time),
                "-i",
                str(preset.source),
                "-frames:v",
                "1",
                "-vf",
                filt,
                "-update",
                "1",
                str(args.preview),
            ]
        )
        print(f"preview={args.preview}")

    if args.render:
        out = args.out if args.out is not None else preset.output
        out.parent.mkdir(parents=True, exist_ok=True)
        palette = Path("/tmp") / f"{out.stem}-palette.png"
        start = preset.start if args.start is None else args.start
        duration = preset.duration if args.duration is None else args.duration
        run(
            [
                "ffmpeg",
                "-hide_banner",
                "-loglevel",
                "error",
                "-y",
                "-ss",
                str(start),
                "-t",
                str(duration),
                "-i",
                str(preset.source),
                "-vf",
                f"{filt},palettegen=stats_mode=diff",
                str(palette),
            ]
        )
        run(
            [
                "ffmpeg",
                "-hide_banner",
                "-loglevel",
                "error",
                "-y",
                "-ss",
                str(start),
                "-t",
                str(duration),
                "-i",
                str(preset.source),
                "-i",
                str(palette),
                "-lavfi",
                f"{filt} [x]; [x][1:v] paletteuse=dither=bayer:bayer_scale=3:diff_mode=rectangle",
                str(out),
            ]
        )
        print(f"rendered={out}")


if __name__ == "__main__":
    main()
