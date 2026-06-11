# 014 - Make live replay metadata prove build adoption and match restart cadence

Goal: make the running bot-canary loop self-verifying. Jonathan wants the AI bot matches to teach us in real time as mechanics are added; the replay/log metadata should prove which build a match is using, when the match should restart, and whether the next match picked up the newest build.

Gauge verification, 2026-06-11:
- Current GUI canary is running from the SPLITROOT launcher as `UnrealEditor.exe` against `/Engine/Maps/Entry?ArchonMainMenu`.
- It auto-hosted `splitroot_valley` with `ArchonMatchLoop` and `ArchonBotMatch`; `Saved/Logs/ArchonFactoryCanary.log` shows `BotMatchSpawned perTeam=8 total=16`, `MatchPhase phase=Live`, `BotState`, `WeaponFired`, `SupplyTick`, `BotRespawned`, and `ReinforcementPurchased`.
- A one-off smoke also completed at `Saved/Proof/last-bot-match-smoke.json` with `ObserveSeconds=180`, bots spawned/fought, core/tower damage, respawns, and both teams fought.
- The module DLL timestamp was `2026-06-11T05:32:57Z`; the GUI and smoke processes launched after that, so this pass could correlate current-build adoption from process/file timestamps.
- The continuous `Proof/playtest-loop.ps1` process was not running when checked. The code supports newest-build recycling, but there is no durable replay metadata marker proving adoption from inside the match stream.
- Default match config is `WarmupSeconds=15`, `MatchTimeLimitSeconds=900`, `ScoreboardSeconds=15`; the natural time-limit cycle is about 930 seconds before travel, unless a core dies first.

Problem to fix:
- `Proof/playtest-loop.ps1` emits build-adoption and recycle events only to stdout. If it runs hidden or under another monitor, later agents cannot reliably audit whether it was running or which build each match adopted.
- The replay metadata does not currently print a build fingerprint or full match timing config at match start. It logs `warmupSeconds`, but not `matchTimeLimitSeconds`, `scoreboardSeconds`, or a per-match build id/DLL timestamp.
- The 180-second smoke proves bot behavior, but not the full time-limit restart cadence.

Recommended implementation:
1. Add a durable loop event log, for example `Saved/Proof/playtest-loop.log`, and tee every `LoopEvent:` from `Proof/playtest-loop.ps1` into it.
2. Persist the adopted DLL timestamp/build fingerprint in `Saved/Proof/playtest-drive-session.json` when a loop session launches.
3. Add an Unreal log/replay marker at match activation with a stable name such as:
   - `ArchonFactoryCanary: BuildFingerprint moduleDllUtc=... processStartUtc=...`
   - `ArchonFactoryCanary: MatchConfig warmupSeconds=15.0 matchTimeLimitSeconds=900.0 scoreboardSeconds=15.0 maxCycleSeconds=930.0`
4. Add or extend a proof script that verifies:
   - the loop process is running;
   - a session launches and records the adopted build;
   - a newer DLL causes `newer build detected - session recycled`;
   - the next launched session records the newer build;
   - match config metadata is present in the replay log.
5. Add a long-cadence proof option or simulated clock policy test that proves the `900 + 15 + 15` restart path without requiring a full manual wait every pass.

Done-when:
- The live metadata/log stream itself can answer "what build is this match using?" without manual timestamp correlation.
- The loop leaves durable evidence that it recycled for a newer build and relaunched on that build.
- The default max match time is visible in metadata and proven by test/smoke as `900` live seconds plus warmup/scoreboard.

Proof:
- Run `powershell -ExecutionPolicy Bypass -File .\Proof\build-locked.ps1`.
- Run the relevant loop/adoption proof added or updated above.
- Run `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\bot-match-smoke.ps1`.
- Report the exact proof JSON/log paths and the observed `BuildFingerprint`, `MatchConfig`, and loop recycle lines.

Report-to:
- Move this file to `Coordination\done\` with `## Result`.
- Include changed files, commands, exit codes, proof artifact paths, and any remaining reason the bot-canary loop cannot be trusted overnight.

## Result

Completed by Hex on 2026-06-11.

