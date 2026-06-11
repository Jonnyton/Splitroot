# SPLITROOT — per-game instance (Layer C)

This directory is the **per-game instance** for SPLITROOT, the first
canary game in the Unreal game-factory branch. It contains everything
specific to SPLITROOT: faction data, asset references, arc proofs,
launchers.

The factory substrate (Layer A — policy libraries, runtime components,
proof harness, discipline patterns) lives at the branch root. The
project brain (Layer B — design docs, decisions, slice plans,
substrate ledger) lives on the Workflow connector at
`https://tinyassets.io/mcp`.

See `pages/concepts/factory-branch-layer-boundaries-convention-2026-05-24`
on the connector for the full convention.

## What lives here

- `FactoryContracts/factions.json` — Verdant Choir, Kinwild Covenant,
  Lenswright Compact registry (with the per-faction `no_gunpowder`
  flag enforcing the Lenswright hill in data).
- `FactoryContracts/approved_assets.html` +
  `FactoryContracts/approved_assets.data.js` — standing approved asset
  URL registry shared by unit, item, and map-building sessions.
- `FactoryContracts/unit_roster.html` +
  `FactoryContracts/unit_roster.data.js` — human tactical roster view
  grouped by faction, tech path, unit/item type, and current asset use.
- `FactoryContracts/unit_ledger.json` / `.md` — Ledger-owned roster,
  prototype stats, mechanics, shop-tech gates, team use, and current
  unit asset selections.
- `Proof/first-60-seconds-smoke.ps1` — SPLITROOT's rung-1 anchor arc
  proof. Validates the goosebumps moment: open the table, order the
  Thornbound squad, watch fog-of-war reveal happen.
- `Launchers/Play-ArchonFactoryCanary.cmd` — launches the SPLITROOT
  canary directly into the FirstPerson level.
- `Launchers/Open-ArchonFactoryCanary.cmd` — opens the editor against
  the SPLITROOT .uproject.

## What still lives in the branch root (deferred moves)

- `ArchonFactoryCanary.uproject` — UE expects this sibling of `Source/`;
  moves at the Phase 3 module rename.
- `Source/ArchonFactoryCanary/` — module contains both substrate
  (e.g. `UArchonMapTableSelectionPolicyLibrary`) and SPLITROOT-specific
  actors (e.g. `AArchonLenswrightBracewrightActor`, `AArchonVerdantOutpostActor`).
  Split moves at Phase 3 after rung-2 ship.
- `FactoryContracts/faction_palette.json`, `faction_audio_palette.json`,
  `asset_manifest.json/.md`, `strategic_audio_events.json` — C++
  headers reference these. Move at Phase 2 when JSONify-factions wires
  a runtime config path.

## Where SPLITROOT's design lives

On the connector wiki:

- Goal: `9171b100de33` — "Design and prototype a fantasy Archon RTS/FPS hybrid"
- Search `splitroot` to find current pages (slice plans, contracts, polish, hero plan, multiplayer plan)
- Rung-1 evidence: `pages/notes/splitroot-rung-1-local-playable-prototype-earned-2026-05-23`
- Rung-2 anchor: `pages/plans/splitroot-second-60-seconds-combat-slice-2026-05-24`
