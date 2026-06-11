import argparse
import json
import math
import random
from datetime import datetime, timezone
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageOps


PROJECT_ROOT = Path(__file__).resolve().parents[3]
WORKBENCH_DIR = PROJECT_ROOT / "Saved" / "Proof" / "MapWorkbench"
OUT_DIR = PROJECT_ROOT / "Saved" / "Proof" / "MapViews"

VALLEY_X = 18000.0
VALLEY_Y = 12000.0
HOT_ZONE_RADIUS = 1200.0

COLORS = {
    "ink": (10, 12, 10),
    "paper": (43, 38, 26),
    "loam_dark": (24, 28, 19),
    "loam": (60, 51, 34),
    "trail": (138, 117, 71),
    "trail_light": (194, 169, 105),
    "verdant": (58, 103, 55),
    "verdant_dark": (24, 55, 30),
    "lens": (139, 91, 55),
    "lens_dark": (60, 36, 31),
    "kinwild": (143, 112, 57),
    "splitroot": (94, 61, 31),
    "splitroot_light": (169, 118, 57),
    "stone": (122, 124, 116),
    "stone_light": (190, 193, 184),
    "ice": (160, 190, 203),
    "ice_dark": (82, 112, 130),
    "water": (79, 136, 154),
    "route_debug": (255, 225, 94),
    "hot": (255, 58, 58),
    "white": (232, 229, 203),
}


def clamp(value: float, low: float, high: float) -> float:
    return max(low, min(high, value))


class MapCanvas:
    def __init__(self, width: int, height: int, margin: int = 84):
        self.width = width
        self.height = height
        self.margin = margin
        self.inner_w = width - margin * 2
        self.inner_h = height - margin * 2

    def xy(self, x: float, y: float) -> tuple[float, float]:
        u = clamp((x + VALLEY_X) / (VALLEY_X * 2.0), 0.0, 1.0)
        v = clamp((VALLEY_Y - y) / (VALLEY_Y * 2.0), 0.0, 1.0)
        return (self.margin + u * self.inner_w, self.margin + v * self.inner_h)

    def radius_x(self, world_radius: float) -> float:
        return world_radius / (VALLEY_X * 2.0) * self.inner_w

    def radius_y(self, world_radius: float) -> float:
        return world_radius / (VALLEY_Y * 2.0) * self.inner_h


def font(size: int):
    try:
        return ImageFont.load_default(size=size)
    except TypeError:
        return ImageFont.load_default()


def rgba(rgb, alpha):
    return (rgb[0], rgb[1], rgb[2], alpha)


def bbox_from_points(a, b):
    return (min(a[0], b[0]), min(a[1], b[1]), max(a[0], b[0]), max(a[1], b[1]))


def stable_seed(value: str) -> int:
    total = 0
    for index, char in enumerate(value):
        total = (total + (index + 1) * ord(char)) & 0xFFFFFFFF
    return total


def load_workbench(variant: str) -> dict:
    path = WORKBENCH_DIR / f"splitroot-valley-{variant}.json"
    if not path.is_file():
        raise FileNotFoundError(f"Run render-map-workbench.ps1 first; missing {path}")
    data = json.loads(path.read_text(encoding="utf-8"))
    data["sourcePath"] = str(path)
    return data


def terrain_base(width: int, height: int, seed: int) -> Image.Image:
    random.seed(seed)
    noise = Image.effect_noise((width, height), 90).convert("L").filter(ImageFilter.GaussianBlur(1.2))
    base = ImageOps.colorize(noise, COLORS["loam_dark"], COLORS["loam"]).convert("RGBA")
    wash = Image.new("RGBA", (width, height), rgba((24, 22, 15), 78))
    base.alpha_composite(wash)
    draw = ImageDraw.Draw(base, "RGBA")
    for _ in range(340):
        x = random.randrange(width)
        y = random.randrange(height)
        rx = random.randrange(40, 180)
        ry = random.randrange(18, 80)
        color = random.choice([
            rgba((35, 50, 29), 26),
            rgba((83, 69, 40), 22),
            rgba((16, 24, 18), 32),
            rgba((105, 91, 53), 16),
        ])
        draw.ellipse((x - rx, y - ry, x + rx, y + ry), fill=color)
    return base.filter(ImageFilter.GaussianBlur(0.3))


