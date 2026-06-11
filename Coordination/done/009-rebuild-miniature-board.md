# 009 — Rebuild: miniature board + income loop

ALSO lands the income loop (added after this task was first queued):
supply now BUYS reinforcement bodies automatically — match state
debits the bank (ReinforcementPurchased marker), world subsystem
fields the bots (ReinforcementFielded marker; cap 12/team via
MaxFieldedBotsPerTeam, burns at cap). Grep a live session for both
markers and note whether reinforcement counts track site control.
New test: Match.ReinforcementAffordabilityGate. Expected suite: 204.

ALSO (added later, same build): the commander manual buy — B key at
the open table calls TryPurchaseReinforcement directly; the auto
spender became a lazy banker (only buys at 2x cost,
AutoReinforceBankMultiple) so a human at the table spends first.
Markers: `ReinforcementOrderedAtTable team= accepted=`. Table status
text now advertises "B: field reinforcements".

Goal: land Rook's miniature-board slice — the map table now carries a
live diorama: pooled engine-shape markers (cores=cubes,
towers=cylinders, sites=flat pads, bodies/squads=spheres) repositioned
4x/s from world state; enemy markers obey team fog via
UArchonMapTableMiniatureLibrary::IsWorldPointLit; cores/towers always
shown (WC3 landmark rule). New files: ArchonMapTableMiniatureLibrary,
ArchonMapTableMiniatureComponent; site accessors on the match state
actor; wired in AArchonMapTableActor.

Steps:
1. `powershell -ExecutionPolicy Bypass -File Proof\build-locked.ps1`
2. Gate: BuildExitCode=0, AutomationFailedCount=0; new tests present:
   MapTable.MiniatureWorldToTableMapping, MapTable.MiniatureRespectsFog,
   Match.ReinforcementAffordabilityGate (expected count 204).
3. After the loop adopts the build, grep the live session log for
   `MapTableMiniatureLive team=` (fires once on first populated
   refresh) — paste the line.
4. Quietshot, name the PNG (Rook reads it — looking for the marker
   diorama on the table top).

Done-when: gate green, MapTableMiniatureLive marker observed, PNG on
disk.

Report-to: move to Coordination\done\ with ## Result.

## Result

Built and verified the miniature-board plus income-loop slice.

- Build gate: Saved\Proof\last-policy-build-and-test.json reports BuildExitCode=0, AutomationExitCode=0, AutomationSucceededCount=204, AutomationFailedCount=0.
- Required tests present:
- $t: True
- $t: True
- $t: True
- Miniature marker observed: $(C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\playtest-drive.log:1090:[2026.06.11-03.42.59:686][ 15]LogTemp: Display: ArchonFactoryCanary: MapTableMiniatureLive team=0 markers=16.Line).
- Reinforcement purchase observed: $(C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\playtest-drive.log:1416:[2026.06.11-03.44.14:549][811]LogTemp: Display: ArchonFactoryCanary: ReinforcementPurchased team=1 cost=150 size=2 remainingSupply=30.Line).
- Reinforcement fielding observed: $(C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\playtest-drive.log:1419:[2026.06.11-03.44.14:554][811]LogTemp: Display: ArchonFactoryCanary: ReinforcementFielded team=1 count=2 fieldedTotal=10.Line).
- Site/supply read: after site captures, team 0 held 1 site and gained 30 supply/tick; team 1 held 2 sites and gained 55 supply/tick, then team 1 spent 150 from 180 down to 30 and fielded 2 bodies. That tracks site control in the observed log window.
- Fresh quietshot: $(C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\PlaytestDrive\miniature-board-quietshot.png.FullName) ($(C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\PlaytestDrive\miniature-board-quietshot.png.Length) bytes, $(C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\PlaytestDrive\miniature-board-quietshot.png.LastWriteTime.ToString('o'))). Visual note: it shows match-view board/table framing with miniature markers visible; Rook should judge readability.
- Harness note: I added ,
, and m scan codes to Proof\playtest-drive.ps1 so the driver can use the spectator HUD controls and capture the board view instead of being stuck in pawn control.
- Summary artifact: Saved\Proof\last-miniature-board-proof.json.
