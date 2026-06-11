# 013 - Build missing reference-game mechanics into the real game

Goal: use the reference games as gameplay targets for SPLITROOT implementation, not as research for its own sake.

Jonathan correction, 2026-06-11:
- C&C Renegade and Savage 2 are references for building the real game.
- The RTS layer should feel like Warcraft 3.
- The FPS layer should feel more like C&C Renegade.
- The fantasy presentation should look more like Savage 2.
- Any reference work must answer: what mechanic is missing from the playable Unreal build, and what slice proves it?

Current local state to start from:
- Renegade-pattern match loop, base cores, tower pressure, map rotation, bot matches, renown policy skeleton, item-shop policy skeleton, and map-table miniature already exist in some form.
- `drafts/research/splitroot-renegade-match-flow-research-2026-06-09.md` and `drafts/research/splitroot-savage-2-mechanics-research-2026-06-11.md` are supporting context only.
- The game still appears to lack the full playable loop that makes those references matter: map-table build verbs, tech buildings that change player-facing shop options, purchase/respawn equipment UI, and RTS controls that feel like Warcraft 3 rather than a debug command surface.

Reference mechanics to implement against:

1. Warcraft 3 RTS feel:
   - box select / click select / clear select;
   - move, attack-move, stop, hold-position, patrol where useful;
   - control groups or at least persistent squad groups;
   - rally points for production/building output;
   - production/build command card, not just one canned order;
   - readable unit selection state, health, and command feedback.

2. C&C Renegade FPS feel:
   - spawn at a real base, run through real base spaces, fight toward enemy base;
   - purchase/equipment terminal or base shop that is physically located;
   - personal wallet / Renown matters at respawn or equipment choice;
   - building loss changes what the team can buy or how expensive it is;
   - base defense towers matter but can be attacked and disabled;
   - final base/core pressure has a clear scoreboard and end-of-match loop.

3. Savage 2 visual/mechanics direction:
   - fantasy RTS/FPS battlefield with strong faction silhouettes and readable tech structures;
   - RTS-built tech unlocks player-facing classes/items/roles;
   - officer/forward-spawn portal is a later tempo slice, not first;
   - personal and team economy can both exist, but must stay understandable;
   - avoid single-commander dependency; keep SPLITROOT's shared Archon table.

Recommended immediate implementation slice:

Build the first real "RTS tech -> FPS shop" loop.

Gauge progress, 2026-06-11:
- Implemented the first source-backed v0 of the loop through the map table.
- `T` while the runtime map table is open submits a BuildStructure intent for `armory`.
- The match state spends Team Supply, records live team tech, and unlocks `pressure_bolt_crossbow`.
- `AArchonTechBuildingActor` now places a visible, killable armory at a deterministic base slot.
- Destroying the armory removes the team tech and re-locks the dependent shop row.
- Proof artifact: `Saved/Proof/last-tech-shop-smoke.json`.

Remaining before this file can move to done:
- Add a physical player-facing purchase/equipment point near spawn or inside base space.
- Surface the unlocked/locked row in a real interaction UI, not just policy/log proof.
- Decide whether this becomes a Renegade-style terminal, respawn equipment screen, or both.
- Keep the v0 armory placeholder but replace the final shape/placement when Quarry's base layout is ready.

Minimum deliverables:
1. Add or wire a tech-building actor with team ownership, health, `TechTag`, alive/destroyed state, and visible placeholder mesh.
2. Add a map-table build command that spends Team Supply and places or activates that tech building at a valid base slot.
3. Make live team tech state queryable by the item shop policy.
4. Expose at least one player-facing shop row that is locked until the tech building exists/alive.
5. Add a physical purchase point or temporary base-shop interaction near the player spawn.
6. Prove the loop in tests/smoke:
   - row locked before tech;
   - map-table builds tech;
   - row unlocks;
   - destroying tech locks or price-inflates the row;
   - bot or scripted player can still reach tower/core objective after the change.

Do not stop at a wiki page. A small source-backed note is fine only if it directly explains a code decision. The done report must lead with the Unreal files changed and the proof artifacts.

Proof:
- Run `powershell -ExecutionPolicy Bypass -File .\Proof\build-locked.ps1`.
- Run the most relevant smoke: start with `games\splitroot\Proof\bot-match-smoke.ps1`; add a new smoke only if the tech/shop loop cannot be proven by existing scripts.
- Gate on `BuildExitCode=0`, `AutomationFailedCount=0`, and explicit log/proof markers for `TechBuilt`, `ShopRowUnlocked`, and `ShopRowLockedOrInflatedAfterTechLoss` or equivalent names.

