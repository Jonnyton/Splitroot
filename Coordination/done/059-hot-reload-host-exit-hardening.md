# Hot Reload Host-Exit Hardening

Goal:
Make current-build hot reload obvious and verifiable so completed slices from
other sessions land in the next match automatically, without Jonathan manually
relaunching the game and without touching the visible Unreal window.

Done-when:
- The desktop/current-build launcher path still routes through the supervisor.
- `Proof/current-build-hot-reload-status.ps1` reports the live status in
  `Saved/Proof/current-build-hot-reload-status.json`.
- The normal boundary and host-exit pending-refresh paths both prove they build
  before relaunching.

## Result

Status: completed

Gauge found a real hole in the supervised hot-reload loop: when a
supervisor-owned host exited while a fresh-build refresh was pending, the loop
could relaunch immediately instead of building first. Normal match-boundary
refresh was already working; the host-exit path was the missed case that could
make a session's completed work look "done" locally but not show up in the next
game Jonathan saw.

Implemented:
- `Proof/host-supervisor-loop.ps1` now treats pending host exit as a build
  boundary, runs the boundary build first, relaunches afterward, and writes
  `ProcessBoundary=host_exit_fresh_process_relaunch` with
  `RefreshTrigger=host_exit_pending_refresh`.
- `Proof/host-supervisor-exit-refresh-proof.ps1` proves that path with a fake
  host, fake source, and fake build while confirming the real visible current
  build PID is not touched.
- `Proof/current-build-hot-reload-status.ps1` is the read-first status probe for
  future sessions. It checks shortcut wiring, launcher flags, supervisor
  liveness, owned host state, pending refresh state, boundary build blocks, and
  latest refresh proof.
- `Proof/local-proof-checks.ps1` now includes the hot-reload status probe in the
  local proof surface.

Proof run by Gauge:
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\Proof\host-supervisor-exit-refresh-proof.ps1`
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\Proof\host-supervisor-hot-reload-proof.ps1`
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\Proof\host-supervisor-refresh-proof.ps1`
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\Proof\host-supervisor-external-takeover-proof.ps1 -Scenario boundary`
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\Proof\current-build-hot-reload-status.ps1`
- `python scripts\check_live_canary_process_safety.py`

Operational note:
The live Unreal game must stay running. If the active supervisor process was
started before this patch, restart only the supervisor loop when appropriate;
do not close or replace the visible game window.
