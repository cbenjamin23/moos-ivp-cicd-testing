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

BOX_INPUT = (70, 298, 306, 440)
BOX_APP = (376, 323, 584, 413)
BOX_OUTPUT = (656, 298, 890, 440)


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
    draw.text((40, 32), title, fill=TEXT, font=FONT_TITLE)
    draw.text((40, 72), subtitle, fill=MUTED, font=FONT_SUB)
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
    x, y, w = 126, 186, 708
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
        arrow(draw, (BOX_INPUT[2], 369), (BOX_APP[0], 369), TEAL, p >= app_mark)
        arrow(draw, (BOX_APP[2], 369), (BOX_OUTPUT[0], 369), TEAL, p >= output_mark)
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


def upokedb_direct_numeric() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "command", ("--host localhost", "--port 15000", "POKE_NUM=42.5"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("POKE_NUM=42.5", "pMissionEval", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "MOOSDB mail", ("POKE_NUM", "double 42.5"), GREEN, p >= 0.54)

    render_sequence(
        "upokedb-direct-numeric.gif",
        "uPokeDB direct numeric",
        "one command-line poke becomes the graded DB value",
        "uPokeDB",
        ((0.12, "command", BLUE), (0.38, "connect", TEAL), (0.54, "publish", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.54,
    )


def upokedb_cached_pokes() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "cache file", ("poke=CACHE_NUM=17", "poke=CACHE_STR:=alpha"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("CACHE_NUM=17", "CACHE_STR=alpha", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "MOOSDB mail", ("CACHE_NUM", "CACHE_STR"), GREEN, p >= 0.56)

    render_sequence(
        "upokedb-cached-pokes.gif",
        "uPokeDB cached pokes",
        "cached numeric and forced-string pokes publish together",
        "uPokeDB",
        ((0.12, "cache", BLUE), (0.36, "--cache", TEAL), (0.56, "publish", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.56,
    )


def uxms_scoped_variable() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "publisher", ("XMS_ALPHA", "value=alpha", "source=uTimerScript"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "captured output", ("XMS_ALPHA", "\"alpha\"", "token checks pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "terminal scope", ("VarName", "XMS_ALPHA"), GREEN, p >= 0.56)

    render_sequence(
        "uxms-scoped-variable.gif",
        "uXMS scoped variable",
        "bounded terminal capture shows the requested mail",
        "uXMS",
        ((0.12, "mail", BLUE), (0.38, "scope", TEAL), (0.56, "render", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.56,
    )


def uxms_history_view() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.56:
            rows = ("XMS_HIST=first", "XMS_HIST=second", "XMS_HIST=third")
        else:
            rows = ("XMS_HIST=first", "XMS_HIST=second")
        card(draw, check, BOX_INPUT, "history mail", rows, BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "captured output", ("XMS_HIST", "\"third\"", "token checks pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "history view", ("latest entry", "\"third\""), GREEN, p >= 0.62)

    render_sequence(
        "uxms-history-view.gif",
        "uXMS history view",
        "repeated mail is rendered as inspectable history",
        "uXMS",
        ((0.12, "updates", BLUE), (0.40, "history", TEAL), (0.62, "render", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.62,
    )


def uquerydb_numeric_condition() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "query", ("QUERY_NUM = 42", "--wait=3"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded result", ("return code 0", "MISSION_READY", "case pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "live MOOSDB", ("QUERY_NUM", "42"), GREEN, p >= 0.56)

    render_sequence(
        "uquerydb-numeric-condition.gif",
        "uQueryDB numeric condition",
        "a live DB value satisfies the query and exits cleanly",
        "uQueryDB",
        ((0.12, "condition", BLUE), (0.38, "subscribe", TEAL), (0.56, "mail seen", GREEN), (0.82, "rc=0", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.56,
    )


def uquerydb_checkvar_formats() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "query", ("QUERY_STR = ready", "--check_var", "--csv"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, ".checkvars", ("QUERY_STR,ready", "token check", "case pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "check variable", ("QUERY_STR", "ready"), GREEN, p >= 0.58)

    render_sequence(
        "uquerydb-checkvar-formats.gif",
        "uQueryDB check variables",
        ".checkvars records the requested value in CSV form",
        "uQueryDB",
        ((0.12, "query", BLUE), (0.40, "check_var", TEAL), (0.58, "write file", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.58,
    )


def pdeadmanpost_deadman_trip() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p < 0.50:
            lines = ("heartbeat idle", "max_noheart=0.5")
        else:
            lines = ("no heartbeat", "elapsed > max")
        card(draw, check, BOX_INPUT, "watchdog", lines, ORANGE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("DEAD_ONCE=true", "GRADE=pass", "count >= 1"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "deadflag", ("DEAD_ONCE", "pending"), RED, p >= 0.58)

    render_sequence(
        "pdeadmanpost-deadman-trip.gif",
        "pDeadManPost deadman trip",
        "stale heartbeat time produces the configured deadflag",
        "pDeadManPost",
        ((0.12, "start", BLUE), (0.48, "stale", ORANGE), (0.58, "post", RED), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.58,
    )


def pdeadmanpost_heartbeat_suppression() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p < 0.52:
            lines = ("HEARTBEAT", "0.2  0.5  0.8")
        else:
            lines = ("fresh heartbeat", "elapsed < max")
        card(draw, check, BOX_INPUT, "keepalive", lines, BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("deadflag absent", "fail condition quiet", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "deadflag", ("DEAD_KEEPALIVE", "not posted"), GREEN, p >= 0.56)

    render_sequence(
        "pdeadmanpost-heartbeat-suppression.gif",
        "pDeadManPost heartbeat suppression",
        "fresh heartbeat mail prevents the deadflag post",
        "pDeadManPost",
        ((0.12, "heartbeat", BLUE), (0.40, "refresh", TEAL), (0.56, "no post", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.56,
    )


def ufield_comms_baseline() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "field mail", ("alpha report", "message to bravo", "range ok"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("DELIVERED=true", "payload seen", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "bravo db", ("NODE_MESSAGE", "pending"), GREEN, p >= 0.58)

    render_sequence(
        "ufield-comms-baseline-broker.gif",
        "uField baseline broker comms",
        "in-range node traffic is delivered to the peer vehicle",
        "uFldNodeComms",
        ((0.12, "reports", BLUE), (0.38, "range ok", TEAL), (0.58, "deliver", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.58,
    )


def ufield_comms_runtime_range() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        rows = ("range=40", "blocked") if p < 0.48 else ("range=180", "runtime mail", "delivered")
        card(draw, check, BOX_INPUT, "range gate", rows, ORANGE if p < 0.48 else BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("blocked first", "delivered after", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "bravo db", ("runtime recovery", "message seen"), GREEN, p >= 0.62)

    render_sequence(
        "ufield-comms-runtime-range.gif",
        "uField runtime range extend",
        "runtime mail widens the gate and communication recovers",
        "uFldNodeComms",
        ((0.12, "blocked", ORANGE), (0.48, "extend", TEAL), (0.62, "deliver", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.62,
    )


def ufield_broker_handshake() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "broker ping", ("NODE_BROKER_PING", "vname=alpha", "shore hears"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("NODE_BROKER_ACK", "UFSB_NODE_COUNT=1", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "shore broker", ("ack pending", "node count"), GREEN, p >= 0.58)

    render_sequence(
        "ufield-broker-handshake.gif",
        "uField broker handshake",
        "node broker registration creates a shore-side ack",
        "uFldShoreBroker",
        ((0.12, "ping", BLUE), (0.38, "register", TEAL), (0.58, "ack", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.58,
    )


def ufield_broker_bridge_tokens() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "bridge config", ("src=$[NODE]", "dest=$[VNAME]", "qbridge"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("PSHARE_CMD", "tokens expanded", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "bridge command", ("route created", "vars expanded"), GREEN, p >= 0.58)

    render_sequence(
        "ufield-broker-bridge-tokens.gif",
        "uField bridge tokens",
        "broker bridge templates expand into pShare commands",
        "uFldShoreBroker",
        ((0.12, "template", BLUE), (0.40, "expand", TEAL), (0.58, "PSHARE_CMD", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.58,
    )


def ufield_route_runtime_recovery() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        rows = ("route missing", "delivery blocked") if p < 0.46 else ("TRY_SHORE_HOST", "runtime route", "delivery returns")
        card(draw, check, BOX_INPUT, "route state", rows, ORANGE if p < 0.46 else BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("blocked then sent", "PSHARE_CMD seen", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "shore db", ("message recovered", "route proof"), GREEN, p >= 0.62)

    render_sequence(
        "ufield-route-runtime-recovery.gif",
        "uField runtime route recovery",
        "runtime route mail repairs a missing route path",
        "pShare route",
        ((0.12, "blocked", ORANGE), (0.46, "route mail", TEAL), (0.62, "recover", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.62,
    )


def ufield_route_vnode_discovery() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "shore vnode", ("vname=bravo", "discovery report", "route request"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("NODE_BROKER_ACK", "route discovered", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "route proof", ("vnode known", "ack pending"), GREEN, p >= 0.58)

    render_sequence(
        "ufield-route-vnode-discovery.gif",
        "uField shore vnode discovery",
        "shore discovery supplies the missing virtual-node route",
        "uFldShoreBroker",
        ((0.12, "discover", BLUE), (0.38, "route", TEAL), (0.58, "ack", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.58,
    )


def plogger_explicit_log() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "configured log", ("Log = LOG_ALPHA", "LOG_ALPHA=ready"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "artifact check", (".alog line kept", "LOG_ALPHA=ready", "case pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, ".alog", ("LOG_ALPHA", "ready"), GREEN, p >= 0.58)

    render_sequence(
        "plogger-explicit-log-capture.gif",
        "pLogger explicit log capture",
        "configured variables are retained in the generated alog",
        "pLogger",
        ((0.12, "mail", BLUE), (0.38, "log rule", TEAL), (0.58, ".alog", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.58,
    )


def plogger_wildcard_omit() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "wildcard log", ("Log = WILD_*", "omit WILD_SKIP", "keep WILD_KEEP"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "artifact check", ("WILD_KEEP kept", "WILD_SKIP absent", "case pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, ".alog", ("keep matched", "skip omitted"), GREEN, p >= 0.58)

    render_sequence(
        "plogger-wildcard-omit.gif",
        "pLogger wildcard omit",
        "wildcard capture keeps allowed mail and omits filtered mail",
        "pLogger",
        ((0.12, "wildcard", BLUE), (0.40, "omit", ORANGE), (0.58, ".alog", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.58,
    )


def pantler_alias_launch() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "ANTLER run", ("uTimerScript", "MOOSName=alias", "posts flag"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("ALIAS_STARTED", "from alias app", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "MOOSDB mail", ("alias flag", "waiting"), GREEN, p >= 0.58)

    render_sequence(
        "pantler-alias-launch.gif",
        "pAntler alias launch",
        "an aliased app starts and emits the graded flag",
        "pAntler",
        ((0.12, "launch", BLUE), (0.38, "alias", TEAL), (0.58, "flag", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.58,
    )


def pantler_multi_alias_launch() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "ANTLER runs", ("alias_one", "alias_two", "same binary"), BLUE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("ALIAS_ONE=true", "ALIAS_TWO=true", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "MOOSDB mail", ("two flags", "both aliases"), GREEN, p >= 0.58)

    render_sequence(
        "pantler-multi-alias-launch.gif",
        "pAntler multi-alias launch",
        "two aliases of one binary publish independent proof flags",
        "pAntler",
        ((0.12, "launch", BLUE), (0.38, "aliases", TEAL), (0.58, "flags", GREEN), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.58,
    )


def testfailure_burn_gap() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "behavior trigger", ("duration complete", "failure_type=burn", "burn_time=2.0"), ORANGE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "graded output", ("ITER_GAP large", "pHelmIvP alive", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "helm evidence", ("PHELMIVP_ITER_GAP", "waiting"), RED, p >= 0.58)

    render_sequence(
        "testfailure-burn-gap.gif",
        "BHV_TestFailure burn gap",
        "completion-triggered busy wait creates measurable helm stall evidence",
        "pHelmIvP",
        ((0.12, "complete", BLUE), (0.38, "burn", ORANGE), (0.58, "gap", RED), (0.82, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.58,
    )


def testfailure_crash_process_loss() -> None:
    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "behavior trigger", ("duration complete", "failure_type=crash", "process exits"), ORANGE, p >= 0.12)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= 0.82:
            card(draw, check, BOX_OUTPUT, "expected failure", ("pHelmIvP missing", "ProcessWatch false", "case pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "watchdog", ("pHelmIvP", "AWOL pending"), RED, p >= 0.58)

    render_sequence(
        "testfailure-crash-process-loss.gif",
        "BHV_TestFailure crash path",
        "process loss is the expected evidence for the crash case",
        "uProcessWatch",
        ((0.12, "complete", BLUE), (0.38, "crash", ORANGE), (0.58, "AWOL", RED), (0.82, "expected fail", GREEN)),
        inputs,
        outputs,
        output_mark_override=0.58,
    )


def pshare_direct() -> None:
    post_at = 0.12
    route_at = 0.38
    receive_at = 0.62
    pass_at = 0.82

    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "sender mail", ("ROUTE_TEST", "value=alpha"), BLUE, p >= post_at)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= pass_at:
            card(draw, check, BOX_OUTPUT, "receiver grade", ("ROUTE_TEST_RX", "alpha", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "receiver db", ("ROUTE_TEST_RX", "waiting"), GREEN, p >= receive_at)

    render_sequence(
        "pshare-direct-route.gif",
        "pShare direct route",
        "one sender variable arrives under one receiver name",
        "pShare",
        ((post_at, "post", BLUE), (route_at, "route", TEAL), (receive_at, "receive", GREEN), (pass_at, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=receive_at,
    )


def pshare_wildcard() -> None:
    post_at = 0.12
    match_at = 0.38
    receive_at = 0.62
    pass_at = 0.82

    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        card(draw, check, BOX_INPUT, "sender mail", ("WILD_SRC", "matches WILD_*"), BLUE, p >= post_at)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= pass_at:
            card(draw, check, BOX_OUTPUT, "receiver grade", ("WILD_SRC", "wildcard", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "receiver db", ("same var name", "WILD_SRC"), GREEN, p >= receive_at)

    render_sequence(
        "pshare-wildcard-route.gif",
        "pShare wildcard route",
        "a source pattern shares matching mail without renaming",
        "pShare",
        ((post_at, "post", BLUE), (match_at, "match", TEAL), (receive_at, "receive", GREEN), (pass_at, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=receive_at,
    )


def pshare_fanin() -> None:
    frames: list[Image.Image] = []
    check = LayoutCheck()
    alpha_at = 0.12
    alpha_route_at = 0.34
    bravo_at = 0.52
    bravo_route_at = 0.70
    pass_at = 0.86
    markers = (
        (alpha_at, "alpha", BLUE),
        (alpha_route_at, "route", TEAL),
        (bravo_at, "bravo", ORANGE),
        (bravo_route_at, "route", TEAL),
        (pass_at, "pass", GREEN),
    )
    alpha = (64, 224, 274, 314)
    bravo = (64, 344, 274, 434)
    app = (376, 270, 584, 360)
    out = (658, 246, 896, 402)

    for i in range(FRAMES):
        p = i / (FRAMES - 1)
        im, draw = draw_base("pShare fan-in", "two senders route independent mail to one evaluator")
        timeline(draw, check, p, markers)
        card(draw, check, alpha, "alpha peer", ("ALPHA_SRC", "FANIN_ALPHA=alpha"), BLUE, p >= alpha_at)
        card(draw, check, bravo, "bravo peer", ("BRAVO_SRC", "FANIN_BRAVO=bravo"), ORANGE, p >= bravo_at)
        draw.rounded_rectangle(app, radius=10, fill=PANEL if p >= alpha_route_at else PANEL_SOFT, outline=TEAL if p >= alpha_route_at else "#4d566b", width=2 if p >= alpha_route_at else 1)
        center_text(draw, check, (app[0], app[1] + 8, app[2], app[1] + 47), "pShare", TEAL if p >= alpha_route_at else MUTED, FONT_LABEL, "fanin app")
        center_text(draw, check, (app[0], app[1] + 43, app[2], app[3] - 9), "two routes" if p >= bravo_route_at else "routing", MUTED, FONT_SMALL, "fanin state")
        arrow(draw, (alpha[2], 269), (app[0], 298), TEAL, p >= alpha_route_at)
        arrow(draw, (bravo[2], 389), (app[0], 332), TEAL, p >= bravo_route_at)
        arrow(draw, (app[2], 315), (out[0], 315), TEAL, p >= bravo_route_at)
        if p >= pass_at:
            card(draw, check, out, "evaluator grade", ("FANIN_ALPHA=alpha", "FANIN_BRAVO=bravo", "GRADE=pass"), GREEN, True)
        else:
            rows = ("FANIN_ALPHA=alpha", "FANIN_BRAVO waiting") if p < bravo_route_at else ("FANIN_ALPHA=alpha", "FANIN_BRAVO=bravo")
            card(draw, check, out, "evaluator db", rows, GREEN, p >= bravo_route_at)
        frames.append(im)

    save_gif(frames, "pshare-topology-fanin.gif", check)


def pshare_multicast_relay() -> None:
    frames: list[Image.Image] = []
    check = LayoutCheck()
    source_at = 0.12
    multicast_at = 0.34
    proof_at = 0.56
    pass_at = 0.82
    markers = (
        (source_at, "source", BLUE),
        (multicast_at, "multicast", TEAL),
        (proof_at, "relay proof", ORANGE),
        (pass_at, "pass", GREEN),
    )
    source = (64, 260, 274, 356)
    channel = (354, 260, 606, 356)
    shore = (688, 224, 900, 320)
    relay = (688, 356, 900, 452)

    for i in range(FRAMES):
        p = i / (FRAMES - 1)
        im, draw = draw_base("pShare multicast relay proof", "one multicast channel reaches shore and relay")
        timeline(draw, check, p, markers)
        card(draw, check, source, "alpha peer", ("MCAST_SRC", "value=multicast"), BLUE, p >= source_at)
        card(draw, check, channel, "multicast_33", ("224.1.1.11", "shared channel"), TEAL, p >= multicast_at)
        arrow(draw, (source[2], 308), (channel[0], 308), TEAL, p >= multicast_at)
        arrow(draw, (channel[2], 292), (shore[0], 272), TEAL, p >= multicast_at)
        arrow(draw, (channel[2], 330), (relay[0], 404), TEAL, p >= multicast_at)
        if p >= pass_at:
            card(draw, check, shore, "shore grade", ("MCAST_TO_SHORE", "RELAY_SEEN=true", "GRADE=pass"), GREEN, True)
        else:
            rows = ("MCAST_TO_SHORE", "waiting for relay") if p < proof_at else ("MCAST_TO_SHORE", "RELAY_SEEN=true")
            card(draw, check, shore, "shore db", rows, GREEN, p >= multicast_at)
        card(draw, check, relay, "relay peer", ("MCAST_TO_RELAY", "proof flag back"), ORANGE, p >= proof_at)
        frames.append(im)

    save_gif(frames, "pshare-topology-multicast-relay-proof.gif", check)


def pspoofnode_static_spoof() -> None:
    frames: list[Image.Image] = []
    check = LayoutCheck()
    config_at = 0.12
    publish_at = 0.42
    pass_at = 0.78
    markers = (
        (config_at, "config", BLUE),
        (publish_at, "report", TEAL),
        (pass_at, "pass", GREEN),
    )
    map_box = (352, 248, 608, 454)

    for i in range(FRAMES):
        p = i / (FRAMES - 1)
        im, draw = draw_base("pSpoofNode static spoof", "one config spoof becomes one structured NODE_REPORT")
        timeline(draw, check, p, markers)
        card(draw, check, BOX_INPUT, "spoof config", ("name=alpha", "type=heron", "group=red", "hdg=90"), BLUE, p >= config_at)
        draw.rounded_rectangle(map_box, radius=10, fill=PANEL, outline=TEAL if p >= publish_at else "#4d566b", width=2 if p >= publish_at else 1)
        draw_checked_text(draw, check, (map_box[0] + 18, map_box[1] + 16), "pSpoofNode", TEAL if p >= publish_at else MUTED, FONT_LABEL, 160, "pspoof static app")
        draw.line((map_box[0] + 34, map_box[3] - 38, map_box[2] - 34, map_box[3] - 38), fill="#536078", width=1)
        draw.line((map_box[0] + 48, map_box[1] + 54, map_box[0] + 48, map_box[3] - 28), fill="#536078", width=1)
        node_x = map_box[0] + 96
        node_y = map_box[3] - 88
        if p >= publish_at:
            draw.ellipse((node_x - 9, node_y - 9, node_x + 9, node_y + 9), fill=YELLOW, outline=WHITE, width=2)
            draw.line((node_x, node_y, node_x + 42, node_y), fill=YELLOW, width=2)
            draw_checked_text(draw, check, (node_x + 18, node_y - 28), "alpha", WHITE, FONT_SMALL, 72, "alpha label")
        arrow(draw, (BOX_INPUT[2], 369), (map_box[0], 352), TEAL, p >= publish_at)
        arrow(draw, (map_box[2], 352), (BOX_OUTPUT[0], 369), TEAL, p >= publish_at)
        if p >= pass_at:
            card(draw, check, BOX_OUTPUT, "alog check", ("NODE_REPORT", "NAME=alpha", "GROUP=red", "GRADE=pass"), GREEN, True)
        else:
            card(draw, check, BOX_OUTPUT, "node report", ("NAME=alpha", "TYPE=heron", "HDG=90"), GREEN, p >= publish_at)
        frames.append(im)

    save_gif(frames, "pspoofnode-static-spoof.gif", check)


def pspoofnode_cancel_by_name() -> None:
    frames: list[Image.Image] = []
    check = LayoutCheck()
    spoof_at = 0.12
    cancel_at = 0.48
    pass_at = 0.82
    markers = (
        (spoof_at, "two contacts", BLUE),
        (cancel_at, "cancel", ORANGE),
        (pass_at, "pass", GREEN),
    )
    map_box = (352, 238, 608, 458)

    for i in range(FRAMES):
        p = i / (FRAMES - 1)
        im, draw = draw_base("pSpoofNode cancel by name", "SPOOF_CANCEL removes one contact and leaves the other")
        timeline(draw, check, p, markers)
        if p < cancel_at:
            card(draw, check, BOX_INPUT, "runtime mail", ("SPOOF foxtrot", "SPOOF golf"), BLUE, p >= spoof_at)
        else:
            card(draw, check, BOX_INPUT, "runtime mail", ("SPOOF_CANCEL", "vname=foxtrot"), ORANGE, True)
        draw.rounded_rectangle(map_box, radius=10, fill=PANEL, outline=TEAL if p >= spoof_at else "#4d566b", width=2 if p >= spoof_at else 1)
        draw_checked_text(draw, check, (map_box[0] + 18, map_box[1] + 16), "active reports", TEAL if p >= spoof_at else MUTED, FONT_LABEL, 160, "cancel app")
        fox = (map_box[0] + 82, map_box[1] + 96)
        golf = (map_box[0] + 172, map_box[1] + 142)
        if p >= spoof_at:
            fox_fill = "#596175" if p >= cancel_at else YELLOW
            fox_text = MUTED if p >= cancel_at else WHITE
            draw.ellipse((fox[0] - 9, fox[1] - 9, fox[0] + 9, fox[1] + 9), fill=fox_fill, outline=WHITE if p < cancel_at else "#6c7486", width=1)
            draw_checked_text(draw, check, (fox[0] - 20, fox[1] - 32), "foxtrot", fox_text, FONT_SMALL, 72, "foxtrot label")
            if p >= cancel_at:
                draw.line((fox[0] - 18, fox[1] - 18, fox[0] + 18, fox[1] + 18), fill=RED, width=3)
                draw.line((fox[0] + 18, fox[1] - 18, fox[0] - 18, fox[1] + 18), fill=RED, width=3)
            draw.ellipse((golf[0] - 9, golf[1] - 9, golf[0] + 9, golf[1] + 9), fill=GREEN, outline=WHITE, width=1)
            draw_checked_text(draw, check, (golf[0] - 11, golf[1] - 32), "golf", WHITE, FONT_SMALL, 56, "golf label")
        arrow(draw, (BOX_INPUT[2], 369), (map_box[0], 352), TEAL, p >= spoof_at)
        arrow(draw, (map_box[2], 352), (BOX_OUTPUT[0], 369), TEAL, p >= cancel_at)
        if p >= pass_at:
            card(draw, check, BOX_OUTPUT, "alog check", ("absent: foxtrot", "present: golf", "GRADE=pass"), GREEN, True)
        else:
            card(draw, check, BOX_OUTPUT, "after cancel", ("NAME=golf", "foxtrot absent"), GREEN, p >= cancel_at)
        frames.append(im)

    save_gif(frames, "pspoofnode-cancel-by-name.gif", check)


def utermcommand_numeric() -> None:
    typed_at = 0.14
    post_at = 0.48
    pass_at = 0.78

    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        line = "num" if p < post_at else "num + enter"
        card(draw, check, BOX_INPUT, "terminal input", (line, "CMD key match"), BLUE, p >= typed_at)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= pass_at:
            card(draw, check, BOX_OUTPUT, "mission grade", ("TERM_NUM=42", "numeric mail", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "MOOS post", ("TERM_NUM", "42"), GREEN, p >= post_at)

    render_sequence(
        "utermcommand-numeric-command.gif",
        "uTermCommand numeric command",
        "stdin key maps to a numeric MOOS post",
        "uTermCommand",
        ((typed_at, "type", BLUE), (post_at, "post", TEAL), (pass_at, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=post_at,
    )


def utermcommand_arrow_syntax() -> None:
    config_at = 0.12
    input_at = 0.42
    pass_at = 0.78

    def inputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p < input_at:
            card(draw, check, BOX_INPUT, "CMD config", ("arrow", "TERM_ARROW", "ok"), BLUE, p >= config_at)
            return
        card(draw, check, BOX_INPUT, "terminal input", ("arrow + enter", "normalized key"), BLUE, True)

    def outputs(draw: ImageDraw.ImageDraw, check: LayoutCheck, p: float) -> None:
        if p >= pass_at:
            card(draw, check, BOX_OUTPUT, "mission grade", ("TERM_ARROW=ok", "arrow syntax", "GRADE=pass"), GREEN, True)
            return
        card(draw, check, BOX_OUTPUT, "MOOS post", ("TERM_ARROW", "ok"), GREEN, p >= input_at)

    render_sequence(
        "utermcommand-arrow-syntax.gif",
        "uTermCommand arrow syntax",
        "CMD key -> var -> value syntax posts the target value",
        "uTermCommand",
        ((config_at, "config", BLUE), (input_at, "post", TEAL), (pass_at, "pass", GREEN)),
        inputs,
        outputs,
        output_mark_override=input_at,
    )


def psearchgrid_grid_delta() -> None:
    frames: list[Image.Image] = []
    check = LayoutCheck()
    report_at = 0.18
    delta_at = 0.50
    pass_at = 0.80
    markers = (
        (report_at, "report", BLUE),
        (delta_at, "delta", TEAL),
        (pass_at, "pass", GREEN),
    )
    grid_box = (360, 246, 600, 448)

    for i in range(FRAMES):
        p = i / (FRAMES - 1)
        im, draw = draw_base("pSearchGrid delta", "one local NODE_REPORT increments one grid cell")
        timeline(draw, check, p, markers)
        card(draw, check, BOX_INPUT, "input mail", ("NODE_REPORT_LOCAL", "NAME=abe", "X=5,Y=5"), BLUE, p >= report_at)
        draw.rounded_rectangle(grid_box, radius=10, fill=PANEL, outline=TEAL if p >= report_at else "#4d566b", width=2 if p >= report_at else 1)
        draw_checked_text(draw, check, (grid_box[0] + 18, grid_box[1] + 15), "2x2 search grid", TEAL if p >= report_at else MUTED, FONT_LABEL, 170, "delta grid title")
        x0, y0 = grid_box[0] + 48, grid_box[1] + 62
        cell = 58
        for gx in range(3):
            draw.line((x0 + gx * cell, y0, x0 + gx * cell, y0 + 2 * cell), fill="#637087", width=1)
        for gy in range(3):
            draw.line((x0, y0 + gy * cell, x0 + 2 * cell, y0 + gy * cell), fill="#637087", width=1)
        if p >= report_at:
            draw.ellipse((x0 + 20, y0 + 78, x0 + 38, y0 + 96), fill=YELLOW, outline=WHITE, width=1)
        if p >= delta_at:
            draw.rectangle((x0 + 2, y0 + cell + 2, x0 + cell - 2, y0 + 2 * cell - 2), outline=GREEN, width=3)
            draw_checked_text(draw, check, (x0 + 18, y0 + cell + 20), "+1", GREEN, FONT_LABEL, 42, "delta plus")
        arrow(draw, (BOX_INPUT[2], 369), (grid_box[0], 352), TEAL, p >= report_at)
        arrow(draw, (grid_box[2], 352), (BOX_OUTPUT[0], 369), TEAL, p >= delta_at)
        if p >= pass_at:
            card(draw, check, BOX_OUTPUT, "alog check", ("VIEW_GRID_DELTA", "cell x +1", "GRADE=pass"), GREEN, True)
        else:
            card(draw, check, BOX_OUTPUT, "delta mail", ("VIEW_GRID_DELTA", "psg@", ",x,1"), GREEN, p >= delta_at)
        frames.append(im)

    save_gif(frames, "psearchgrid-grid-delta.gif", check)


def psearchgrid_full_grid_report() -> None:
    frames: list[Image.Image] = []
    check = LayoutCheck()
    config_at = 0.12
    report_at = 0.42
    pass_at = 0.80
    markers = (
        (config_at, "full grid", BLUE),
        (report_at, "update", TEAL),
        (pass_at, "pass", GREEN),
    )
    grid_box = (360, 246, 600, 448)

    for i in range(FRAMES):
        p = i / (FRAMES - 1)
        im, draw = draw_base("pSearchGrid full report", "report_deltas=false publishes the full VIEW_GRID state")
        timeline(draw, check, p, markers)
        card(draw, check, BOX_INPUT, "config + mail", ("report_deltas=false", "NODE_REPORT", "X=5,Y=5"), BLUE, p >= config_at)
        draw.rounded_rectangle(grid_box, radius=10, fill=PANEL, outline=TEAL if p >= config_at else "#4d566b", width=2 if p >= config_at else 1)
        draw_checked_text(draw, check, (grid_box[0] + 18, grid_box[1] + 15), "VIEW_GRID state", TEAL if p >= config_at else MUTED, FONT_LABEL, 170, "full grid title")
        x0, y0 = grid_box[0] + 48, grid_box[1] + 62
        cell = 58
        for gx in range(3):
            draw.line((x0 + gx * cell, y0, x0 + gx * cell, y0 + 2 * cell), fill="#637087", width=1)
        for gy in range(3):
            draw.line((x0, y0 + gy * cell, x0 + 2 * cell, y0 + gy * cell), fill="#637087", width=1)
        if p >= report_at:
            draw.rectangle((x0 + 2, y0 + cell + 2, x0 + cell - 2, y0 + 2 * cell - 2), fill="#31553f", outline=GREEN, width=3)
            draw_checked_text(draw, check, (x0 + 18, y0 + cell + 20), "x=1", GREEN, FONT_LABEL, 48, "full grid x")
            draw.ellipse((x0 + 20, y0 + 78, x0 + 38, y0 + 96), fill=YELLOW, outline=WHITE, width=1)
        arrow(draw, (BOX_INPUT[2], 369), (grid_box[0], 352), TEAL, p >= config_at)
        arrow(draw, (grid_box[2], 352), (BOX_OUTPUT[0], 369), TEAL, p >= report_at)
        if p >= pass_at:
            card(draw, check, BOX_OUTPUT, "alog check", ("VIEW_GRID", "cell=0:x:1", "GRADE=pass"), GREEN, True)
        else:
            card(draw, check, BOX_OUTPUT, "full grid mail", ("VIEW_GRID", "cell=0:x:1"), GREEN, p >= report_at)
        frames.append(im)

    save_gif(frames, "psearchgrid-full-grid-report.gif", check)


def main() -> None:
    hostinfo_route()
    hostinfo_invalid()
    loadwatch_near()
    loadwatch_trigger()
    processwatch_mapping()
    processwatch_awol()
    pshare_direct()
    pshare_wildcard()
    pshare_fanin()
    pshare_multicast_relay()
    pspoofnode_static_spoof()
    pspoofnode_cancel_by_name()
    utermcommand_numeric()
    utermcommand_arrow_syntax()
    psearchgrid_grid_delta()
    psearchgrid_full_grid_report()
    upokedb_direct_numeric()
    upokedb_cached_pokes()
    uxms_scoped_variable()
    uxms_history_view()
    uquerydb_numeric_condition()
    uquerydb_checkvar_formats()
    pdeadmanpost_deadman_trip()
    pdeadmanpost_heartbeat_suppression()
    ufield_comms_baseline()
    ufield_comms_runtime_range()
    ufield_broker_handshake()
    ufield_broker_bridge_tokens()
    ufield_route_runtime_recovery()
    ufield_route_vnode_discovery()
    plogger_explicit_log()
    plogger_wildcard_omit()
    pantler_alias_launch()
    pantler_multi_alias_launch()
    testfailure_burn_gap()
    testfailure_crash_process_loss()


if __name__ == "__main__":
    main()
