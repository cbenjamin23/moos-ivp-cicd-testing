#!/usr/bin/env python3
"""Render minimal generated GIFs for mission utility harness pages."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Callable

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "gifs"

W, H = 960, 540
FRAMES = 42
FRAME_MS = 100
FINAL_HOLD_MS = 1000

BG = "#202433"
GRID = "#2a3042"
PANEL = "#252b3a"
PANEL_DIM = "#222838"
TEXT = "#eef2f7"
MUTED = "#aeb7c7"
BLUE = "#79a7ff"
TEAL = "#39c8b7"
GREEN = "#76d58e"
YELLOW = "#f5ec60"
ORANGE = "#dc9852"
RED = "#f06c64"
WHITE = "#f8fbff"


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    candidates = [
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf" if bold else "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
    ]
    for candidate in candidates:
        try:
            return ImageFont.truetype(candidate, size)
        except OSError:
            continue
    return ImageFont.load_default()


TITLE = font(28, True)
SUB = font(15)
LABEL = font(16, True)
BODY = font(15)
SMALL = font(13)


@dataclass
class LayoutCheck:
    violations: list[str]

    def require_width(self, filename: str, label: str, text: str, width: int, measured: int) -> None:
        if measured > width:
            self.violations.append(
                f"{filename}: {label} {text!r} is {measured}px wide for {width}px"
            )

    def assert_clean(self) -> None:
        if self.violations:
            raise RuntimeError("Layout check failed:\n" + "\n".join(self.violations))


def clamp(value: float) -> float:
    return max(0.0, min(1.0, value))


def ease(value: float) -> float:
    value = clamp(value)
    return value * value * (3.0 - 2.0 * value)


def text_width(draw: ImageDraw.ImageDraw, text: str, text_font: ImageFont.ImageFont) -> int:
    box = draw.textbbox((0, 0), text, font=text_font)
    return box[2] - box[0]


def checked_text(
    draw: ImageDraw.ImageDraw,
    check: LayoutCheck,
    filename: str,
    xy: tuple[int, int],
    text: str,
    fill: str,
    text_font: ImageFont.ImageFont,
    max_width: int,
    label: str,
) -> None:
    check.require_width(filename, label, text, max_width, text_width(draw, text, text_font))
    draw.text(xy, text, fill=fill, font=text_font)


def base(title: str, subtitle: str) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    im = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(im, "RGBA")
    for x in range(0, W, 96):
        draw.line((x, 0, x, H), fill=GRID, width=1)
    for y in range(0, H, 96):
        draw.line((0, y, W, y), fill=GRID, width=1)
    draw.text((40, 28), title, fill=TEXT, font=TITLE)
    draw.text((40, 66), subtitle, fill=MUTED, font=SUB)
    return im, draw


def card(
    draw: ImageDraw.ImageDraw,
    check: LayoutCheck,
    filename: str,
    box: tuple[int, int, int, int],
    title: str,
    rows: tuple[str, ...],
    accent: str,
    active: bool,
) -> None:
    fill = PANEL if active else PANEL_DIM
    outline = accent if active else "#4d566b"
    draw.rounded_rectangle(box, radius=8, fill=fill, outline=outline, width=2 if active else 1)
    checked_text(draw, check, filename, (box[0] + 16, box[1] + 14), title, outline, LABEL, box[2] - box[0] - 32, f"{title} title")
    for idx, row in enumerate(rows[:4]):
        checked_text(
            draw,
            check,
            filename,
            (box[0] + 16, box[1] + 48 + idx * 24),
            row,
            TEXT if active else MUTED,
            BODY if idx == 0 else SMALL,
            box[2] - box[0] - 32,
            f"{title} row {idx}",
        )


def arrow(draw: ImageDraw.ImageDraw, start: tuple[int, int], end: tuple[int, int], color: str, active: bool) -> None:
    fill = color if active else "#4b5366"
    draw.line((*start, *end), fill=fill, width=3)
    draw.polygon(((end[0], end[1]), (end[0] - 10, end[1] - 6), (end[0] - 10, end[1] + 6)), fill=fill)


def timeline(
    draw: ImageDraw.ImageDraw,
    check: LayoutCheck,
    filename: str,
    progress: float,
    markers: tuple[tuple[float, str, str], ...],
) -> None:
    x, y, width = 112, 146, 736
    checked_text(draw, check, filename, (x, y - 39), "evidence flow", TEXT, LABEL, 140, "timeline title")
    draw.line((x, y, x + width, y), fill="#596175", width=2)
    px = x + int(width * clamp(progress))
    draw.ellipse((px - 7, y - 7, px + 7, y + 7), fill=YELLOW, outline=WHITE, width=1)
    for mark, label, color in markers:
        mx = x + int(width * mark)
        draw.line((mx, y - 14, mx, y + 14), fill=color, width=2)
        label_width = text_width(draw, label, SMALL)
        lx = int(max(x, min(x + width - label_width, mx - label_width / 2)))
        checked_text(draw, check, filename, (lx, y + 22), label, color, SMALL, 120, f"marker {label}")


def center_app(
    draw: ImageDraw.ImageDraw,
    check: LayoutCheck,
    filename: str,
    name: str,
    state: str,
    accent: str,
    active: bool,
) -> None:
    box = (374, 262, 586, 360)
    draw.rounded_rectangle(box, radius=9, fill=PANEL if active else PANEL_DIM, outline=accent if active else "#4d566b", width=2 if active else 1)
    name_width = text_width(draw, name, LABEL)
    state_width = text_width(draw, state, SMALL)
    check.require_width(filename, "app name", name, box[2] - box[0] - 28, name_width)
    check.require_width(filename, "app state", state, box[2] - box[0] - 28, state_width)
    draw.text((box[0] + (box[2] - box[0] - name_width) / 2, box[1] + 25), name, fill=accent if active else MUTED, font=LABEL)
    draw.text((box[0] + (box[2] - box[0] - state_width) / 2, box[1] + 58), state, fill=MUTED, font=SMALL)


def render_flow(
    filename: str,
    title: str,
    subtitle: str,
    app: str,
    app_state: str,
    markers: tuple[tuple[float, str, str], ...],
    left_rows: Callable[[float], tuple[str, ...]],
    right_rows: Callable[[float], tuple[str, ...]],
    left_title: str,
    right_title: str,
    left_color: str,
    right_color: str,
) -> None:
    frames: list[Image.Image] = []
    check = LayoutCheck([])
    if len(markers) < 3:
        raise ValueError(f"{filename} needs at least three evidence-flow checkpoints")
    left_at = markers[0][0]
    app_at = markers[1][0]
    right_at = markers[2][0]
    for i in range(FRAMES):
        p = i / (FRAMES - 1)
        im, draw = base(title, subtitle)
        timeline(draw, check, filename, p, markers)
        card(draw, check, filename, (62, 242, 316, 404), left_title, left_rows(p), left_color, p >= left_at)
        center_app(draw, check, filename, app, app_state if p >= app_at else "waiting", TEAL, p >= app_at)
        card(draw, check, filename, (644, 226, 898, 420), right_title, right_rows(p), right_color, p >= right_at)
        arrow(draw, (316, 323), (374, 323), TEAL, p >= app_at)
        arrow(draw, (586, 323), (644, 323), TEAL, p >= right_at)
        frames.append(im)
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    frames[0].save(
        OUT_DIR / filename,
        save_all=True,
        append_images=frames[1:],
        duration=[FRAME_MS] * (len(frames) - 1) + [FRAME_MS + FINAL_HOLD_MS],
        loop=0,
        optimize=True,
    )
    print(f"Wrote {OUT_DIR / filename}")
    check.assert_clean()


def pmissioneval_baseline() -> None:
    render_flow(
        "pmissioneval-baseline-evaluation.gif",
        "pMissionEval baseline",
        "lead and pass mail become one mission result row",
        "pMissionEval",
        "grading",
        ((0.15, "lead", BLUE), (0.37, "pass", GREEN), (0.66, "report", TEAL), (0.84, "finish", GREEN)),
        lambda p: ("LEAD_INPUT=true", "PASS_INPUT=true", "REPORT_NOTE=ok"),
        lambda p: ("MISSION_RESULT=pass", "RESULT_FLAG=true", "report row written", "MISSION_EVALUATED=true"),
        "scripted mail",
        "graded output",
        BLUE,
        GREEN,
    )


def pmissioneval_sequence() -> None:
    render_flow(
        "pmissioneval-two-stage-logic.gif",
        "pMissionEval two-stage logic",
        "ordered checks wait for both stages before grading pass",
        "pMissionEval",
        "sequencing",
        ((0.18, "stage 1", BLUE), (0.45, "stage 2", BLUE), (0.70, "grade", GREEN)),
        lambda p: ("SEQ_STEP=ready", "SEQ_STEP=armed", "PASS_INPUT=true"),
        lambda p: ("stage_count=2", "sequence complete", "MISSION_RESULT=pass"),
        "ordered mail",
        "sequence state",
        BLUE,
        GREEN,
    )


def pmissionhash_custom() -> None:
    render_flow(
        "pmissionhash-custom-vars.gif",
        "pMissionHash custom vars",
        "configured names publish the long and short hash mail",
        "pMissionHash",
        "publishing",
        ((0.18, "config", BLUE), (0.48, "hash", TEAL), (0.74, "grade", GREEN)),
        lambda p: ("hash_var=CUSTOM_HASH", "short_var=SHORT_HASH", "mission_file=meta.moos"),
        lambda p: ("CUSTOM_HASH=7f3a...", "SHORT_HASH=7f3a", "report macros match"),
        "hash config",
        "published mail",
        BLUE,
        GREEN,
    )


def pmissionhash_reset() -> None:
    render_flow(
        "pmissionhash-reset.gif",
        "pMissionHash reset",
        "RESET_MHASH forces a second hash publication",
        "pMissionHash",
        "refreshing",
        ((0.16, "first hash", TEAL), (0.48, "RESET_MHASH", ORANGE), (0.75, "new hash", GREEN)),
        lambda p: ("MISSION_HASH=old", "RESET_MHASH=true", "mission file touched"),
        lambda p: ("MISSION_HASH=new", "SHORT_MHASH changed", "grade=pass"),
        "runtime mail",
        "refreshed hash",
        ORANGE,
        GREEN,
    )


def umayfinish_default() -> None:
    render_flow(
        "umayfinish-default-exit.gif",
        "uMayFinish default exit",
        "MISSION_EVALUATED=true produces a clean zero exit",
        "uMayFinish",
        "watching",
        ((0.22, "watch", BLUE), (0.54, "match", GREEN), (0.78, "exit 0", GREEN)),
        lambda p: ("var=MISSION_EVALUATED", "value=true", "max_time=8"),
        lambda p: ("matched true", "exit_code=0", "harness result=pass"),
        "finish config",
        "process result",
        BLUE,
        GREEN,
    )


def umayfinish_timeout() -> None:
    render_flow(
        "umayfinish-custom-value-timeout.gif",
        "uMayFinish value mismatch",
        "wrong value is ignored until max_time exits nonzero",
        "uMayFinish",
        "watching",
        ((0.18, "watch", BLUE), (0.46, "wrong value", ORANGE), (0.78, "timeout", RED)),
        lambda p: ("var=CUSTOM_DONE", "value=complete", "CUSTOM_DONE=wrong"),
        lambda p: ("no match", "max_time reached", "exit_code=1"),
        "finish config",
        "process result",
        ORANGE,
        RED,
    )


def main() -> None:
    pmissioneval_baseline()
    pmissioneval_sequence()
    pmissionhash_custom()
    pmissionhash_reset()
    umayfinish_default()
    umayfinish_timeout()


if __name__ == "__main__":
    main()
