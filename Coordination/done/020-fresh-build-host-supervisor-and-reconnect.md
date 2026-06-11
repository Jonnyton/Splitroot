# 020 - Fresh-build host supervisor and reconnect continuity

Goal: make Jonathan's hosted game run forever without manual relaunch while each new match can adopt the latest built game version and connected humans have a pending/rejoin path instead of being stranded.

Jonathan correction, 2026-06-11:
- Once a host starts a game, the game should continue match to match until the host intentionally quits.
- Humans should not have to reopen the desktop icon after a match or after a host-process refresh.
- All connected human players should stay in the game flow: scoreboard/pending next match, then ready/joined into the next match.
- Each new match should pick up latest game content and new maps. JSON-authored map registry changes are already possible in-process; newly compiled C++ still needs a fresh process boundary.
- "Fresh build" must not be interpreted as an agent killing Jonathan's visible current-build window. If Unreal cannot hot-load a compiled C++ DLL into the running process, preserve the live game and emit pending-build metadata until a host-safe supervisor/reconnect path exists.

Current state:
- Default current-build launcher uses in-process Unreal `ServerTravel`.
- In-process travel preserves `listen`, `ArchonBotMatch`, and connected players better than a process restart.
- In-process travel can pick up runtime `games/splitroot/FactoryContracts/map_registry.json` changes for map rotation on the next match.
- In-process travel cannot reload newly compiled `UnrealEditor-ArchonFactoryCanary.dll`.
- Gauge fixed the visible-loop crash class by making the human playtest screenshot recorder opt-in (`ArchonPlaytestRecorder`) and added `games/splitroot/Proof/rendered-host-loop-forever-smoke.ps1`.
- Jonathan has explicitly told the agent team to stop closing the running game. Treat the visible SPLITROOT Current Build as an always-running live service and telemetry source unless Jonathan intentionally quits or explicitly hands it over for a build/restart.

Required design:
1. Keep in-process `ServerTravel` as the normal player-continuity path.
2. Add a supervised host mode that can intentionally refresh between matches only when a fresh build is available or requested, without silently dropping the visible game to desktop.
3. Before a fresh-process restart, emit a durable pending state: next map, host endpoint, restart reason, build fingerprint, and expected reconnect window.
4. Add a client pending/reconnect path. A connected client should show "host refreshing build / reconnecting" instead of looking like the game quit.
5. Host intentional quit must stop the supervisor. Crashes may restart; deliberate quit should not.
6. The desktop shortcut behavior must be explicit: either normal in-process forever hosting, or supervised fresh-build hosting if that becomes ready enough for Jonathan's visible build.
7. Any proof/build script that launches and kills its own Unreal process must prove it is not touching Jonathan's visible current-build process.
8. Until this supervisor is proven, scripts should log `pendingHostSafeRefresh=true` for newer C++ builds and keep the live session running.

Proof needed:
- Existing in-process proof remains green:
  - `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\host-loop-forever-smoke.ps1`
  - `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\rendered-host-loop-forever-smoke.ps1`
- New supervisor proof should show a process boundary:
  - match 1 `BuildFingerprint processStartUtc=A`;
  - pending refresh state emitted;
  - host process exits intentionally for refresh, not crash;
  - match 2 boots with `processStartUtc=B`;
  - latest module timestamp or build id is recorded;
  - client/reconnect placeholder path sees pending/rejoin metadata.

Done-when:
- Jonathan can start the host once and leave it alone.
- A normal match end never silently drops to desktop.
- If the process refreshes for a new build, the supervisor brings the host back and clients have a recorded reconnect flow.
- The exact remaining gap between "same-process seamless travel" and "fresh-process build adoption" is visible in proof JSON, not hidden in comments.

Report-to:
- Move this file to `Coordination/done/` with changed files, proof commands, proof JSON/log paths, and whether the desktop shortcut was switched to supervised mode or left on in-process mode.

## Result

