# Gauge live adoption and mechanics audit - 2026-06-11

Agent lens: Gauge, cross-lane gameplay-mechanics generalist. This pass stayed
non-disruptive: no build, relaunch, stop, minimize, focus steal, import, or
proof process replaced Jonathan's visible SPLITROOT current build.

## Current live evidence

- Visible process: `UnrealEditor.exe` PID `7680`, title
  `Archon Factory Canary (64-bit Development PCD3D_SM6)`.
- Latest live snapshot:
  `Saved/Proof/last-live-bot-match-log-snapshot.json`,
  `SnapshotUtc=2026-06-11T16:59:09.6673010Z`.
- Live build fingerprint:
  `moduleDllUtc=2026-06-11T15:59:42.000Z`,
  `processStartUtc=2026-06-11T16:57:25.543Z`.
- Live loop state: `EvidenceReady=true`, `MatchLoopActive=true`,
  `BotMatchSpawned=true`.
- Host refresh snapshot:
  `Saved/Proof/host-refresh-pending.json`,
  `Pending=false`, `SupervisorMode=not_running`,
  `SupervisorOwnsHost=false`,
  `BoundaryPolicy=restart_on_next_match_when_supervisor_owns_host`.
- Live adoption ledger:
  `Saved/Proof/live-adoption-ledger.json`, observed
  `2026-06-11T17:02:06.1502537Z`, reports `SourceFilesNewerThanDllCount=33`,
  `ContractFilesNewerThanDllCount=2`, `InboxHexCount=0`, `BlockedCount=9`,
  and `StagedNotLiveCount=7`.
- Coordination status summary:
  `Saved/Proof/coordination-status-summary.json` reports `DoneCount=28`,
  `ActiveInboxCount=0`, `BlockedRequiresHandoffCount=3`,
  `StagedNotLiveCount=2`, `ProofOnlyCount=1`, `CompletedCount=1`, and
  `LegacyUnknownCount=21` before the live ledger's broader heuristic counts.
- Process safety check:
  `python scripts/check_live_canary_process_safety.py` returned
  `OK - no broad live-canary process stop patterns found`.

## Main finding

The live game is a useful always-on canary, but it is not currently the newest
source tree. Many source and contract files are newer than
`Binaries/Win64/UnrealEditor-ArchonFactoryCanary.dll`. The running match can
prove the old live build is healthy, but it cannot prove newer mechanics until
a safe build/adoption boundary occurs.

Newer-than-DLL examples found in this pass:

- Bot strategy tuning loader and bot brain/controller edits.
- Bot unstick query and firing-position policy edits.
- Team rally point / map-table rally / reinforcement rally edits.
- Item shop policy and item shop tests.
- Splitroot Valley layout and block actor edits.
- `games/splitroot/FactoryContracts/bot_strategy_tuning.json`.
- `games/splitroot/FactoryContracts/unit_asset_candidates.json`.

## Adoption gaps

1. Coordination `done/` currently contains a mix of completed, proof-only,
   blocked, and staged work. `021-bot-objective-firing-position-adoption.md`
   and `022-rally-point-reinforcement-adoption.md` are in `done/`, but both
   say they are blocked by required live-session handoff and explicitly make no
   live-adoption claim.

2. Fresh C++ adoption is not automatic for the visible desktop host. The
   default launcher correctly preserves player continuity through in-process
   `ServerTravel`, but that path cannot reload the compiled game module. The
   supervisor path exists as a proof/ops canary and only refreshes hosts it
   owns.

3. `host-refresh-pending.json` reports `Pending=false` even though source files
   are newer than the loaded DLL. The current pending contract appears to track
   module/DLL freshness, not the broader "source tree contains mechanics that
   are not in the live process" condition.

4. Vector's bot tuning contract is staged, but not live. The latest snapshot
   reads `BotStrategyTuningRevision=vector-2026-06-11-live-tuning-v1` from the
   JSON file, while `BotStrategyTuningLoadedCount=0` and
   `LatestBotStrategyTuningLoaded=null`.

5. Bot unstick and firing-position improvements are not live. The latest
   snapshot still reports `BotUnstickQueryCount=0`,
   `BotFiringPositionCount=0`, and `TakeFiringPositionCovered=false`, while
   `BotStuckRecoveryCount=96` in the current tail.

