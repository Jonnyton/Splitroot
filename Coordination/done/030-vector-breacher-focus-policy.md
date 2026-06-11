# 030 - Vector breacher-focus policy

Status: staged_not_live

Result: staged by Vector on 2026-06-11 as a second native AI strategy pass on
top of `028-vector-route-stall-policy.md`. No Unreal process was stopped,
relaunched, minimized, focus-stolen, or replaced by this session.

Changed files:
- `Source/ArchonFactoryCanary/Public/ArchonBotStrategyTuningLibrary.h`
- `Source/ArchonFactoryCanary/Private/ArchonBotStrategyTuningLibrary.cpp`
- `Source/ArchonFactoryCanary/Private/Tests/ArchonBotStrategyTuningTests.cpp`
- `Source/ArchonFactoryCanary/Private/ArchonBotBrainComponent.cpp`
- `games/splitroot/FactoryContracts/bot_strategy_tuning.json`
- `games/splitroot/Proof/live-bot-match-log-snapshot.ps1`
- `games/splitroot/Proof/vector-bot-replay-diagnosis.ps1`

Behavior staged:
- Superseded the JSON revision with
  `vector-2026-06-11-breacher-focus-v3`.
- Added role-specific tuning fields:
  - `breacher_pursuit_memory_window_seconds: 2.5`;
  - `breacher_threat_window_seconds: 3.5`.
- The bot brain now gives breachers those shorter pursuit and pain-memory
  windows while fighters keep the existing global windows.
- `BotConfigured` logging now includes `pursuitMemory` and `threatWindow`.
- Vector diagnosis now reports `TopStateByRole` so post-adoption reviews can
  verify whether breacher `Pursuing` and `HuntingThreat` churn drops.
- Vector diagnosis now appends compact trend rows to
  `Saved\Proof\vector-bot-replay-diagnosis-history.jsonl`.

Read-only verification:
- `python -m json.tool .\games\splitroot\FactoryContracts\bot_strategy_tuning.json`:
  exit `0`.
- PowerShell parser checks for
  `games\splitroot\Proof\vector-bot-replay-diagnosis.ps1`,
  `games\splitroot\Proof\live-bot-match-log-snapshot.ps1`, and
  `games\splitroot\Proof\bot-match-smoke.ps1`: all parse ok.
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\vector-bot-replay-diagnosis.ps1 -TailLines 10000`:
  exit `0`; wrote
  `Saved\Proof\last-vector-bot-replay-diagnosis.json`.
- Follow-up `vector-bot-replay-diagnosis.ps1 -TailLines 5000`: exit `0`;
  appended a compact row to
  `Saved\Proof\vector-bot-replay-diagnosis-history.jsonl`.
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1 -TailLines 5000`:
  exit `0`; wrote
  `Saved\Proof\last-live-bot-match-log-snapshot.json`.
- `python .\scripts\check_live_canary_process_safety.py`: exit `0`;
  no broad live-canary stop patterns found.

Live evidence from the latest read-only Vector diagnosis:
- Visible current-build process remained `UnrealEditor.exe` PID `7680`.
- The log still showed module DLL timestamp `2026-06-11T15:59:42.000Z`.
- `BotStrategyTuningRevision=vector-2026-06-11-breacher-focus-v3` was read
  from JSON, but `BotStrategyTuningLoadedCount=0`; no live adoption yet.
- `MatchEndCount=1`, latest match ended at the 900-second time limit with
  TeamB ahead on points.
- `BotStuckRecoveryCount=1238`, `MaxObservedStuckAttempt=264`,
  `BotUnstickQueryCount=0`, `BotFiringPositionCount=0`,
  `BotRouteWaypointAbandonedCount=0`, `BotObjectiveLaneShiftCount=0`.
- `TopStateByRole` now shows current pre-adoption churn, including
  `role=fighter|state=Pursuing=220`, `role=fighter|state=DefendingBase=122`,
  `role=breacher|state=HuntingThreat=22`, and
  `role=breacher|state=Pursuing=7`.
- `SuggestedNextTuningBlockedReason=strategy_loader_not_live`.
- The history row records the same v3 revision, `BotStrategyTuningLoadedCount=0`,
  `BotStuckRecoveryCount=554`, `MaxObservedStuckAttempt=141`, top stuck zones,
  and top state-by-role counts for trend comparison.

Bounded conclusion:
- v3 is staged source/contract behavior only. The live loop is still valuable
  as replay evidence, but it is not running this code yet.
- Post-adoption acceptance should compare `TopStateByRole`, stuck counts, and
  route/lane markers against this pre-adoption baseline.
