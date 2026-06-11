# 002 — Pull Jonathan's new FAB library assets into the project

Goal: Jonathan added assets to the FAB library (Epic launcher, signed
in as Jonnyton). Get them into the canary so design can use them.

CONSTRAINT (Jonathan 2026-06-10): ONLY completely free assets. No
paid, no subscription-gated, no "free this month" that converts to
paid licensing weirdness — check each item's license is free outright.
We are not making our own assets this early; the library additions he
made (added but NOT yet downloaded) are the starting set.

Steps:
1. Epic launcher → Fab Library: list what Jonathan added (names +
   types + license). Write the list into the Result section even if
   import fails.
2. Download + "Add to project" the free-licensed ones into
   ArchonFactoryCanary (or a vault project, then copy Content/ in).
   Priority: characters > trees/rocks with collision > buildings >
   props. Also still wanted from handoff: Game Animation Sample,
   1-2 Paragon characters (both Epic-free).
3. Only a credential/2FA prompt bounces to Jonathan.

Done-when: assets on disk under Content/ (list paths), project still
opens (no need for full build — 004 covers that).

Report-to: move to Coordination\done\ with ## Result listing asset
names, disk paths, and anything that wouldn't import.

## Result

Attempted by Hex on 2026-06-10 America/Los_Angeles.

Verdict: blocked by missing local Fab listing/download surface in this Codex
session. No credential or 2FA prompt appeared; the blocker is that I do not
have an OS GUI-control tool to operate Epic Launcher, and the local launcher
metadata did not expose Jonathan's newly added Fab Library items.

Local evidence checked:

- `C:\ProgramData\Epic\EpicGamesLauncher\Data\Manifests\*.item`
  - Found installed items only: Unreal Engine 5.7, Fab UE Plugin, Quixel Bridge.
  - No content-pack manifests for Game Animation Sample, Paragon, or new Fab assets.
- `C:\ProgramData\Epic\EpicGamesLauncher\VaultCache\FabLibrary\listings_v1.db`
  - Tables present: `catalog`, `download_meta`, `format`, `listing_acquisition`, `local_listing`, `tag`.
  - Row counts: all `0`.
- `C:\Users\Jonathan\AppData\Local\FabPlugins\settings_v1.json`
  - Plugin settings only; no listing/acquisition rows.
- Epic Launcher log:
  - Shows navigation to `/ue/library` and `https://www.fab.com/plugins/egl/library`.
  - Does not list asset names/licenses or downloaded project paths.
- Webcache scan:
  - Found generic Fab/cache/cookie data but no reliable asset library list usable for import.

Imported assets: none.

Disk paths added under `Content/`: none.

Anything that would not import:

- Jonathan's newly added Fab Library assets: names/types/licenses unavailable from local metadata.
- Game Animation Sample: not locally listed/downloaded.
- Paragon characters: not locally listed/downloaded.

Next workable handoff:

- A GUI-capable session should open Epic Launcher -> Fab Library and list the asset names/types/licenses, or Jonathan should use Launcher/Fab to download/add them once so the vault/project manifests exist locally.
- After assets are visible on disk, Hex can copy/import/verify them with normal host scripts.
