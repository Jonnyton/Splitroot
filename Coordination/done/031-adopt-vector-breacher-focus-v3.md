# 031 - Adopt Vector breacher-focus v3

Goal:
Adopt and verify Vector's latest staged bot strategy build,
`vector-2026-06-11-breacher-focus-v3`, without violating the always-running
canary rule.

Context:
- `029-adopt-vector-route-stall-policy.md` is already acknowledged as
  `blocked_requires_handoff`; do not reopen or overwrite that result.
- v3 supersedes the v2 adoption target. It includes:
  - route waypoint abandonment after repeated stuck recovery;
  - objective-lane shifting after repeated tower/core stand-off stalls;
  - breacher-specific short pursuit/threat windows.
- The visible game must not be closed, killed, restarted, minimized, or
  focus-stolen unless Jonathan explicitly hands over that live session or a
  supervisor-owned host refresh owns the boundary.

Steps:
1. Re-enter through `.agents/hex.md`, `AGENTS.md`, and
   `Coordination/README.md`.
2. Confirm whether the host-safe fresh-build/hot-load path can adopt native
   C++ into the live loop without closing Jonathan's current window.
3. If not, move this task to `Coordination/done/` with
   `Status: blocked_requires_handoff`.
4. If adoption is safe, adopt the staged native changes and keep the visible
   current-build loop alive.
5. After at least one fresh match window, run:
   - `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1 -TailLines 5000`
   - `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\vector-bot-replay-diagnosis.ps1 -TailLines 10000`
   - `python .\scripts\check_live_canary_process_safety.py`

Done-when:
- Move this file to `Coordination/done/` with a `## Result` section and a
  machine-readable status line.
- For `Status: completed`, attach key fields from
  `Saved\Proof\last-live-bot-match-log-snapshot.json` and
  `Saved\Proof\last-vector-bot-replay-diagnosis.json`.
- Completed evidence should show:
  - `BotStrategyTuningLoadedCount > 0`;
  - `LatestBotStrategyTuningLoaded` includes
    `revision=vector-2026-06-11-breacher-focus-v3`;
  - `BotUnstickQueryCount > 0`;
  - `BotConfigured` / `BotStrategyTuningLoaded` evidence includes
    `breacherPursuit=2.5` and `breacherThreat=3.5` or equivalent
    configured bot logs with `pursuitMemory=2.5` and `threatWindow=3.5`;
  - repeated stuck pressure either produces `BotRouteWaypointAbandonedCount > 0`
    or `BotObjectiveLaneShiftCount > 0`, or the report explains that the match
    window did not generate enough stuck pressure to exercise those branches;
  - `TopStateByRole` is present so Vector can compare breacher distraction
    churn against the pre-adoption baseline in
    `030-vector-breacher-focus-policy.md`.

Report-to:
Vector and Rook via `Coordination/done/`.

## Result

Status: blocked_requires_handoff

Hex result time: 2026-06-11T17:23Z.

Live-session boundary:
- Visible current-build process remained `UnrealEditor.exe` PID `7680`.
- No Unreal process was closed, killed, restarted, minimized, focus-stolen, or
  replaced.
- No build, relink, asset import, editor commandlet, or supervisor promotion was
  attempted.
- v3 is native C++ plus JSON contract behavior. The currently running desktop
  game cannot adopt the native loader or bot brain changes without a
  supervisor-owned match-boundary refresh or Jonathan's explicit live-session
  handoff.

Proof:
- `games\splitroot\FactoryContracts\bot_strategy_tuning.json`
  - `revision=vector-2026-06-11-breacher-focus-v3`
  - `roles.breacher_pursuit_memory_window_seconds=2.5`
  - `roles.breacher_threat_window_seconds=3.5`
- `Saved\Proof\last-live-bot-match-log-snapshot.json`
  - `SnapshotUtc=2026-06-11T17:22:25.3617104Z`
  - `BotStrategyTuningRevision=vector-2026-06-11-breacher-focus-v3`
  - `BotStrategyTuningLoadedCount=0`
  - `LatestBotStrategyTuningLoaded=null`
  - `BotUnstickQueryCount=0`
  - `BotFiringPositionCount=0`
  - `BotRouteWaypointAbandonedCount=0`
  - `BotObjectiveLaneShiftCount=0`
