#!/usr/bin/env python3
"""Render alog-backed explanatory GIFs for H01 COLREGS classification.

The renderer expects a temp evidence root containing one directory per case,
with original MOOS `.alog` files from the harness run. It reads the first
matching `COLREGS_AVOID_IX_BEN` publication from the shoreside alog, aligns
that timestamp to the vehicle alogs via `LOGSTART`, and interpolates vehicle
`NAV_X`, `NAV_Y`, `NAV_HEADING`, and `NAV_SPEED` from the original logs.
"""

from __future__ import annotations

import argparse
import bisect
import math
import re
from dataclasses import dataclass
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "assets" / "gifs"
W, H = 1600, 900
SCALE = W / 1280
FRAMES = 64
FRAME_MS = 125

BG = "#242836"
GRID = "#2d3242"
GRID_STRONG = "#343a4c"
TEXT = "#e9edf5"
MUTED = "#98a2b8"
YELLOW = "#f3ea5f"
RED = "#ef6262"
TEAL = "#38c6b5"
ORANGE = "#d9904f"
BLUE = "#70b9ff"
WHITE_FAINT = "#d9e2f055"

MAP_LEFT, MAP_TOP = int(92 * SCALE), int(88 * SCALE)
MAP_RIGHT, MAP_BOTTOM = W - int(82 * SCALE), H - int(128 * SCALE)


@dataclass(frozen=True)
class CaseSpec:
    slug: str
    title: str
    output: str
    target_ix: int
    expected_mode: str
    bounds: tuple[float, float, float, float]
    subtitle: str


@dataclass
class VehicleData:
    name: str
    logstart: float
    series: dict[str, list[tuple[float, float]]]


@dataclass
class CaseData:
    spec: CaseSpec
    classify_shore_time: float
    classify_abs_time: float
    mode: str
    summary: str
    avdcol_range: str
    abe: VehicleData
    ben: VehicleData


CASES = [
    CaseSpec(
        slug="head_on_colregs_pass",
        title="Head-on Classification",
        output="colregs-classification-head-on.gif",
        target_ix=10,
        expected_mode="headon",
        bounds=(-34.0, 34.0, -138.0, -34.0),
        subtitle="alog: COLREGS_AVOID_IX_BEN=10 / mode=headon",
    ),
    CaseSpec(
        slug="crossing_starboard_giveway_pass",
        title="Crossing Give-way",
        output="colregs-classification-crossing.gif",
        target_ix=20,
        expected_mode="giveway:stern",
        bounds=(-82.0, 56.0, -136.0, -18.0),
        subtitle="alog: COLREGS_AVOID_IX_BEN=20 / mode=giveway:stern",
    ),
]


def px(value: float) -> int:
    return int(round(value * SCALE))


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


FONT_TINY = font(px(15))
FONT_SMALL = font(px(19))
FONT_SMALL_BOLD = font(px(19), True)
FONT_MED = font(px(26), True)


def logstart(path: Path) -> float:
    for line in path.read_text(errors="ignore").splitlines():
        if "LOGSTART" in line:
            return float(line.split()[-1])
    raise ValueError(f"missing LOGSTART in {path}")


def raw_events(path: Path, var: str) -> list[tuple[float, str]]:
    out: list[tuple[float, str]] = []
    with path.open(errors="ignore") as handle:
        for line in handle:
            if not line or line[0] in "%#":
                continue
            parts = line.split(None, 3)
            if len(parts) >= 4 and parts[1] == var:
                try:
                    out.append((float(parts[0]), parts[3].strip()))
                except ValueError:
                    pass
    return out


def numeric_events(path: Path, var: str, log_start: float | None = None) -> list[tuple[float, float]]:
    out: list[tuple[float, float]] = []
    for t, value in raw_events(path, var):
        try:
            numeric = float(value.split()[0])
        except (ValueError, IndexError):
            continue
        out.append(((log_start + t) if log_start is not None else t, numeric))
    return out


