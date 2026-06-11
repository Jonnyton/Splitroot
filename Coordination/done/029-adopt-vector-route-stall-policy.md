# 029 - Adopt Vector route-stall policy

Goal:
Adopt and verify Vector's staged route-stall bot strategy build without
violating the always-running canary rule.

Context:
- Vector staged C++ route-stall behavior in
  `Coordination/done/028-vector-route-stall-policy.md`.
- The live process at the time of staging was `UnrealEditor.exe` PID `7680`.
- Read-only live metadata still showed `BotStrategyTuningLoadedCount=0`,
  `BotUnstickQueryCount=0`, `BotFiringPositionCount=0`,
  `BotRouteWaypointAbandonedCount=0`, and `BotObjectiveLaneShiftCount=0`.
- The visible game must not be closed, killed, restarted, minimized, or
  focus-stolen unless Jonathan explicitly hands over that live session.

Steps:
1. Re-enter through `.agents/hex.md`, `AGENTS.md`, and
   `Coordination/README.md`.
2. Confirm whether the host-safe fresh-build/hot-load path is available.
3. If the visible canary is still running and no safe adoption path exists,
   do not stop it; report `blocked_requires_handoff`.
4. If a host-safe adoption path exists, adopt the staged native changes and
   keep the visible current-build loop alive.
5. Run only proof that is safe for the current live-game state. At minimum,
   after adoption and a fresh match window, run:
   - `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1 -TailLines 5000`
   - `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\vector-bot-replay-diagnosis.ps1 -TailLines 10000`
   - `python .\scripts\check_live_canary_process_safety.py`

Done-when:
- Move this file to `Coordination/done/` with a `## Result` section.
- Include a machine-readable status line.
- Attach the paths and key fields from:
  - `Saved\Proof\last-live-bot-match-log-snapshot.json`;
  - `Saved\Proof\last-vector-bot-replay-diagnosis.json`.
- A completed adoption report should show:
  - `BotStrategyTuningLoadedCount > 0`;
  - `LatestBotStrategyTuningLoaded` includes
    `revision=vector-2026-06-11-route-stall-v2`;
  - `BotUnstickQueryCount > 0`;
  - repeated stuck pressure either produces `BotRouteWaypointAbandonedCount > 0`
    or `BotObjectiveLaneShiftCount > 0`, or the report explains that the match
    window did not generate enough stuck pressure to exercise those branches.

Report-to:
Vector and Rook via `Coordination/done/`.

## Result

Status: blocked_requires_handoff

Hex result time: 2026-06-11T17:19Z.

Live-session boundary:
- Visible current-build process remained `UnrealEditor.exe` PID `7680`.
- No Unreal process was closed, killed, restarted, minimized, focus-stolen, or
  replaced.
- No build, relink, asset import, editor commandlet, or supervisor promotion was
  attempted.
- Host-safe adoption is not available for Jonathan's already-running desktop
  game in this state. The route-stall work is native C++ and the current policy
  requires a supervisor-owned match-boundary refresh or Jonathan's explicit
  live-session handoff before the running DLL can adopt it.

Proof:
- `Saved\Proof\last-live-bot-match-log-snapshot.json`
  - `SnapshotUtc=2026-06-11T17:19:19.5465978Z`
  - `LinesRead=5000`
  - `EvidenceReady=true`
  - `MatchLoopActive=false`
  - `BotStrategyTuningRevision=vector-2026-06-11-route-stall-v2`
  - `BotStrategyTuningLoadedCount=0`
  - `BotUnstickQueryCount=0`
  - `BotFiringPositionCount=0`
  - `BotRouteWaypointAbandonedCount=0`
  - `BotObjectiveLaneShiftCount=0`
  - `TakeFiringPositionCovered=false`
- `Saved\Proof\last-vector-bot-replay-diagnosis.json`
  - `SnapshotUtc=2026-06-11T17:19:20.4396110Z`
  - `TailLinesRead=10000`
  - `HasCombatWindow=true`
  - `BotStrategyTuningRevision=vector-2026-06-11-route-stall-v2`
  - `BotStrategyTuningLoadedCount=0`
  - `BotStuckRecoveryCount=1763`
  - `BotUnstickQueryCount=0`
  - `BotFiringPositionCount=0`
  - `BotRouteWaypointAbandonedCount=0`
  - `BotObjectiveLaneShiftCount=0`
  - `SuggestedNextTuningBlockedReason=strategy_loader_not_live`
  - top zone: `x=-5500;y=5500`
- `Saved\Proof\host-refresh-pending.json`
  - `Pending=true`
  - `Reason=route_stall_policy_staged_requires_supervisor_handoff`
  - `BoundaryPolicy=restart_on_next_match_when_supervisor_owns_host`
  - `SupervisorOwnsHost=false`
  - `CurrentProcessId=7680`
- `Saved\Proof\live-route-hotspot-manifest.json`
  - `VectorStuckRecoveryCount=1763`
  - `TrendAggregateVectorTopStuckZone=x=-5500;y=5500`
  - `LatestVectorDiagnosisTopStuckZone=x=-5500;y=5500`
  - `MissingBotUnstickQueryCoverage=true`
  - `MissingBotFiringPositionCoverage=true`
  - `NextAction=Vector should adopt/prove BotUnstickQuery and BotFiringPosition before Quarry treats these as terrain-only blockers.`
- `python .\scripts\check_live_canary_process_safety.py`: exit `0`,
  `OK - no broad live-canary process stop patterns found`.

Bounded conclusion:
- The staged route-stall policy is visible as a JSON revision, but current live
  metadata does not prove live DLL adoption.
- Adoption remains blocked until a supervisor-owned refresh or Jonathan's
  explicit handoff permits the native module boundary to move.

## Post-Adoption Reconciliation

Status: superseded

Hex reconciliation time: 2026-06-11T18:03Z.

Reason:
- The original v2 route-stall adoption task was superseded by
  `031-adopt-vector-breacher-focus-v3.md`.
- The v3 contract kept the route-stall thresholds and is now loaded by the
  supervised current build.

Proof:
- `Saved\Proof\last-live-bot-match-log-snapshot.json`
  - `LogPath=C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\playtest-drive.log`
  - `BotStrategyTuningLoadedCount=4`
  - `LatestBotStrategyTuningLoaded` includes
    `revision=vector-2026-06-11-breacher-focus-v3`
  - `BotUnstickQueryCount=69`
  - `BotObjectiveLaneShiftCount=7`
  - `BotRouteWaypointAbandonedCount=0`
- `Saved\Proof\last-vector-bot-replay-diagnosis.json`
  - `BotStuckRecoveryCount=69`
  - top stuck zone `x=12500;y=8000`

Bounded conclusion:
- v2's exact revision did not become the live target; v3 superseded it.
- Route-stall lane shifting is proven under v3, but route-waypoint abandonment
  is still unproven in this sampled window.
