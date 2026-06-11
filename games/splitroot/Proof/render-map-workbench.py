import argparse
import json
import math
from dataclasses import dataclass, asdict
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


PROJECT_ROOT = Path(__file__).resolve().parents[3]
OUT_DIR = PROJECT_ROOT / "Saved" / "Proof" / "MapWorkbench"

VALLEY_X = 18000.0
VALLEY_Y = 12000.0
PLAYER_HEIGHT = 192.0
COVER_SPACING = 1500.0

COLORS = {
    "ground": (41, 33, 20),
    "cover": (89, 89, 89),
    "splitroot": (115, 80, 38),
    "verdant": (77, 140, 64),
    "verdant_dark": (28, 58, 31),
    "lens": (140, 77, 38),
    "lens_dark": (61, 31, 26),
    "kinwild": (166, 128, 62),
    "kinwild_grey": (125, 138, 140),
    "site": (148, 163, 168),
    "tree": (54, 105, 48),
    "rock": (120, 118, 112),
    "ice": (140, 174, 190),
    "route": (255, 238, 180),
    "hot": (255, 92, 92),
    "warning": (255, 196, 0),
    "white": (236, 236, 220),
    "black": (12, 12, 10),
}

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

ROUTES = [
    ("west_to_center", (-15000.0, 0.0), (-2400.0, 0.0)),
    ("west_to_south_site", (-15000.0, 0.0), (-4000.0, -7000.0)),
    ("lens_to_center", (12000.0, 8500.0), (2000.0, 1400.0)),
    ("kinwild_to_center", (12000.0, -10000.0), (2000.0, -1400.0)),
    ("north_site_to_center", (0.0, 7000.0), (0.0, -3000.0)),
    ("lens_mid_to_center", (2000.0, 1400.0), (0.0, -3000.0)),
    ("kinwild_mid_to_center", (2000.0, -1400.0), (0.0, -3000.0)),
]

ROUTE_SETS = {
    "current": ROUTES,
    "wide_sites": ROUTES,
    "lens_reseat": [
        ("west_to_center", (-15000.0, 0.0), (-2400.0, 0.0)),
        ("west_to_south_site", (-15000.0, 0.0), (-4000.0, -7000.0)),
        ("lens_to_lower_bend", (14500.0, 5200.0), (7800.0, 1200.0)),
        ("lens_lower_bend_to_center", (7800.0, 1200.0), (2000.0, 1400.0)),
        ("kinwild_to_center", (12000.0, -10000.0), (2000.0, -1400.0)),
        ("north_site_to_center", (0.0, 7000.0), (0.0, -3000.0)),
        ("lens_mid_to_center", (2000.0, 1400.0), (0.0, -3000.0)),
        ("kinwild_mid_to_center", (2000.0, -1400.0), (0.0, -3000.0)),
    ],
}


@dataclass
class Placement:
    id: str
    piece: str
    x: float
    y: float
    z: float
    sx: float
    sy: float
    sz: float
    yaw: float = 0.0
    team: int | None = None
    asset_group: str = ""


def clamp(value, low, high):
    return max(low, min(high, value))


def dist(a, b):
    return math.hypot(a[0] - b[0], a[1] - b[1])


def distance_to_segment(point, a, b):
    ab = (b[0] - a[0], b[1] - a[1])
    length_sq = ab[0] * ab[0] + ab[1] * ab[1]
    if length_sq <= 1.0:
        return dist(point, a)
    alpha = clamp(((point[0] - a[0]) * ab[0] + (point[1] - a[1]) * ab[1]) / length_sq, 0.0, 1.0)
    closest = (a[0] + ab[0] * alpha, a[1] + ab[1] * alpha)
    return dist(point, closest)


def push_hot(point, safe_radius):
    x, y = point
    for hx, hy in HOT_ZONES:
        ox, oy = x - hx, y - hy
        d = math.hypot(ox, oy)
        if d < safe_radius:
            dx, dy = (ox / d, oy / d) if d > 1.0 else (1.0, 0.0)
            x, y = hx + dx * safe_radius, hy + dy * safe_radius
    return (
        clamp(x, -VALLEY_X + 500.0, VALLEY_X - 500.0),
        clamp(y, -VALLEY_Y + 500.0, VALLEY_Y - 500.0),
    )