def first_target_ix(path: Path, target_ix: int) -> tuple[float, float]:
    for t, value in numeric_events(path, "COLREGS_AVOID_IX_BEN"):
        if abs(value - target_ix) < 1e-6:
            return t, value
    raise ValueError(f"missing COLREGS_AVOID_IX_BEN={target_ix} in {path}")


def last_event_at_or_before(path: Path, var: str, t_limit: float) -> str:
    value = ""
    for t, candidate in raw_events(path, var):
        if t <= t_limit + 0.05:
            value = candidate
    return value


def result_field(case_dir: Path, field: str) -> str:
    result = case_dir / "results.txt"
    if not result.exists():
        return ""
    text = result.read_text(errors="ignore")
    match = re.search(rf"\b{re.escape(field)}=([^ ]+)", text)
    return match.group(1) if match else ""


def find_one(case_dir: Path, pattern: str) -> Path:
    matches = sorted(case_dir.glob(pattern))
    if not matches:
        raise FileNotFoundError(f"{case_dir}: no {pattern}")
    return matches[0]


def vehicle_data(case_dir: Path, name: str) -> VehicleData:
    alog = find_one(case_dir, f"LOG_{name.upper()}_*/*.alog")
    start = logstart(alog)
    series = {
        var: numeric_events(alog, var, start)
        for var in ("NAV_X", "NAV_Y", "NAV_HEADING", "NAV_SPEED")
    }
    return VehicleData(name=name, logstart=start, series=series)


def load_case(evidence_root: Path, spec: CaseSpec) -> CaseData:
    case_dir = evidence_root / spec.slug
    shore = find_one(case_dir, "XLOG_*/*.alog")
    shore_start = logstart(shore)
    classify_shore_time, _ = first_target_ix(shore, spec.target_ix)
    classify_abs_time = shore_start + classify_shore_time
    return CaseData(
        spec=spec,
        classify_shore_time=classify_shore_time,
        classify_abs_time=classify_abs_time,
        mode=last_event_at_or_before(shore, "COLREGS_AVOID_MODE_BEN", classify_shore_time),
        summary=last_event_at_or_before(shore, "COLREGS_SUMMARY_BEN", classify_shore_time),
        avdcol_range=result_field(case_dir, "avdcol_range"),
        abe=vehicle_data(case_dir, "abe"),
        ben=vehicle_data(case_dir, "ben"),
    )


def interp(series: list[tuple[float, float]], t: float) -> float:
    if not series:
        return 0.0
    times = [p[0] for p in series]
    i = bisect.bisect_left(times, t)
    if i <= 0:
        return series[0][1]
    if i >= len(series):
        return series[-1][1]
    t0, v0 = series[i - 1]
    t1, v1 = series[i]
    if t1 == t0:
        return v1
    return v0 + (v1 - v0) * (t - t0) / (t1 - t0)


def state_at(vehicle: VehicleData, t: float) -> dict[str, float]:
    return {
        "x": interp(vehicle.series["NAV_X"], t),
        "y": interp(vehicle.series["NAV_Y"], t),
        "heading": interp(vehicle.series["NAV_HEADING"], t),
        "speed": interp(vehicle.series["NAV_SPEED"], t),
    }


def first_moving_time(vehicle: VehicleData) -> float:
    for t, speed in vehicle.series["NAV_SPEED"]:
        if speed >= 0.5:
            return t
    return vehicle.series["NAV_X"][0][0]


def world_to_screen(p: tuple[float, float], bounds: tuple[float, float, float, float]) -> tuple[float, float]:
    x_min, x_max, y_min, y_max = bounds
    x, y = p
    sx = MAP_LEFT + (x - x_min) / (x_max - x_min) * (MAP_RIGHT - MAP_LEFT)
    sy = MAP_BOTTOM - (y - y_min) / (y_max - y_min) * (MAP_BOTTOM - MAP_TOP)
    return sx, sy


