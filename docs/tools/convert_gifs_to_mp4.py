#!/usr/bin/env python3
"""Convert generated documentation GIFs to GitHub Pages-friendly MP4s."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
GIF_DIR = ROOT / "assets" / "gifs"


def convert_gif(path: Path, crf: int, overwrite: bool) -> Path:
    output = path.with_suffix(".mp4")
    if output.exists() and not overwrite:
        return output

    cmd = [
        "ffmpeg",
        "-y" if overwrite else "-n",
        "-hide_banner",
        "-loglevel",
        "error",
        "-i",
        str(path),
        "-vf",
        "pad=ceil(iw/2)*2:ceil(ih/2)*2:color=white,format=yuv420p",
        "-movflags",
        "+faststart",
        "-c:v",
        "libx264",
        "-crf",
        str(crf),
        "-preset",
        "slow",
        str(output),
    ]
    subprocess.run(cmd, check=True)
    return output


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--gif-dir", type=Path, default=GIF_DIR)
    parser.add_argument("--crf", type=int, default=18)
    parser.add_argument("--delete-source-gifs", action="store_true")
    parser.add_argument("--no-overwrite", action="store_true")
    args = parser.parse_args()

    gif_dir = args.gif_dir.resolve()
    gifs = sorted(gif_dir.glob("*.gif"))
    if not gifs:
        print(f"No GIF files found in {gif_dir}")
        return

    total_gif_bytes = 0
    total_mp4_bytes = 0
    for gif in gifs:
        total_gif_bytes += gif.stat().st_size
        mp4 = convert_gif(gif, args.crf, overwrite=not args.no_overwrite)
        total_mp4_bytes += mp4.stat().st_size
        if args.delete_source_gifs:
            gif.unlink()

    reduction = 100.0 * (1.0 - (total_mp4_bytes / total_gif_bytes))
    print(f"Converted {len(gifs)} GIF(s) in {gif_dir}")
    print(f"GIF total: {total_gif_bytes / (1024 * 1024):.1f} MiB")
    print(f"MP4 total: {total_mp4_bytes / (1024 * 1024):.1f} MiB")
    print(f"Reduction: {reduction:.1f}%")


if __name__ == "__main__":
    main()
