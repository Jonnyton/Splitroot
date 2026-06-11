# Local Operating Rules

This folder is the local Unreal canary for the SPLITROOT game — one
sub-project of the reusable game-factory branch on the Workflow
connector at `https://tinyassets.io/mcp`.

**Project knowledge does not live in this folder.** Game design, vision,
factions, mechanics, monetization, decisions, status, plan — those live
on the connector wiki. Local files would go stale. The connector is the
shared live brain that all AIs and human contributors read and write.

This file covers only the things that are genuinely local-only: how to
operate the canary, what tools live here, what discipline applies to
local work.

## First action in any session

1. Read the connector at `https://tinyassets.io/mcp` — start with
   `pages/plans/chatbot-builder-behaviors.md`. It's the canonical guide
   for how chatbots and local AIs should operate against this brain.
2. Read the project goal `9171b100de33` for the current factory state
   and gate ladder.
3. Search the wiki for `splitroot` to find the project pages (notes,
   plans, research, persona, decisions).
4. Then read this file for the local operating rules.

If you cannot reach the connector, you cannot meaningfully contribute
to the project's direction — you can only verify the local canary
build. Say so to the user and limit your scope accordingly.

## Local entry points

- Engine: Unreal Engine 5.7 at `C:\Program Files\Epic Games\UE_5.7`.
- Project (deferred per-game rename): `ArchonFactoryCanary.uproject`.
- Desktop shortcut: `C:\Users\Jonathan\Desktop\SPLITROOT Current Build.lnk`
  (the only one - do not create additional shortcuts). It must always
  launch the current merged playable build so Jonathan can peek at
  progress without asking an AI session.
- Current build launcher (factory-level, stable): `Launchers/Play-CurrentBuild.cmd`.
  The desktop shortcut must point here. Update this launcher when the
  current playable surface changes from editor-driven canary to a
  packaged build, OR when a different `games/<id>/` becomes the active
  game.
- Canary play launcher (per-game): `games/splitroot/Launchers/Play-ArchonFactoryCanary.cmd`.
- Editor launcher (per-game): `games/splitroot/Launchers/Open-ArchonFactoryCanary.cmd`.
- C++ substrate module (deferred per-game split): `Source/ArchonFactoryCanary/`.
- Factory contracts (Layer A — schemas + shared docs): `FactoryContracts/`.
- Per-game contracts (Layer C — instance data): `games/splitroot/FactoryContracts/`.

## Layer convention

This branch follows the three-layer convention codified at
`pages/concepts/factory-branch-layer-boundaries-convention-2026-05-24`
on the connector. Layer A is the factory substrate (this branch root);
Layer B is the MCP brain; Layer C is per-game instance data at
`games/<game-id>/`. SPLITROOT lives at `games/splitroot/`. Files
specific to a single game MUST NOT be added to Layer A.

## Proof scripts (run before claims)

Local verification tools. Run them; report what they say. No claim
beyond what they verify.

Factory-level (Layer A — apply to any game):

- `Proof/build-and-test-policy.ps1` — compiles editor module + runs
  automation tests.
- `Proof/unreal-map-smoke.ps1` — headless map smoke (runtime map table
  + FPS pawn possession + input bridge install).
- `Proof/local-proof-checks.ps1` — local file-evidence checks without
  launching Unreal.
- `Proof/playtest-render.ps1` — rendered playtest capture for the
  `unreal-canary-playtest` skill.

Per-game (Layer C — SPLITROOT-specific arc proofs):

- `games/splitroot/Proof/first-60-seconds-smoke.ps1` — rung-1 anchor
  arc proof (Verdant root-vault + table-ordered Thornbound squad +
  fog-of-war loop).

## Live game discipline

Jonathan's visible current-build game window is a live metadata source.
Do not close, kill, recycle, or replace it just to run proof. Before
running any script that launches Unreal with a self-quit flag, calls
`playtest-drive.ps1 -Action stop`, uses `Stop-Process`, or creates a
build lock that requires the module DLL, first check for a running
current-build `UnrealEditor.exe` process. If the visible live game is
running, limit work to log/replay inspection and source edits, or ask
Jonathan to explicitly hand over the session.

Fresh C++ adoption for the visible game must be coordinated. The
desktop/current-build launcher keeps the same playable process across
match travel; it does not silently quit and relaunch Jonathan's window.
`Proof/playtest-loop.ps1` keeps the live session running and logs pending
fresh-build adoption instead of recycling the process.

Run `python scripts/check_live_canary_process_safety.py` after changing proof,
launcher, build, import, or playtest automation. The check fails on broad
UnrealEditor stop patterns and stop escape hatches.

## Skills

Canonical skills live in `.agents/skills/`. Claude Code mirror lives in
`.claude/skills/`. After editing canonical skills, run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\sync-skills.ps1
python .\scripts\validate_skills.py --root .
```

Use `unreal-archon-game-factory` for the core game-factory work. Use
`game-prototyping` for generic patterns when the Unreal-specific skill
doesn't fit.

## Persona

The Claude Code lead session uses a persona at `.claude/rook.md` —
Rook. Claude-specific. Other providers don't inherit this; see the
connector page `pages/notes/pages-notes-splitroot-rook-lead-persona-2026-05-23`
for the cross-provider summary (or just search `splitroot rook` on the
connector).

Codex sessions use `.agents/hex.md` — Hex. Codex-specific. This gives
Codex a local soul file without changing Rook or the connector's shared
project authority.

Agent team members (`developer`, `verifier`, `navigator`,
`game-designer` under `.claude/agents/`) speak as their scoped roles,
not as Rook.

## Engineering discipline

- Read code before editing it.
- Keep changes scoped to the requested milestone.
- Prefer native Unreal/C++ seams for reusable proof logic.
- Run the proof scripts before reporting success.
- Treat the running SPLITROOT canary as a shared live telemetry and playtest
  service. Do not close, minimize, restart, kill, focus-steal from, or dismiss
  crash/report dialogs for Jonathan's live game instance unless Jonathan has
  explicitly authorized that specific action. If an import/build step needs the
  editor or game closed because files are locked, ask first and wait.
- One desktop shortcut. Don't create more. Keep
  `C:\Users\Jonathan\Desktop\SPLITROOT Current Build.lnk` pointed at
  `Launchers/Play-CurrentBuild.cmd` in the same change that updates the
  current playable build.
- No destructive git commands without explicit user approval.
- If the folder isn't a git checkout, say so; don't pretend branch
  operations exist.
- Direct, concrete status reports. No hype.
- Per the connector's chatbot-builder-behaviors: don't reorganize the
  repo or wiki structure unprompted. Prefer small shipped artifacts
  over large design documents.
- When wiki content needs to be authored, draft via
  `wiki action=write` using existing categories
  (notes/plans/concepts/research/etc.); the wiki uses a built-in
  draft → promote flow.
