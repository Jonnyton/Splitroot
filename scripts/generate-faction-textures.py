"""
Generate per-faction binary PNG textures from the canonical palette.

Spec source: FactoryContracts/faction_palette.json. Per-faction Primary +
Secondary + Tertiary + Accent slot values define the 4 colors used per banner.

Outputs:
  games/splitroot/Content/Textures/Factions/T_Faction_<Name>_Banner.png  (512x768 banner)
  games/splitroot/Content/Textures/Factions/T_Faction_<Name>_HudFill.png (256x32 health-bar fill)
  games/splitroot/Content/Textures/Factions/T_Faction_<Name>_Reticle.png (64x64 reticle)
  games/splitroot/Content/Textures/Factions/T_Neutral_CoverStone.png     (128x128 neutral)

These PNGs are real binary game assets — Unreal auto-imports PNGs as UTexture2D
at the corresponding paths under /Game/Textures/Factions/. The art-direction
polish plan rule "every sourced asset MUST get a tint pass that pulls it into
the faction palette" is satisfied automatically here: the textures are
generated FROM the palette in the first place.

No Lenswright forbidden imagery (no muzzle-flash glow, no firearm silhouettes)
— Lenswright Accent is cyan alchemical glow per faction_palette.json.
"""

from __future__ import annotations

from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
OUT_DIR = ROOT / "games" / "splitroot" / "Content" / "Textures" / "Factions"

# Mirrors FactoryContracts/faction_palette.json sRGB hex values.
FACTIONS = {
    "Verdant": {
        "primary":   (0x4D, 0x8C, 0x40),
        "secondary": (0xF5, 0xE8, 0xC8),
        "tertiary":  (0x1C, 0x3A, 0x1F),
        "accent":    (0xA8, 0xD0, 0x60),
        "motif":     "vertical bars (forest growth, 'choir')",
    },
    "Lenswright": {
        "primary":   (0x8C, 0x4D, 0x26),
        "secondary": (0xD9, 0xA8, 0x6B),
        "tertiary":  (0x3D, 0x1F, 0x1A),
        "accent":    (0x5E, 0xC8, 0xE0),  # cyan alchemical, NOT muzzle-flash
        "motif":     "concentric gear rings (clockwork compact)",
    },
    "Kinwild": {
        "primary":   (0xA6, 0x80, 0x3E),
        "secondary": (0x7D, 0x8A, 0x8C),
        "tertiary":  (0x3B, 0x28, 0x10),
        "accent":    (0x3F, 0x6E, 0x7A),
        "motif":     "diagonal hash marks (pack-hunt tally)",
    },
}

NEUTRAL_COVER_STONE = (0x59, 0x59, 0x59)


def lerp(a: int, b: int, t: float) -> int:
    return int(round(a + (b - a) * t))


def lerp_color(c1, c2, t):
    return (lerp(c1[0], c2[0], t), lerp(c1[1], c2[1], t), lerp(c1[2], c2[2], t))


def make_banner(faction_name: str, palette: dict) -> Image.Image:
    """512x768 faction banner — vertical gradient + motif overlay + accent stripe."""
    W, H = 512, 768
    img = Image.new("RGB", (W, H), palette["tertiary"])
    draw = ImageDraw.Draw(img)

    # Vertical gradient: primary at top -> tertiary at bottom
    for y in range(H):
        t = y / (H - 1)
        c = lerp_color(palette["primary"], palette["tertiary"], t)
        draw.line([(0, y), (W - 1, y)], fill=c)

    # Faction-specific motif overlay
    if faction_name == "Verdant":
        # Vertical bars (choir / growth)
        bar_w = 24
        spacing = 80
        for x in range(spacing, W - spacing, spacing):
            draw.rectangle([x, 120, x + bar_w, H - 120], fill=palette["secondary"])
            # Accent tip
            draw.rectangle([x, 120, x + bar_w, 130], fill=palette["accent"])
    elif faction_name == "Lenswright":
        # Concentric gear rings (3 rings)
        cx, cy = W // 2, H // 2
        for radius, width in [(180, 18), (130, 14), (80, 10)]:
            draw.ellipse(
                [cx - radius, cy - radius, cx + radius, cy + radius],
                outline=palette["secondary"], width=width,
            )
        # Cyan accent center dot (alchemical glow, NOT muzzle-flash)
        draw.ellipse([cx - 28, cy - 28, cx + 28, cy + 28], fill=palette["accent"])
    elif faction_name == "Kinwild":
        # Diagonal hash marks (pack-hunt tally)
        for i in range(-10, 30):
            x_start = i * 50
            draw.line(
                [(x_start, 80), (x_start + 200, H - 80)],
                fill=palette["secondary"], width=8,
            )
        # Accent ritual mark in center
        cx, cy = W // 2, H // 2
        draw.ellipse([cx - 50, cy - 50, cx + 50, cy + 50], fill=palette["accent"])
        draw.ellipse([cx - 30, cy - 30, cx + 30, cy + 30], fill=palette["tertiary"])

    # Top accent stripe (faction identification at distance)
    draw.rectangle([0, 0, W, 60], fill=palette["accent"])
    # Bottom accent stripe
    draw.rectangle([0, H - 60, W, H], fill=palette["accent"])

    return img


