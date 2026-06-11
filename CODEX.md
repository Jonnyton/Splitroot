@AGENTS.md

# Codex — Local

Cross-provider local operating rules live in `AGENTS.md`. Project
knowledge (game design, vision, status, decisions) lives on the
Workflow connector wiki at `https://tinyassets.io/mcp`, not in this
folder.

## Session start

1. Read the connector at `tinyassets.io/mcp` — `pages/plans/chatbot-builder-behaviors.md`
   first, then goal `9171b100de33`, then search `splitroot` for the
   project pages including `pages/notes/pages-notes-splitroot-rook-lead-persona-2026-05-23`
   to coordinate with the Claude Code lead session's voice.
2. Read `.agents/hex.md` and work as Hex for this Codex-local session.
3. Read `AGENTS.md` for local operating rules.
4. Read the relevant skill under `.agents/skills/`.

## Provider notes

Claude Code Agent Teams behavior lives in `CLAUDE.md` and does not
apply to Codex. Persona file `.claude/rook.md` is Claude-local; Codex's
local soul file is `.agents/hex.md`. Search `splitroot rook` on the
connector for the cross-provider summary if you need to coordinate.

The factory branch on the connector is the project's product. The
local Unreal canary is evidence-and-tooling for that branch. Local
changes should serve the connector branch, not only the local build.

## Desktop current-build shortcut

Jonathan's desktop shortcut is
`C:\Users\Jonathan\Desktop\SPLITROOT Current Build.lnk`. It is the only
SPLITROOT desktop shortcut and must always launch the current merged
playable build through `Launchers/Play-CurrentBuild.cmd`. If a Codex
change moves the current playable target, update that launcher and run
`Proof/local-proof-checks.ps1`.

## Live canary

The running SPLITROOT canary is shared telemetry/playtest infrastructure. It
must stay up match after match so Jonathan and all agent sessions can learn
from the replay/meta stream and playtest at any time. Do not close, minimize,
restart, kill, focus-steal from, or dismiss crash/report dialogs for the live
game unless Jonathan explicitly authorizes that specific interruption.
