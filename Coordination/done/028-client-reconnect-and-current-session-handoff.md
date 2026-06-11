# 028 - Client reconnect and current-session supervisor handoff

Goal: finish the gap between Gauge's supervised hot-reload promotion and the
fully seamless player experience Jonathan wants: connected humans stay in the
flow while the host refreshes at a match boundary, and the current unmanaged
visible session gets a safe handoff path.

Current state, 2026-06-11:
- `Launchers/Play-CurrentBuild.cmd` now routes through the SPLITROOT launcher,
  and `games/splitroot/Launchers/Play-ArchonFactoryCanary.cmd` starts
  `Proof/host-supervisor-loop.ps1` with `-AllowBoundaryRefresh`,
  `-BuildAtBoundary`, `-RestartOnCrash`, and `-ForceLaunch`.
- A hidden supervisor is currently running as PID `21252`. It is observing the
  existing visible unmanaged Unreal PID `7680` and explicitly will not attach
  to or stop it.
- Gauge added source/build detection in `Proof/host-supervisor-loop.ps1`.
  When the supervisor owns the host, source newer than the module DLL causes a
  build at `NextMatchPending`, then a fresh-process relaunch for the next
  match.
- Gauge added `Proof/host-supervisor-hot-reload-proof.ps1`. The proof passes
  with a fake host: pending source detected, own host stopped, fake build run,
  new fake host launched, module timestamp advanced, external Unreal PID kept
  alive.
- Runtime C++ build stamp source is staged but not live in PID `7680`:
  `BuildVersion`, `BuildEffectiveUtc`, and `BuildModuleUtc` replicate through
  `FArchonMatchNetSnapshot`, `BuildVersionEffective` logs at match start, and
  a top-right HUD stamp renders `BUILD ... / EFFECTIVE ...`.
- The staged C++ was not compiled because `Proof/build-locked.ps1` correctly
  refused while visible Unreal PID `7680` was still running.

Steps:
1. Do not stop PID `7680` unless Jonathan explicitly hands over the visible
   session or it exits naturally.
2. If PID `7680` exits naturally, confirm supervisor PID `21252` starts the
   next host. Then wait for the first supervised match boundary and verify the
   source-newer-than-DLL build/relaunch path with real `BuildVersionEffective`
   log markers.
3. Replace the current client reconnect placeholder with a visible client UI
   state for `host_refreshing_build_reconnecting`. Clients should show a
   pending/rejoin state instead of looking booted to desktop.
4. After the first safe real build adoption, run:
   - `python .\scripts\check_live_canary_process_safety.py`
   - `powershell -ExecutionPolicy Bypass -File .\Proof\host-supervisor-hot-reload-proof.ps1`
   - `powershell -ExecutionPolicy Bypass -File .\Proof\host-supervisor-refresh-proof.ps1`
   - the smallest Unreal smoke that proves the top-right HUD stamp is visible.
5. Update `Saved/Proof/live-adoption-ledger.json` so the build stamp and
   supervisor hot-reload mechanism move from staged/proof-only to live-adopted.

Done-when:
- A supervisor-owned real match reaches `NextMatchPending`, builds if source is
  newer than the DLL, relaunches the host, and the next match logs a newer
  `BuildVersionEffective`.
- The top-right HUD visibly shows the effective build version and timestamp in
  the real running game.
- Connected clients have a real pending/reconnect state, not only metadata.
- Proof JSON records that the external unmanaged PID was not force-stopped.

Report-to:
- Move this file to `Coordination/done/` with `Status: completed`,
  `Status: staged_not_live`, or `Status: blocked_requires_handoff`, plus proof
  command outputs and artifact paths.

## Result

Status: blocked_requires_handoff

Hex result time: 2026-06-11T17:30Z.

Live-session boundary:
- Visible current-build process remained `UnrealEditor.exe` PID `7680`.
- Hidden supervisor process `powershell.exe` PID `21252` is running
  `Proof\host-supervisor-loop.ps1` with `-AllowBoundaryRefresh`,
  `-BuildAtBoundary`, `-RestartOnCrash`, and `-ForceLaunch`.
- `Saved\Proof\host-supervisor-loop.log` shows the supervisor is observing
  existing live current-build PID `7680` and "will not attach or stop it".
- No Unreal process was closed, killed, restarted, minimized, focus-stolen, or
  replaced by this task.
- No build, relink, asset import, editor commandlet, or real supervisor
  promotion was attempted.

Proof:
- Process check:
  - PID `7680`: `UnrealEditor.exe` running
    `ArchonFactoryCanary.uproject /Engine/Maps/Entry?ArchonMainMenu -game`.
  - PID `21252`: `powershell.exe` running `Proof\host-supervisor-loop.ps1`.