Report-to:
- Move this file to `Coordination\done\` with `## Result`.
- Include changed files, test names, smoke commands, proof JSON/log paths, and the exact mechanic still missing after this slice.

## Result

Completed by Hex on 2026-06-11.

Changed files:
- `Source/ArchonFactoryCanary/Private/ArchonCanaryWorldSubsystem.cpp`
- `games/splitroot/Proof/tech-shop-smoke.ps1`
- `Proof/build-and-test-policy.ps1`

Implemented / hardened slice:
- The runtime tech/shop proof now exercises the Renegade-style function-loss loop by proving the armory tech exists, finding the placed armory actor, destroying it through health damage, and requiring the shop row to lock again after tech loss.
- `games/splitroot/Proof/tech-shop-smoke.ps1` now gates on `TechBuildingPlaced`, `TechBuildingDestroyed`, `ShopRowLockedOrInflatedAfterTechLoss`, and `RuntimeTechShopProof completed=true ... destroyed=true ... armoryLost=true`.
- `Proof/build-and-test-policy.ps1` now clears `Saved/Proof/Automation/` before each run and ignores non-fresh automation reports, preventing a failed automation process from reusing an old `index.json`.

Proof commands and outcomes:
- `powershell -ExecutionPolicy Bypass -File .\Proof\build-locked.ps1`: exit `0` after proof-script stale-report fix. `Saved/Proof/last-policy-build-and-test.json` reports `BuildExitCode=0`, `AutomationExitCode=0`, `AutomationSucceededCount=214`, `AutomationFailedCount=0`.
- `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\tech-shop-smoke.ps1`: exit `0`. `Saved/Proof/last-tech-shop-smoke.json` reports `ShopRowLockedBefore=true`, `BuildOrderAccepted=true`, `TechBuilt=true`, `TechBuildingPlaced=true`, `ShopRowUnlocked=true`, `TechBuildingDestroyed=true`, `ShopRowLockedAfterTechLoss=true`, `ProofCompleted=true`.
- `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\bot-match-smoke.ps1`: exit `0`. `Saved/Proof/last-bot-match-smoke.json` reports `ObserveSeconds=180`, `BotMatchSpawned=true`, `BotsEngaged=true`, `BotRespawned=true`, `TowerDamaged=true`, `CoreDamaged=true`, `ObjectivePushReached=true`, `CoreAttackReached=true`, `BothTeamsFought=true`.

Key proof markers:
- `Saved/Proof/last-tech-shop-smoke.log`: `TechBuilt team=0 tech=armory cost=5 remainingSupply=0`.
- `Saved/Proof/last-tech-shop-smoke.log`: `ShopRowUnlocked team=0 item=pressure_bolt_crossbow requiredTech=armory finalCost=100 affordable=false reason=insufficient_supply`.
- `Saved/Proof/last-tech-shop-smoke.log`: `TechBuildingPlaced team=0 tech=armory health=450`.
- `Saved/Proof/last-tech-shop-smoke.log`: `ShopRowLockedOrInflatedAfterTechLoss team=0 item=pressure_bolt_crossbow requiredTech=armory reason=missing_tech`.
- `Saved/Proof/last-tech-shop-smoke.log`: `TechBuildingDestroyed team=0 tech=armory`.
- `Saved/Proof/last-bot-match.log`: `DefenseTowerDamaged`, `BotStructureHit`, `BotState ... state=AttackCore`, `BaseCoreDamaged`, and `BotRespawned` all appeared in the 180-second smoke.

Surprise:
- The first build gate after this edit returned nonzero because Unreal exited with `AutomationExitCode=-1`, while the summary still showed 213/0 by reading a stale report from 2026-06-10. The proof script now deletes the old report directory before automation; the rerun produced a fresh clean 214/0 result.

Still missing after this slice:
- The reference-game loop now has a proven first `RTS tech -> FPS shop -> tech loss locks shop` path, but it is still a proof-scripted slice rather than a polished player-facing purchase terminal/equipment UI.
- The Warcraft 3 side still lacks full command card breadth: production queues, rally points, control groups, patrol/hold-position, and richer readable unit command feedback are not proven by this task.