def dashed_line(
    draw: ImageDraw.ImageDraw,
    a: tuple[float, float],
    b: tuple[float, float],
    fill: str,
    width: int = px(2),
    dash: int = px(12),
    gap: int = px(10),
) -> None:
    ax, ay = a
    bx, by = b
    dist = math.hypot(bx - ax, by - ay)
    if dist <= 0:
        return
    ux, uy = (bx - ax) / dist, (by - ay) / dist
    pos = 0.0
    while pos < dist:
        end = min(pos + dash, dist)
        draw.line((ax + ux * pos, ay + uy * pos, ax + ux * end, ay + uy * end), fill=fill, width=width)
        pos += dash + gap


def draw_base(data: CaseData) -> tuple[Image.Image, ImageDraw.ImageDraw]:
    im = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(im, "RGBA")
    for x in range(0, W, px(40)):
        draw.line((x, 0, x, H), fill=GRID, width=1)
    for y in range(0, H, px(40)):
        draw.line((0, y, W, y), fill=GRID, width=1)
    for x in range(0, W, px(200)):
        draw.line((x, 0, x, H), fill=GRID_STRONG, width=1)
    for y in range(0, H, px(200)):
        draw.line((0, y, W, y), fill=GRID_STRONG, width=1)
    draw.text((px(42), px(34)), data.spec.title, fill=TEXT, font=FONT_SMALL_BOLD)
    draw.text((px(42), px(62)), data.spec.subtitle, fill=MUTED, font=FONT_TINY)
    draw.text((W - px(462), H - px(54)), f"H01 {data.spec.slug}", fill=MUTED, font=FONT_TINY)
    return im, draw


def draw_track(
    draw: ImageDraw.ImageDraw,
    vehicle: VehicleData,
    bounds: tuple[float, float, float, float],
    start_t: float,
    now_t: float,
    color: str,
) -> None:
    samples: list[tuple[float, float]] = []
    steps = 28
    for i in range(steps + 1):
        t = start_t + (now_t - start_t) * i / steps
        s = state_at(vehicle, t)
        samples.append(world_to_screen((s["x"], s["y"]), bounds))
    if len(samples) >= 2:
        draw.line(samples, fill=color + "66", width=px(3))


def draw_vehicle(
    draw: ImageDraw.ImageDraw,
    bounds: tuple[float, float, float, float],
    state: dict[str, float],
    name: str,
    color: str,
) -> tuple[float, float]:
    x, y = world_to_screen((state["x"], state["y"]), bounds)
    heading = math.radians(90 - state["heading"])
    tip = (x + math.cos(heading) * px(24), y - math.sin(heading) * px(24))
    left = (x + math.cos(heading + 2.45) * px(17), y - math.sin(heading + 2.45) * px(17))
    right = (x + math.cos(heading - 2.45) * px(17), y - math.sin(heading - 2.45) * px(17))
    draw.polygon([tip, left, right], fill=color, outline="#fff7a8" if color == YELLOW else "#ffd5d5")
    label = f"{name} ({state['x']:.1f},{state['y']:.1f}) h={state['heading']:.0f}"
    draw.text((x + px(18), y - px(8)), label, fill=TEXT, font=FONT_TINY)
    return x, y


def small_tag(draw: ImageDraw.ImageDraw, p: tuple[float, float], text: str, color: str) -> None:
    x, y = p
    bbox = draw.textbbox((0, 0), text, font=FONT_TINY)
    w = bbox[2] - bbox[0] + px(18)
    h = bbox[3] - bbox[1] + px(12)
    draw.rounded_rectangle((x, y, x + w, y + h), radius=px(5), fill="#252b3a", outline=color, width=1)
    draw.text((x + px(9), y + px(6)), text, fill=color, font=FONT_TINY)


