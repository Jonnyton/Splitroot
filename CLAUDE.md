@AGENTS.md

# Claude Code — Local

This file contains Claude Code specifics only. Cross-provider local
operating rules live in `AGENTS.md`. Project knowledge (game design,
vision, status, decisions) lives on the Workflow connector wiki at
`https://tinyassets.io/mcp`, not in this folder.

## Session start

1. Read the connector at `tinyassets.io/mcp` — `pages/plans/chatbot-builder-behaviors.md`
   first, then goal `9171b100de33`, then search `splitroot` for the
   project pages.
2. Read `.claude/rook.md` and re-enter character as Rook (lead session
   only; agent-team roles don't inherit).
3. Read `AGENTS.md` for the local operating rules.
4. Read `.claude/skills/using-agent-skills/SKILL.md` for the skill
   discipline.
5. For game-factory work, use `.claude/skills/unreal-archon-game-factory/SKILL.md`.

## MCP connector

The project-local `.mcp.json` points Claude at the Workflow connector
(`workflow-live`). This is the project's live brain — the place
everything authoritative lives. Read first; write contributions back
as wiki drafts using existing categories. Do not invent parallel
structures.

## Desktop current-build shortcut

Jonathan's desktop shortcut is
`C:\Users\Jonathan\Desktop\SPLITROOT Current Build.lnk`. It is the only
SPLITROOT desktop shortcut and must always launch the current merged
playable build through `Launchers/Play-CurrentBuild.cmd`. If a Claude
change moves the current playable target, update that launcher and run
`Proof/local-proof-checks.ps1`.

## Live canary

The running SPLITROOT canary is shared telemetry/playtest infrastructure. It
must stay up match after match so Jonathan and all agent sessions can learn
from the replay/meta stream and playtest at any time. Do not close, minimize,
restart, kill, focus-steal from, or dismiss crash/report dialogs for the live
game unless Jonathan explicitly authorizes that specific interruption.

## Agent teams

This project supports Claude Code Agent Teams. Spawn the standing team
when work benefits from persistent teammates:

- `navigator` — scope, route work
- `developer` — implement scoped Unreal/game-factory changes
- `verifier` — run proof scripts and review diffs
- `game-designer` — protect RTS/FPS feel and faction/gameplay design

Every task names: `Files:` (exact write boundary or read-only),
`Depends:` (blockers or none), `Deliverable:` (concrete output),
`Verification:` (who checks what). Don't let two agents edit the same
file set at once.

## Claude-specific caveats

- Teammates don't inherit the lead's chat history. Put critical
  context in task prompts and point teammates at `AGENTS.md`, the
  connector wiki pages they need, and the relevant skill.
- `.claude/skills/` is a mirror of `.agents/skills/`. Edit the
  canonical version and run `scripts/sync-skills.ps1`.
- `verifier` is read-only. It reports failures; `developer` fixes them.
