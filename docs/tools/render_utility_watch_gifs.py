#!/usr/bin/env python3
"""Render generated GIFs for the utility infrastructure harness pages.

These are deliberately minimal documentation views for headless utility tests.
The mission harness results remain the source of truth; the GIFs show only the
evaluated MOOS mail path and the output variables that make each case distinct.
"""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "gifs"

W, H = 960, 540
FRAMES = 52
FRAME_MS = 120

BG = "#202433"
PANEL = "#252b3a"
PANEL_SOFT = "#23293a"
LINE = "#485268"
TEXT = "#eef2f7"
MUTED = "#aeb7c7"
YELLOW = "#f3ea5f"
GREEN = "#75d38b"
TEAL = "#38c6b5"
ORANGE = "#d9904f"
RED = "#f06c64"
BLUE = "#70b9ff"
WHITE = "#f7f9fd"

BOX_INPUT = (70, 242, 306, 384)
BOX_APP = (376, 267, 584, 357)
BOX_OUTPUT = (656, 226, 890, 402)


class LayoutCheck:
    def __init__(self) -> None:
        self.violations: list[str] = []

    def text(self, label: str, text: str, width: int, measured: int) -> None:
        if measured > width:
            self.violations.append(f"{label}: {text!r} is {measured}px wide for {width}px box")

    def assert_clean(self, filename: str) -> None:
        if self.violations:
            details = "\n".join(self.violations)
            raise RuntimeError(f"Layout check failed for {filename}:\n{details}")


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


FONT_TITLE = font(28, True)
FONT_SUB = font(15)
FONT_LABEL = font(16, True)
FONT_TEXT = font(15)
FONT_SMALL = font(13)
FONT_TINY = font(12)


