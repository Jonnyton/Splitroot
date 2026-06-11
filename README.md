# SPLITROOT — Unreal Canary

This folder is the local Unreal 5.7 canary for **SPLITROOT** — a fantasy
RTS/FPS Archon hybrid being built as the first canary game inside a
reusable game-factory branch on the Workflow connector at
`https://tinyassets.io/mcp`.

The folder contains only the game build, code, proof scripts, launchers,
and provider/skill configuration. **The project's design, vision,
decisions, current state, and plans all live on the connector.** Local
files would go stale; the connector is the live brain.

## Where the project actually lives

- Workflow connector: `https://tinyassets.io/mcp`
- Project goal: `9171b100de33` — "Design and prototype a fantasy Archon
  RTS/FPS hybrid"
- Search the connector wiki for `splitroot` to find current project
  pages (status, design, lineage research, persona, decisions).
- Read `pages/plans/chatbot-builder-behaviors.md` on the connector
  before authoring new content there.

## Layer convention (factory vs per-game)

This branch follows a three-layer convention codified on the connector
at `pages/concepts/factory-branch-layer-boundaries-convention-2026-05-24`.
The branch itself (Layer A) holds only genre-agnostic substrate;
per-game data + per-game proof + per-game launchers live in
`games/<game-id>/` (Layer C). SPLITROOT is the first instance at
`games/splitroot/`.

## What this folder is

**Factory substrate (Layer A — branch root):**

- `ArchonFactoryCanary.uproject` — Unreal 5.7 project file. (Deferred
  rename to per-game location pending Source/ module split.)
- `Source/ArchonFactoryCanary/` — C++ substrate module (route,
  entitlement, map-table, shared-RTS, visibility, combat, faction
  movement, player interaction, runtime input seams). (Deferred split
  into `Source/Archon/` + `games/splitroot/Source/Splitroot/`.)
- `FactoryContracts/` — factory-level contracts (`session_routes.json`,
  `entitlement_policy.json`, `archon_adapter_contract.md`). Per-game
  contracts that still live here are explicitly deferred Phase 2 moves
  pending the JSONify-factions runtime config.
- `Proof/` — factory-level verification scripts
  (`build-and-test-policy.ps1`, `unreal-map-smoke.ps1`,
  `local-proof-checks.ps1`, `playtest-render.ps1`). Per-game arc
  proofs live under `games/<game-id>/Proof/`.
- `Launchers/Play-CurrentBuild.cmd` — stable desktop-target launcher;
  delegates to the current game's per-game launcher.
- `AGENTS.md`, `CLAUDE.md`, `CODEX.md`, `README.md` — local operating
  rules and provider session notes.
- `.agents/skills/`, `.claude/skills/`, `.claude/agents/` — skill +
  agent definitions (canonical + Claude Code mirror).
- `.mcp.json` — connector configuration.

**Per-game instance (Layer C — `games/splitroot/`):**

- `games/splitroot/README.md` — what SPLITROOT is, connector design pointers.
- `games/splitroot/FactoryContracts/factions.json` — Verdant Choir /
  Kinwild Covenant / Lenswright Compact data.
- `games/splitroot/Proof/first-60-seconds-smoke.ps1` — SPLITROOT's
  rung-1 arc proof.
- `games/splitroot/Launchers/Play-ArchonFactoryCanary.cmd`,
  `Open-ArchonFactoryCanary.cmd` — SPLITROOT-specific launchers (will
  rename `Play-Splitroot.cmd` / `Open-Splitroot.cmd` at the Phase 3
  module rename).

## How to open the project

Double-click `C:\Users\Jonathan\Desktop\SPLITROOT Current Build.lnk`
(the only SPLITROOT desktop shortcut). It must always point at
`Launchers/Play-CurrentBuild.cmd`, which launches the current merged
playable build. Today that delegates to
`games/splitroot/Launchers/Play-ArchonFactoryCanary.cmd` and starts
the canary map directly; when a packaged build becomes the current
build, update `Play-CurrentBuild.cmd` instead of creating another
desktop icon.

Run `games/splitroot/Launchers/Open-ArchonFactoryCanary.cmd` when you
need the editor.

## Public Git boundary

This checkout is intended to be public source history, not a full binary
asset archive. Git tracks the Unreal project, C++ source, config,
contracts, proof scripts, coordination notes, launchers, provider setup,
and small SPLITROOT canary fixtures under `games/splitroot/Content/`.

Raw Fab/vendor asset downloads under `Content/Fab/` and `Content/Fab_*`
stay local-only. They include very large licensed payloads and are
restored through the local import/proof workflow rather than committed
directly to GitHub. Generated Unreal output (`Binaries/`, `Intermediate/`,
`DerivedDataCache/`, `Saved/`) is also ignored.

## For contributors without local file access

You don't need a clone of this repo to contribute to SPLITROOT's
design, lineage research, faction balance discussion, or factory-branch
architecture. Open Cowork (or any MCP client), connect to
`https://tinyassets.io/mcp`, find goal `9171b100de33`, read the
`splitroot` pages, and contribute via the wiki draft → promote flow.

Local repo access is only needed for the Unreal canary build itself —
the implementation that earns the factory branch's claims.