Completed by Hex on 2026-06-11 as a non-disruptive supervisor canary slice. Jonathan's visible SPLITROOT current-build process was not stopped, relaunched, minimized, focus-stolen, or replaced.

Changed files:
- `Proof/host-refresh-pending-state.ps1`
- `Proof/host-supervisor-loop.ps1`
- `Proof/host-supervisor-refresh-proof.ps1`
- `Proof/playtest-loop.ps1`
- `Proof/playtest-loop-adoption-proof.ps1`
- `Launchers/Play-CurrentBuild.cmd`
- `games/splitroot/Launchers/Play-ArchonFactoryCanary.cmd`

Behavior added:
- `Proof/playtest-loop.ps1` now records `Saved/Proof/host-refresh-pending.json` when a newer C++ DLL or build lock is detected while a live session is running. It keeps the session alive and marks `pendingHostSafeRefresh=true` instead of recycling.
- `Proof/host-refresh-pending-state.ps1` writes the durable pending/reconnect contract: current process id, current/pending module timestamps, next map, host endpoint, reconnect window, supervisor ownership, and client placeholder state.
- `Proof/host-supervisor-loop.ps1` only refreshes a host it launched and recorded as `SupervisorOwnsHost=true`; existing desktop current-build sessions are observed, not attached to or stopped.
- Desktop/current-build launcher remains normal in-process mode. It was not switched to supervised mode. The comments now state that fresh C++ adoption belongs to the opt-in supervisor path after promotion.

Proof outcomes:
- PowerShell parser check: passed for `host-refresh-pending-state.ps1`, `host-supervisor-loop.ps1`, `host-supervisor-refresh-proof.ps1`, `playtest-loop.ps1`, and `playtest-loop-adoption-proof.ps1`.
- `python scripts/check_live_canary_process_safety.py`: exit `0`, `OK - no broad live-canary process stop patterns found`.
- `powershell -ExecutionPolicy Bypass -File .\Proof\playtest-loop-adoption-proof.ps1`: exit `0`; `Saved/Proof/playtest-loop-adoption-proof.json` reports `LaunchCount=1`, `PendingDetectedCount=1`, `OldRecycleCount=0`, `SessionStayedOnOriginalDll=true`, `PendingStateWritten=true`, `PendingStateReason=newer_cxx_build_detected_live_session_kept_running`, `PendingStateSupervisorOwnsHost=false`.
- `powershell -ExecutionPolicy Bypass -File .\Proof\host-supervisor-refresh-proof.ps1`: exit `0`; `Saved/Proof/last-host-supervisor-refresh-proof.json` reports `ProcessBoundary=fresh_process_relaunch`, fake host `BeforePid=12676`, fake host `AfterPid=18192`, `BoundaryPidChanged=true`, `PendingStateEmitted=true`, `FinalPendingCleared=true`, `ClientReconnectPlaceholder=true`, `SupervisorOwnedOnly=true`, and `ExternalCurrentBuildStillAlive=true`.
- `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1`: exit `0`; `Saved/Proof/last-live-bot-match-log-snapshot.json` reports the real live `UnrealEditor.exe` PID `26024`, `MatchLoopActive=true`, `BotMatchSpawned=true`, and `EvidenceReady=true`.
- `powershell -ExecutionPolicy Bypass -File .\Proof\host-refresh-pending-state.ps1 -Action snapshot`: exit `0`; `Saved/Proof/host-refresh-pending.json` records current live PID `26024`, `Pending=false`, `CurrentModuleDllUtc=2026-06-11T15:59:42.000Z`, `NextMap=splitroot_valley`, and `BoundaryPolicy=restart_on_next_match_when_supervisor_owns_host`.

Bounded caveats:
- No real Unreal process-boundary refresh was performed, because Jonathan's visible live canary is currently the shared telemetry source and was not handed over for interruption.
- Client reconnect is currently a durable metadata placeholder, not a compiled in-game UI flow. The next slice should surface `host_refreshing_build_reconnecting` in the client pending/scoreboard UI after a safe build handoff.