def draw_label(draw: ImageDraw.ImageDraw, xy, text: str, fill, label_font, anchor="mm"):
    draw.text(
        xy,
        text,
        fill=fill,
        font=label_font,
        anchor=anchor,
        stroke_width=3,
        stroke_fill=rgba(COLORS["ink"], 225),
    )


def draw_route(draw: ImageDraw.ImageDraw, canvas: MapCanvas, start, end, debug=False, opacity=1.0):
    ax, ay = canvas.xy(start[0], start[1])
    bx, by = canvas.xy(end[0], end[1])
    if debug:
        outer = rgba(COLORS["route_debug"], round(120 * opacity))
        inner = rgba((255, 248, 180), round(210 * opacity))
        draw.line((ax, ay, bx, by), fill=outer, width=28)
        draw.line((ax, ay, bx, by), fill=inner, width=5)
    else:
        draw.line((ax, ay, bx, by), fill=rgba((58, 45, 27), 90), width=48)
        draw.line((ax, ay, bx, by), fill=rgba(COLORS["trail"], 150), width=34)
        draw.line((ax, ay, bx, by), fill=rgba(COLORS["trail_light"], 90), width=8)


def draw_jagged_blob(draw: ImageDraw.ImageDraw, center, rx: float, ry: float, sides: int, color, outline=None, seed=0):
    random.seed(seed)
    points = []
    for i in range(sides):
        angle = math.tau * i / sides
        jitter = 0.68 + random.random() * 0.48
        points.append((center[0] + math.cos(angle) * rx * jitter, center[1] + math.sin(angle) * ry * jitter))
    draw.polygon(points, fill=color, outline=outline)


def draw_territory_washes(image: Image.Image, canvas: MapCanvas):
    overlay = Image.new("RGBA", image.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay, "RGBA")
    draw.ellipse(bbox_from_points(canvas.xy(-15000, 0), canvas.xy(-4000, 7000)), fill=rgba(COLORS["verdant_dark"], 55))
    draw.ellipse(bbox_from_points(canvas.xy(4500, 9800), canvas.xy(17300, 2400)), fill=rgba(COLORS["lens_dark"], 58))
    draw.ellipse(bbox_from_points(canvas.xy(3300, -3000), canvas.xy(17800, -11300)), fill=rgba(COLORS["kinwild"], 42))
    cx, cy = canvas.xy(0, 0)
    draw.ellipse((cx - 210, cy - 165, cx + 210, cy + 165), fill=rgba((15, 24, 16), 90))
    image.alpha_composite(overlay)


