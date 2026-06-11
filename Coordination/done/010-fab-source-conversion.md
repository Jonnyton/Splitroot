# 010 — Convert priority FAB source packs into UE assets

Goal: 008 landed 42 packs under Content\Fab_* as RAW SOURCE payloads
(fbx/glb/obj/unity) — Unreal ignores non-uasset files, so nothing is
usable yet. Convert the priority picks via the proven headless Python
import channel (ONE import_asset_tasks call per commandlet run, exit 3
with assets-on-disk = success, auto_generate_collision=True,
FBXIT_STATIC_MESH for static things).

Priority order (temp-asset rule: things that re-dress existing game
objects first):
1. FANTASTIC - Village Pack (lists FBX + native Unreal Engine format —
   if it has a .uproject/Content payload inside, just copy that
   instead of importing) — base/settlement buildings.
2. 3 English Oak — better trees than Kenney miniatures.
3. FREE Windmill & Village Buildings Pack + Large Medieval Building +
   free fantasy architecture w2r_16 — structures for sites/outposts.
4. Castle of Rocca Calascio — landmark candidate.
5. Fantasy Game Character + Free Stylized Assassin — characters
   (SKELETAL — import but do NOT wire; rigging/retarget is its own
   slice).
6. Forest ground soil pine — ground material upgrade candidate.

Rules:
- Import into /Game/Fab/<ShortPackName>/ (NOT StandIns).
- Static meshes: auto collision ON. Note triangle counts; flag
  anything absurd (>500k tris) as photogrammetry-heavy.
- Skip Unity-only payloads; list them.
- Build-lock window per run; map smoke after the wave; no
  Failed-to-find lines for /Game/Fab/.

Done-when: priority packs 1-4 have uassets on disk under /Game/Fab/,
smoke green, per-pack inventory (mesh names + tri counts) in the
Result. 5-6 are bonus.

Report-to: move to Coordination\done\ with ## Result.

## Result

Completed 2026-06-10 by Hex.

Implemented:
- Added `Saved/Proof/import_fab_priority_assets.py`.
- Converted the priority source wave into real Unreal assets under `Content/Fab/` (`/Game/Fab/...`).
- Kept the conversion curated, not a full 602-FBX import of FANTASTIC - Village Pack.
- Skipped Unity-only payloads: `Content/Fab_52529a12_FANTASTIC_Village_Pack/fbx/fbx_and_textures_fantast_extracted/3d/collision_unity` (`112` files).

Import behavior:
- Unreal headless import saved each selected asset, then returned exit `3` on the known Slate assertion:
  `Assertion failed: CurrentApplication.IsValid()`.
- I therefore ran one source asset per commandlet run, preserving the rule of one `import_asset_tasks` call per commandlet run.
- Per-asset import logs and summary: `Saved/Proof/FabImport/import-wave-summary.json`.
- Aggregate load/inventory pass exited `0` and wrote: `Saved/Proof/last-fab-priority-import.json`.

Per-pack inventory from `Saved/Proof/last-fab-priority-import.json`:
- FANTASTIC - Village Pack -> `/Game/Fab/FANTASTIC_Village_Pack/`
  - `SM_BLD_base_v01_01` tris `416`, collision `1`
  - `SM_BLD_body_v01_01` tris `1708`, collision `1`
  - `SM_BLD_body_v04_01` tris `1644`, collision `1`
  - `SM_BLD_body_v07_01` tris `2548`, collision `1`
  - `SM_BLD_windmill_sail` tris `788`, collision `1`
  - `SM_PROP_market_v01_01` tris `772`, collision `1`
  - `SM_PROP_wall_wood_gate_frame_01` tris `544`, collision `1`
  - `SM_PROP_watchtower_wood_01` tris `1412`, collision `1`
- 3 English Oak -> `/Game/Fab/English_Oak/`
  - `Object_2` tris `2078`, collision `1`
  - `Object_3` tris `2433`, collision `1`
  - `Object_4` tris `2316`, collision `1`
  - `Object_5` tris `2246`, collision `1`
  - `Object_6` tris `4782`, collision `1`
- FREE Windmill & Village Buildings Pack -> `/Game/Fab/Free_Windmill_Village/`
  - `FreeWindmillVillage_plane` tris `32`, collision `1`
- Large Medieval Building -> `/Game/Fab/Large_Medieval_Building/`
  - `large_medieval_tavern` tris `607039`, collision `1`, photogrammetry-heavy `true`
- free fantasy architecture w2r_16 -> `/Game/Fab/Fantasy_Architecture_W2R16/`
  - `FantasyArchitectureW2R16_plane` tris `32`, collision `1`
- Castle of Rocca Calascio -> `/Game/Fab/Castle_Rocca_Calascio/`
  - `Object_4` tris `1986`, collision `1`
  - `Object_5` tris `664`, collision `1`
  - `Object_6` tris `393`, collision `1`
  - `Object_7` tris `2141`, collision `1`
  - `Object_8` tris `1388`, collision `1`
  - `Object_9` tris `1654`, collision `1`
  - `Object_10` tris `1765`, collision `1`
  - `Object_11` tris `263`, collision `1`
  - `Object_12` tris `892`, collision `1`
  - `Object_13` tris `371`, collision `1`
  - `Object_14` tris `389`, collision `1`
- Bonus Fantasy Game Character -> `/Game/Fab/Fantasy_Game_Character/`
  - `fantasy_game_character_f` imported as `SkeletalMesh`; not wired.
- Bonus Forest ground soil pine -> `/Game/Fab/Forest_Ground_Soil_Pine/`
  - `lesni_hlina` tris `500000`, collision `1`; not flagged heavy because the task threshold is `>500k`.

Proof:
- `python -m py_compile Saved\Proof\import_fab_priority_assets.py` -> exit `0`.
- Inventory commandlet -> exit `0`, `success=true`, no failed required packs, no missing sources.
- `powershell -ExecutionPolicy Bypass -File .\Proof\unreal-map-smoke.ps1` -> exit `0`.
- `/Game/Fab` failure scan of `Saved/Proof/last-unreal-map-smoke.log` found no `/Game/Fab` failed-to-find/load-error lines. The only matched failures were generic missing profiler DLL lines (`aqProf.dll`, `VtuneApi.dll`, `VtuneApi32e.dll`, `WinPixGpuCapturer.dll`).