Changed files:
- `Source/ArchonFactoryCanary/Public/ArchonMatchTypes.h`
- `Source/ArchonFactoryCanary/Public/ArchonMatchStateActor.h`
- `Source/ArchonFactoryCanary/Private/ArchonMatchStateActor.cpp`
- `Source/ArchonFactoryCanary/Private/ArchonCanaryWorldSubsystem.cpp`
- `Source/ArchonFactoryCanary/Private/Tests/ArchonMatchPolicyTests.cpp`
- `Proof/playtest-loop.ps1`
- `Proof/playtest-loop-adoption-proof.ps1`
- `games/splitroot/Proof/live-match-restart-smoke.ps1`

Proof outcomes:
- `Saved/Proof/last-policy-build-and-test.json`: `BuildExitCode=0`, `AutomationExitCode=0`, `AutomationSucceededCount=213`, `AutomationFailedCount=0`.
- `Saved/Proof/playtest-loop-adoption-proof.json`: `LoopProcessStarted=true`, `LoopProcessExited=true`, `LaunchCount=2`, `RecycleCount=1`, `SessionEvidenceHasBuildFingerprint=true`, `SessionEvidenceHasMapUrl=true`, `SessionEvidenceHasBotCount=true`, `FakeReplayBuildFingerprintPresent=true`, `FakeReplayMatchConfigPresent=true`, `RealRestartSmokeBuildFingerprintPresent=true`, `RealRestartSmokeMatchConfigPresent=true`.
- `Saved/Proof/last-live-match-restart-smoke.json`: `ExitCode=0`, `BuildFingerprintLogged=true`, `MatchConfigLogged=true`, `NextMatchPending=true`, `NextMatchLoading=true`, `RestartedSameMatchMap=true`, `BotFlagSurvivedTravel=true`, `QuitCommandHonored=true`.
- `Saved/Proof/last-bot-match-smoke.json`: `ObserveSeconds=300`, `BotMatchSpawned=true`, `MatchLoopActive=true`, `BotsEngaged=true`, `BotsFired=true`, `BotRespawned=true`, `CoreDamaged=true`, `TowerDamaged=true`, `ObjectivePushReached=true`, `CoreAttackReached=true`, `BothTeamsFought=true`.

Observed metadata:
- `Saved/Proof/last-live-match-restart-smoke.log`: `BuildFingerprint moduleDllUtc=2026-06-11T06:26:54.000Z processStartUtc=2026-06-11T06:28:42.451Z modulePath=C:/Users/Jonathan/Projects/Users/archon-rts-fps-factory/Binaries/Win64/UnrealEditor-ArchonFactoryCanary.dll`.
- `Saved/Proof/last-live-match-restart-smoke.log`: proof match config logged `warmupSeconds=2.0 matchTimeLimitSeconds=900.0 scoreboardSeconds=3.0 maxCycleSeconds=905.0`; post-travel default config logged `warmupSeconds=15.0 matchTimeLimitSeconds=900.0 scoreboardSeconds=15.0 maxCycleSeconds=930.0`.
- `Saved/Proof/last-live-match-restart-smoke.log`: `NextMatchPending nextMap=splitroot_valley countdownSeconds=3.0` and `NextMatchLoading map=splitroot_valley`.
- `Saved/Proof/PlaytestLoopAdoptionProof/Saved/Proof/playtest-loop.log`: `LoopEvent: bot match session launched (adopting build 2026-06-11T06:12:38.6363254Z)`, `LoopEvent: newer build detected - session recycled oldBuild=2026-06-11T06:12:38.6363254Z newBuild=2026-06-11T06:27:40.2737102Z`, then `LoopEvent: bot match session launched (adopting build 2026-06-11T06:27:40.2737102Z)`.
- `Saved/Proof/last-bot-match.log`: structure pressure reached `DefenseTowerDamaged`, `BotStructureHit`, `AttackCore`, and `BaseCoreDamaged` markers during the 300-second bot smoke.

Remaining bounded risk:
- The overnight loop can now be audited for build adoption, recycle events, match config, pending next map, and travel/load metadata from durable proof logs.
- Bot pathing/body-pressure is still the next canary risk: the passing durable bot proof used a 300-second observation window, and the log still shows many `BotStuckRecovery` markers while objective pressure ramps up.
