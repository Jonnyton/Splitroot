# 022 - Rally point reinforcement adoption proof

Goal: adopt Gauge's team rally-point mechanic into the running build and prove
that table-set rally points steer purchased reinforcements.

Context:
- Gauge is acting as gameplay-mechanics generalist; this slice is RTS/economy,
  not Vector's AI-intelligence lane.
- Added `EArchonRtsOrderKind::SetRallyPoint`.
- `UArchonTeamRtsStateComponent` now stores replicated team rally state and
  logs `TeamRallyPointSet`.
- `UArchonMapTableWidget` can submit `MapTableRallyPoint` without a selected
  squad.
- `UArchonPlayerInputBridgeComponent` exposes `R` while the map table is open
  and logs `RuntimeRallyPointSet`.
- `UArchonCanaryWorldSubsystem::HandleReinforcementPurchased` uses the team
  rally as the first route waypoint and logs `ReinforcementRallyApplied`.
- Added `games/splitroot/Proof/rally-reinforcement-smoke.ps1`.
- Gauge did not compile/run the smoke because Jonathan's visible current build
  was running and must not be recycled without explicit handoff.

Steps:
1. When Jonathan hands over the visible current build, or when supervisor
   adoption can apply fresh C++ without booting players, run the policy build.
2. Run:
   `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\rally-reinforcement-smoke.ps1`
3. Relaunch/verify the current build through the existing desktop shortcut path
   only:
   `C:\Users\Jonathan\Desktop\SPLITROOT Current Build.lnk`
4. In the live build, open the map table and press `R`; then buy
   reinforcements with `B` once supply allows. The log should show rally and
   reinforcement markers.

Done-when:
- Build and automation pass.
- `Saved\Proof\last-rally-reinforcement-smoke.json` passes with:
  `RuntimeRallySubmitted=true`, `TeamRallyStored=true`,
  `ReinforcementRallyApplied=true`, and `ProofCompleted=true`.
- The visible current build is running again after adoption, with PID/log state
  reported.

Report-to:
- Move this file to `Coordination\done\` with `## Result`.
- Include commands, exit codes, proof JSON/log paths, sample `TeamRallyPointSet`,
  `RuntimeRallyPointSet`, and `ReinforcementRallyApplied` lines.

## Result

Status: blocked_requires_handoff

Blocked by required live-session handoff on 2026-06-11. This task requires compiling/adopting new C++ and running `games/splitroot/Proof/rally-reinforcement-smoke.ps1`. Jonathan's visible SPLITROOT current-build process was live and not handed over, so no build, smoke, relaunch, or real process-boundary refresh was run.

Evidence checked without disturbing the live game:
- `Saved/Proof/last-live-bot-match-log-snapshot.json`: live `UnrealEditor.exe` PID `26024`, `MatchLoopActive=true`, `BotMatchSpawned=true`, `EvidenceReady=true`.
- `Saved/Proof/host-refresh-pending.json`: `Pending=false`, `CurrentProcessId=26024`, `BoundaryPolicy=restart_on_next_match_when_supervisor_owns_host`.
- `python scripts/check_live_canary_process_safety.py`: exit `0`, no broad live-canary stop patterns found.

Bounded outcome:
- No claim that `RuntimeRallyPointSet`, `TeamRallyPointSet`, or `ReinforcementRallyApplied` works in the live build.
- Next required action is an explicit Jonathan handoff or a promoted supervisor-owned host launch, then build/adopt the C++ DLL and run `games/splitroot/Proof/rally-reinforcement-smoke.ps1`.
