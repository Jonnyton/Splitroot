# 036 - Adopt Vector stuck-recovery context telemetry

Goal:
Adopt Vector's non-behavioral C++ telemetry marker
`BotStuckRecoveryContext` into the live loop so replay metadata explains why
stuck recovery did or did not trigger route waypoint abandonment.

Context:
- Vector added a source-only telemetry log in
  `Source/ArchonFactoryCanary/Private/ArchonBotBrainComponent.cpp`.
- The marker is expected to look like:
  `ArchonFactoryCanary: BotStuckRecoveryContext bot=... phase=... routeActive=... routeReached=... routeStallAttempts=... routeThreshold=... objectiveTarget=... objectiveStallAttempts=... laneShift=... distance=...`
- Vector parsers already understand the marker:
  - `games/splitroot/Proof/vector-bot-replay-diagnosis.ps1`
  - `games/splitroot/Proof/live-bot-match-log-snapshot.ps1`
- Current proof before native adoption:
  - `BotStrategyTuningRevision=vector-2026-06-11-route-abandon-v5`
  - `LatestBotStrategyTuningLoadedRevision=vector-2026-06-11-route-abandon-v5`
  - `BotStuckRecoveryCount>0`
  - `BotStuckRecoveryContextCount=0`

Steps:
1. Re-enter through `.agents/hex.md`, `AGENTS.md`, and `Coordination/README.md`.
2. Keep Jonathan's visible current-build game running. Do not close, kill,
   restart, minimize, focus-steal from, or replace it.
3. If supervisor/hot reload can adopt the C++ source safely at a match
   boundary, let it adopt. Do not force a build lock against Jonathan's live
   process.
4. After adoption, run:
   - `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1 -TailLines 12000`
   - `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\vector-bot-replay-diagnosis.ps1 -TailLines 12000`
   - `python .\scripts\check_live_canary_process_safety.py`

Done-when:
- Move this file to `Coordination/done/` with a `## Result` section.
- Use `Status: completed` if `BotStuckRecoveryContextCount > 0` appears in
  either proof output.
- Use `Status: staged_not_live` if the source and parsers exist but the live
  build has not adopted the marker yet.
- Include current PID/build fingerprint and the latest
  `LatestBotStuckRecoveryContext` line if available.

Report-to:
Vector and Rook via `Coordination/done/`.

## Result

Status: completed

Hex completed the adoption proof at 2026-06-11T18:37Z without closing,
killing, restarting, minimizing, focus-stealing from, or replacing Jonathan's
visible current-build game. A supervisor-owned match-boundary refresh had
already advanced the live host to PID 25396; I only ran read-only proof scripts
after that boundary.

Proof commands and outcomes:
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1 -TailLines 12000`
  wrote `Saved\Proof\last-live-bot-match-log-snapshot.json`.
  Snapshot `2026-06-11T18:37:03.7205147Z` reported
  `BotStuckRecoveryCount=7`, `BotStuckRecoveryContextCount=7`,
  `LatestBotStrategyTuningLoadedRevision=vector-2026-06-11-route-abandon-v5`,
  `BotStrategyTuningRevisionAdopted=true`, and `EvidenceReady=true`.
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\vector-bot-replay-diagnosis.ps1 -TailLines 12000`
  wrote `Saved\Proof\last-vector-bot-replay-diagnosis.json` and appended
  `Saved\Proof\vector-bot-replay-diagnosis-history.jsonl`.
  Snapshot `2026-06-11T18:37:04.0435450Z` reported
  `BotStuckRecoveryContextCount=7`, top context phase `route`, and
  `VectorTuningGate.Status=observe_more` because the current window is early.
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\Proof\host-refresh-pending-state.ps1 -Action snapshot -ObservedLogPath .\Saved\Proof\playtest-drive.log`
  returned `Pending=false` at `2026-06-11T18:37:03.5386756Z`.
- `python .\scripts\check_live_canary_process_safety.py` returned
  `OK - no broad live-canary process stop patterns found`.

Current live host:
- PID: 25396.
- Build fingerprint:
  `moduleDllUtc=2026-06-11T18:35:50.000Z runtimeInputUtc=2026-06-11T18:30:57.000Z processStartUtc=2026-06-11T18:36:16.075Z version=v20260611-183550`.

Latest context marker:
`ArchonFactoryCanary: BotStuckRecoveryContext bot=10 team=1 attempt=4 phase=route routeActive=1 routeReached=0 routeStallAttempts=3 routeThreshold=5 objectiveTarget=none objectiveStallAttempts=0 laneShift=0 distance=10016`

Bounded read:
The marker is live and parseable. This proof does not claim that the current
v5 route-abandon tuning is final; the latest Vector gate remains
`observe_more` for the current early match window.
