# 033 - Client reconnect UI for host refresh

Goal: replace the current host-refresh metadata placeholder with real player
UX for connected clients when the supervisor refreshes the host at a match
boundary.

Steps:
1. Read `Coordination/done/032-hot-reload-live-boundary-adoption.md` and
   `Saved/Proof/last-host-supervisor-refresh.json`.
2. Add a visible client state for `host_refreshing_build_reconnecting` so
   humans are not left looking booted or disconnected without explanation.
3. Keep players in the game flow across host refresh where possible: scoreboard
   or pending-next-match state first, reconnect/join affordance second, desktop
   exit only when the user quits.
4. Extend replay/meta output so refresh start, refresh completion, old PID,
   new PID, build version, and reconnect UI state are captured together.

Done-when:
- A client connected before boundary refresh gets a visible pending/reconnect
  state instead of a silent drop.
- The next match after refresh shows the latest top-right build stamp.
- Replay/meta data contains the reconnect state and the final adopted build
  fingerprint.

Report-to:
- Move this file to `Coordination/done/` with `Status: completed`,
  `Status: staged_not_live`, or `Status: blocked_requires_handoff`.

## Result

Status: blocked_requires_handoff

Hex result time: 2026-06-11T18:27Z.
Gauge follow-up proof time: 2026-06-11T18:42Z.

Read:
- `Coordination/done/032-hot-reload-live-boundary-adoption.md`.
- `Saved/Proof/last-host-supervisor-refresh.json`.
- `Saved/Proof/host-refresh-pending.json`.

Bounded proof:
- Latest supervisor runtime-contract refresh proof:
  `Saved/Proof/last-host-supervisor-refresh.json`.
  - `BeforePid=11920`.
  - `AfterPid=7128`.
  - `PendingReason=runtime_contract_newer_than_process_boundary_refresh_pending`.
  - `BuildAttempted=false`.
  - `BuildRequired=false`.
  - `ClientReconnectPlaceholder=true`.
  - `TouchedExternalLiveProcess=false`.
- Current host-refresh pending state:
  `Saved/Proof/host-refresh-pending.json`.
  - `Pending=true`.
  - `Reason=source_newer_than_module_boundary_build_pending`.
  - `CurrentProcessId=7128`.
  - `SupervisorOwnsHost=true`.
  - `SupervisorMode=supervisor_owned_waiting_for_match_boundary`.

Outcome:
- This task is not implemented. The current proof still shows placeholder
  reconnect metadata, not a visible connected-client reconnect UI.
- A real done condition needs C++/UI work plus connected-client evidence across
  a supervisor-owned match-boundary refresh. That requires a future adoption
  boundary and client proof; Hex did not force or manually trigger that in this
  pass.

Gauge follow-up:
- The pending source refresh completed at the next supervisor-owned boundary.
- `Saved/Proof/last-host-supervisor-refresh.json` now shows `BeforePid=15248`,
  `AfterPid=25396`, `BuildAttempted=true`, `PolicyBuildExitCode=0`,
  `BuildSucceeded=true`, and `ClientReconnectPlaceholder=true`.
- `Saved/Proof/host-refresh-pending.json` now shows `Pending=false`,
  `Reason=supervisor_host_started_or_refreshed`.
- `Saved/Proof/last-live-bot-match-log-snapshot.json` now shows
  `version=v20260611-183856` and
  `effectiveUtc=2026-06-11T18:40:52.282Z`.
- Current live supervisor is PID `17384` and current Unreal host is PID `5064`.
- The live supervisor is running with `-ForceLaunch`; the normal launcher path
  also includes `-ForceLaunch`, so host relaunch is not blocked by Jonathan
  using the machine.
- This confirms hot reload adoption, but it also confirms the client reconnect
  task remains open because the proof still says `ClientReconnectPlaceholder=true`.

Next:
- Implement visible client state for `host_refreshing_build_reconnecting`.
- Extend replay/meta markers to pair refresh start/completion with old PID, new
  PID, build fingerprint, and reconnect UI state.
- Prove with connected-client or harness evidence before marking completed.
