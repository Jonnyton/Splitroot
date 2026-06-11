---
name: unreal-archon-game-factory
description: Use when working on this Unreal Engine Archon RTS/FPS game-factory branch, including native Unreal gameplay code, map-table RTS/FPS interaction, local proof scripts, desktop launcher behavior, connector evidence, or reusable game factory orchestration.
---

# Unreal Archon Game Factory

## Purpose

Use the Workflow connector at `https://tinyassets.io/mcp` to build a reusable
game-factory branch. This repo is the first local Unreal canary and proof
surface for that branch, not a one-off prototype.

The first game is a fantasy RTS/FPS hybrid: mostly standard RTS, mostly
standard first-person fantasy game, joined by Archon-style shared team RTS
control from map tables. The larger goal is a branch that can eventually
one-shot this game and remix into other high-quality games from changed seed
packets and constraints.

## Start Here

1. Read the Workflow connector at `https://tinyassets.io/mcp`. Project
   knowledge lives there, not in this repo. Start with:
   - `pages/plans/chatbot-builder-behaviors.md` (canonical chatbot guide);
   - goal `9171b100de33` (factory state and gate ladder);
   - search `splitroot` for current project pages (status, design, lineage,
     persona, decisions).
2. Read `AGENTS.md` and `README.md` in this repo for local operating
   rules (Unreal version, launchers, proof scripts, skill paths).
3. Inspect the files you will touch.
4. Choose the smallest vertical slice that moves toward playable evidence.
5. State how the work improves the connector branch's ability to generate,
   test, or remix games.

## Connector-First Rule

Treat the Workflow branch as the product being hardened. Treat the local Unreal
game as the first executable evidence that the branch can drive real game
creation.

Every major change should produce or clarify:

- a connector-facing seed/contract/design field;
- a reusable Unreal seam;
- local proof evidence the branch can cite;
- a remix boundary for future games;
- future tests for anything not yet proven.

## Non-Negotiable Design Rules

- FPS controls stay standard for PC mouse and keyboard.
- RTS controls stay conceptually standard.
- Archon mode means any teammate can input; finalization behaves like one RTS
  controller.
- Do not add command tokens, commander elections, votes, soft locks, priority
  queues, or anti-grief governance unless the user explicitly changes scope.
- Local/offline/LAN/private modes are full-free.
- SteamOnline paid gameplay can unlock horizontal hero variety and live RTS
  map-table execution only.
- Paid heroes never add raw power.

## Implementation Pattern

Prefer native Unreal C++ seams that can be tested:

- policy libraries for route/entitlement/RTS authority decisions;
- actor/component wrappers for runtime behavior;
- automation tests for contracts and regressions;
- smoke scripts for map startup and runtime proof logs;
- launchers for user-facing entry points.

Avoid hiding reusable logic inside binary Blueprint assets when a native seam is
reasonable. Blueprint assets may still be used for presentation once the C++
contract is stable.

## Proof Pattern

Run proof in this order after substantial changes:

```powershell
powershell -ExecutionPolicy Bypass -File .\Proof\build-and-test-policy.ps1
powershell -ExecutionPolicy Bypass -File .\Proof\unreal-map-smoke.ps1
powershell -ExecutionPolicy Bypass -File .\Proof\local-proof-checks.ps1
```

When skills or Claude setup changes, also run:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\sync-skills.ps1
python .\scripts\validate_skills.py --root .
```

## Claim Boundaries

Only claim what was proven by local commands or manual evidence.

Do not claim:

- AAA quality;
- sell-ready state;
- packaged Windows build;
- Steam integration;
- manual gameplay verification;
- production RTS UI;
- complete hero/respawn system.

## Current Target

The next high-value vertical slice is:

1. Single desktop shortcut launches the game canary.
2. Player has standard FPS control.
3. Tab opens the map table.
4. Map table commands a visible RTS squad.
5. The squad visibly moves or attacks.
6. Player exits the table and resumes FPS control.

This target matters because it would prove the connector branch can orchestrate
a real mixed FPS/RTS loop, not because the canary alone is the final product.

## Connector Use

Use `https://tinyassets.io/mcp` for Workflow branch/goal evidence and reusable
factory orchestration. Connector runs are not a substitute for local Unreal
proof. Record both separately, and keep the connector branch updated after
meaningful local milestones.

Per the project-folder discipline established 2026-05-23: project knowledge
(design, status, plan, decisions) lives on the connector wiki, not in this
repo. Search `splitroot` for current project pages.
ove the connector branch can orchestrate
a real mixed FPS/RTS loop, not because the canary alone is the final product.

## Connector Use

Use `https://tinyassets.io/mcp` for Workflow branch/goal evidence and reusable
factory orchestration. Connector runs are not a substitute for local Unreal
proof. Record both separately, and keep the connector branch updated after
meaningful local milestones.
