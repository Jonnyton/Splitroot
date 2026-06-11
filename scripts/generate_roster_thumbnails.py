from __future__ import annotations

import json
from pathlib import Path

from PIL import Image, ImageOps


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "games" / "splitroot" / "FactoryContracts" / "thumbs" / "generated"
TARGET_SIZE = (1280, 720)


THUMBNAILS = [
    {
        "id": "fab_15e2afd1",
        "title": "Rogue Character Model",
        "source": ROOT / "Content" / "Fab_15e2afd1_Rogue_Character_Model" / "fbx" / "thumbnail.png",
        "output": OUT_DIR / "rogue_character_model.png",
        "crop": (625, 40, 1345, 875),
        "quality": "local_preview_cropped",
        "note": "Cropped from local source preview to remove marketplace text and focus the central body.",
    },
    {
        "id": "fab_f065c0cd",
        "title": "Fantasy Game Character",
        "source": ROOT / "Content" / "Fab_f065c0cd_Fantasy_Game_Character" / "fbx" / "thumbnail.png",
        "output": OUT_DIR / "fantasy_game_character.png",
        "crop": None,
        "quality": "local_preview",
        "note": "Local source preview; render pass still needed before treating it as in-game art evidence.",
    },
    {
        "id": "fab_bf70b6c1",
        "title": "Lizard Kin Warrior Character Rig",
        "source": ROOT
        / "Content"
        / "Fab_bf70b6c1_Lizard_Kin_Warrior_Character_Rig"
        / "glb"
        / "converted"
        / "thumbnail.jpeg",
        "output": OUT_DIR / "lizard_kin_warrior.png",
        "crop": None,
        "quality": "local_preview",
        "note": "Local source preview with readable single-subject silhouette.",
    },
    {
        "id": "fab_c5747f8f",
        "title": "Low Poly wolf",
        "source": ROOT / "Content" / "Fab_c5747f8f_Low_Poly_wolf" / "fbx" / "thumbnail.png",
        "output": OUT_DIR / "low_poly_wolf.png",
        "crop": (530, 380, 1460, 980),
        "quality": "local_preview_cropped",
        "note": "Cropped from local source preview to bring the animal silhouette forward.",
    },
]


def make_thumbnail(item: dict) -> dict:
    source = item["source"]
    output = item["output"]
    if not source.exists():
        return {
            "id": item["id"],
            "title": item["title"],
            "status": "missing_source",
            "source": str(source.relative_to(ROOT)),
        }

    image = Image.open(source).convert("RGB")
    if item["crop"]:
        image = image.crop(item["crop"])
    image = ImageOps.pad(image, TARGET_SIZE, method=Image.Resampling.LANCZOS, color=(31, 34, 34))

    output.parent.mkdir(parents=True, exist_ok=True)
    image.save(output, "PNG", optimize=True)

    return {
        "id": item["id"],
        "title": item["title"],
        "status": "generated",
        "quality": item["quality"],
        "source": str(source.relative_to(ROOT)),
        "output": str(output.relative_to(ROOT)),
        "note": item["note"],
        "size": TARGET_SIZE,
    }


def main() -> int:
    results = [make_thumbnail(item) for item in THUMBNAILS]
    manifest = {
        "schema": "tinyassets.splitroot.roster_generated_thumbnails.v1",
        "purpose": "Static roster-safe thumbnails generated from local source previews without launching Unreal.",
        "target_size": TARGET_SIZE,
        "items": results,
        "blocked_render_notes": [
            {
                "id": "infinity_blade_weapons",
                "reason": "Infinity Blade is approved but absent from project content and local vault inventory; real bow/weapon thumbnails require pack import or a selected substitute mesh.",
            },
            {
                "id": "infinity_blade_warriors",
                "reason": "Approved but absent locally; use source-pack preview only until the actual character meshes are present.",
            },
            {
                "id": "infinity_blade_adversaries",
                "reason": "Approved but absent locally; use source-pack preview only until the actual creature meshes are present.",
            },
            {
                "id": "infinity_blade_effects",
                "reason": "Approved but absent locally; VFX thumbnails require a real Niagara/Cascade or captured in-engine preview later.",
            },
        ],
    }
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUT_DIR / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(manifest, indent=2))
    return 1 if any(item["status"] != "generated" for item in results) else 0


if __name__ == "__main__":
    raise SystemExit(main())