def pill(draw: ImageDraw.ImageDraw, xy: tuple[int, int, int, int], title: str, detail: str, ok: bool) -> None:
    outline = TEAL if ok else "#444b5f"
    fill = "#273142" if ok else "#262b39"
    draw.rounded_rectangle(xy, radius=px(8), fill=fill, outline=outline, width=px(2) if ok else 1)
    draw.text((xy[0] + px(18), xy[1] + px(15)), title, fill=TEAL if ok else TEXT, font=FONT_SMALL_BOLD)
    draw.text((xy[0] + px(18), xy[1] + px(46)), detail, fill=MUTED, font=FONT_TINY)


def save_gif(frames: list[Image.Image], name: str) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    frames[0].save(
        OUT_DIR / name,
        save_all=True,
        append_images=frames[1:],
        duration=FRAME_MS,
        loop=0,
        disposal=2,
        optimize=True,
    )


def render_case(data: CaseData) -> None:
    frames: list[Image.Image] = []
    start_t = min(first_moving_time(data.abe), first_moving_time(data.ben))
    classify_t = data.classify_abs_time
    end_t = classify_t + 2.0
    for i in range(FRAMES):
        raw = i / (FRAMES - 1)
        move_phase = min(raw / 0.78, 1.0)
        t = start_t + (classify_t - start_t) * move_phase
        im, draw = draw_base(data)

        draw_track(draw, data.abe, data.spec.bounds, start_t, t, YELLOW)
        draw_track(draw, data.ben, data.spec.bounds, start_t, t, RED)

        abe_state = state_at(data.abe, t)
        ben_state = state_at(data.ben, t)
        ax, ay = draw_vehicle(draw, data.spec.bounds, abe_state, "abe", YELLOW)
        bx, by = draw_vehicle(draw, data.spec.bounds, ben_state, "ben", RED)
        dashed_line(draw, (ax, ay), (bx, by), fill=WHITE_FAINT)

        class_abe = state_at(data.abe, classify_t)
        class_ben = state_at(data.ben, classify_t)
        if raw >= 0.78:
            cax, cay = world_to_screen((class_abe["x"], class_abe["y"]), data.spec.bounds)
            cbx, cby = world_to_screen((class_ben["x"], class_ben["y"]), data.spec.bounds)
            draw.ellipse((cax - px(7), cay - px(7), cax + px(7), cay + px(7)), fill=TEAL)
            draw.ellipse((cbx - px(7), cby - px(7), cbx + px(7), cby + px(7)), fill=TEAL)
            mid = ((cax + cbx) / 2, (cay + cby) / 2)
            range_anchor = (mid[0] + px(12), mid[1] - px(42))
            if data.spec.slug == "crossing_starboard_giveway_pass":
                range_anchor = (mid[0] - px(72), mid[1] + px(18))
            small_tag(draw, range_anchor, f"range={data.avdcol_range}m", TEAL)
            detail = (
                f"t={data.classify_shore_time:.5f} / "
                f"abe=({class_abe['x']:.1f},{class_abe['y']:.1f}) h={class_abe['heading']:.0f} / "
                f"ben=({class_ben['x']:.1f},{class_ben['y']:.1f}) h={class_ben['heading']:.0f}"
            )
            pill(
                draw,
                (px(42), H - px(132), px(760), H - px(44)),
                f"COLREGS_AVOID_IX_BEN = {data.spec.target_ix}  mode={data.mode}",
                detail,
                True,
            )
        else:
            pill(
                draw,
                (px(42), H - px(132), px(520), H - px(44)),
                "waiting for COLREGS classification",
                "vehicles are moving under logged NAV_X/NAV_Y/NAV_HEADING",
                False,
            )
        frames.append(im)
    save_gif(frames, data.spec.output)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", required=True, type=Path)
    args = parser.parse_args()
    for spec in CASES:
        render_case(load_case(args.evidence_root, spec))


if __name__ == "__main__":
    main()
