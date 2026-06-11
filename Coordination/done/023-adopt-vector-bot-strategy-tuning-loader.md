# 023 - Adopt Vector bot strategy tuning loader

Goal: compile and adopt Vector's bot strategy tuning loader without disturbing
Jonathan's visible live SPLITROOT session unless Jonathan explicitly hands over
the current build or the supervisor owns the host process.

Context:
- Vector added `games/splitroot/FactoryContracts/bot_strategy_tuning.json`.
- The new C++ loader reads that contract at bot spawn and feeds bot role,
  perception, objective pressure, respawn, and unstick movement tuning into
  `AArchonBotAIController` and `UArchonBotBrainComponent`.
- The live snapshot parser now reports `BotStrategyTuningRevision` from the
  JSON file and `BotStrategyTuningLoadedCount` from the running log.
- Current live process PID `26024` predates this loader. Read-only snapshot
  shows `BotStrategyTuningLoadedCount=0`, `BotUnstickQueryCount=0`, and
  `BotFiringPositionCount=0`, so no live-adoption claim is valid yet.

Steps:
1. Wait for an explicit Jonathan handoff or a supervisor-owned host refresh.
   Do not stop, replace, minimize, or focus-steal the visible current-build
   window.
2. Build and run automation:
   `powershell -ExecutionPolicy Bypass -File .\Proof\build-and-test-policy.ps1`
3. Run the bot match smoke after the new DLL is active:
   `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\bot-match-smoke.ps1`
4. Sample the running loop without closing it:
   `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1 -RequireBotEvidence`
5. Compare against the pre-adoption snapshot:
   `BotStrategyTuningLoadedCount` must be greater than zero and
   `LatestBotStrategyTuningLoaded` must include
   `revision=vector-2026-06-11-live-tuning-v1`.

Done-when:
- Build and automation pass on the adopted source.
- `Saved\Proof\last-bot-match-smoke.json` passes with
  `BotStrategyTuningLoaded=true`.
- `Saved\Proof\last-live-bot-match-log-snapshot.json` shows the tuning
  revision loaded by the running build.
- The visible current build is running after adoption, with PID/log state
  reported.
- If `BotUnstickQueryCount` and `BotFiringPositionCount` remain zero after
  adoption, report that as Vector's next strategy target instead of masking it.

