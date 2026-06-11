import argparse
import json
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[4]
TOOL_DIR = Path(__file__).resolve().parent

COLORS = {
    "verdant_primary": "#4D8C40",
    "verdant_secondary": "#F5E8C8",
    "verdant_tertiary": "#1C3A1F",
    "verdant_accent": "#A8D060",
    "lens_primary": "#8C4D26",
    "lens_accent": "#5EC8E0",
    "kinwild_primary": "#A6803E",
    "stone": "#595959",
    "earth": "#6F6551",
    "wood": "#735026",
    "sky": "#94A3A8",
    "text": "#F3F0D5",
    "muted": "#AEB99C",
    "warn": "#E6C46B",
    "danger": "#E07B5F",
    "black": "#050806",
}


def rgba(hex_color, alpha=255):
    value = hex_color.lstrip("#")
    return (
        int(value[0:2], 16),
        int(value[2:4], 16),
        int(value[4:6], 16),
        alpha,
    )


def font(size, bold=False):
    names = ["seguisb.ttf", "segoeuib.ttf"] if bold else ["segoeui.ttf", "arial.ttf"]
    for name in names:
        path = Path("C:/Windows/Fonts") / name
        if path.exists():
            return ImageFont.truetype(str(path), size=size)
    return ImageFont.load_default(size=size)


def draw_text(draw, xy, text, size=18, fill=None, bold=False, anchor="la"):
    draw.text(xy, str(text), font=font(size, bold), fill=fill or rgba(COLORS["text"]), anchor=anchor)