def make_hud_fill(palette: dict) -> Image.Image:
    """256x32 HUD bar fill — Tertiary base, Primary fill mid, Accent highlight on top."""
    W, H = 256, 32
    img = Image.new("RGB", (W, H), palette["tertiary"])
    draw = ImageDraw.Draw(img)
    # Primary horizontal fill, slight gradient
    for x in range(W):
        t = x / (W - 1)
        c = lerp_color(palette["primary"], palette["secondary"], t * 0.3)
        draw.line([(x, 4), (x, H - 4)], fill=c)
    # Accent highlight at top edge
    draw.line([(0, 2), (W - 1, 2)], fill=palette["accent"])
    return img


def make_reticle(palette: dict) -> Image.Image:
    """64x64 reticle — transparent center + colored crosshair + accent dot."""
    W, H = 64, 64
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    cx, cy = W // 2, H // 2
    primary_rgba = palette["primary"] + (255,)
    accent_rgba = palette["accent"] + (255,)
    # 4 ticks at N/E/S/W
    tick_len = 14
    tick_width = 3
    for dx, dy in [(0, -1), (1, 0), (0, 1), (-1, 0)]:
        x0 = cx + dx * 8 - (tick_width // 2 if dx == 0 else 0)
        y0 = cy + dy * 8 - (tick_width // 2 if dy == 0 else 0)
        x1 = cx + dx * (8 + tick_len) + (tick_width // 2 if dx == 0 else 0)
        y1 = cy + dy * (8 + tick_len) + (tick_width // 2 if dy == 0 else 0)
        x0, y0, x1, y1 = sorted([x0, x1])[0], sorted([y0, y1])[0], sorted([x0, x1])[1], sorted([y0, y1])[1]
        # ensure 1-pixel min thickness
        if x1 == x0:
            x1 = x0 + tick_width
        if y1 == y0:
            y1 = y0 + tick_width
        draw.rectangle([x0, y0, x1, y1], fill=primary_rgba)
    # Center accent dot
    draw.ellipse([cx - 2, cy - 2, cx + 2, cy + 2], fill=accent_rgba)
    return img


def make_neutral_cover_stone() -> Image.Image:
    """128x128 neutral grey cover-stone tint — used by non-faction blockout actors."""
    W, H = 128, 128
    img = Image.new("RGB", (W, H), NEUTRAL_COVER_STONE)
    draw = ImageDraw.Draw(img)
    # Slight noise/texture suggestion via subtle band variation
    for y in range(0, H, 8):
        c = (NEUTRAL_COVER_STONE[0] + (y % 16 - 8) // 2,
             NEUTRAL_COVER_STONE[1] + (y % 16 - 8) // 2,
             NEUTRAL_COVER_STONE[2] + (y % 16 - 8) // 2)
        draw.rectangle([0, y, W, y + 1], fill=tuple(max(0, min(255, v)) for v in c))
    return img


def main() -> int:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    artifacts = []

    for name, palette in FACTIONS.items():
        banner = make_banner(name, palette)
        banner_path = OUT_DIR / f"T_Faction_{name}_Banner.png"
        banner.save(banner_path, "PNG", optimize=True)
        artifacts.append(banner_path)

        hud = make_hud_fill(palette)
        hud_path = OUT_DIR / f"T_Faction_{name}_HudFill.png"
        hud.save(hud_path, "PNG", optimize=True)
        artifacts.append(hud_path)

        ret = make_reticle(palette)
        ret_path = OUT_DIR / f"T_Faction_{name}_Reticle.png"
        ret.save(ret_path, "PNG", optimize=True)
        artifacts.append(ret_path)

    stone = make_neutral_cover_stone()
    stone_path = OUT_DIR / "T_Neutral_CoverStone.png"
    stone.save(stone_path, "PNG", optimize=True)
    artifacts.append(stone_path)

    print(f"Wrote {len(artifacts)} PNG files to {OUT_DIR}")
    total = 0
    for p in artifacts:
        size = p.stat().st_size
        total += size
        print(f"  {p.name:48s} {size:>8,} bytes")
    print(f"Total: {total:,} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