def route_set(variant):
    return ROUTE_SETS.get(variant, ROUTES)


def push_routes(point, safe_radius, routes):
    x, y = point
    for _, a, b in routes:
        ab = (b[0] - a[0], b[1] - a[1])
        length_sq = ab[0] * ab[0] + ab[1] * ab[1]
        if length_sq <= 1.0:
            continue
        alpha = clamp(((x - a[0]) * ab[0] + (y - a[1]) * ab[1]) / length_sq, 0.0, 1.0)
        closest = (a[0] + ab[0] * alpha, a[1] + ab[1] * alpha)
        ox, oy = x - closest[0], y - closest[1]
        d = math.hypot(ox, oy)
        if d < safe_radius:
            if d > 1.0:
                dx, dy = ox / d, oy / d
            else:
                pdx, pdy = -ab[1], ab[0]
                plen = math.hypot(pdx, pdy)
                dx, dy = pdx / plen, pdy / plen
            x, y = closest[0] + dx * safe_radius, closest[1] + dy * safe_radius
    return (
        clamp(x, -VALLEY_X + 500.0, VALLEY_X - 500.0),
        clamp(y, -VALLEY_Y + 500.0, VALLEY_Y - 500.0),
    )


def keep_decor_out_of_gameplay_reads(point, routes):
    return push_hot(push_routes(push_hot(point, 1800.0), 1500.0, routes), 1800.0)


def push_cover(point):
    return push_hot(point, 1200.0)


def placement(piece_id, piece, loc, scale, yaw=0.0, team=None, asset_group=""):
    return Placement(piece_id, piece, loc[0], loc[1], loc[2], scale[0], scale[1], scale[2], yaw, team, asset_group)


def append_cover_lane(layout, prefix, start, end):
    dx, dy = end[0] - start[0], end[1] - start[1]
    length = math.hypot(dx, dy)
    steps = max(2, math.ceil(length / COVER_SPACING))
    direction = (dx / length, dy / length)
    perp = (-direction[1], direction[0])
    for step in range(1, steps):
        alpha = step / steps
        jitter = 400.0 if step % 2 == 0 else -400.0
        point = push_cover((start[0] + dx * alpha + perp[0] * jitter, start[1] + dy * alpha + perp[1] * jitter))
        layout.append(placement(f"{prefix}_stone_{step}", "CoverStone", (point[0], point[1], 175.0), (2.4, 2.4, 2.2), 15.0 if jitter > 0 else -15.0, asset_group="snow_rock"))


def append_decor_cluster(layout, prefix, piece, center, radius, count, seed, base_scale, asset_group, routes):
    for index in range(count):
        t = index + 1
        angle = math.radians((t * 137.5 + seed) % 360.0)
        radial = 0.25 + 0.75 * (((t * 37.0 + seed) % 100.0) / 100.0)
        x = clamp(center[0] + math.cos(angle) * radius[0] * radial, -VALLEY_X + 500.0, VALLEY_X - 500.0)
        y = clamp(center[1] + math.sin(angle) * radius[1] * radial, -VALLEY_Y + 500.0, VALLEY_Y - 500.0)
        point = keep_decor_out_of_gameplay_reads((x, y), routes)
        jitter = ((t * 11.0 + seed) % 100.0) / 100.0
        yaw = (t * 73.0 + seed) % 360.0
        s = base_scale + jitter * 0.55
        layout.append(placement(f"{prefix}_{index:02d}", piece, (point[0], point[1], 0.0), (s, s, s), yaw, asset_group=asset_group))


