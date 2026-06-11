import argparse
import json
import math
import shutil
from datetime import datetime, timezone
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


PROJECT_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_OUT_DIR = PROJECT_ROOT / "Saved" / "Proof" / "MapViews"
WORKBENCH_DIR = PROJECT_ROOT / "Saved" / "Proof" / "MapWorkbench"

VALLEY_X = 18000.0
VALLEY_Y = 12000.0
HOT_ZONE_RADIUS = 1200.0
TOPDOWN_WIDTH = 1800
TOPDOWN_HEIGHT = 1200

HOT_ZONES = [
    (-6000.0, 7000.0),
    (-6000.0, 5000.0),
    (-5000.0, 5000.0),
    (5000.0, 4000.0),
    (5000.0, 5000.0),
    (11000.0, 8000.0),
    (10000.0, 9000.0),
    (12000.0, 9000.0),
]

ROUTE_SETS = {
    "current": [
        ("west_to_center", (-15000.0, 0.0), (-2400.0, 0.0)),
        ("west_to_south_site", (-15000.0, 0.0), (-4000.0, -7000.0)),
        ("lens_to_center", (12000.0, 8500.0), (2000.0, 1400.0)),
        ("kinwild_to_center", (12000.0, -8500.0), (2000.0, -1400.0)),
        ("north_site_to_center", (0.0, 7000.0), (0.0, -3000.0)),
        ("lens_mid_to_center", (2000.0, 1400.0), (0.0, -3000.0)),
        ("kinwild_mid_to_center", (2000.0, -1400.0), (0.0, -3000.0)),
    ],
    "wide_sites": [
        ("west_to_center", (-15000.0, 0.0), (-2400.0, 0.0)),
        ("west_to_south_site", (-15000.0, 0.0), (-4000.0, -7000.0)),
        ("lens_to_center", (12000.0, 8500.0), (2000.0, 1400.0)),
        ("kinwild_to_center", (12000.0, -8500.0), (2000.0, -1400.0)),
        ("north_site_to_center", (0.0, 7000.0), (0.0, -3000.0)),
        ("lens_mid_to_center", (2000.0, 1400.0), (0.0, -3000.0)),
        ("kinwild_mid_to_center", (2000.0, -1400.0), (0.0, -3000.0)),
    ],
    "lens_reseat": [
        ("west_to_center", (-15000.0, 0.0), (-2400.0, 0.0)),
        ("west_to_south_site", (-15000.0, 0.0), (-4000.0, -7000.0)),
        ("lens_to_lower_bend", (14500.0, 5200.0), (7800.0, 1200.0)),
        ("lens_lower_bend_to_center", (7800.0, 1200.0), (2000.0, 1400.0)),
        ("kinwild_to_center", (12000.0, -8500.0), (2000.0, -1400.0)),
        ("north_site_to_center", (0.0, 7000.0), (0.0, -3000.0)),
        ("lens_mid_to_center", (2000.0, 1400.0), (0.0, -3000.0)),
        ("kinwild_mid_to_center", (2000.0, -1400.0), (0.0, -3000.0)),
    ],
}

MARKER_STYLES = {
    "VerdantOutpost": {"label": "VERDANT", "color": (84, 207, 91)},
    "LenswrightOutpost": {"label": "LENS", "color": (230, 159, 71)},
    "KinwildCamp": {"label": "KINWILD", "color": (207, 177, 93)},
    "ResourceSite": {"label": "SITE", "color": (164, 230, 255)},
    "PlayerSpawn": {"label": "PLAYER", "color": (105, 190, 255)},
    "SquadSpawn": {"label": "SQUAD", "color": (235, 238, 117)},
    "MapTable": {"label": "TABLE", "color": (115, 166, 255)},
}


def utc_stamp() -> str:
    return datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S")


def clamp(value: float, low: float, high: float) -> float:
    return max(low, min(high, value))


def font(size: int):
    try:
        return ImageFont.load_default(size=size)
    except TypeError:
        return ImageFont.load_default()


def load_workbench_variant(variant: str) -> dict:
    json_path = WORKBENCH_DIR / f"splitroot-valley-{variant}.json"
    if not json_path.is_file():
        return {"path": str(json_path), "exists": False, "placements": [], "score": None}
    data = json.loads(json_path.read_text(encoding="utf-8"))
    data["path"] = str(json_path)
    data["exists"] = True
    return data