- `Saved\Proof\last-vector-bot-replay-diagnosis.json`
  - `SnapshotUtc=2026-06-11T17:22:26.2048035Z`
  - `TailLinesRead=10000`
  - `BotStrategyTuningRevision=vector-2026-06-11-breacher-focus-v3`
  - `BotStrategyTuningLoadedCount=0`
  - `LatestBotStrategyTuningLoaded=null`
  - `BotStuckRecoveryCount=1238`
  - `BotUnstickQueryCount=0`
  - `BotFiringPositionCount=0`
  - `BotRouteWaypointAbandonedCount=0`
  - `BotObjectiveLaneShiftCount=0`
  - `TopStateByRole` is present, including
    `role=fighter|state=EngageEnemy=257`,
    `role=fighter|state=Pursuing=220`,
    `role=breacher|state=MoveToObjective=23`,
    `role=breacher|state=HuntingThreat=22`, and
    `role=breacher|state=Pursuing=7`.
  - `SuggestedNextTuningBlockedReason=strategy_loader_not_live`
- `Saved\Proof\host-refresh-pending.json`
  - `Pending=true`
  - `Reason=breacher_focus_v3_staged_requires_supervisor_handoff`
  - `BoundaryPolicy=restart_on_next_match_when_supervisor_owns_host`
  - `SupervisorOwnsHost=false`
  - `CurrentProcessId=7680`
- `Saved\Proof\live-route-hotspot-manifest.json`
  - `AdoptionState=blocked_requires_handoff`
  - `PendingNativeRefresh=true`
  - `HostRefreshReason=breacher_focus_v3_staged_requires_supervisor_handoff`
  - `TrendAggregateVectorTopStuckZone=x=-2500;y=-5000`
  - `LatestVectorDiagnosisTopStuckZone=x=-2500;y=-5000`
- `python .\scripts\check_live_canary_process_safety.py`: exit `0`,
  `OK - no broad live-canary process stop patterns found`.

Bounded conclusion:
- v3 is staged and visible in the contract, but the running canary has not
  loaded it.
- Adoption remains blocked until a supervisor-owned refresh or Jonathan's
  explicit handoff permits the native module boundary to move.

## Post-Adoption Reconciliation

Status: completed

Hex reconciliation time: 2026-06-11T18:03Z.

Reason:
- A supervisor-owned boundary refresh occurred after the original blocked
  result.
- The active current build is now `UnrealEditor.exe` PID `25584`, launched with
  `-ArchonBotMatch -ArchonBotCountPerTeam=8` and
  `-abslog=Saved\Proof\playtest-drive.log`.

Proof:
- `Saved\Proof\last-live-bot-match-log-snapshot.json`
  - `LogPath=C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\playtest-drive.log`
  - `EvidenceReady=true`
  - `MatchLoopActive=true`
  - `BotMatchSpawned=true`
  - `BotStrategyTuningLoadedCount=4`
  - `LatestBotStrategyTuningLoaded` includes
    `revision=vector-2026-06-11-breacher-focus-v3`,
    `breacherPursuit=2.5`, and `breacherThreat=3.5`
  - `BotUnstickQueryCount=69`
  - `BotFiringPositionCount=25`
  - `TakeFiringPositionCovered=true`
  - `BotObjectiveLaneShiftCount=7`
  - `BotRouteWaypointAbandonedCount=0`
- `Saved\Proof\last-vector-bot-replay-diagnosis.json`
  - `BotStrategyTuningLoadedCount=4`
  - `BotStuckRecoveryCount=69`
  - top stuck zone `x=12500;y=8000`
- `Saved\Proof\live-route-hotspot-manifest.json`
  - `AdoptionState=no_pending_refresh`
  - `PendingNativeRefresh=false`
  - `LiveBotStrategyTuningLoadedCount=4`

Bounded conclusion:
- Breacher-focus v3 is now proven live in the supervised current build.
- Unstick query, firing-position, and objective-lane-shift markers are proven.
- Route-waypoint abandonment remains unproven in this sampled window.
