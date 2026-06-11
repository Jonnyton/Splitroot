# 016 — Host supervisor and latest-build boundary

Goal: decide and implement the next host-loop layer after Gauge's file-driven map rotation slice. Live ServerTravel keeps players in the match flow, but it cannot reload newly compiled game code. If Jonathan wants each new match to pick up a newly compiled DLL without manually relaunching, the canary needs a supervised process/reconnect design, not just map travel.

Current state from Gauge:

- Normal hosted bot matches travel to the next match map in-process and preserve `ArchonBotMatch` plus `listen`.
- `games/splitroot/FactoryContracts/map_registry.json` is now intended to be the runtime map-rotation source, so Quarry can add a shipped second match map there without touching C++ literals.
- Proof restart mode no longer carries `ArchonMatchProofQuit` unless the URL explicitly includes `ArchonMatchProofQuitAfterTravel`.
- `games/splitroot/Proof/host-loop-forever-smoke.ps1` is the proof that restart travel boots a second match without an accidental proof quit; it kills the process only after observing the second match.

Gauge proof artifacts from this pass:

- `Saved/Proof/last-policy-build-and-test.json` — build `0`, automation `0`, 213/0 tests.
- `Saved/Proof/last-host-loop-forever-smoke.json` — second match booted, bot flag survived, no proof quit carried.
- `Saved/Proof/last-live-match-restart-smoke.json` — controlled quit-after-travel proof still works when explicitly requested.
- `Saved/Proof/last-map-rotation-smoke.json` — rotation proof still works with the explicit quit-after-travel flag.

Open implementation choice:

1. Keep in-process ServerTravel as the primary gameplay path. This preserves players and can pick up JSON-authored map rotation changes at the next match, but not newly compiled C++.
2. Add a supervised host process mode. The launcher restarts Unreal between matches so the latest built module loads, but clients need a pending/reconnect path and it will not be truly seamless until reconnect UX exists.
3. Hybrid: default desktop icon uses in-process forever hosting; add a separate proof/ops launcher for fresh-process-per-match build adoption.

Done-when:

- The chosen behavior is explicit in `Launchers/Play-CurrentBuild.cmd` and `games/splitroot/Launchers/Play-ArchonFactoryCanary.cmd`.
- The host can stop intentionally without being relaunched by the supervisor.
- A proof records whether the next match reused the same process or loaded a new process, using `BuildFingerprint processStartUtc`.
- Coordination with `015-next-match-pending-ui-and-metadata.md` is maintained so players see a pending-next-match state instead of a silent quit/relaunch.

Report-to:

- Move this file to `Coordination/done/` with proof JSON paths and the exact launcher behavior.

## Result

Completed by Hex on 2026-06-11.

Chosen behavior:
- Hybrid.
- `Launchers/Play-CurrentBuild.cmd` and `games/splitroot/Launchers/Play-ArchonFactoryCanary.cmd` now explicitly state that Jonathan's visible current-build launcher uses in-process Unreal ServerTravel for match-to-match continuity.
- The desktop/current-build launcher can adopt JSON-authored map rotation at the next match, but it does not reload newly compiled C++ DLLs.
- Fresh C++ build adoption remains the supervised ops/proof path in `Proof/playtest-loop.ps1`, which stops for `Saved/Proof/build-lock.flag`, relaunches after the build, and writes durable adoption events to `Saved/Proof/playtest-loop.log`.

Changed files:
- `Launchers/Play-CurrentBuild.cmd`
- `games/splitroot/Launchers/Play-ArchonFactoryCanary.cmd`
- `games/splitroot/Proof/host-loop-forever-smoke.ps1`

Proof outcomes:
- `powershell -ExecutionPolicy Bypass -File .\Proof\local-proof-checks.ps1`: exit `0`; stdout confirmed the desktop shortcut exists and targets `C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Launchers\Play-CurrentBuild.cmd` with working directory `C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory`.
- `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\host-loop-forever-smoke.ps1`: exit `0`; proof killed its own process only after observing the second match.
- `Saved/Proof/last-host-loop-forever-smoke.json`: `ProcessBoundary=in_process_server_travel`, `BuildFingerprintCount=2`, `ProcessStartUtcValues=["2026-06-11T15:48:46.882Z","2026-06-11T15:48:46.882Z"]`, `UniqueProcessStartUtcValues=["2026-06-11T15:48:46.882Z"]`, `ReusedSameProcessAcrossTravel=true`, `NextMatchPending=true`, `NextMatchLoading=true`, `NoProofQuitCarried=true`, `NoQuitCommandBeforeHarness=true`.

Key proof markers:
- `Saved/Proof/last-host-loop-forever-smoke.log`: first match `BuildFingerprint moduleDllUtc=2026-06-11T15:44:57.000Z processStartUtc=2026-06-11T15:48:46.882Z`.
- `Saved/Proof/last-host-loop-forever-smoke.log`: `NextMatchPending nextMap=splitroot_valley countdownSeconds=3.0`, then `TravelRequested reason=match_complete`, then `NextMatchLoading map=splitroot_valley`.
- `Saved/Proof/last-host-loop-forever-smoke.log`: second match `BuildFingerprint moduleDllUtc=2026-06-11T15:44:57.000Z processStartUtc=2026-06-11T15:48:46.882Z`, proving this path reused the same Unreal process.

Boundary note:
- This closes the default launcher decision, not reconnect UX. A future fresh-process-per-match player-facing mode still needs client reconnect/pending UI before it should replace visible in-process hosting.