def panel(base, box, fill, outline=None, width=1):
    overlay = Image.new("RGBA", base.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(overlay)
    d.rectangle(box, fill=fill, outline=outline, width=width)
    base.alpha_composite(overlay)


def draw_bar(draw, box, ratio, fill, back=(0, 0, 0, 128)):
    x1, y1, x2, y2 = box
    draw.rectangle(box, fill=back, outline=(245, 232, 200, 42))
    clamped = max(0.0, min(1.0, ratio))
    draw.rectangle((x1, y1, x1 + int((x2 - x1) * clamped), y2), fill=fill)


def truncate(text, max_chars):
    text = str(text)
    return text if len(text) <= max_chars else text[: max_chars - 1] + "."


def draw_background(img, state):
    d = ImageDraw.Draw(img, "RGBA")
    w, h = img.size
    d.rectangle((0, 0, w, h), fill=rgba("#102014"))
    d.polygon(
        [(-80, 160), (w + 80, 60), (w + 80, 560), (-80, 640)],
        fill=(45, 74, 43, 120),
    )
    for i in range(-80, w + 120, 48):
        d.line((i, 155, i + 250, 620), fill=(111, 101, 81, 46), width=22)
    d.polygon(
        [(540, 245), (1010, 205), (1085, 460), (485, 505)],
        fill=(12, 26, 17, 92),
        outline=(245, 232, 200, 52),
    )
    for x in range(540, 1060, 48):
        d.line((x, 235, x - 50, 492), fill=(168, 208, 96, 36), width=1)
    for y in range(232, 490, 38):
        d.line((520, y + 35, 1040, y - 4), fill=(168, 208, 96, 34), width=1)

    for event in state.get("worldEvents", []):
        x = int(w * event.get("x", 50) / 100)
        y = int(h * event.get("y", 50) / 100)
        kind = event.get("type", "alert")
        color = COLORS["danger"] if kind == "skirmish" else COLORS["verdant_accent"] if kind == "move" else COLORS["warn"]
        d.polygon([(x, y - 10), (x + 10, y), (x, y + 10), (x - 10, y)], fill=rgba(color, 86), outline=rgba(color, 220))
        draw_text(d, (x + 14, y + 2), event.get("label", ""), 12, fill=rgba(COLORS["verdant_secondary"], 170), bold=True)


def draw_topbar(img, state):
    d = ImageDraw.Draw(img, "RGBA")
    panel(img, (28, 22, 1572, 82), fill=(10, 17, 13, 224), outline=(190, 222, 146, 78))
    d.polygon([(48, 52), (66, 34), (84, 52), (66, 70)], fill=rgba(COLORS["verdant_tertiary"]), outline=rgba(COLORS["verdant_accent"]))
    d.line((55, 52, 77, 52), fill=rgba(COLORS["verdant_secondary"], 220), width=3)
    draw_text(d, (104, 41), "SPLITROOT COMMAND", 23, bold=True)
    draw_text(d, (106, 66), state.get("faction", ""), 12, fill=rgba(COLORS["muted"]), bold=True)

    resources = state.get("resources", {})
    sites = state.get("sites", {})
    items = [
        (655, f"MATCH {state.get('clock', '--:--')}   {state.get('phase', 'UNKNOWN')}", "shared Archon table"),
        (900, f"SITES OWN {sites.get('own', 0)}   ENEMY {sites.get('enemy', 0)}   NEUTRAL {sites.get('neutral', 0)}", "map control income"),
        (
            1218,
            f"GOLD {resources.get('gold', '--')}   LUMBER {resources.get('lumber', '--')}   FOOD {resources.get('foodUsed', '--')}/{resources.get('foodCap', '--')}",
            f"{resources.get('upkeep', '')}   {resources.get('baseAlert', '')}".strip(),
        ),
    ]
    for x, primary, secondary in items:
        draw_text(d, (x, 43), primary, 15, bold=True)
        draw_text(d, (x, 64), secondary, 12, fill=rgba(COLORS["muted"]), bold=True)


def draw_tactical_map(img, state):
    d = ImageDraw.Draw(img, "RGBA")
    box = (44, 92, 1556, 588)
    panel(img, box, fill=(24, 64, 42, 62), outline=(168, 208, 96, 58))
    for x in range(box[0], box[2], 88):
        d.line((x, box[1], x, box[3]), fill=(245, 232, 200, 20), width=1)
    for y in range(box[1], box[3], 64):
        d.line((box[0], y, box[2], y), fill=(245, 232, 200, 20), width=1)

    draw_text(d, (62, 120), "TACTICAL MAP", 21, fill=rgba(COLORS["verdant_secondary"], 242), bold=True)
    visibility = state.get("visibility", {})
    draw_text(
        d,
        (62, 148),
        f"VISION lit={visibility.get('lit', 0)} fog={visibility.get('fog', 0)} black={visibility.get('black', 0)} newlyLit={visibility.get('newlyLit', 0)}",
        13,
        fill=rgba("#D6E5B6", 224),
        bold=True,
    )
    d.line((180, 366, 1390, 248), fill=rgba(COLORS["verdant_accent"], 92), width=3)
    d.line((650, 432, 995, 350), fill=rgba(COLORS["verdant_accent"], 220), width=3)
    d.rectangle((628, 378, 722, 444), fill=(168, 208, 96, 22), outline=rgba(COLORS["verdant_accent"], 220))
    d.rectangle((672, 412, 698, 438), fill=(77, 140, 64, 135), outline=rgba(COLORS["verdant_accent"], 245), width=2)
    d.ellipse((1160, 400, 1186, 426), fill=(140, 77, 38, 148), outline=rgba(COLORS["danger"], 230), width=2)
    for x, y in [(560, 302), (890, 340), (1060, 258)]:
        d.polygon([(x, y - 12), (x + 12, y), (x, y + 12), (x - 12, y)], fill=(89, 89, 89, 118), outline=rgba(COLORS["verdant_secondary"], 190))
    d.line((980, 350, 980, 308), fill=rgba(COLORS["lens_accent"], 230), width=4)
    d.line((980, 308, 1010, 308), fill=rgba(COLORS["lens_accent"], 230), width=4)

    selection = state.get("selection", {})
    panel(img, (1168, 538, 1538, 590), fill=(4, 8, 6, 148), outline=(168, 208, 96, 62))
    draw_text(
        d,
        (1184, 559),
        f"ORDERS {selection.get('orders', 0)}   SEQUENCE {selection.get('sequence', 0)}   {selection.get('name', 'NO SELECTION')}",
        14,
        fill=rgba(COLORS["warn"], 232),
        bold=True,
    )


def draw_minimap(draw, state, x, y):
    cells = state.get("mapCells", [])
    colors = {
        "black": (2, 4, 3, 235),
        "lit": rgba(COLORS["verdant_primary"], 180),
        "fog": (70, 77, 66, 184),
        "own": rgba(COLORS["verdant_accent"], 194),
        "enemy": rgba(COLORS["lens_primary"], 210),
        "neutral": rgba(COLORS["stone"], 184),
    }
    size = 36
    gap = 4
    for index, kind in enumerate(cells[:16]):
        cx = x + (index % 4) * (size + gap)
        cy = y + (index // 4) * (size + gap)
        draw.rectangle((cx, cy, cx + size, cy + size), fill=colors.get(kind, colors["fog"]), outline=(245, 232, 200, 32))
    width = 4 * size + 3 * gap
    height = width
    for signal in state.get("minimapSignals", []):
        sx = x + int(width * signal.get("x", 50) / 100)
        sy = y + int(height * signal.get("y", 50) / 100)
        kind = signal.get("type", "ping")
        if kind == "camera":
            draw.rectangle((sx - 22, sy - 15, sx + 22, sy + 15), outline=rgba(COLORS["lens_accent"], 235), fill=rgba(COLORS["lens_accent"], 26), width=2)
        elif kind == "attack":
            draw.ellipse((sx - 7, sy - 7, sx + 7, sy + 7), outline=rgba(COLORS["danger"], 235), fill=rgba(COLORS["danger"], 68), width=2)
        else:
            draw.rectangle((sx - 6, sy - 6, sx + 6, sy + 6), outline=rgba(COLORS["warn"], 235), fill=rgba(COLORS["warn"], 68), width=2)


def draw_bottom_hud(img, state):
    d = ImageDraw.Draw(img, "RGBA")
    panel(img, (22, 612, 1578, 880), fill=(4, 8, 6, 234), outline=(190, 222, 146, 78))

    # Minimap
    draw_text(d, (38, 634), "MINIMAP", 16, bold=True, fill=rgba(COLORS["verdant_secondary"]))
    d.line((38, 662, 218, 662), fill=(190, 222, 146, 86), width=1)
    draw_minimap(d, state, 38, 676)
    visibility = state.get("visibility", {})
    total = visibility.get("lit", 0) + visibility.get("fog", 0) + visibility.get("black", 0)
    draw_text(d, (38, 838), f"VISION {visibility.get('lit', 0)}/{total}   PINGS {len(state.get('minimapSignals', []))}", 12, fill=rgba(COLORS["muted"]), bold=True)
    for index, group in enumerate(state.get("controlGroups", [])[:4]):
        gx = 38 + (index % 2) * 94
        gy = 846 + (index // 2) * 17
        active = group.get("active", False)
        panel(
            img,
            (gx, gy, gx + 86, gy + 14),
            fill=(77, 140, 64, 116) if active else (22, 34, 26, 166),
            outline=rgba(COLORS["verdant_accent"], 160 if active else 48),
        )
        draw_text(d, (gx + 6, gy + 11), group.get("key", ""), 9, fill=rgba(COLORS["verdant_accent"]), bold=True)
        draw_text(d, (gx + 30, gy + 11), truncate(group.get("label", ""), 8), 9, fill=rgba(COLORS["verdant_secondary"] if active else COLORS["muted"]), bold=True)

    # Selection
    left = 258
    draw_text(d, (left, 634), "SELECTED UNITS", 16, bold=True, fill=rgba(COLORS["verdant_secondary"]))
    d.line((left, 662, 1128, 662), fill=(190, 222, 146, 86), width=1)
    selection = state.get("selection", {})
    draw_text(
        d,
        (left, 688),
        f"{selection.get('name', 'NO SELECTION')}   GROUP {selection.get('controlGroup', '-')}   {selection.get('type', 'selection')}   {selection.get('stance', '')}",
        14,
        bold=True,
    )

    portrait = (left, 708, left + 128, 862)
    panel(img, portrait, fill=(9, 15, 11, 204), outline=rgba(COLORS["verdant_accent"], 72))
    d.rectangle((left + 10, 718, left + 118, 812), fill=(31, 75, 43, 186), outline=rgba(COLORS["verdant_secondary"], 48))
    d.ellipse((left + 42, 732, left + 88, 778), fill=rgba(COLORS["verdant_accent"], 94), outline=rgba(COLORS["verdant_secondary"], 120), width=2)
    d.polygon(
        [(left + 64, 738), (left + 96, 762), (left + 82, 804), (left + 46, 804), (left + 32, 762)],
        fill=(4, 8, 6, 112),
        outline=rgba(COLORS["verdant_secondary"], 130),
    )
    draw_text(d, (left + 10, 831), truncate(selection.get("portrait", selection.get("name", "")), 16), 11, fill=rgba(COLORS["verdant_secondary"]), bold=True)
    draw_text(d, (left + 10, 849), f"LVL {selection.get('level', '-')}", 10, fill=rgba(COLORS["muted"]), bold=True)
    draw_bar(d, (left + 46, 841, left + 118, 847), selection.get("health", 0) / max(1, selection.get("maxHealth", 1)), rgba(COLORS["verdant_accent"]))
    draw_bar(d, (left + 46, 852, left + 118, 858), selection.get("mana", 0) / max(1, selection.get("maxMana", 1)), rgba(COLORS["lens_accent"]))

    stats = [
        ("HP", f"{selection.get('health', 0)}/{selection.get('maxHealth', 0)}"),
        ("MANA", f"{selection.get('mana', 0)}/{selection.get('maxMana', 0)}"),
        ("DMG", selection.get("damage", "-")),
        ("ARMOR", selection.get("armor", "-")),
        ("RANGE", selection.get("range", "-")),
        ("UNITS", selection.get("units", 0)),
        ("TARGET", selection.get("lastTarget", "none")),
        ("SEL", selection.get("selected", 0)),
    ]
    stat_x = left + 140
    for index, (label, value) in enumerate(stats):
        cx = stat_x + (index % 4) * 116
        cy = 708 + (index // 4) * 40
        panel(img, (cx, cy, cx + 108, cy + 34), fill=(5, 10, 7, 138), outline=(245, 232, 200, 26))
        draw_text(d, (cx + 7, cy + 13), label, 9, fill=rgba(COLORS["muted"]), bold=True)
        draw_text(d, (cx + 7, cy + 28), truncate(value, 15), 11, fill=rgba(COLORS["verdant_secondary"]), bold=True)

    units = state.get("units", [])
    card_y = 794
    for index, unit in enumerate(units[:5]):
        x = stat_x + index * 112
        panel(img, (x, card_y, x + 104, card_y + 62), fill=(24, 43, 31, 204), outline=(168, 208, 96, 56))
        draw_text(d, (x + 7, card_y + 8), truncate(unit.get("name", ""), 12), 11, fill=rgba(COLORS["verdant_accent"]), bold=True)
        draw_text(d, (x + 7, card_y + 26), f"{unit.get('role', '')} / {unit.get('state', '')}", 10, fill=rgba(COLORS["muted"]), bold=True)
        draw_bar(d, (x + 7, card_y + 42, x + 96, card_y + 48), unit.get("hp", 0) / max(1, unit.get("maxHp", 1)), rgba(COLORS["verdant_accent"]))
        draw_bar(d, (x + 7, card_y + 53, x + 96, card_y + 59), unit.get("mana", 0) / max(1, unit.get("maxMana", 1)), rgba(COLORS["lens_accent"]))

    panel(img, (left + 630, 708, 1128, 862), fill=(22, 43, 29, 144), outline=(168, 208, 96, 46))
    production = state.get("production", {})
    px = left + 642
    draw_text(d, (px, 730), f"ORDERS {selection.get('orders', 0)}   WIDGET {selection.get('widgetOrders', 0)}   SEQ {selection.get('sequence', 0)}", 12, fill=rgba("#EAEDD2"), bold=True)
    draw_text(d, (px, 754), f"TRAIN {production.get('train', '--')}   COST {production.get('trainCost', '--')}", 12, fill=rgba(COLORS["warn"]), bold=True)
    draw_text(d, (px, 777), f"ARMORY {production.get('armory', '--')}   CROSSBOW {production.get('crossbow', '--')}", 12, fill=rgba(COLORS["warn"]), bold=True)
    for index, item in enumerate(production.get("queue", [])[:2]):
        qx = px
        qy = 800 + index * 28
        panel(img, (qx, qy, qx + 220, qy + 22), fill=(5, 10, 7, 148), outline=(245, 232, 200, 26))
        draw_text(d, (qx + 7, qy + 14), item.get("name", ""), 10, fill=rgba(COLORS["muted"]), bold=True)
        draw_bar(d, (qx + 96, qy + 8, qx + 212, qy + 14), item.get("progress", 0), rgba(COLORS["verdant_accent"]))

    # Command card
    cmd_left = 1160
    draw_text(d, (cmd_left, 634), "COMMAND CARD", 16, bold=True, fill=rgba(COLORS["verdant_secondary"]))
    d.line((cmd_left, 662, 1540, 662), fill=(190, 222, 146, 86), width=1)
    commands = state.get("commands", [])
    for index, command in enumerate(commands[:12]):
        key = command.get("key", "")
        name = command.get("name", "")
        detail = command.get("detail", "")
        enabled = command.get("enabled", True)
        kind = command.get("kind", "order")
        cx = cmd_left + (index % 4) * 96
        cy = 676 + (index // 4) * 58
        alpha = 232 if enabled else 118
        if kind == "hybrid":
            fill = (15, 42, 46, alpha)
            outline = rgba(COLORS["lens_accent"], 98 if enabled else 46)
        elif kind in {"train", "build"}:
            fill = (35, 32, 20, alpha)
            outline = rgba(COLORS["warn"], 88 if enabled else 38)
        else:
            fill = (22, 34, 26, alpha)
            outline = (245, 232, 200, 42)
        panel(img, (cx, cy, cx + 90, cy + 54), fill=fill, outline=outline)
        color = COLORS["verdant_accent"] if enabled else COLORS["muted"]
        draw_text(d, (cx + 7, cy + 15), key, 12, fill=rgba(color, 255 if enabled else 150), bold=True)
        draw_text(d, (cx + 7, cy + 33), truncate(name, 11), 11, fill=rgba(COLORS["text"], 255 if enabled else 150), bold=True)
        draw_text(d, (cx + 7, cy + 49), detail, 10, fill=rgba(COLORS["muted"], 230 if enabled else 132), bold=True)

    log_lines = state.get("commandLog", [])[:3]
    panel(img, (cmd_left, 852, 1540, 872), fill=(4, 8, 6, 148), outline=(245, 232, 200, 26))
    draw_text(d, (cmd_left + 8, 866), "   ".join(log_lines)[:56], 10, fill=rgba(COLORS["muted"]), bold=True)


def render(state, out_path, width, height):
    img = Image.new("RGBA", (width, height), rgba(COLORS["black"]))
    draw_background(img, state)
    draw_topbar(img, state)
    draw_tactical_map(img, state)
    draw_bottom_hud(img, state)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    img.convert("RGB").save(out_path, "PNG")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--state", default=str(TOOL_DIR / "state.json"))
    parser.add_argument("--out", default=str(ROOT / "Saved" / "RtsUiPreview" / "splitroot-rts-ui-preview.png"))
    parser.add_argument("--width", type=int, default=1600)
    parser.add_argument("--height", type=int, default=900)
    args = parser.parse_args()

    state_path = Path(args.state).resolve()
    out_path = Path(args.out).resolve()
    state = json.loads(state_path.read_text(encoding="utf-8"))
    render(state, out_path, args.width, args.height)
    print(json.dumps({
        "ok": True,
        "state": str(state_path),
        "screenshot": str(out_path),
        "width": args.width,
        "height": args.height,
    }, indent=2))


if __name__ == "__main__":
    main()