def build_layout(variant):
    routes = route_set(variant)
    layout = []
    layout.append(placement("valley_floor", "GroundPlane", (0, 0, -50), (VALLEY_X / 50, VALLEY_Y / 50, 1), asset_group="loam"))
    layout.append(placement("ridge_north", "RidgeSlab", (0, VALLEY_Y + 600, 850), (2 * VALLEY_X / 100 + 20, 14, 18), asset_group="ice_wall"))
    layout.append(placement("ridge_south", "RidgeSlab", (0, -(VALLEY_Y + 600), 850), (2 * VALLEY_X / 100 + 20, 14, 18), asset_group="ice_wall"))
    layout.append(placement("central_splitroot_trunk", "CentralTreeTrunk", (0, 0, 4500), (12, 12, 90), asset_group="stump"))
    layout.append(placement("central_splitroot_canopy", "CentralTreeCanopy", (0, 0, 9400), (55, 55, 9), asset_group="canopy"))
    for i in range(6):
        yaw = 60.0 * i
        rad = math.radians(yaw)
        layout.append(placement(f"central_splitroot_root_{i}", "RootButtress", (math.cos(rad) * 1500, math.sin(rad) * 1500, 220), (22, 4, 4.4), yaw, asset_group="root"))

    verdant = (-15000, 0, 0)
    lens = (14500, 5200, 0) if variant == "lens_reseat" else (12000, 8500, 0)
    kinwild = (12000, -8500, 0)
    layout.extend([
        placement("verdant_base", "VerdantOutpost", (verdant[0], verdant[1], 200), (1, 1, 1), team=0, asset_group="verdant_outpost"),
        placement("verdant_base_core", "BaseCore", (verdant[0] - 1400, verdant[1] + 1200, 600), (6, 6, 12), team=0, asset_group="verdant_core"),
        placement("verdant_defense_tower", "DefenseTower", (verdant[0] - 200, verdant[1] + 1000, 250), (1.2, 1.2, 5), team=0, asset_group="tower"),
        placement("lenswright_base", "LenswrightOutpost", (lens[0], lens[1], 300), (1, 1, 1), -135, team=1, asset_group="lens_outpost"),
        placement("lenswright_base_core", "BaseCore", (lens[0] + 1400, lens[1] + 1100, 600), (6, 6, 12), team=1, asset_group="lens_core"),
        placement("lenswright_defense_tower", "DefenseTower", (lens[0] + 600, lens[1] + 900, 250), (1.2, 1.2, 5), team=1, asset_group="tower"),
        placement("kinwild_base", "KinwildCamp", (kinwild[0], kinwild[1], 250), (24, 24, 12), 135, team=2, asset_group="tavern_camp"),
    ])

    append_cover_lane(layout, "lane_west", (-12800, 0), (-2400, 0))
    if variant == "lens_reseat":
        append_cover_lane(layout, "lane_northeast", (12800, 4300), (2200, 1300))
    else:
        append_cover_lane(layout, "lane_northeast", (10400, 7200), (2000, 1400))
    append_cover_lane(layout, "lane_southeast", (10400, -7200), (2000, -1400))

    site_positions = {
        "current": [(0, -3000, "resource_site_central"), (0, 7000, "resource_site_north"), (-4000, -7000, "resource_site_south")],
        "wide_sites": [(-700, -2600, "resource_site_central"), (-1800, 6300, "resource_site_north"), (-5300, -6200, "resource_site_south")],
    }
    for x, y, name in site_positions.get(variant, site_positions["current"]):
        layout.append(placement(name, "ResourceSite", (x, y, 30), (12, 12, 0.6), asset_group="nonblocking_fountain"))

    layout.append(placement("valley_map_table", "MapTable", (verdant[0] - 1800, verdant[1] - 1200, 120), (1, 1, 1), team=0, asset_group="map_table"))

    if variant == "wide_sites":
        append_decor_cluster(layout, "deadwood_west", "DecorTree", (-11100, 4200), (4200, 1800), 24, 11, 1.25, "english_oak", routes)
        append_decor_cluster(layout, "deadwood_southwest", "DecorTree", (-7800, -7700), (3300, 1700), 16, 29, 1.05, "english_oak", routes)
        append_decor_cluster(layout, "lens_ruin_stones", "DecorRock", (8600, 6500), (3200, 1900), 22, 43, 1.2, "castle_ruin", routes)
        append_decor_cluster(layout, "north_ice_shelf", "DecorRock", (-4700, 10100), (7600, 1200), 22, 67, 1.5, "ice_wall", routes)
        append_decor_cluster(layout, "kinwild_stonebreak", "DecorRock", (9200, -7700), (3700, 1900), 24, 89, 1.25, "snow_rock", routes)
    elif variant == "lens_reseat":
        append_decor_cluster(layout, "deadwood_west", "DecorTree", (-11100, 4200), (4200, 1800), 24, 11, 1.25, "english_oak", routes)
        append_decor_cluster(layout, "deadwood_southwest", "DecorTree", (-7800, -7700), (3300, 1700), 16, 29, 1.05, "english_oak", routes)
        append_decor_cluster(layout, "lens_ruin_stones", "DecorRock", (10000, 6500), (2900, 1700), 24, 43, 1.25, "castle_ruin", routes)
        append_decor_cluster(layout, "north_ice_shelf", "DecorRock", (-4700, 10100), (7600, 1200), 22, 67, 1.5, "ice_wall", routes)
        append_decor_cluster(layout, "kinwild_stonebreak", "DecorRock", (9200, -7700), (3700, 1900), 24, 89, 1.25, "snow_rock", routes)
    else:
        append_decor_cluster(layout, "deadwood_west", "DecorTree", (-10600, 3500), (4700, 2200), 28, 11, 1.2, "english_oak", routes)
        append_decor_cluster(layout, "deadwood_southwest", "DecorTree", (-7200, -7300), (3600, 2100), 18, 29, 1.0, "english_oak", routes)
        append_decor_cluster(layout, "lens_ruin_stones", "DecorRock", (7600, 6700), (4200, 2500), 26, 43, 1.15, "castle_ruin", routes)
        append_decor_cluster(layout, "north_ice_shelf", "DecorRock", (-3400, 10100), (8500, 1300), 24, 67, 1.45, "ice_wall", routes)
        append_decor_cluster(layout, "kinwild_stonebreak", "DecorRock", (8300, -7200), (4700, 2300), 28, 89, 1.2, "snow_rock", routes)

    layout.append(placement("valley_player_spawn", "PlayerSpawn", (verdant[0] - 2600, verdant[1] - 1600, 180), (1, 1, 1), team=0))
    layout.append(placement("valley_squad_spawn", "SquadSpawn", (verdant[0] - 1800, verdant[1] - 2300, 120), (1, 1, 1), team=0))
    return layout, routes