- `Saved\Proof\host-supervisor-loop.pid.json`
  - `Pid=21252`
  - `AllowBoundaryRefresh=true`
  - `BuildAtBoundary=true`
  - `BuildScriptPath=...\Proof\build-locked.ps1`
- `Saved\Proof\host-refresh-pending.json`
  - `Pending=true`
  - `Reason=breacher_focus_v3_staged_requires_supervisor_handoff`
  - `BoundaryPolicy=restart_on_next_match_when_supervisor_owns_host`
  - `SupervisorOwnsHost=false`
  - `CurrentProcessId=7680`
  - `ReconnectWindowSeconds=30`
- `Saved\Proof\last-host-supervisor-hot-reload-proof.json`
  - `SupervisorExitCode=0`
  - `ProcessBoundary=fresh_process_relaunch`
  - `BoundaryPidChanged=true`
  - `PendingStateEmitted=true`
  - `FinalPendingCleared=true`
  - `BuildAttempted=true`
  - `BuildSucceeded=true`
  - `ModuleTimestampAdvanced=true`
  - `ClientReconnectPlaceholder=true`
  - `SupervisorOwnedOnly=true`
  - `ExternalCurrentBuildStillAlive=true`
  - external PIDs before/after: `7680` / `7680`
- `Saved\Proof\hex-live-canary-watch-latest.json`
  - `RunningUnrealProcesses[0].Id=7680`
  - `HostRefresh.Pending=true`
  - `HostRefresh.SupervisorOwnsHost=false`
  - `HotspotManifest.AdoptionState=blocked_requires_handoff`
  - `HotspotManifest.LiveBotStrategyTuningLoadedCount=0`
- `python .\scripts\check_live_canary_process_safety.py`: exit `0`,
  `OK - no broad live-canary process stop patterns found`.

Bounded conclusion:
- The supervisor-owned fake-host proof covers the safe build/relaunch mechanism
  and proves external PID `7680` is not touched by that harness.
- The real desktop current build is still unmanaged. Until PID `7680` exits
  naturally or Jonathan explicitly hands over the visible session, the
  supervisor must keep observing and must not attach, stop, or replace it.
- The client reconnect UX remains a placeholder in pending metadata; a real
  visible client UI state is not live-adopted in PID `7680`.

Post-move verification:
- A later local check showed the supervisor loop PID had rolled from `21252` to
  `19472`.
- `Saved\Proof\host-supervisor-loop.pid.json` now reports `Pid=19472`,
  `AllowBoundaryRefresh=true`, and `BuildAtBoundary=true`.
- `Saved\Proof\host-supervisor-loop.log` still reports the supervisor is
  observing existing live current-build PID `7680` and will not attach or stop
  it.
- Visible Unreal PID `7680` remained running.

Gauge follow-up correction, 2026-06-11T17:32Z:
- Patched `Proof/host-supervisor-loop.ps1` so the unmanaged-session observer
  reads `Saved\Logs\ArchonFactoryCanary.log` and refreshes
  `host-refresh-pending.json` while it observes a direct desktop launch.
- Fixed supervisor lock return-value pollution by changing
  `Write-SupervisorEvent` to avoid emitting into function pipelines; duplicate
  launcher calls now leave only one resident supervisor.
- Current resident supervisor is PID `19472`; visible Unreal remains PID
  `7680`.
- Current `Saved\Proof\host-refresh-pending.json` reports
  `Pending=true`, `Reason=source_newer_than_module_boundary_build_pending`,
  `SupervisorMode=observing_existing_live_session_no_stop`,
  `SupervisorOwnsHost=false`, and `CurrentProcessId=7680`.
- Final harness proofs rerun after the correction:
  - `Saved\Proof\last-host-supervisor-hot-reload-proof.json`:
    `BuildAttempted=true`, `BuildSucceeded=true`,
    `ModuleTimestampAdvanced=true`, `ExternalCurrentBuildStillAlive=true`.
  - `Saved\Proof\last-host-supervisor-refresh-proof.json`:
    `ProcessBoundary=fresh_process_relaunch`, `BoundaryPidChanged=true`,
    `ExternalCurrentBuildStillAlive=true`.

Final launcher-normalization check, 2026-06-11T17:34Z:
- `Launchers/Play-CurrentBuild.cmd` and
  `games/splitroot/Launchers/Play-ArchonFactoryCanary.cmd` now normalize
  `PROJECT_ROOT` before starting the supervisor.
- Resident supervisor PID rolled to `580` after restarting only the hidden
  supervisor process. Visible Unreal PID `7680` remained running.
- `Saved\Proof\host-supervisor-loop.pid.json` now records
  `ProjectRoot=C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory`,
  `Pid=580`, `AllowBoundaryRefresh=true`, and `BuildAtBoundary=true`.
