# 024 - Vector bot strategy tuning contract

Status: staged_not_live

Result: completed by Vector on 2026-06-11 as a non-disruptive AI strategy
iteration slice. Jonathan's visible SPLITROOT current-build process was not
stopped, relaunched, minimized, focus-stolen, or replaced.

Changed files:
- `games/splitroot/FactoryContracts/bot_strategy_tuning.json`
- `Source/ArchonFactoryCanary/Public/ArchonBotStrategyTuningLibrary.h`
- `Source/ArchonFactoryCanary/Private/ArchonBotStrategyTuningLibrary.cpp`
- `Source/ArchonFactoryCanary/Private/Tests/ArchonBotStrategyTuningTests.cpp`
- `Source/ArchonFactoryCanary/Public/ArchonBotAIController.h`
- `Source/ArchonFactoryCanary/Private/ArchonBotAIController.cpp`
- `Source/ArchonFactoryCanary/Public/ArchonBotBrainComponent.h`
- `Source/ArchonFactoryCanary/Private/ArchonBotBrainComponent.cpp`
- `Source/ArchonFactoryCanary/Public/ArchonCanaryWorldSubsystem.h`
- `Source/ArchonFactoryCanary/Private/ArchonCanaryWorldSubsystem.cpp`
- `games/splitroot/Proof/live-bot-match-log-snapshot.ps1`
- `games/splitroot/Proof/bot-match-smoke.ps1`

Connector record:
- `pages/notes/splitroot-vector-bot-strategy-tuning-contract-2026-06-11.md`

Behavior added:
- Bot strategy values now have a SPLITROOT contract:
  `games/splitroot/FactoryContracts/bot_strategy_tuning.json`.
- The tuning file carries role cycle, fighter/breacher engage ranges, native
  perception radii, objective attack/standoff distances, respawn timing, and
  stuck/unstick movement knobs.
- `AArchonBotAIController` consumes the tuning for native UE sight/hearing
  configuration.
- `UArchonBotBrainComponent` consumes the tuning for role choice, fair-senses,
  objective pressure, respawn, and unstick thresholds.
- `UArchonCanaryWorldSubsystem` loads tuning once per bot-match spawn and
  once per reinforcement batch, keeping all bots in that wave on the same
  revision.
- The live snapshot parser now reports both the JSON tuning revision and
  whether the running game has loaded it.

Read-only proof outcomes:
- JSON parser check via PowerShell: passed; revision
  `vector-2026-06-11-live-tuning-v1`, role cycle
  `fighter,fighter,breacher`, unstick lateral `650`.
- `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1`: exit `0`;
  `Saved\Proof\last-live-bot-match-log-snapshot.json` reports live
  `UnrealEditor.exe` PID `26024`, `MatchLoopActive=true`,
  `BotMatchSpawned=true`, `EvidenceReady=true`,
  `BotStrategyTuningRevision=vector-2026-06-11-live-tuning-v1`, and
  `BotStrategyTuningLoadedCount=0`.
- `python scripts\check_live_canary_process_safety.py`: exit `0`;
  no broad live-canary stop patterns found.
- `powershell -ExecutionPolicy Bypass -File .\Proof\host-refresh-pending-state.ps1 -Action snapshot`: exit `0`;
  `Saved\Proof\host-refresh-pending.json` reports current live PID `26024`,
  `Pending=false`, and `BoundaryPolicy=restart_on_next_match_when_supervisor_owns_host`.

Bounded caveats:
- No C++ build, automation run, bot-match smoke, or process-boundary refresh
  was run because the visible live game is protected telemetry and was not
  handed over.
- The running process predates the loader, so `BotStrategyTuningLoadedCount=0`
  is expected and must not be reported as live adoption.
- Hex adoption/status task
  `Coordination/done/023-adopt-vector-bot-strategy-tuning-loader.md` recorded
  `blocked_requires_handoff`; no live-adoption claim exists yet.

Next Vector lens:
- After adoption, watch `BotStrategyTuningLoadedCount`, `BotUnstickQueryCount`,
  `BotFiringPositionCount`, `BotStuckRecoveryCount`, and
  `LatestFeatureCoverage`.
- First data-only experiment should not change ranges until the loader appears
  in the log; otherwise the JSON is only staged, not active.
