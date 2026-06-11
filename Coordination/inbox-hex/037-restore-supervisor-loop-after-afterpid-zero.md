# 037 - Restore supervisor loop after AfterPid zero

Goal:
Restore or clearly classify the current-build supervisor loop after a
match-boundary refresh left the live game running but the supervisor itself
reported `SupervisorMode=not_running`.

Context:
- Vector is iterating AI strategy from live replay metadata and should not own
  hot-reload infrastructure.
- A prior host refresh snapshot recorded `BeforePid=25396`, `AfterPid=0`,
  `BuildSucceeded=true`, and `RefreshTrigger=next_match_boundary`.
- No Unreal process was running at that moment, so Vector relaunched only the
  configured current-build launcher:
  `Launchers/Play-CurrentBuild.cmd`.
- Current live game after recovery:
  - PID: 5064
  - latest loaded tuning revision: `vector-2026-06-11-objective-lane-v6`
  - latest build fingerprint version: `v20260611-183856`
- Current supervisor snapshot:
  - `SupervisorMode=not_running`
  - `SupervisorOwnsHost=false`
  - `CurrentProcessId=5064`
  - `Pending=false`

Steps:
1. Re-enter via `.agents/hex.md`, then `AGENTS.md` and `Coordination/README.md`.
2. Do not close, kill, restart, minimize, focus-steal from, or replace
   Jonathan's visible SPLITROOT current-build window.
3. Inspect the host/supervisor scripts and state files to determine why
   the supervisor stopped after the `AfterPid=0` refresh.
4. If supervisor ownership can be restored around the already-running PID 5064
   without touching the visible game, do that.
5. If restoring ownership requires a visible-game handoff or restart, do not do
   it; report `Status: blocked_requires_handoff`.
6. Run:
   - `powershell -NoProfile -ExecutionPolicy Bypass -File .\Proof\host-refresh-pending-state.ps1 -Action snapshot -ObservedLogPath .\Saved\Proof\playtest-drive.log`
   - `python .\scripts\check_live_canary_process_safety.py`

Done-when:
- Move this file to `Coordination/done/` with a `## Result` section.
- Use `Status: completed` only if the snapshot shows supervisor ownership
  restored without disrupting the visible game.
- Use `Status: blocked_requires_handoff` if a handoff is required.
- Include the current PID, `SupervisorMode`, `SupervisorOwnsHost`, and the
  latest build fingerprint from the proof output.

Report-to:
Vector and Rook via `Coordination/done/`.
