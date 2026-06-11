# 032 - Hot reload live boundary adoption

Goal: make the running SPLITROOT host adopt newer code at match boundaries
without Jonathan relaunching the game, and show the loaded build version in
the top-right HUD.

## Result

Status: completed

Gauge result time: 2026-06-11T18:22Z.
Gauge follow-up proof time: 2026-06-11T18:42Z.

Implemented:
- `Proof/host-supervisor-loop.ps1` can build at a match boundary, relaunch the
  supervisor-owned host, adopt an unmanaged host at `NextMatchPending`, and use
  a stale-live fallback when a pending build waits too long.
- `Launchers/Play-CurrentBuild.cmd` and
  `games/splitroot/Launchers/Play-ArchonFactoryCanary.cmd` start the hidden
  supervisor for the desktop current build.
- `AArchonMatchStateActor` now logs and replicates build version, effective
  UTC, module DLL UTC, and runtime input UTC.
- `UArchonCanaryWorldSubsystem` shows the top-right `BUILD ... / EFFECTIVE ...`
  HUD stamp.
- Runtime/content inputs under `Config`, `FactoryContracts`, `Content`,
  `games/splitroot/FactoryContracts`, and `games/splitroot/Content` are tracked
  for next-boundary refresh, not only C++ source.
- Generated thumbnail previews under `FactoryContracts/thumbs/generated` are
  excluded from runtime adoption so asset-preview churn does not cause endless
  host refreshes.

Live proof:
- External unmanaged handoff: direct-launched PID `7680` was adopted at
  `2026-06-11T17:51:59Z`; supervisor relaunched PID `25584`.
- First supervised code adoption: PID `25584` stopped at
  `2026-06-11T18:09:45Z`; boundary build advanced the module from
  `2026-06-11T17:53:03.5114874Z` to `2026-06-11T18:09:57.5300563Z`; new PID
  `7792` launched and logged `version=v20260611-180957`.
- A later pending source change initially failed to compile in
  `ArchonPlayerInputBridgeComponent.cpp`; Gauge fixed the Slate
  `Text_Lambda` rvalue binding.
- Final adoption: PID `4996` stopped at `2026-06-11T18:19:07Z`; boundary build
  advanced the module from `2026-06-11T18:09:57.5300563Z` to
  `2026-06-11T18:19:14.9705517Z`; new PID `11920` launched.
- Live log proof:
  `BuildVersionEffective version=v20260611-181914 effectiveUtc=2026-06-11T18:19:41.114Z moduleDllUtc=2026-06-11T18:19:14.000Z runtimeInputUtc=2026-06-11T18:03:04.000Z`.
- Runtime-input adoption proof: PID `11920` stopped at
  `2026-06-11T18:23:00Z` for
  `runtime_contract_newer_than_process_boundary_refresh_pending`; new PID
  `7128` launched and logged
  `BuildVersionEffective version=v20260611-182204 effectiveUtc=2026-06-11T18:23:08.945Z moduleDllUtc=2026-06-11T18:19:14.000Z runtimeInputUtc=2026-06-11T18:22:04.000Z`.
- Follow-up source adoption proof: PID `7128` stopped at the next match
  boundary; boundary build advanced the module to
  `2026-06-11T18:27:59.000Z`; new PID `21524` launched and logged
  `BuildVersionEffective version=v20260611-182759 effectiveUtc=2026-06-11T18:28:24.916Z moduleDllUtc=2026-06-11T18:27:59.000Z runtimeInputUtc=2026-06-11T18:22:04.000Z`.
- Final source/scanner adoption proof: PID `15248` stopped at
  `2026-06-11T18:35:44Z`; boundary build advanced the module to
  `2026-06-11T18:35:50.1564113Z`; new PID `25396` launched and logged
  `BuildVersionEffective version=v20260611-183550 effectiveUtc=2026-06-11T18:36:16.075Z moduleDllUtc=2026-06-11T18:35:50.000Z runtimeInputUtc=2026-06-11T18:30:57.000Z`.
- The PowerShell supervisor was restarted without stopping Unreal so the live
  loop uses the patched runtime-input policy.
- A temporary supervisor restart without `-ForceLaunch` exposed the active-user
  relaunch guard; Gauge restarted only the supervisor with `-ForceLaunch`
  restored. The normal desktop/per-game launcher already includes `-ForceLaunch`.
- Current live proof: supervisor PID `17384`, Unreal PID `5064`,
  `BuildVersionEffective version=v20260611-183856 effectiveUtc=2026-06-11T18:40:52.282Z moduleDllUtc=2026-06-11T18:35:50.000Z runtimeInputUtc=2026-06-11T18:38:56.000Z`, bot evidence ready.

Proof artifacts:
- `Saved/Proof/last-host-supervisor-refresh.json`: latest proof has
  `BeforePid=15248`, `AfterPid=25396`,
  `RefreshTrigger=next_match_boundary`, `BuildAttempted=true`,
  `PolicyBuildExitCode=0`, `ModuleTimestampAdvanced=true`,
  `BuildSucceeded=true`, `ClientReconnectPlaceholder=true`.
- `Saved/Proof/host-refresh-pending.json`: `Pending=false`,
  current host process `5064`.
- `Saved/Proof/last-live-bot-match-log-snapshot.json`: latest build fingerprint
  is `v20260611-183856`; evidence ready; bot match spawned; tuning loaded.
- `Saved/Proof/live-adoption-ledger.json`: running Unreal PID `5064`, pending
  source count `0`, host refresh pending `false`; one contract JSON is newer
  than the DLL but already adopted by the current process runtime timestamp.
- `python .\scripts\check_live_canary_process_safety.py`: exit 0,
  `OK - no broad live-canary process stop patterns found`.
- `Proof/host-supervisor-hot-reload-proof.ps1`: exit 0, fresh process relaunch,
  pending state emitted and cleared, build attempted/succeeded, module timestamp
  advanced, real live PID stayed alive.

Known remaining gaps:
- Connected-client reconnect is still placeholder metadata, not a visible
  reconnect/pending UI. See `Coordination/done/033-client-reconnect-ui-for-host-refresh.md`.
- Policy automation still has one failing map-layout test:
  `ArchonFactory.Valley.ThreeFactionBasesFormTriangle`, line 177,
  `Lenswright and Kinwild far apart`. See
  `Coordination/done/034-quarry-valley-triangle-layout-test.md`.