def projection_quad(width: int, height: int) -> dict:
    return {
        "topLeft": (0.27 * width, 0.06 * height),
        "topRight": (0.73 * width, 0.06 * height),
        "bottomLeft": (0.13 * width, 0.97 * height),
        "bottomRight": (0.86 * width, 0.97 * height),
    }


def lerp(a: tuple[float, float], b: tuple[float, float], t: float) -> tuple[float, float]:
    return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t)


def project_world(x: float, y: float, quad: dict) -> tuple[float, float]:
    u = clamp((x + VALLEY_X) / (VALLEY_X * 2.0), 0.0, 1.0)
    v = clamp((VALLEY_Y - y) / (VALLEY_Y * 2.0), 0.0, 1.0)
    top = lerp(quad["topLeft"], quad["topRight"], u)
    bottom = lerp(quad["bottomLeft"], quad["bottomRight"], u)
    return lerp(top, bottom, v)


def world_to_topdown(x: float, y: float, width: int = TOPDOWN_WIDTH, height: int = TOPDOWN_HEIGHT) -> tuple[float, float]:
    u = clamp((x + VALLEY_X) / (VALLEY_X * 2.0), 0.0, 1.0)
    v = clamp((VALLEY_Y - y) / (VALLEY_Y * 2.0), 0.0, 1.0)
    return (u * width, v * height)


def rgba(rgb: tuple[int, int, int], alpha: int, opacity: float) -> tuple[int, int, int, int]:
    return (rgb[0], rgb[1], rgb[2], max(0, min(255, round(alpha * opacity))))


def circle_polygon(cx: float, cy: float, radius: float, quad: dict, samples: int = 48):
    points = []
    for i in range(samples):
        angle = math.tau * i / samples
        points.append(project_world(cx + math.cos(angle) * radius, cy + math.sin(angle) * radius, quad))
    return points


def draw_label(draw: ImageDraw.ImageDraw, xy: tuple[float, float], text: str, fill, label_font):
    bbox = draw.textbbox(xy, text, font=label_font, anchor="mm", stroke_width=1)
    pad = 4
    draw.rounded_rectangle(
        (bbox[0] - pad, bbox[1] - pad, bbox[2] + pad, bbox[3] + pad),
        radius=4,
        fill=(0, 0, 0, 150),
    )
    draw.text(xy, text, font=label_font, fill=fill, anchor="mm", stroke_width=1, stroke_fill=(0, 0, 0, 220))


def make_standard_topdown(base: Image.Image, width: int = TOPDOWN_WIDTH, height: int = TOPDOWN_HEIGHT) -> Image.Image:
    quad = projection_quad(*base.size)
    source_quad = (
        quad["topLeft"][0], quad["topLeft"][1],
        quad["bottomLeft"][0], quad["bottomLeft"][1],
        quad["bottomRight"][0], quad["bottomRight"][1],
        quad["topRight"][0], quad["topRight"][1],
    )
    return base.transform(
        (width, height),
        Image.Transform.QUAD,
        source_quad,
        resample=Image.Resampling.BICUBIC,
    ).convert("RGB")