def clamp(v: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return max(lo, min(hi, v))


def draw_checked_text(
    draw: ImageDraw.ImageDraw,
    check: LayoutCheck,
    xy: tuple[int, int],
    text: str,
    fill: str,
    text_font: ImageFont.FreeTypeFont | ImageFont.ImageFont,
    max_width: int,
    label: str,
) -> None:
    bbox = draw.textbbox((0, 0), text, font=text_font)
    check.text(label, text, max_width, bbox[2] - bbox[0])
    draw.text(xy, text, fill=fill, font=text_font)


def draw_base(title: str, subtitle: str) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    im = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(im, "RGBA")
    for x in range(0, W, 96):
        draw.line((x, 0, x, H), fill="#293044", width=1)
    for y in range(0, H, 96):
        draw.line((0, y, W, y), fill="#293044", width=1)
    draw.text((40, 28), title, fill=TEXT, font=FONT_TITLE)
    draw.text((40, 66), subtitle, fill=MUTED, font=FONT_SUB)
    return im, draw


def center_text(
    draw: ImageDraw.ImageDraw,
    check: LayoutCheck,
    box: tuple[int, int, int, int],
    text: str,
    fill: str,
    text_font: ImageFont.FreeTypeFont | ImageFont.ImageFont,
    label: str,
) -> None:
    bbox = draw.textbbox((0, 0), text, font=text_font)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    check.text(label, text, box[2] - box[0] - 22, tw)
    x = box[0] + (box[2] - box[0] - tw) / 2
    y = box[1] + (box[3] - box[1] - th) / 2
    draw.text((x, y), text, fill=fill, font=text_font)


def timeline(
    draw: ImageDraw.ImageDraw,
    check: LayoutCheck,
    progress: float,
    markers: tuple[tuple[float, str, str], ...],
) -> None:
    x, y, w = 126, 150, 708
    draw_checked_text(draw, check, (x, y - 40), "key events", TEXT, FONT_LABEL, 120, "timeline heading")
    draw.line((x, y, x + w, y), fill="#596175", width=2)
    px = x + int(w * clamp(progress))
    draw.ellipse((px - 7, y - 7, px + 7, y + 7), fill=YELLOW, outline=WHITE, width=1)
    for mark, label, color in markers:
        mx = x + int(w * mark)
        draw.line((mx, y - 14, mx, y + 14), fill=color, width=2)
        bbox = draw.textbbox((0, 0), label, font=FONT_TINY)
        lw = bbox[2] - bbox[0]
        lx = int(clamp(mx - lw / 2, x, x + w - lw))
        draw_checked_text(draw, check, (lx, y + 21), label, color, FONT_TINY, 120, f"timeline {label}")


def card(
    draw: ImageDraw.ImageDraw,
    check: LayoutCheck,
    box: tuple[int, int, int, int],
    title: str,
    lines: tuple[str, ...],
    color: str,
    active: bool,
) -> None:
    fill = PANEL if active else PANEL_SOFT
    outline = color if active else "#4d566b"
    draw.rounded_rectangle(box, radius=9, fill=fill, outline=outline, width=2 if active else 1)
    draw_checked_text(draw, check, (box[0] + 16, box[1] + 15), title, outline, FONT_LABEL, box[2] - box[0] - 32, f"{title} title")
    for idx, line in enumerate(lines[:4]):
        draw_checked_text(
            draw,
            check,
            (box[0] + 16, box[1] + 49 + idx * 23),
            line,
            TEXT if active else MUTED,
            FONT_TEXT if idx == 0 else FONT_SMALL,
            box[2] - box[0] - 32,
            f"{title} line {idx}",
        )


def app_card(draw: ImageDraw.ImageDraw, check: LayoutCheck, name: str, color: str, active: bool) -> None:
    fill = "#283244" if active else PANEL_SOFT
    outline = color if active else "#4d566b"
    draw.rounded_rectangle(BOX_APP, radius=10, fill=fill, outline=outline, width=2 if active else 1)
    center_text(draw, check, (BOX_APP[0], BOX_APP[1] + 8, BOX_APP[2], BOX_APP[1] + 47), name, outline, FONT_LABEL, "app name")
    center_text(
        draw,
        check,
        (BOX_APP[0], BOX_APP[1] + 43, BOX_APP[2], BOX_APP[3] - 9),
        "publishing" if active else "waiting",
        MUTED,
        FONT_SMALL,
        "app state",
    )


def arrow(draw: ImageDraw.ImageDraw, a: tuple[int, int], b: tuple[int, int], color: str, active: bool) -> None:
    fill = color if active else "#4b5366"
    draw.line((*a, *b), fill=fill, width=3)
    draw.polygon(((b[0], b[1]), (b[0] - 10, b[1] - 6), (b[0] - 10, b[1] + 6)), fill=fill)


def save_gif(frames: list[Image.Image], filename: str, check: LayoutCheck) -> None:
    check.assert_clean(filename)
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / filename
    frames[0].save(path, save_all=True, append_images=frames[1:], duration=FRAME_MS, loop=0, optimize=True)
    print(f"Wrote {path}")


def render_sequence(
    filename: str,
    title: str,
    subtitle: str,
    app_name: str,
    markers: tuple[tuple[float, str, str], ...],
    input_fn,
    output_fn,
    output_mark_override: float | None = None,
) -> None:
    frames: list[Image.Image] = []
    check = LayoutCheck()
    first_mark = markers[0][0]
    app_mark = markers[1][0] if len(markers) > 1 else first_mark
    output_mark = output_mark_override if output_mark_override is not None else markers[-2][0] if len(markers) > 2 else app_mark
    for i in range(FRAMES):
        p = i / (FRAMES - 1)
        im, draw = draw_base(title, subtitle)
        timeline(draw, check, p, markers)
        input_fn(draw, check, p)
        app_card(draw, check, app_name, TEAL, p >= app_mark)
        arrow(draw, (BOX_INPUT[2], 313), (BOX_APP[0], 313), TEAL, p >= app_mark)
        arrow(draw, (BOX_APP[2], 313), (BOX_OUTPUT[0], 313), TEAL, p >= output_mark)
        output_fn(draw, check, p)
        frames.append(im)
    save_gif(frames, filename, check)


def hostinfo_route() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "input", ("host=127.0.0.7", "route=127.0.0.1:11111"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.78:
            card(draw, check, BOX_OUTPUT, "graded output", ("PHI_HOST_INFO", "route present", "GRADE=pass"), GREEN, True)
            return
        card(
            draw,
            check,
            BOX_OUTPUT,
            "graded output",
            ("PHI_HOST_INFO", "port_udp=11111", "iroute=127.0.0.1:11111"),
            GREEN,
            p >= 0.48,
        )

    render_sequence(
        "hostinfo-pshare-route-payload.gif",
        "pHostInfo route payload",
        "one route becomes one exact PHI_HOST_INFO field",
        "pHostInfo",
        ((0.12, "launch", BLUE), (0.48, "publish", TEAL), (0.78, "pass", GREEN)),
        inputs,
        outputs,
    )


def hostinfo_invalid() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "input", ("host=127.0.0.7", "route=bad_route"), ORANGE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.78:
            card(draw, check, BOX_OUTPUT, "graded output", ("PHI_HOST_INFO", "route omitted", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "graded output", ("PHI_HOST_INFO", "no pshare_iroutes"), GREEN, p >= 0.48)

    render_sequence(
        "hostinfo-invalid-route-omitted.gif",
        "pHostInfo invalid route",
        "malformed route input is omitted from output",
        "pHostInfo",
        ((0.12, "bad route", ORANGE), (0.48, "publish", TEAL), (0.78, "pass", GREEN)),
        inputs,
        outputs,
    )


def loadwatch_near() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "input", ("UTILITY_ITER_GAP", "gap=2.6", "hard=3.0"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.78:
            card(draw, check, BOX_OUTPUT, "graded output", ("NEAR_COUNT=1", "BREACH_COUNT=0", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "graded output", ("NEAR_COUNT=1", "BREACH_COUNT=0"), ORANGE, p >= 0.48)

    render_sequence(
        "loadwatch-near-breach-band.gif",
        "uLoadWatch near breach",
        "near counter changes while hard breach stays zero",
        "uLoadWatch",
        ((0.12, "iter gap", BLUE), (0.48, "near", ORANGE), (0.78, "pass", GREEN)),
        inputs,
        outputs,
    )


def loadwatch_trigger() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "input", ("breach_trigger=1", "gap=3.5 then 3.7", "hard=3.0"), ORANGE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.84:
            card(draw, check, BOX_OUTPUT, "graded output", ("NEAR_COUNT=2", "BREACH_COUNT=1", "GRADE=pass"), GREEN, True)
            return
        active = p >= 0.44
        text = ("NEAR_COUNT=2", "BREACH_COUNT=1") if p >= 0.68 else ("NEAR_COUNT=1", "BREACH_COUNT=0")
        card(draw, check, BOX_OUTPUT, "graded output", text, RED if p >= 0.68 else ORANGE, active)

    render_sequence(
        "loadwatch-trigger-holdoff.gif",
        "uLoadWatch trigger holdoff",
        "first crossing is held; second crossing breaches",
        "uLoadWatch",
        ((0.12, "config", BLUE), (0.44, "first", ORANGE), (0.68, "second", RED), (0.84, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.44,
    )


def processwatch_mapping() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "input", ("watch 3 apps", "summary -> UTIL_PROC"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("UTIL_PROC_SUMMARY", "All Present", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "graded output", ("ALL_OK=true", "UTIL_PROC_SUMMARY", "All Present"), GREEN, p >= 0.62)

    render_sequence(
        "processwatch-mapped-summary.gif",
        "uProcessWatch mapped summary",
        "the mapped summary carries the all-present result",
        "uProcessWatch",
        ((0.12, "watch", BLUE), (0.36, "map", TEAL), (0.62, "summary", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
    )


def processwatch_awol() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "input", ("watch pGhost", "no registration"), ORANGE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("ALL_OK=false", "AWOL:pGhost", "expected fail"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "graded output", ("ALL_OK=false", "SUMMARY=AWOL:pGhost"), RED, p >= 0.58)

    render_sequence(
        "processwatch-awol-detection.gif",
        "uProcessWatch AWOL detection",
        "a missing process flips all-ok false",
        "uProcessWatch",
        ((0.12, "watch", ORANGE), (0.58, "AWOL", RED), (0.82, "expected fail", GREEN)),
        inputs,
        outputs,
    )


def main() -> None:
    hostinfo_route()
    hostinfo_invalid()
    loadwatch_near()
    loadwatch_trigger()
    processwatch_mapping()
    processwatch_awol()


if __name__ == "__main__":
    main()