def draw_clean_map(data: dict, width: int, height: int) -> Image.Image:
    canvas = MapCanvas(width, height)
    image = terrain_base(width, height, seed=7331)
    draw_territory_washes(image, canvas)
    draw = ImageDraw.Draw(image, "RGBA")

    routes = data.get("routes", [])
    for route in routes:
        draw_route(draw, canvas, route["start"], route["end"])

    placements = data.get("placements", [])
    for placement in placements:
        piece = placement["piece"]
        x, y = canvas.xy(placement["x"], placement["y"])
        seed = stable_seed(placement["id"])
        if piece == "RidgeSlab":
            draw_jagged_blob(draw, (x, y), 360, 55, 18, rgba(COLORS["ice"], 168), rgba(COLORS["ice_dark"], 210), seed)
        elif piece == "DecorTree":
            r = 9 + placement["sx"] * 4
            draw.ellipse((x - r, y - r, x + r, y + r), fill=rgba((26, 58, 28), 185), outline=rgba((69, 113, 54), 180))
        elif piece == "DecorRock":
            r = 8 + placement["sx"] * 4
            color = rgba(COLORS["ice"], 190) if placement.get("asset_group") == "ice_wall" else rgba(COLORS["stone"], 190)
            draw_jagged_blob(draw, (x, y), r * 1.35, r, 7, color, rgba(COLORS["stone_light"], 120), seed)
        elif piece == "CoverStone":
            r = 12
            draw_jagged_blob(draw, (x, y), r * 1.4, r, 8, rgba(COLORS["stone"], 210), rgba(COLORS["stone_light"], 160), seed)
        elif piece == "ResourceSite":
            rx = canvas.radius_x(650)
            ry = canvas.radius_y(650)
            draw.ellipse((x - rx, y - ry, x + rx, y + ry), fill=rgba(COLORS["water"], 155), outline=rgba(COLORS["white"], 200), width=3)
            draw.ellipse((x - rx * 0.45, y - ry * 0.45, x + rx * 0.45, y + ry * 0.45), fill=rgba((205, 220, 206), 150))
        elif piece == "CentralTreeCanopy":
            draw.ellipse((x - 155, y - 120, x + 155, y + 120), fill=rgba((16, 57, 26), 220), outline=rgba((91, 142, 74), 190), width=5)
        elif piece == "CentralTreeTrunk":
            draw.ellipse((x - 43, y - 43, x + 43, y + 43), fill=rgba(COLORS["splitroot"], 240), outline=rgba(COLORS["splitroot_light"], 210), width=4)
        elif piece == "RootButtress":
            draw.line((x - 34, y, x + 34, y), fill=rgba(COLORS["splitroot"], 165), width=8)
        elif piece == "VerdantOutpost":
            draw.regular_polygon((x, y, 54), 6, rotation=math.radians(30), fill=rgba(COLORS["verdant"], 235), outline=rgba((165, 220, 132), 230))
        elif piece == "LenswrightOutpost":
            draw.regular_polygon((x, y, 54), 4, rotation=math.radians(45), fill=rgba(COLORS["lens"], 235), outline=rgba((226, 177, 102), 230))
        elif piece == "KinwildCamp":
            draw.regular_polygon((x, y, 58), 3, rotation=math.radians(30), fill=rgba(COLORS["kinwild"], 235), outline=rgba((212, 192, 119), 220))
        elif piece == "BaseCore":
            draw.rectangle((x - 28, y - 28, x + 28, y + 28), fill=rgba((178, 59, 45), 230), outline=rgba((245, 189, 151), 220), width=3)
        elif piece == "DefenseTower":
            draw.ellipse((x - 22, y - 22, x + 22, y + 22), fill=rgba((57, 52, 39), 230), outline=rgba((228, 205, 126), 210), width=3)
        elif piece == "MapTable":
            draw.rounded_rectangle((x - 24, y - 16, x + 24, y + 16), radius=4, fill=rgba((73, 119, 173), 230), outline=rgba((190, 225, 255), 210), width=2)
        elif piece == "PlayerSpawn":
            draw.polygon([(x, y - 18), (x - 14, y + 14), (x + 14, y + 14)], fill=rgba((92, 188, 255), 225), outline=rgba((220, 245, 255), 210))
        elif piece == "SquadSpawn":
            draw.rectangle((x - 12, y - 12, x + 12, y + 12), fill=rgba((227, 229, 113), 220), outline=rgba((255, 255, 215), 200), width=2)

    title_font = font(25)
    label_font = font(20)
    small_font = font(16)
    draw_label(draw, canvas.xy(-15000, 0), "VERDANT", rgba((172, 236, 143), 245), label_font)
    draw_label(draw, canvas.xy(14500, 5200), "LENSWRIGHT", rgba((239, 190, 110), 245), label_font)
    draw_label(draw, canvas.xy(12000, -8500), "KINWILD", rgba((235, 212, 134), 245), label_font)
    draw_label(draw, canvas.xy(0, 0), "CENTRAL SPLITROOT", rgba((202, 155, 88), 245), label_font)
    draw.text((width // 2, 24), "SPLITROOT VALLEY", fill=rgba(COLORS["white"], 235), font=title_font, anchor="ma", stroke_width=3, stroke_fill=rgba(COLORS["ink"], 230))
    draw.text((width - 120, 36), "N", fill=rgba(COLORS["white"], 230), font=label_font, anchor="mm", stroke_width=2, stroke_fill=rgba(COLORS["ink"], 230))
    draw.polygon([(width - 120, 58), (width - 134, 90), (width - 106, 90)], fill=rgba(COLORS["white"], 210), outline=rgba(COLORS["ink"], 230))
    draw.text((width - 36, height - 32), "metadata-derived top-down", fill=rgba((198, 190, 156), 200), font=small_font, anchor="rd")
    draw.rectangle((canvas.margin, canvas.margin, width - canvas.margin, height - canvas.margin), outline=rgba((225, 217, 177), 130), width=3)
    return image.convert("RGB")


def draw_debug_overlay(base: Image.Image, data: dict, opacity: float) -> Image.Image:
    canvas = MapCanvas(base.width, base.height)
    overlay = Image.new("RGBA", base.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay, "RGBA")
    small_font = font(14)
    label_font = font(18)

    for hx, hy in [
        (-6000.0, 7000.0),
        (-6000.0, 5000.0),
        (-5000.0, 5000.0),
        (5000.0, 4000.0),
        (5000.0, 5000.0),
        (11000.0, 8000.0),
        (10000.0, 9000.0),
        (12000.0, 9000.0),
    ]:
        x, y = canvas.xy(hx, hy)
        rx = canvas.radius_x(HOT_ZONE_RADIUS)
        ry = canvas.radius_y(HOT_ZONE_RADIUS)
        draw.ellipse((x - rx, y - ry, x + rx, y + ry), fill=rgba(COLORS["hot"], round(80 * opacity)), outline=rgba(COLORS["hot"], round(220 * opacity)), width=3)

    for route in data.get("routes", []):
        draw_route(draw, canvas, route["start"], route["end"], debug=True, opacity=opacity)
        ax, ay = canvas.xy(route["start"][0], route["start"][1])
        bx, by = canvas.xy(route["end"][0], route["end"][1])
        draw.text(((ax + bx) / 2, (ay + by) / 2 + 17), route["name"], fill=rgba(COLORS["route_debug"], round(230 * opacity)), font=small_font, anchor="mm", stroke_width=1, stroke_fill=rgba(COLORS["ink"], 180))

    for placement in data.get("placements", []):
        if placement["piece"] not in {"PlayerSpawn", "SquadSpawn", "MapTable"}:
            continue
        x, y = canvas.xy(placement["x"], placement["y"])
        draw.ellipse((x - 13, y - 13, x + 13, y + 13), fill=rgba((104, 180, 255), round(210 * opacity)), outline=rgba(COLORS["white"], round(240 * opacity)), width=2)
        draw_label(draw, (x, y - 24), placement["id"].replace("valley_", "").upper(), rgba((180, 220, 255), round(245 * opacity)), label_font)

    title = f"DEBUG OVERLAY opacity {opacity:.2f}"
    draw.rectangle((18, 16, 18 + len(title) * 9 + 18, 50), fill=rgba(COLORS["ink"], round(150 * opacity)))
    draw.text((28, 25), title, fill=rgba(COLORS["white"], round(245 * opacity)), font=label_font)
    return Image.alpha_composite(base.convert("RGBA"), overlay).convert("RGB")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--variant", default="lens_reseat", choices=["current", "wide_sites", "lens_reseat"])
    parser.add_argument("--debug-opacity", type=float, default=0.35)
    parser.add_argument("--width", type=int, default=2400)
    parser.add_argument("--height", type=int, default=1600)
    parser.add_argument("--out-dir", default=str(OUT_DIR))
    parser.add_argument("--name-prefix", default="splitroot-standard-map")
    args = parser.parse_args()

    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)
    data = load_workbench(args.variant)
    opacity = clamp(args.debug_opacity, 0.0, 1.0)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S")
    prefix = f"{args.name_prefix}-{args.variant}-{stamp}"
    clean_path = out_dir / f"{prefix}.png"
    debug_path = out_dir / f"{prefix}-debug{round(opacity * 100):02d}.png"
    json_path = out_dir / f"{prefix}.json"
    last_path = out_dir / "last-standard-map-view.json"

    clean = draw_clean_map(data, args.width, args.height)
    clean.save(clean_path)
    debug = draw_debug_overlay(clean, data, opacity)
    debug.save(debug_path)

    result = {
        "snapshotUtc": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "variant": args.variant,
        "mode": "metadata_derived_standard_topdown",
        "northUp": True,
        "pixelSize": [args.width, args.height],
        "sourceWorkbenchPath": data["sourcePath"],
        "score": data.get("score", {}).get("score"),
        "standardMapPath": str(clean_path),
        "debugMapPath": str(debug_path),
        "debugOpacity": opacity,
        "jsonPath": str(json_path),
    }
    text = json.dumps(result, indent=2)
    json_path.write_text(text, encoding="utf-8")
    last_path.write_text(text, encoding="utf-8")
    print(text)


if __name__ == "__main__":
    main()
