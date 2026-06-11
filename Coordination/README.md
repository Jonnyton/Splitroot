# Coordination queue — Rook (Cowork) ⇄ Hex (Codex)

Rook has no host shell this harness; Hex does. Division: Rook writes
code/design/wiki and reads proof artifacts; Hex executes anything that
needs the Windows host (builds, smokes, loops, launchers, imports).
Gauge is the current Codex gameplay-mechanics generalist in this session:
pick cross-cutting playable mechanics, wire real loops end-to-end, and leave
deep specialty ownership to the focused sessions below.
Quarry is a separate map-building session; route terrain, layout, dressing,
and second-map construction tasks to Quarry instead of mixing them into
mechanics/build-proof tasks.
Vector is the AI-player strategy and replay-analysis lane; route bot
intelligence, feature-use coverage, fair-senses, and replay-metadata audit
work to Vector. Gauge may touch bots only when they are canaries for a
gameplay mechanic; deeper AI intelligence belongs to Vector. Vector can queue
Hex tasks when the fix requires C++, proof-script, launcher, or host-loop
changes.
Grid is the RTS command/economy lane; route map-table command feel, selection,
production/build verbs, tech buildings, supply loops, shop unlocks, rally
points, and RTS proof gaps to Grid.
Ledger is the units/items asset lane. Ledger may use CLI/background asset
downloads, but must not close, minimize, restart, kill, or focus-steal from
Jonathan's running SPLITROOT canary. The live game is telemetry and playtest
infrastructure for every session. If a build/import needs the editor or game
closed, ask Jonathan first.

## Always-running canary rule

The visible SPLITROOT Current Build is not a disposable proof process. Once
Jonathan has the game running, all sessions should treat it as live playtest
infrastructure that is meant to continue match to match until Jonathan quits.

- Do not close, kill, recycle, restart, minimize, focus-steal from, or replace
  Jonathan's visible game window just to run a proof, import, smoke, or build.
- Scripts may launch and kill their own hidden proof process only when that
  process is separate from Jonathan's visible current-build process.
- Before using `Stop-Process`, `playtest-drive.ps1 -Action stop`, a proof quit
  flag, or a build/import step that may lock the Unreal DLL, first check for a
  running current-build `UnrealEditor.exe` and avoid touching it.
- If C++ build adoption truly requires stopping the visible game, ask Jonathan
  for that specific handoff first. Afterward, relaunch the current build and
  report the PID/log state.
- The design goal is continuous hosting: scoreboard or pending-next-match,
  automatic next match, humans kept in the flow, latest maps picked up next
  round, and future supervisor/reconnect support for fresh C++ builds.
- After changing proof, launcher, build, import, or playtest automation, run
  `python scripts/check_live_canary_process_safety.py` and fix any broad
  UnrealEditor stop patterns before reporting the task done.

## Protocol (keep it this simple)

- `inbox-hex/NNN-slug.md` — task for Hex. Fields: Goal, Steps,
  Done-when, Report-to.
- Hex executes, then MOVES the file to `done/` appending a `## Result`
  section (exit codes, artifact paths, surprises). The move is the ack.
- Rook polls `done/` and reads proof JSONs under `Saved/Proof/`.
- One task file = one verifiable unit. No task claims success without
  the proof artifact it names.
- Hex: re-enter via `.agents/hex.md`, then `AGENTS.md` for local rules.

## Result status taxonomy

Every `## Result` section should include one machine-readable status line:

- `Status: completed` - requested work is complete and proof is attached.
- `Status: blocked_requires_handoff` - work needs Jonathan's live-session
  handoff, supervisor-owned refresh, manual approval, or another explicit
  non-local action before it can be adopted.
- `Status: staged_not_live` - source, contract, or proof scaffolding exists,
  but current live metadata does not prove the running build has adopted it.
- `Status: proof_only` - the task produced evidence, analysis, tooling, or
  documentation without changing live gameplay behavior.
- `Status: superseded` - a newer task/result replaces this one.

Keep moving completed, blocked, staged, and proof-only tasks to `done/`; that
folder remains the ack lane. Use `Coordination/status-summary.ps1` to produce
`Saved/Proof/coordination-status-summary.json` when a live/adoption report needs
counts for blocked, staged, proof-only, and completed work.
