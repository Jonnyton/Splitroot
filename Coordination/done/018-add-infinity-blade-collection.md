# 018 - Add approved Infinity Blade Collection packs

Goal: bring Jonathan-approved Infinity Blade Collection content into the
SPLITROOT project as soon as it is locally obtainable. This is a unit/item
asset-source task for Ledger's lane.

Context:
- Jonathan approved the Infinity Blade Collection for ASAP add/import.
- Local source approval ledger:
  `Coordination/asset-manifest.md`.
- Connector page:
  `pages/notes/splitroot-approved-unit-item-asset-sources-2026-06-11.md`.
- Current local check by Ledger on 2026-06-11: no `Infinity`/`Blade` pack
  exists under project `Content/` or
  `C:\ProgramData\Epic\EpicGamesLauncher\VaultCache\`; fresh launcher manifests
  are UE 5.7, Fab UE Plugin, Quixel Bridge, and Fortnite only.
- Legendary CLI has since been installed and authenticated for entitlement
  checks/downloads. Continue with CLI/background work first.
- Do not close, minimize, restart, kill, focus-steal from, or dismiss crash
  dialogs for Jonathan's running SPLITROOT canary while doing this asset task.
  The live game is shared telemetry/playtest infrastructure. If a download,
  import, or build step needs the editor/game closed because files are locked,
  ask Jonathan first and wait.

Approved pack priority:

1. Infinity Blade: Weapons
   - https://www.unrealengine.com/marketplace/en-US/product/infinity-blade-weapons
   - First import priority for item-shop weapon rows.
2. Infinity Blade: Warriors
   - https://www.unrealengine.com/marketplace/en-US/product/infinity-blade-characters
   - Heroic humanoid armor/body options.
3. Infinity Blade: Adversaries
   - https://www.unrealengine.com/marketplace/en-US/product/infinity-blade-enemies
   - Creature/monster bodies, Kinwild and neutral encounter options.
4. Infinity Blade: Effects
   - https://www.unrealengine.com/marketplace/en-US/product/infinity-blade-effects
   - Spell, item, beacon, hit, and tech-building VFX.
5. Infinity Blade: Grass Lands
   - https://www.unrealengine.com/marketplace/en-US/product/infinity-blade-plain-lands
   - Environment dressing if still available.
6. Infinity Blade: Fire Lands
   - https://www.unrealengine.com/marketplace/en-US/product/infinity-blade-fire-lands
   - Forge/siege/tech-building dressing if still available.
7. Infinity Blade: Ice Lands
   - https://www.unrealengine.com/marketplace/en-US/product/infinity-blade-ice-lands
   - Ruin/stone/ice dressing if still available.

Do not chase `Infinity Blade: Sounds` unless a current official Epic/Fab page
proves it is available again. Epic's 2015 announcement says the Sounds pack was
removed from Marketplace after rights review.

Steps:

1. Use Legendary CLI/background download paths first. If GUI Epic Launcher/Fab
   is needed to add/download the approved packs, do not disturb the running
   canary window; ask Jonathan before any action that would affect it. Do not
   download the three explicitly rejected lower-priority assets from
   `Coordination/asset-manifest.md`.
2. Inventory the resulting vault/project payloads. Record exact source paths,
   pack names, sizes, and whether each pack is native UE `Content/` or raw
   source files.
3. Copy/import each available pack into isolated project folders under
   `Content/InfinityBlade/<PackName>/` or `Content/Fab/InfinityBlade_<PackName>/`.
   Do not merge into `StandIns`.
4. For native UE content packs, preserve materials, skeletons, animations, and
   FX folder structure. For raw source payloads, use the existing one-task-per-
   commandlet pattern from `Proof/import-fab-library-assets.ps1`.
5. Run `powershell -ExecutionPolicy Bypass -File .\Proof\unreal-map-smoke.ps1`.
   Grep the log for failed loads under the new Infinity Blade paths.
6. Write a result section listing:
   - imported pack paths,
   - top-level asset classes found,
   - likely SPLITROOT use by faction/shop row,
   - skipped/unavailable packs with exact reason,
   - proof artifact paths and exit codes.

Done-when: available Infinity Blade packs are copied/imported into isolated
project folders, map smoke is green, and the result clearly says which packs
remain unavailable. If the packs are still not locally downloadable, move this
task to `done/` with a blocked result and exact launcher/vault evidence instead
of pretending import happened.

Report-to: move to `Coordination/done/` with `## Result`.

## Result

Blocked as unavailable locally on 2026-06-11. No Infinity Blade pack payload was found in the project, Epic VaultCache, FabLibrary cache, Epic Launcher manifests, or Legendary entitlement list, so no copy/import or Unreal map smoke was run.

Commands/evidence:
- `python -m legendary.cli --version`: exit `0`, Legendary `0.20.34`.
- `C:\Users\Jonathan\AppData\Roaming\Python\Python314\Scripts\legendary.exe status`: exit `0`, Epic account `Jonnyton`, login session reused.
- `C:\Users\Jonathan\AppData\Roaming\Python\Python314\Scripts\legendary.exe list --include-ue`: exit `0`, saved to `Saved/Proof/legendary-list-include-ue-20260611.log`; listed Fab UE Plugin, Quixel Bridge, Fortnite, and Unreal Engine entries only.
- `C:\Users\Jonathan\AppData\Roaming\Python\Python314\Scripts\legendary.exe list --include-ue --third-party --json`: exit `0`, saved to `Saved/Proof/legendary-list-include-ue-third-party-20260611.json`.
- Parsed Legendary summary: `Saved/Proof/legendary-entitlement-summary-20260611.json`; `Total=25`, `InfinityBladeMatches=0`.
- Vault inventory: `Saved/Proof/vault-infinity-blade-inventory-20260611.json`; `VaultTopLevelCount=1`, `FabLibraryCount=42`, `InfinityBladeVaultMatches=[]`.
- Project content inventory: `Saved/Proof/project-content-infinity-blade-inventory-20260611.json`; `ProjectContentInfinityBladeMatches=0`.
- Epic Launcher manifest directory `C:\ProgramData\Epic\EpicGamesLauncher\Data\Manifests` contains only current UE/Fab/Bridge-style manifests in this check; no Infinity Blade manifest was found.
- Direct current checks of the approved Marketplace URLs for `infinity-blade-weapons` and `infinity-blade-effects` returned HTTP `404 Not Found` through the web checker; search found only third-party Fab listings referencing Infinity Blade assets as demo dependencies, not official downloadable pack listings.

Unavailable packs from this run:
- Infinity Blade: Weapons
- Infinity Blade: Warriors
- Infinity Blade: Adversaries
- Infinity Blade: Effects
- Infinity Blade: Grass Lands
- Infinity Blade: Fire Lands
- Infinity Blade: Ice Lands

Bounded outcome:
- No assets were imported.
- No `Content/InfinityBlade/` or `Content/Fab/InfinityBlade_*` folders were created.
- `Proof/unreal-map-smoke.ps1` was not run because there was no imported payload to validate and the live canary should not be disturbed.
- Next action requires adding/downloading the packs into the Epic Launcher/Fab library first, or providing an already-downloaded local source path.