def score_layout(layout, routes):
    counts = {}
    route_violations = []
    route_hot_violations = []
    hot_violations = []
    asset_counts = {}
    for p in layout:
        counts[p.piece] = counts.get(p.piece, 0) + 1
        if p.asset_group:
            asset_counts[p.asset_group] = asset_counts.get(p.asset_group, 0) + 1
        if p.piece in {"DecorTree", "DecorRock"}:
            nearest_route = min(distance_to_segment((p.x, p.y), a, b) for _, a, b in routes)
            nearest_hot = min(dist((p.x, p.y), h) for h in HOT_ZONES)
            if nearest_route < 1450.0:
                route_violations.append({"id": p.id, "distance": round(nearest_route, 1)})
            if nearest_hot < 1700.0:
                hot_violations.append({"id": p.id, "distance": round(nearest_hot, 1)})
        if p.piece == "CoverStone":
            nearest_hot = min(dist((p.x, p.y), h) for h in HOT_ZONES)
            if nearest_hot < 1100.0:
                hot_violations.append({"id": p.id, "distance": round(nearest_hot, 1)})

    site_distances = {}
    bases = {p.id: p for p in layout if p.piece in {"VerdantOutpost", "LenswrightOutpost", "KinwildCamp"}}
    sites = [p for p in layout if p.piece == "ResourceSite"]
    for base_id, base in bases.items():
        site_distances[base_id] = {site.id: round(dist((base.x, base.y), (site.x, site.y)), 1) for site in sites}

    for route_name, a, b in routes:
        nearest_hot = min(distance_to_segment(h, a, b) for h in HOT_ZONES)
        if nearest_hot < 1800.0:
            route_hot_violations.append({"route": route_name, "distance": round(nearest_hot, 1)})

    lens_base = next((p for p in layout if p.id == "lenswright_base"), None)
    lens_start = (lens_base.x, lens_base.y) if lens_base else (12000.0, 8500.0)
    lens_start_hot = min(dist(lens_start, h) for h in HOT_ZONES)
    score = 100
    score -= len(route_violations) * 5
    score -= len(route_hot_violations) * 10
    score -= len(hot_violations) * 3
    if counts.get("DecorTree", 0) < 36:
        score -= 8
    if counts.get("DecorRock", 0) < 64:
        score -= 8
    if lens_start_hot < 1200.0:
        score -= 6

    return {
        "score": max(score, 0),
        "counts": counts,
        "assetGroups": asset_counts,
        "routeViolations": route_violations,
        "routeHotZoneViolations": route_hot_violations,
        "hotZoneViolations": hot_violations,
        "siteDistances": site_distances,
        "lensBaseDistanceToNearestObservedHotZone": round(lens_start_hot, 1),
        "read": [
            "Decorations stay off primary travel corridors." if not route_violations else "Route corridor violations need map edits.",
            "Primary routes avoid logged stuck zones." if not route_hot_violations else "Primary routes cross logged stuck zones.",
            "Logged stuck hot zones stay open." if not hot_violations else "Hot-zone clearance violations need map edits.",
            "Resource site visuals are modeled as non-blocking; route balance depends on positions, not collision.",
        ],
    }


