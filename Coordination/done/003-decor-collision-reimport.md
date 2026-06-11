# 003 — Collision re-import wave for decor (trees/rocks walk-through)

Goal: the 48 trees + 16 rocks (Kenney nature-kit) imported without
collision — players/bots walk through them. Re-import with collision.

Steps:
1. Build-lock window required (imports touch loaded packages):
   create Saved\Proof\build-lock.flag, wait for editors to drain
   (see Proof\build-locked.ps1 pattern), do the work, delete flag.
2. Headless Python re-import per the proven pattern (HANDOFF.md "asset
   pipeline" section): ONE import_asset_tasks call per commandlet run,
   options.mesh_type_to_import=FBXIT_STATIC_MESH,
   static_mesh_import_data.auto_generate_collision=True.
   FBX sources: Kenney nature-kit (re-download via direct zip if the
   old tmp extraction is gone — see memory reference_ue5_fast_production.md).
   Exit 3 with assets-on-disk = SUCCESS (post-save Slate assert).
3. While in the lock window, also clean the table packages: delete the
   dead /Game/StandIns/MapTable/wooden_table_02 package (no StaticMesh
   export inside) — do NOT rename 021; C++ now references
   wooden_table_021 directly.
4. Bonus if time: grass/flower/bush scatter FBXs from nature-kit into
   /Game/StandIns/Nature/.

Done-when: a map smoke (Proof\unreal-map-smoke.ps1) passes and a fresh
session log shows no Failed-to-find for StandIns assets; tree/rock
meshes have collision (spot-check one via asset registry or a PIE walk).

Report-to: move to Coordination\done\ with ## Result.

## Result

Completed under `Saved\Proof\build-lock.flag`; the flag was removed after
each locked window.

- Re-imported the five runtime decor meshes used by the 48 tree + 16 rock
  placements: `tree_pineTallA`, `tree_default`, `tree_detailed`,
  `rock_largeA`, and `cliff_blockHalf_rock`.
- Imported bonus foliage meshes into `/Game/StandIns/Nature/`:
  `grass_large`, `grass_leafsLarge`, `flower_yellowA`, `flower_redA`,
  `flower_purpleA`, `plant_bush`, `plant_bushLarge`, and
  `plant_bushSmall`.
- Each FBX commandlet used one `import_asset_tasks` call and exited `3`
  after saving its package, matching the known UE post-save Slate assert
  pattern. Artifact: `Saved\Proof\last-decor-collision-import-runs.json`.
- Deleted `/Game/StandIns/MapTable/wooden_table_02`. The next smoke exposed
  a stale material dependency from `wooden_table_021`; repaired that mesh to
  use `/Engine/BasicShapes/BasicShapeMaterial` while keeping `021` named as-is
  and `wooden_table_02` deleted. Artifact:
  `Saved\Proof\last-map-table-material-repair.json`.
- Collision verification passed. Artifact:
  `Saved\Proof\last-decor-collision-verify.json` reports `success=true`;
  each of the five runtime tree/rock meshes loaded as `StaticMesh` with
  `convex_elems=1` and `collision_total=1`.
- Final map smoke passed. Artifact:
  `Saved\Proof\last-unreal-map-smoke.json` reports `ExitCode=0` with all
  required map-table/FPS/RTS proof booleans true.
- Fresh final smoke log grep found no `Failed to find`, `Failed to load`,
  `LoadErrors`, or skipped-package lines for StandIns. The only StandIns lines
  were successful `MapTableMeshLoaded loaded=true` lines for
  `/Game/StandIns/MapTable/wooden_table_021.wooden_table_021`.