6. Rally-point reinforcements are not live. No current live claim exists for
   `RuntimeRallyPointSet`, `TeamRallyPointSet`, or
   `ReinforcementRallyApplied`; task `022` is a blocked adoption task.

7. The RTS tech-to-shop loop has proof-script evidence, but the current
   player-facing gap remains: a physical Renegade-style purchase/equipment
   terminal or respawn equipment UI is not yet proven as normal gameplay.

8. Map and asset lanes have newer contracts, but the live replay stream does
   not yet say which map registry hash, unit asset candidate revision, item
   catalog, or content manifest the match is using.

## Replay metadata gaps to add

The live log already has valuable markers such as `BuildFingerprint`,
`MatchConfig`, `NextMatchPending`, bot feature coverage, and combat pressure.
For multi-session iteration, the match stream also needs:

- `ReplaySchema version=...`
- `MatchBuildManifest matchId=... buildId=... moduleDllUtc=...
  processStartUtc=... sourceState=clean|newer_source_pending
  sourceNewestUtc=...`
- `MechanicCatalog mechanic=... sourceStatus=coded|built|adopted|blocked
  liveStatus=enabled|not_loaded|unproven owner=...`
- `MechanicAdoption task=... status=adopted|pending_build|blocked_handoff
  reason=...`
- `ContentCatalogLoaded mapRegistryHash=... unitCatalogHash=...
  itemCatalogHash=... botTuningRevision=... botTuningLoaded=true|false`
- `MapRegistryLoaded mapCount=... activeMap=... registryHash=...`
- `HostRefreshState pending=... supervisorOwnsHost=... reason=...
  reconnectWindowSeconds=...`
- `CommandDenied actor=... command=... reason=...` for RTS/shop/respawn input
  failures that otherwise look like nothing happened.

## Mechanics matrix

Mechanics that are already useful live canaries:

- Match loop, warmup/live/end/travel markers.
- Bot match spawn and combat pressure.
- Bot feature coverage for `perceive_enemy`, `march_objective`,
  `react_to_base_attack`, `respawn`, `capture_site`, `attack_tower`, and
  `attack_core`.
- Tower/core damage and respawns.

Mechanics coded or contracted but not yet proven in the visible live build:

- Bot strategy tuning loader.
- Bot unstick query.
- Bot objective firing-position lanes.
- Team rally point and reinforcement rally route.
- Latest item shop policy edits.
- Latest Valley layout/block actor edits.
- Unit asset candidate catalog consumption.

Mechanics still missing or not proven enough for the real game loop:

- Physical Renegade-style purchase/equipment terminal in base space.
- Respawn equipment/class selection that uses personal/team economy.
- Warcraft 3-feeling RTS command breadth: attack-move, stop, hold position,
  patrol, production queues, command card, and control groups.
- Clear selection/command feedback while using the Archon table.
- Player-facing explanation when tech loss locks or inflates shop rows.
- Map registry/content manifest telemetry for Quarry and Ledger outputs.
- Human retention/reconnect metadata: connected humans, pending state, rejoin
  result, and whether they stayed in the flow across match boundaries.
- Bot/human takeover telemetry: which bot body became human-controlled and
  which AI body returned to automation.

## Queued handoffs

- `Coordination/done/023-adopt-vector-bot-strategy-tuning-loader.md` was moved
  while this audit was being written. Its result is `blocked by live-session
  handoff`, with read-only evidence captured and no live-adoption claim.
- `Coordination/done/025-live-adoption-ledger-and-replay-manifest.md` is
  completed as `Status: proof_only`. The read-only ledger now exists; the
  remaining value is next-safe-C++ match manifest markers such as
  `ReplaySchema`, `MatchBuildManifest`, `MechanicCatalog`, and
  `ContentCatalogLoaded`.
- `Coordination/done/026-coordination-status-taxonomy.md` is completed. It
  added result statuses to the coordination protocol and the
  `Coordination/status-summary.ps1` proof script.

## Gauge recommendation

Do not add another broad mechanic before the next-safe-C++ replay manifest
markers are in the match stream. The read-only ledger now exists, but the
running match still needs to say exactly which mechanics, contracts, maps, and
catalogs it contains from inside Unreal. After that, the first player-facing
mechanic slice should be the Renegade-style base purchase/equipment terminal,
because it connects RTS tech, FPS base space, economy, respawn, and replay
metadata into one real game loop.
