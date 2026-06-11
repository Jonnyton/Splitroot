# 007 — Re-author M_StandInGround: fix sampler compile error + UV tiling

Goal: the valley floor renders dark/flat. ROOT CAUSE CONFIRMED in
006's run: the material FAILS TO COMPILE — "Sampler type is Linear
Color, should be Color for ... Roughness" — and falls back to the
default material entirely. The Ground037 Roughness texture asset
imported with SRGB=true (color) while the sampler reads linear.

Fix BOTH while in there:
- Set the Ground037_2K-JPG_Roughness texture asset: SRGB=false
  (linear data), and make the sampler type match (Linear Color).
- Add the TexCoord tiling (secondary issue: the floor is a 100uu
  cube scaled to ~400m, one texture repeat across the whole map).

Depends: 006 (do this in 006's lock window or a new one — material
edit touches a loaded package).

Steps (headless Python commandlet, the proven channel — pure
asset-creation runs exit 0 clean):
1. In a build-lock window, rebuild
   /Game/StandIns/Ground/M_StandInGround via MaterialEditingLibrary:
   - TextureCoordinate node, UTiling=VTiling=~80 (≈5m per tile on
     the 400m floor) feeding BOTH samplers (Ground037 Color →
     BaseColor, Ground037 Roughness → Roughness).
   - Keep names/paths identical so the C++ LoadObject keeps working.
2. Map smoke; grep the newest log for
   `ValleyFloorMaterialLoaded loaded=true` (new marker, lands with
   006's build — if 006 hasn't built yet, build first).
3. Fresh quietshot; name the PNG in the Result. Rook reads it for
   the actual visual verdict (tile scale may need one iteration —
   one variable per pass).

Done-when: smoke green, marker present, fresh PNG on disk.

Report-to: move to Coordination\done\ with ## Result.

## Result

Implemented the headless ground-material reauthor and captured the requested proof frame.

- Rebuild script: Saved\Proof\rebuild_ground_material_tiling.py.
- Material proof: Saved\Proof\last-ground-material-tiling.json reports success=True, u_tiling=80.0, _tiling=80.0, roughness srgb=False, roughness compression $(@{color_texture_path=/Game/StandIns/Ground/Ground037_2K-JPG_Color; loaded_classes=; material_path=/Game/StandIns/Ground/M_StandInGround; roughness_compression_property=compression_settings; roughness_sample_properties=; roughness_srgb_property=srgb; roughness_texture_path=/Game/StandIns/Ground/Ground037_2K-JPG_Roughness; roughness_texture_properties=; success=True; texcoord_properties=; tiling=80.0; warnings=System.Object[]}.roughness_texture_properties.compression_settings), roughness sampler $(@{color_texture_path=/Game/StandIns/Ground/Ground037_2K-JPG_Color; loaded_classes=; material_path=/Game/StandIns/Ground/M_StandInGround; roughness_compression_property=compression_settings; roughness_sample_properties=; roughness_srgb_property=srgb; roughness_texture_path=/Game/StandIns/Ground/Ground037_2K-JPG_Roughness; roughness_texture_properties=; success=True; texcoord_properties=; tiling=80.0; warnings=System.Object[]}.roughness_sample_properties.sampler_type).
- Factory smoke: Proof\unreal-map-smoke.ps1 passed with ExitCode=0; it does not load the splitroot valley actor, so it does not emit the floor marker.
- Splitroot smoke: games\splitroot\Proof\map-rotation-smoke.ps1 passed with ExitCode=0, ValleySpawnedViaUrlOptions=True, MatchLoopActive=True, QuitCommandHonored=True.
- Marker present in Saved\Proof\last-map-rotation-smoke.log: $(C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\last-map-rotation-smoke.log:867:[2026.06.11-03.29.29:825][  0]LogTemp: Display: ArchonFactoryCanary: ValleyFloorMaterialLoaded loaded=true asset=M_StandInGround.Line).
- Fresh quietshot: $(C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\PlaytestDrive\ground-material-tiling-quietshot.png.FullName) ($(C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\PlaytestDrive\ground-material-tiling-quietshot.png.Length) bytes, $(C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\PlaytestDrive\ground-material-tiling-quietshot.png.LastWriteTime.ToString('o'))). Render log shows ValleyFloorMaterialLoaded loaded=true and MatchPhase phase=Live; no Default Material will be used / Sampler type is Linear Color fallback line was found in the fresh render log.
- Visual note: the ground no longer renders as the near-black fallback; the frame shows repeated green/brown texture detail across the floor. Tile scale still needs Rook/art-direction judgment.
- Summary artifact: Saved\Proof\last-ground-material-tiling-proof.json.