class Canvas:
    def __init__(self, width, height, margin=70):
        self.width = width
        self.height = height
        self.margin = margin
        self.scale = min((width - margin * 2) / (VALLEY_X * 2), (height - margin * 2) / (VALLEY_Y * 2))
        self.cx = width / 2
        self.cy = height / 2

    def xy(self, x, y):
        return (self.cx + x * self.scale, self.cy - y * self.scale)

    def rect(self, x, y, sx, sy):
        px, py = self.xy(x, y)
        w = max(2, sx * 100 * self.scale)
        h = max(2, sy * 100 * self.scale)
        return (px - w / 2, py - h / 2, px + w / 2, py + h / 2)


def blend(color, alpha):
    return color + (alpha,)


def draw_label(draw, xy, text, font, fill=COLORS["white"], anchor="mm"):
    draw.text(xy, text, fill=fill, font=font, anchor=anchor, stroke_width=2, stroke_fill=COLORS["black"])


def render(layout, score, out_png, routes):
    width, height = 2400, 1600
    image = Image.new("RGB", (width, height), (18, 20, 17))
    overlay = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    odraw = ImageDraw.Draw(overlay)
    c = Canvas(width, height)
    font = ImageFont.load_default()
    big_font = ImageFont.load_default(size=24)

    floor = c.rect(0, 0, VALLEY_X / 50, VALLEY_Y / 50)
    draw.rounded_rectangle(floor, radius=18, fill=COLORS["ground"], outline=(96, 78, 48), width=4)

    for _, a, b in routes:
        ax, ay = c.xy(*a)
        bx, by = c.xy(*b)
        odraw.line((ax, ay, bx, by), fill=blend(COLORS["route"], 82), width=max(18, int(1500 * c.scale)))
        draw.line((ax, ay, bx, by), fill=(195, 174, 105), width=3)

    for hx, hy in HOT_ZONES:
        px, py = c.xy(hx, hy)
        r = 1200 * c.scale
        odraw.ellipse((px - r, py - r, px + r, py + r), fill=blend(COLORS["hot"], 70), outline=blend(COLORS["hot"], 160), width=3)

    image = Image.alpha_composite(image.convert("RGBA"), overlay).convert("RGB")
    draw = ImageDraw.Draw(image)

    for p in layout:
        if p.piece in {"GroundPlane", "PlayerSpawn", "SquadSpawn", "MapTable"}:
            continue
        x, y = c.xy(p.x, p.y)
        if p.piece == "RidgeSlab":
            draw.rounded_rectangle(c.rect(p.x, p.y, p.sx, p.sy), radius=6, fill=COLORS["ice"], outline=(210, 230, 238), width=2)
        elif p.piece == "CentralTreeTrunk":
            r = 580 * c.scale
            draw.ellipse((x - r, y - r, x + r, y + r), fill=COLORS["splitroot"], outline=(180, 125, 62), width=3)
        elif p.piece == "CentralTreeCanopy":
            r = 2600 * c.scale
            draw.ellipse((x - r, y - r, x + r, y + r), fill=COLORS["verdant_dark"], outline=COLORS["verdant"], width=4)
        elif p.piece == "RootButtress":
            draw.rounded_rectangle(c.rect(p.x, p.y, p.sx, p.sy), radius=5, fill=(105, 67, 31), outline=(160, 101, 46))
        elif p.piece == "VerdantOutpost":
            draw.regular_polygon((x, y, 520 * c.scale), 6, rotation=math.radians(30), fill=COLORS["verdant"], outline=(170, 235, 140))
            draw_label(draw, (x, y - 34), "VERDANT", font)
        elif p.piece == "LenswrightOutpost":
            draw.regular_polygon((x, y, 540 * c.scale), 4, rotation=math.radians(45), fill=COLORS["lens"], outline=(225, 176, 107))
            draw_label(draw, (x, y - 34), "LENS", font)
        elif p.piece == "KinwildCamp":
            draw.regular_polygon((x, y, 560 * c.scale), 3, rotation=math.radians(30), fill=COLORS["kinwild"], outline=COLORS["kinwild_grey"])
            draw_label(draw, (x, y + 34), "KINWILD", font)
        elif p.piece == "BaseCore":
            draw.rectangle(c.rect(p.x, p.y, p.sx * 0.8, p.sy * 0.8), fill=(210, 55, 55), outline=(255, 210, 210), width=2)
        elif p.piece == "DefenseTower":
            draw.ellipse(c.rect(p.x, p.y, 5.0, 5.0), fill=(55, 55, 45), outline=(240, 220, 155), width=2)
        elif p.piece == "CoverStone":
            draw.ellipse(c.rect(p.x, p.y, p.sx * 1.2, p.sy * 1.2), fill=COLORS["cover"], outline=(160, 160, 155))
        elif p.piece == "ResourceSite":
            r = 640 * c.scale
            draw.ellipse((x - r, y - r, x + r, y + r), fill=COLORS["site"], outline=(230, 240, 240), width=3)
            draw_label(draw, (x, y), p.id.replace("resource_site_", ""), font, COLORS["black"])
        elif p.piece == "DecorTree":
            r = (210 + p.sx * 60) * c.scale
            draw.ellipse((x - r, y - r, x + r, y + r), fill=COLORS["tree"], outline=(95, 150, 85))
        elif p.piece == "DecorRock":
            color = COLORS["ice"] if p.asset_group == "ice_wall" else COLORS["rock"]
            r = (190 + p.sx * 70) * c.scale
            draw.regular_polygon((x, y, r), 5, rotation=math.radians(p.yaw), fill=color, outline=(170, 170, 165))

    for p in layout:
        if p.piece == "PlayerSpawn":
            x, y = c.xy(p.x, p.y)
            draw.polygon([(x, y - 14), (x - 10, y + 10), (x + 10, y + 10)], fill=(90, 190, 255), outline=(220, 250, 255))
        elif p.piece == "SquadSpawn":
            x, y = c.xy(p.x, p.y)
            draw.rectangle((x - 9, y - 9, x + 9, y + 9), fill=(220, 240, 120), outline=(255, 255, 220))
        elif p.piece == "MapTable":
            x, y = c.xy(p.x, p.y)
            draw.rounded_rectangle((x - 13, y - 9, x + 13, y + 9), radius=3, fill=(80, 160, 255), outline=(220, 245, 255))

    draw_label(draw, (width / 2, 28), f"SPLITROOT Valley Map Workbench - score {score['score']}", big_font)
    legend_y = 58
    for line in score["read"]:
        draw.text((70, legend_y), line, fill=COLORS["white"], font=font)
        legend_y += 18
    draw.text((70, height - 50), "Yellow bands = primary routes, red circles = observed stuck hot zones, icons = bases/objectives/decor.", fill=COLORS["white"], font=font)

    image.save(out_png)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--variant", default="current", choices=["current", "wide_sites", "lens_reseat"])
    parser.add_argument("--out-dir", default=str(OUT_DIR))
    args = parser.parse_args()

    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    layout, routes = build_layout(args.variant)
    score = score_layout(layout, routes)
    result = {
        "variant": args.variant,
        "layoutCount": len(layout),
        "score": score,
        "routes": [{"name": name, "start": start, "end": end} for name, start, end in routes],
        "placements": [asdict(p) for p in layout],
    }
    png_path = out_dir / f"splitroot-valley-{args.variant}.png"
    json_path = out_dir / f"splitroot-valley-{args.variant}.json"
    render(layout, score, png_path, routes)
    json_path.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps({"variant": args.variant, "png": str(png_path), "json": str(json_path), "score": score}, indent=2))


if __name__ == "__main__":
    main()
