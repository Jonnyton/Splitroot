# 021 - Bot objective firing-position adoption proof

Goal: adopt Gauge's bot objective firing-position mechanic into the running
build and prove the live replay stream sees bots using it.

Context:
- Gauge added a deterministic stand-off lane policy in
  `UArchonBotSteeringPolicyLibrary::ComputeObjectiveStandOffTarget`.
- `UArchonBotBrainComponent` now routes post-waypoint base attackers toward
  tower/core firing lanes instead of the exact structure origin.
- The mechanic logs:
  - `ArchonFactoryCanary: BotFiringPosition ...`
  - `BotFeatureUse ... feature=take_firing_position ...`
  - `BotFeatureCoverage feature=take_firing_position ...`
- Gauge did not run a C++ build or bot-match smoke because Jonathan's visible
  current-build Unreal process was live and must not be closed/recycled without
  explicit handoff.
- Last read-only live snapshot before adoption showed old DLL evidence:
  `BotFiringPositionCount=0`, `TakeFiringPositionCovered=false`,
  `BotStuckRecoveryCount=531`, running visible PID `26024`.

Steps:
1. When Jonathan explicitly hands over the visible current build, or when the
   host supervisor can adopt fresh C++ without booting players, compile the
   module and automation tests.
2. Relaunch/verify the current build through the existing desktop shortcut
   path only:
   `C:\Users\Jonathan\Desktop\SPLITROOT Current Build.lnk`.
3. Run the bot proof after the new DLL is active:
   `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\bot-match-smoke.ps1`
4. Sample the live loop without closing it:
   `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1 -RequireBotEvidence`
5. Compare the first adopted-match snapshot against the pre-adoption baseline:
   `BotFiringPositionCount` should be greater than zero,
   `TakeFiringPositionCovered` should be true, and `BotStuckRecoveryCount`
   should trend down over comparable match windows if the lane mechanic is
   helping.

Done-when:
- Build and automation pass on the adopted source.
- `Saved\Proof\last-bot-match-smoke.json` passes with
  `BotFiringPositionLogged=true` and `TakeFiringPositionCovered=true`.
- `Saved\Proof\last-live-bot-match-log-snapshot.json` from the running loop
  shows `TakeFiringPositionCovered=true`.
- The visible current build is running again after adoption, with PID/log
  state reported.

Report-to:
- Move this file to `Coordination\done\` with a `## Result` section.
- Include build/proof commands, exit codes, JSON/log artifact paths, sample
  `BotFiringPosition` and `BotFeatureCoverage feature=take_firing_position`
  lines, and whether stuck-recovery frequency improved or still needs Vector.

## Result

Status: blocked_requires_handoff

Blocked by required live-session handoff on 2026-06-11. The task requires compiling/adopting new C++ and then proving the newly adopted DLL in the running match stream. Jonathan's visible SPLITROOT current-build process was live and was not handed over for interruption, so no build, relaunch, bot-match smoke, or process-boundary refresh was run.

Evidence checked without disturbing the live game:
- `Saved/Proof/last-live-bot-match-log-snapshot.json` from `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1`: `EvidenceReady=true`, live `UnrealEditor.exe` PID `26024`, `MatchLoopActive=true`, `BotMatchSpawned=true`.
- Same snapshot still reports `BotFiringPositionCount=0` and `TakeFiringPositionCovered=false`, so the currently running DLL has not adopted the firing-position mechanic.
- `Saved/Proof/host-refresh-pending.json` from `powershell -ExecutionPolicy Bypass -File .\Proof\host-refresh-pending-state.ps1 -Action snapshot`: `Pending=false`, `CurrentProcessId=26024`, `BoundaryPolicy=restart_on_next_match_when_supervisor_owns_host`.
- `python scripts/check_live_canary_process_safety.py`: exit `0`, no broad live-canary stop patterns found.

Bounded outcome:
- No claim that `BotFiringPosition` or `take_firing_position` works in the live build.
- Next required action is an explicit Jonathan handoff or a promoted supervisor-owned host launch, then build/adopt the C++ DLL and run the requested bot proofs.