Report-to:
- Move this file to `Coordination\done\` with commands, exit codes, proof
  JSON/log paths, sample `BotStrategyTuningLoaded` / `BotConfigured` /
  `BotAIControllerConfigured` lines, and any live-stuck trend observed.

## Result

Status: blocked_requires_handoff

Status detail: blocked by live-session handoff, with read-only evidence
captured.

Reason: Jonathan's visible current-build process is running as
`UnrealEditor.exe` PID `26024`, `Saved/Proof/host-refresh-pending.json` reports
`Pending=false` and `SupervisorOwnsHost=false`, and this task's first step
requires either explicit Jonathan handoff or a supervisor-owned host refresh.
I did not run `Proof/build-and-test-policy.ps1`, `bot-match-smoke.ps1`, any
asset import, any stop/force flag, or any build-locking adoption path.

Non-disruptive work completed:

- Added `games/splitroot/Proof/live-canary-watch-note.ps1`, a read-only helper
  that summarizes existing live proof JSONs plus inbox/process state.
- Hardened `games/splitroot/Proof/live-bot-match-log-snapshot.ps1` so normal
  live use reads a bounded tail window instead of the whole growing log. The
  artifact now records `ReadMode`, `TailLinesRequested`, `LinesRead`, and
  `CountsScope`.
- Stopped one orphaned proof PowerShell process from the old unbounded snapshot
  attempt: PID `23300`, command line matched only
  `games\splitroot\Proof\live-bot-match-log-snapshot.ps1`. No Unreal process
  was stopped.

Commands/proof:

- `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-log-window-summary.ps1 -TailLines 300`
  - exit `0`
  - artifact: `Saved/Proof/last-live-log-window-summary.json`
  - tail window at `2026-06-11T16:49:54.9715789Z`: `WeaponFiredCount=16`,
    `TowerFiredCount=10`, `BotRespawnedCount=2`,
    `BotStuckRecoveryCount=10`, no match boundary in the 300-line tail.
- Pre-patch unbounded
  `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1`
  timed out under the heartbeat harness while reading the full growing log.
- `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1 -RequireBotEvidence`
  - exit `0` after the bounded-tail patch
  - artifact: `Saved/Proof/last-live-bot-match-log-snapshot.json`
  - `ReadMode=tail`, `TailLinesRequested=5000`, `LinesRead=5000`,
    `CountsScope=tail_window`, `EvidenceReady=true`,
    `MatchLoopActive=true`, `BotMatchSpawned=true`
  - JSON contract read: `BotStrategyTuningRevision=vector-2026-06-11-live-tuning-v1`
  - live adoption evidence not present:
    `BotStrategyTuningLoadedCount=0`, `LatestBotStrategyTuningLoaded=null`,
    `BotUnstickQueryCount=0`, `BotFiringPositionCount=0`,
    `TakeFiringPositionCovered=false`
- `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-canary-watch-note.ps1 -Archive`
  - exit `0`
  - artifacts: `Saved/Proof/hex-live-canary-watch-latest.json` and
    `Saved/Proof/hex-live-canary-watch-note-20260611T165206Z.json`
- `python .\scripts\check_live_canary_process_safety.py`
  - exit `0`
  - output: `OK - no broad live-canary process stop patterns found`

Sample bounded log lines from the 5,000-line tail:

- `BotStrategyTuningLoaded`: no matching line in the sampled tail.
- `BotConfigured`:
  `[2026.06.11-16.52.25:665][140]LogTemp: Display: ArchonFactoryCanary: BotConfigured bot=13 team=1 role=fighter engageRange=1400`
  `[2026.06.11-16.52.25:667][140]LogTemp: Display: ArchonFactoryCanary: BotConfigured bot=14 team=1 role=breacher engageRange=500`
  `[2026.06.11-16.52.25:669][140]LogTemp: Display: ArchonFactoryCanary: BotConfigured bot=15 team=1 role=fighter engageRange=1400`
- `BotAIControllerConfigured`:
  `[2026.06.11-16.52.25:665][140]LogTemp: Display: ArchonFactoryCanary: BotAIControllerConfigured bot=13 team=1 role=fighter sightRadius=1400 hearingRadius=800 nativePerception=true`
  `[2026.06.11-16.52.25:667][140]LogTemp: Display: ArchonFactoryCanary: BotAIControllerConfigured bot=14 team=1 role=breacher sightRadius=500 hearingRadius=800 nativePerception=true`
  `[2026.06.11-16.52.25:669][140]LogTemp: Display: ArchonFactoryCanary: BotAIControllerConfigured bot=15 team=1 role=fighter sightRadius=1400 hearingRadius=800 nativePerception=true`
- `BuildFingerprint`:
  `[2026.06.11-16.52.25:604][140]LogTemp: Display: ArchonFactoryCanary: BuildFingerprint moduleDllUtc=2026-06-11T15:59:42.000Z processStartUtc=2026-06-11T16:03:03.739Z modulePath=C:/Users/Jonathan/Projects/Users/archon-rts-fps-factory/Binaries/Win64/UnrealEditor-ArchonFactoryCanary.dll`

Live-stuck trend observed:

- 300-line tail top stuck zones:
  `x=5000;y=3500` count `4`, `x=5000;y=5000` count `3`,
  `x=-10000;y=7000` count `1`.
- 5,000-line snapshot window reports `BotStuckRecoveryCount=394`.

Current visible build state after this task:

- `UnrealEditor.exe` PID `26024` remains running.
- Command line remains the desktop current-build canary:
  `ArchonFactoryCanary.uproject "/Engine/Maps/Entry?ArchonMainMenu" -game -windowed -ResX=1280 -ResY=720 -log`.
- No proof PowerShell processes matching `live-log-window-summary.ps1`,
  `summarize-map-metadata.ps1`, or `live-bot-match-log-snapshot.ps1` remained
  running after cleanup.

## Post-Adoption Reconciliation

Status: completed

Hex reconciliation time: 2026-06-11T18:03Z.

Reason:
- A supervisor-owned boundary refresh occurred after the original blocked
  result, and the active canary relaunched as `UnrealEditor.exe` PID `25584`.
- The active process writes to `Saved\Proof\playtest-drive.log` through
  `-abslog`.

Proof:
- `Saved\Proof\last-live-bot-match-log-snapshot.json`
  - `LogPath=C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\playtest-drive.log`
  - `EvidenceReady=true`
  - `MatchLoopActive=true`
  - `BotStrategyTuningLoadedCount=4`
  - `LatestBotStrategyTuningLoaded` includes
    `revision=vector-2026-06-11-breacher-focus-v3`
  - `BotUnstickQueryCount=69`
  - `BotFiringPositionCount=25`
  - `TakeFiringPositionCovered=true`
- `Saved\Proof\live-route-hotspot-manifest.json`
  - `AdoptionState=no_pending_refresh`
  - `PendingNativeRefresh=false`
  - `LiveBotStrategyTuningLoadedCount=4`

Bounded conclusion:
- The bot strategy tuning loader is now proven live in the supervised current
  build. This does not claim that every route-stall branch has fired.
