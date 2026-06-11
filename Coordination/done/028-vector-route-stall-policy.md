# 028 - Vector route-stall policy

Status: staged_not_live

Result: staged by Vector on 2026-06-11 as a native AI strategy iteration.
No Unreal process was stopped, relaunched, minimized, focus-stolen, or replaced
by this session.

Changed files:
- `Source/ArchonFactoryCanary/Public/ArchonBotSteeringPolicyLibrary.h`
- `Source/ArchonFactoryCanary/Private/ArchonBotSteeringPolicyLibrary.cpp`
- `Source/ArchonFactoryCanary/Private/Tests/ArchonBotSteeringPolicyTests.cpp`
- `Source/ArchonFactoryCanary/Public/ArchonBotStrategyTuningLibrary.h`
- `Source/ArchonFactoryCanary/Private/ArchonBotStrategyTuningLibrary.cpp`
- `Source/ArchonFactoryCanary/Private/Tests/ArchonBotStrategyTuningTests.cpp`
- `Source/ArchonFactoryCanary/Public/ArchonBotBrainComponent.h`
- `Source/ArchonFactoryCanary/Private/ArchonBotBrainComponent.cpp`
- `games/splitroot/FactoryContracts/bot_strategy_tuning.json`
- `games/splitroot/Proof/live-bot-match-log-snapshot.ps1`
- `games/splitroot/Proof/vector-bot-replay-diagnosis.ps1`
- `games/splitroot/Proof/bot-match-smoke.ps1`

Behavior staged:
- Added pure steering policy hooks:
  - `ShouldAbandonRouteWaypointAfterStalls`;
  - `ComputeObjectiveLaneShift`.
- Added tests for route-stall abandonment and bounded objective-lane shifting.
- Added tuning fields to `bot_strategy_tuning.json` revision
  `vector-2026-06-11-route-stall-v2`:
  - `route_stall_max_unstick_attempts: 8`;
  - `objective_stall_lane_shift_attempts: 6`;
  - `objective_stall_max_lane_shift: 3`.
- Bot brain now:
  - counts repeated stuck recoveries while chasing an assigned route waypoint;
  - logs `BotRouteWaypointAbandoned` and resumes base pressure after the route
    stall threshold;
  - counts repeated stuck recoveries at tower/core stand-off objectives;
  - logs `BotObjectiveLaneShift` and shifts the bot stand-off lane within the
    configured cap.
- Replay/smoke scripts now report:
  - `BotRouteWaypointAbandonedCount`;
  - `LatestRouteWaypointAbandoned`;
  - `BotObjectiveLaneShiftCount`;
  - `LatestObjectiveLaneShift`;
  - the new JSON threshold values.

Read-only verification:
- `python -m json.tool .\games\splitroot\FactoryContracts\bot_strategy_tuning.json`:
  exit `0`.
- PowerShell parser checks for
  `games\splitroot\Proof\vector-bot-replay-diagnosis.ps1`,
  `games\splitroot\Proof\live-bot-match-log-snapshot.ps1`, and
  `games\splitroot\Proof\bot-match-smoke.ps1`: all parse ok.
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\vector-bot-replay-diagnosis.ps1 -TailLines 2000`:
  exit `0`; wrote
  `Saved\Proof\last-vector-bot-replay-diagnosis.json`.
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1 -TailLines 2000`:
  exit `0`; wrote
  `Saved\Proof\last-live-bot-match-log-snapshot.json`.
- `python .\scripts\check_live_canary_process_safety.py`: exit `0`;
  no broad live-canary stop patterns found.

Live evidence from the 2000-line tail:
- Visible current-build process remained `UnrealEditor.exe` PID `7680`.
- `BotStrategyTuningRevision=vector-2026-06-11-route-stall-v2` was read from
  JSON, but `BotStrategyTuningLoadedCount=0`; the running DLL has not adopted
  the loader.
- `HasCombatWindow=true`, `WeaponFiredCount=461`, `TowerFiredCount=89`,
  `BotStuckRecoveryCount=225`, `MaxObservedStuckAttempt=48`,
  `BotUnstickQueryCount=0`, `BotFiringPositionCount=0`,
  `BotRouteWaypointAbandonedCount=0`, `BotObjectiveLaneShiftCount=0`.
- Current top stuck zones are still concentrated around
  `x=-2500;y=-5000`, `x=-5500;y=5500`, `x=-1500;y=-6000`,
  `x=-5500;y=6500`, and `x=-6000;y=6500`.
- `SuggestedNextTuningBlockedReason=strategy_loader_not_live`.

Bounded conclusion:
- This is staged C++ plus proof telemetry, not live behavior yet.
- After the host-safe build/adoption path lands, the first acceptance check is
  a new Vector diagnosis with `BotStrategyTuningLoadedCount > 0`,
  `BotUnstickQueryCount > 0`, and either route/lane markers under repeated
  stuck pressure or a materially lower stuck count.