def draw_topdown_debug_overlay(base: Image.Image, variant: str, opacity: float, workbench: dict) -> Image.Image:
    width, height = base.size
    overlay = Image.new("RGBA", base.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    label_font = font(18)
    small_font = font(13)

    route_color = (255, 225, 94)
    hot_color = (255, 64, 64)
    routes = ROUTE_SETS.get(variant, ROUTE_SETS["current"])
    route_width = max(8, round(width * 0.013))
    center_width = max(2, route_width // 5)

    for hx, hy in HOT_ZONES:
        x, y = world_to_topdown(hx, hy, width, height)
        rx = HOT_ZONE_RADIUS / (VALLEY_X * 2.0) * width
        ry = HOT_ZONE_RADIUS / (VALLEY_Y * 2.0) * height
        draw.ellipse(
            (x - rx, y - ry, x + rx, y + ry),
            fill=rgba(hot_color, 78, opacity),
            outline=rgba(hot_color, 215, opacity),
            width=2,
        )

    for route_name, start, end in routes:
        ax, ay = world_to_topdown(start[0], start[1], width, height)
        bx, by = world_to_topdown(end[0], end[1], width, height)
        draw.line((ax, ay, bx, by), fill=rgba(route_color, 120, opacity), width=route_width)
        draw.line((ax, ay, bx, by), fill=rgba((255, 255, 210), 230, opacity), width=center_width)
        draw.text(
            ((ax + bx) * 0.5, (ay + by) * 0.5 + 15),
            route_name,
            font=small_font,
            fill=rgba(route_color, 230, opacity),
            anchor="mm",
            stroke_width=1,
            stroke_fill=(0, 0, 0, 180),
        )

    for placement in workbench.get("placements", []):
        piece = placement.get("piece", "")
        style = MARKER_STYLES.get(piece)
        if not style:
            continue
        x, y = world_to_topdown(float(placement["x"]), float(placement["y"]), width, height)
        color = style["color"]
        marker_radius = 10 if piece != "ResourceSite" else 8
        draw.ellipse(
            (x - marker_radius, y - marker_radius, x + marker_radius, y + marker_radius),
            fill=rgba(color, 215, opacity),
            outline=rgba((255, 255, 255), 240, opacity),
            width=2,
        )
        if piece in {"VerdantOutpost", "LenswrightOutpost", "KinwildCamp", "ResourceSite", "MapTable"}:
            label = style["label"]
            if piece == "ResourceSite":
                label = placement.get("id", "site").replace("resource_site_", "").upper()
            draw_label(draw, (x, y - marker_radius - 18), label, rgba(color, 245, opacity), label_font)

    title = f"STANDARD TOP-DOWN DEBUG - {variant} - opacity {opacity:.2f}"
    draw.rectangle((16, 14, 16 + len(title) * 9 + 18, 48), fill=(0, 0, 0, round(145 * opacity)))
    draw.text((26, 23), title, font=label_font, fill=rgba((255, 255, 255), 245, opacity))
    return Image.alpha_composite(base.convert("RGBA"), overlay).convert("RGB")


def draw_debug_overlay(base: Image.Image, variant: str, opacity: float, workbench: dict) -> Image.Image:
    width, height = base.size
    quad = projection_quad(width, height)
    overlay = Image.new("RGBA", base.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    label_font = font(max(12, min(18, width // 75)))
    small_font = font(max(10, min(14, width // 95)))

    routes = ROUTE_SETS.get(variant, ROUTE_SETS["current"])
    route_color = (255, 225, 94)
    hot_color = (255, 64, 64)
    route_width = max(8, round(min(width, height) * 0.022))
    center_width = max(2, route_width // 6)

    for _, hx, hy in [("hot", x, y) for x, y in HOT_ZONES]:
        poly = circle_polygon(hx, hy, HOT_ZONE_RADIUS, quad)
        draw.polygon(poly, fill=rgba(hot_color, 80, opacity), outline=rgba(hot_color, 210, opacity))

    for route_name, start, end in routes:
        ax, ay = project_world(start[0], start[1], quad)
        bx, by = project_world(end[0], end[1], quad)
        draw.line((ax, ay, bx, by), fill=rgba(route_color, 130, opacity), width=route_width)
        draw.line((ax, ay, bx, by), fill=rgba((255, 255, 210), 230, opacity), width=center_width)
        mx, my = ((ax + bx) * 0.5, (ay + by) * 0.5)
        draw.text((mx + 6, my + 6), route_name, font=small_font, fill=rgba(route_color, 215, opacity), anchor="mm")

    for placement in workbench.get("placements", []):
        piece = placement.get("piece", "")
        style = MARKER_STYLES.get(piece)
        if not style:
            continue
        x, y = project_world(float(placement["x"]), float(placement["y"]), quad)
        color = style["color"]
        marker_radius = max(5, round(min(width, height) * 0.01))
        if piece == "ResourceSite":
            marker_radius = max(4, marker_radius - 2)
        draw.ellipse(
            (x - marker_radius, y - marker_radius, x + marker_radius, y + marker_radius),
            fill=rgba(color, 210, opacity),
            outline=rgba((255, 255, 255), 240, opacity),
            width=max(1, marker_radius // 4),
        )
        if piece in {"VerdantOutpost", "LenswrightOutpost", "KinwildCamp", "ResourceSite", "MapTable"}:
            label = style["label"]
            if piece == "ResourceSite":
                label = placement.get("id", "site").replace("resource_site_", "").upper()
            draw_label(draw, (x, y - marker_radius - 12), label, rgba(color, 240, opacity), label_font)

    title = f"debug overlay opacity {opacity:.2f} - {variant}"
    draw.rectangle((12, 12, 12 + len(title) * 8 + 18, 42), fill=(0, 0, 0, round(140 * opacity)))
    draw.text((22, 20), title, font=label_font, fill=rgba((255, 255, 255), 245, opacity))
    return Image.alpha_composite(base.convert("RGBA"), overlay).convert("RGB")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--actual", required=True, help="Live in-game screenshot PNG.")
    parser.add_argument("--variant", default="lens_reseat", choices=sorted(ROUTE_SETS))
    parser.add_argument("--debug-opacity", type=float, default=0.35)
    parser.add_argument("--out-dir", default=str(DEFAULT_OUT_DIR))
    parser.add_argument("--name-prefix", default="")
    args = parser.parse_args()

    actual_source = Path(args.actual).resolve()
    if not actual_source.is_file():
        raise FileNotFoundError(actual_source)

    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    opacity = clamp(args.debug_opacity, 0.0, 1.0)
    prefix = args.name_prefix.strip() or actual_source.stem
    stamp = utc_stamp()
    actual_out = out_dir / f"{prefix}-{stamp}-actual.png"
    overlay_out = out_dir / f"{prefix}-{stamp}-debug{round(opacity * 100):02d}.png"
    topdown_out = out_dir / f"{prefix}-{stamp}-standard-topdown.png"
    topdown_debug_out = out_dir / f"{prefix}-{stamp}-standard-topdown-debug{round(opacity * 100):02d}.png"
    json_out = out_dir / f"{prefix}-{stamp}-map-views.json"
    last_json = out_dir / "last-map-views.json"

    shutil.copy2(actual_source, actual_out)
    actual_image = Image.open(actual_source).convert("RGB")
    workbench = load_workbench_variant(args.variant)
    overlay_image = draw_debug_overlay(actual_image, args.variant, opacity, workbench)
    overlay_image.save(overlay_out)
    topdown_image = make_standard_topdown(actual_image)
    topdown_image.save(topdown_out)
    topdown_debug_image = draw_topdown_debug_overlay(topdown_image, args.variant, opacity, workbench)
    topdown_debug_image.save(topdown_debug_out)

    result = {
        "snapshotUtc": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "variant": args.variant,
        "debugOpacity": opacity,
        "actualSourcePath": str(actual_source),
        "actualPath": str(actual_out),
        "debugOverlayPath": str(overlay_out),
        "standardTopDownPath": str(topdown_out),
        "standardTopDownDebugPath": str(topdown_debug_out),
        "jsonPath": str(json_out),
        "projection": {
            "mode": "approx_current_match_view",
            "quadPixels": projection_quad(*actual_image.size),
            "caveat": "Overlay is a diagnostic alignment over real pixels, not an Unreal camera/depth pass.",
        },
        "standardTopDown": {
            "mode": "perspective_rectified_live_render",
            "northUp": True,
            "pixelSize": [TOPDOWN_WIDTH, TOPDOWN_HEIGHT],
            "worldBounds": {
                "minX": -VALLEY_X,
                "maxX": VALLEY_X,
                "minY": -VALLEY_Y,
                "maxY": VALLEY_Y,
            },
            "caveat": "This is derived from live rendered pixels by rectifying the visible valley plane; it is not yet a native orthographic Unreal capture.",
        },
        "routes": [{"name": name, "start": start, "end": end} for name, start, end in ROUTE_SETS[args.variant]],
        "hotZones": [{"center": center, "radius": HOT_ZONE_RADIUS} for center in HOT_ZONES],
        "workbench": {
            "path": workbench.get("path"),
            "exists": workbench.get("exists", False),
            "score": workbench.get("score", {}).get("score") if isinstance(workbench.get("score"), dict) else None,
            "layoutCount": workbench.get("layoutCount"),
        },
    }
    json_text = json.dumps(result, indent=2)
    json_out.write_text(json_text, encoding="utf-8")
    last_json.write_text(json_text, encoding="utf-8")
    print(json_text)


if __name__ == "__main__":
    main()
