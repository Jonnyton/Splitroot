# 015 - Add visible next-match pending state after match end

Goal: make the end-of-match transition feel like a real multiplayer game flow. Gauge fixed the mechanical restart so live matches skip non-match template maps and restart Splitroot Valley until another match-playable map exists. The remaining UX gap is that players should clearly see they are pending the next match, not wonder whether the game quit or froze.

Current state after Gauge fix, 2026-06-11:
- Live match travel now uses match-playable rotation only.
- With only `splitroot_valley` match-playable, `TravelingTo map=splitroot_valley` starts the next match.
- `ArchonBotMatch` survives travel, so bot canaries respawn in the next match.
- `listen` survives travel, so a desktop-hosted listen session stays hosted across match restart.
- Proof exists at `Saved/Proof/last-live-match-restart-smoke.json`.

Missing gameplay-facing slice:
1. During `MatchEnd`, show a simple end screen or HUD strip with winner, reason, and next-match countdown.
2. During `Traveling`, show/log a pending-next-match state, including the next map id.
3. Add replay metadata markers such as:
   - `ArchonFactoryCanary: NextMatchPending nextMap=splitroot_valley countdownSeconds=15.0`
   - `ArchonFactoryCanary: NextMatchLoading map=splitroot_valley`
4. Ensure the player stays in the same running process/session across the transition; no lobby return.
5. Keep the implementation native/simple until the final UI style exists.

Proof:
- Extend or add a smoke that verifies `MatchEnd`, `NextMatchPending`, `TravelingTo`, and second `MatchLoopActive`.
- Run `powershell -ExecutionPolicy Bypass -File .\Proof\build-locked.ps1`.
- Run `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-match-restart-smoke.ps1`.

Report-to:
- Move this file to `Coordination\done\` with `## Result`.
- Include changed files, proof JSON/log paths, and the exact markers added.

## Result

Completed by Hex on 2026-06-11T06:15Z.

Changed files:
- `Source/ArchonFactoryCanary/Public/ArchonMatchTypes.h`
- `Source/ArchonFactoryCanary/Public/ArchonMatchStateActor.h`
- `Source/ArchonFactoryCanary/Private/ArchonMatchStateActor.cpp`
- `Source/ArchonFactoryCanary/Private/ArchonCanaryWorldSubsystem.cpp`
- `games/splitroot/Proof/live-match-restart-smoke.ps1`

What changed:
- Added replicated match snapshot fields for `ScoreboardSeconds`, `NextMapId`, and `MatchEndReason`.
- Added match activation metadata: `ArchonFactoryCanary: BuildFingerprint moduleDllUtc=... processStartUtc=... modulePath=...`.
- Added match config metadata: `ArchonFactoryCanary: MatchConfig warmupSeconds=... matchTimeLimitSeconds=... scoreboardSeconds=... maxCycleSeconds=...`.
- Added match-end replay marker: `ArchonFactoryCanary: NextMatchPending nextMap=splitroot_valley countdownSeconds=...`.
- Added travel replay marker: `ArchonFactoryCanary: NextMatchLoading map=splitroot_valley`.
- Updated the native HUD strip so `MatchEnd` shows winner, score, reason, next map, and countdown; `Traveling` shows the next map.
- Extended `live-match-restart-smoke.ps1` to assert `BuildFingerprintLogged`, `MatchConfigLogged`, `NextMatchPending`, and `NextMatchLoading`.

Proof:
- `powershell -ExecutionPolicy Bypass -File .\Proof\build-locked.ps1`
  - `BuildExitCode=0`
  - `AutomationExitCode=0`
  - `AutomationSucceeded=true`
  - `AutomationSucceededCount=212`
  - `AutomationFailedCount=0`
  - JSON: `Saved/Proof/last-policy-build-and-test.json`
  - Build log: `Saved/Proof/last-policy-build.log`
  - Automation log: `Saved/Proof/last-policy-automation.log`
- `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-match-restart-smoke.ps1`
  - `ExitCode=0`
  - `FirstMatchLoopActive=true`
  - `BuildFingerprintLogged=true`
  - `MatchConfigLogged=true`
  - `FirstMatchEnded=true`
  - `NextMatchPending=true`
  - `TravelRequested=true`
  - `NextMatchLoading=true`
  - `RestartedSameMatchMap=true`
  - `SecondMatchLoopActive=true`
  - `BotFlagSurvivedTravel=true`
  - `SecondWorldQuitScheduled=true`
  - `QuitCommandHonored=true`
  - JSON: `Saved/Proof/last-live-match-restart-smoke.json`
  - Log: `Saved/Proof/last-live-match-restart-smoke.log`

Observed marker lines in `Saved/Proof/last-live-match-restart-smoke.log`:
- `ArchonFactoryCanary: BuildFingerprint moduleDllUtc=2026-06-11T06:09:40.000Z processStartUtc=2026-06-11T06:13:47.867Z modulePath=C:/Users/Jonathan/Projects/Users/archon-rts-fps-factory/Binaries/Win64/UnrealEditor-ArchonFactoryCanary.dll`
- `ArchonFactoryCanary: MatchConfig warmupSeconds=2.0 matchTimeLimitSeconds=900.0 scoreboardSeconds=3.0 maxCycleSeconds=905.0`
- `ArchonFactoryCanary: MatchEnd winner=TeamA reason=core_destroyed pointsA=20000 pointsB=0 liveSeconds=26.0 sitesA=1 sitesB=1`
- `ArchonFactoryCanary: NextMatchPending nextMap=splitroot_valley countdownSeconds=3.0`
- `ArchonFactoryCanary: TravelRequested reason=match_complete`
- `ArchonFactoryCanary: NextMatchLoading map=splitroot_valley`
- `ArchonFactoryCanary: TravelingTo map=splitroot_valley url=/Engine/Maps/Entry?ArchonSplitrootValley?ArchonMatchLoop?game=/Script/ArchonFactoryCanary.ArchonMatchGameMode?ArchonMapId=splitroot_valley?ArchonMatchProofQuit?ArchonBotMatch?listen`
- Second world: `ArchonFactoryCanary: MatchLoopActive sites=3 proof=false warmupSeconds=15.0`
- Second world: `ArchonFactoryCanary: MatchConfig warmupSeconds=15.0 matchTimeLimitSeconds=900.0 scoreboardSeconds=15.0 maxCycleSeconds=930.0`

Bounded remaining gap:
- This proves the native end-of-match and restart metadata/HUD strip in the smoke path. It does not prove the perpetual `Proof/playtest-loop.ps1` newest-build recycle path; that remains in task `014`.
