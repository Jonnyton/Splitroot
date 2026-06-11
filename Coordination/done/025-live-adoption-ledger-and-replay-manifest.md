# 025 - Live adoption ledger and replay manifest

Goal: make each live match prove what source, mechanics, maps, and contracts it
actually contains, without stopping Jonathan's visible current-build process.

Context:
- Gauge's 2026-06-11 audit found the live match loop healthy on PID `26024`,
  but the running DLL is older than several source and contract files.
- Current live metadata can prove the loaded DLL timestamp and match config,
  but it cannot answer "which newer mechanics are coded, built, adopted,
  blocked by handoff, or missing from this match?"
- `host-refresh-pending.json` currently reports `Pending=false` even when
  source files newer than the DLL mean the live process cannot contain the
  latest C++ mechanics.
- Do not close, restart, replace, minimize, or focus-steal from Jonathan's
  visible canary for this task.

Steps:
1. Add a non-disruptive adoption ledger script or extend an existing read-only
   proof script. Suggested artifact:
   `Saved/Proof/live-adoption-ledger.json`.
2. The ledger should compare:
   - current live `BuildFingerprint`;
   - current module DLL UTC;
   - newest source UTC under `Source/ArchonFactoryCanary`;
   - newest per-game contract UTC under `games/splitroot/FactoryContracts`;
   - active `Coordination/inbox-hex` tasks;
   - blocked/staged adoption tasks in `Coordination/done`.
3. Distinguish these states:
   - `live_adopted`: marker exists in the running log for the loaded build;
   - `built_not_live`: proof/build artifact exists, but live marker absent;
   - `coded_not_built`: source is newer than the loaded DLL;
   - `contract_staged_not_loaded`: JSON/contract exists, live marker absent;
   - `blocked_requires_handoff`: task result says it needs a live-session
     handoff or supervisor-owned host;
   - `not_started`: no code/proof marker found.
4. Add or queue C++ log markers for the next safe build boundary:
   - `ReplaySchema version=...`
   - `MatchBuildManifest matchId=... buildId=... moduleDllUtc=...
     processStartUtc=... sourceState=... sourceNewestUtc=...`
   - `MechanicCatalog mechanic=... sourceStatus=... liveStatus=... owner=...`
   - `ContentCatalogLoaded mapRegistryHash=... unitCatalogHash=...
     itemCatalogHash=... botTuningRevision=... botTuningLoaded=...`
   - `HostRefreshState pending=... supervisorOwnsHost=... reason=...`
5. Extend the live snapshot parser to include adoption states for at least:
   - bot strategy tuning loader;
   - bot unstick query;
   - bot firing positions;
   - rally-point reinforcements;
   - tech/shop loop;
   - map registry;
   - unit asset candidate catalog.
6. Keep the first pass read-only if no live-session handoff is available.

Done-when:
- Running the ledger script does not disturb PID `26024`.
- `Saved/Proof/live-adoption-ledger.json` tells a future agent exactly which
  mechanics are live, which are staged, and which are blocked.
- Source-newer-than-DLL is represented as a separate pending source/adoption
  state, not hidden behind `Pending=false`.
- The next-match replay manifest markers are implemented or explicitly queued
  for the next safe C++ adoption boundary.

Report-to:
- Move this file to `Coordination/done/` with `## Result`.
- Include commands, exit codes, JSON/log artifact paths, and a compact table of
  live/staged/blocked mechanics.

## Result

Status: proof_only

Changed scripts/artifacts:

- Used existing `games/splitroot/Proof/live-adoption-ledger.ps1` and updated it
  to:
  - identify this pass as `Agent=Hex`;
  - include `CoordinationStatus` from
    `Saved/Proof/coordination-status-summary.json`;
  - emit `QueuedReplayManifestMarkers` with
    `Status=queued_for_next_safe_cxx_boundary`.
- Did not implement runtime C++ replay markers in the running build, because
  that requires a future safe C++ build/adoption boundary. The ledger queues
  the marker set explicitly:
  - `ReplaySchema version=...`
  - `MatchBuildManifest matchId=... buildId=... moduleDllUtc=... processStartUtc=... sourceState=... sourceNewestUtc=...`
  - `MechanicCatalog mechanic=... sourceStatus=... liveStatus=... owner=...`
  - `ContentCatalogLoaded mapRegistryHash=... unitCatalogHash=... itemCatalogHash=... botTuningRevision=... botTuningLoaded=...`
  - `HostRefreshState pending=... supervisorOwnsHost=... reason=...`

Commands/proof:

- `powershell -ExecutionPolicy Bypass -File .\Coordination\status-summary.ps1`
  - exit `0`
  - artifact: `Saved/Proof/coordination-status-summary.json`
  - after task 026 moved to done: `DoneCount=27`, `ActiveInboxCount=1`,
    `BlockedRequiresHandoffCount=3`, `StagedNotLiveCount=2`,
    `ProofOnlyCount=0`, `CompletedAndLiveCount=0`.
- `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-adoption-ledger.ps1 -TailLines 5000`
  - exit `0`
  - artifact: `Saved/Proof/live-adoption-ledger.json`
  - `Schema=tinyassets.splitroot.live_adoption_ledger.v1`
  - `SourceFilesNewerThanDllCount=33`
  - `ContractFilesNewerThanDllCount=2`
  - `HostRefresh.Pending=false`
  - `HostRefresh.SupervisorOwnsHost=false`
  - `RunningUnrealProcesses[0].Id=7680`
- `python .\scripts\check_live_canary_process_safety.py`
  - exit `0`
  - output: `OK - no broad live-canary process stop patterns found`

Compact mechanic table from `Saved/Proof/live-adoption-ledger.json`:

| mechanic | adoption state | live tail markers | snapshot count | blocked task |
| --- | --- | ---: | ---: | --- |
| `bot_strategy_tuning_loader` | `blocked_requires_handoff` | 0 | 0 | true |
| `bot_unstick_query` | `coded_not_built` | 0 | 0 | false |
| `bot_firing_positions` | `blocked_requires_handoff` | 0 | 0 | true |
| `rally_point_reinforcements` | `blocked_requires_handoff` | 0 | 0 | true |
| `tech_shop_loop` | `coded_not_built` | 0 | 0 | false |
| `map_registry_rotation` | `coded_not_built` | 0 | 0 | false |
| `unit_asset_candidate_catalog` | `blocked_requires_handoff` | 0 | 0 | true |

Bounded live-process note:

- Earlier heartbeat evidence observed visible `UnrealEditor.exe` PID `26024`.
- During this pass, read-only process checks and proof artifacts observed the
  current visible `UnrealEditor.exe` as PID `7680` with the same current-build
  command line.
- Hex did not stop, restart, relaunch, minimize, focus-steal from, or replace
  Unreal. No stop/force flag, C++ build, smoke, or import was run.
