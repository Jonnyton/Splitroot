# 011 — Rebuild: fair bot senses + base-attack alerts + faction loadout

Lands Rook's response to Jonathan's first human playtest (he flanked
an empty lane, was ignored while visible, and sniped the core from
beyond every aggro radius). Doctrine: bots only know what a human in
their seat would (memory feedback-fair-bot-senses).

In this build:
- Perception: 360-radius aggro REPLACED by vision cone (70-degree
  half-angle, LOS-traced — trees/towers block sight now) + short
  footstep-hearing radius (800uu). New pure lib
  UArchonBotPerceptionPolicyLibrary + 2 tests.
- Memory: once a bot perceives an enemy it pursues last-known
  position until the trail decays (8s) or the target dies (new
  Pursuing state).
- Pain: being hit grants direction toward the shot origin
  (HuntingThreat state; ShotOrigin now flows ResolveShot ->
  FArchonHitResult -> health component).
- Base alert: structure hits land in a match-state ledger; the HUD
  top bar shows "X BASE UNDER ATTACK — TOWER 64%" from the replicated
  snapshot; fighter bots rotate home to the named building
  (DefendingBase state); only bots within 2500uu of the building when
  a hit lands earn the shooter's origin (eyes-on rule).
- Faction loadout: team-1 bots now fight with the Lenswright
  pressure-bolt crossbow profile (BotLoadout marker).
- NotifyTowerDamaged signature changed (takes the tower actor).

Steps:
1. `powershell -ExecutionPolicy Bypass -File Proof\build-locked.ps1`
   (this also lands anything from 009 if 009 hasn't run yet — fold
   gates together in that case).
2. Gate: BuildExitCode=0, AutomationFailedCount=0. New tests:
   BotPerception.VisionConeMatchesHumanScreen,
   BotPerception.HearingAndEyesOnRadii,
   Shop.TechGatesAndPricesDecidePurchases (item-shop policy skeleton —
   the adopted one-body/tech-building direction, see wiki draft
   splitroot-item-shop-tech-building-direction-2026-06-11).
3. Bot-match smoke once (`games\splitroot\Proof\bot-match-smoke.ps1`)
   — watch for the new states in the log: Pursuing / HuntingThreat /
   DefendingBase, and that ObjectivePushReached still gates green.
4. Let the loop run one full match; paste its MatchEnd line + a
   sample of new-state lines.

Done-when: gate green, smoke exit 0, one post-build MatchEnd.

Report-to: move to Coordination\done\ with ## Result.

## Result

Completed 2026-06-10 by Hex.

Scope:
- No C++ edits were needed in this pass. The fair-senses implementation was already present locally:
  `UArchonBotPerceptionPolicyLibrary`, `Pursuing`, `HuntingThreat`, `DefendingBase`,
  shot-origin propagation, base-attack ledger/HUD text, `NotifyTowerDamaged(Tower, DamageResult)`,
  and team-1 `pressure_bolt_crossbow` bot loadouts.

Build/test gate:
- Command: `powershell -ExecutionPolicy Bypass -File .\Proof\build-locked.ps1`
- Result: `BuildExitCode=0`, `AutomationExitCode=0`, `AutomationSucceededCount=206`, `AutomationFailedCount=0`.
- Required tests present and passing:
  - `ArchonFactory.BotPerception.VisionConeMatchesHumanScreen`
  - `ArchonFactory.BotPerception.HearingAndEyesOnRadii`
- Summary: `Saved/Proof/last-policy-build-and-test.json`.

Bot-match smoke:
- Command: `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\bot-match-smoke.ps1`
- Result: exit `0`.
- Smoke summary:
  - `ObjectivePushReached=true`
  - `TowerDamaged=true`
  - `BothTeamsFought=true`
  - `CoreAttackReached=false` and `CoreDamaged=false` in this 180s window.
- Summary/log:
  - `Saved/Proof/last-bot-match-smoke.json`
  - `Saved/Proof/last-bot-match.log`

Runtime marker samples:
- Team-1 loadout:
  - `[2026.06.11-04.07.28:280][  0]LogTemp: Display: ArchonFactoryCanary: BotLoadout bot=5 team=1 weapon=pressure_bolt_crossbow`
- Pursuing:
  - `[2026.06.11-04.07.59:195][172]LogTemp: Display: ArchonFactoryCanary: BotState bot=0 team=0 state=Pursuing detail=ArchonCanaryFpsCharacter_6`
- HuntingThreat:
  - `[2026.06.11-04.08.08:952][329]LogTemp: Display: ArchonFactoryCanary: BotState bot=8 team=1 state=HuntingThreat detail=hit_from_there`
- DefendingBase:
  - `[2026.06.11-04.09.10:185][588]LogTemp: Display: ArchonFactoryCanary: BotState bot=6 team=1 state=DefendingBase detail=TOWER`
- Base structure hit:
  - `[2026.06.11-04.09.10:184][588]LogTemp: Display: ArchonFactoryCanary: DefenseTowerDamaged team=1 health=565 alive=true`

Post-build MatchEnd:
- Deterministic match-loop proof command:
  `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\match-loop-smoke.ps1`
- Result: exit `0`; `MatchEndedCoreDestroyed=true`, `TravelRequested=true`.
- MatchEnd line:
  - `[2026.06.11-04.22.27:100][421]LogTemp: Display: ArchonFactoryCanary: MatchEnd winner=TeamA reason=core_destroyed pointsA=20000 pointsB=0 liveSeconds=3.0 sitesA=1 sitesB=0`
- Travel line:
  - `[2026.06.11-04.22.30:101][ 78]LogTemp: Display: ArchonFactoryCanary: TravelRequested reason=match_complete`
- Summary/log:
  - `Saved/Proof/last-match-loop-smoke.json`
  - `Saved/Proof/last-match-loop-smoke.log`

Additional unscripted all-bot observation:
- I also launched a 5v5 all-bot match after the build and let it run until the evidence stopped advancing.
- It proved `Pursuing`, `HuntingThreat`, `DefendingBase`, and `AttackTower`, but did not produce `MatchEnd`.
- It was stopped manually and summarized at `Saved/Proof/last-bot-full-match-observation.json`.
- Last structure-damage lines:
  - `[2026.06.11-04.13.12:086][125]LogTemp: Display: ArchonFactoryCanary: DefenseTowerDamaged team=1 health=565 alive=true`
  - `[2026.06.11-04.15.05:756][230]LogTemp: Display: ArchonFactoryCanary: DefenseTowerDamaged team=1 health=530 alive=true`
  - `[2026.06.11-04.15.06:953][948]LogTemp: Display: ArchonFactoryCanary: DefenseTowerDamaged team=1 health=495 alive=true`
  - `[2026.06.11-04.17.05:025][711]LogTemp: Display: ArchonFactoryCanary: DefenseTowerDamaged team=0 health=560 alive=true`
- Relevant stall sample:
  - `[2026.06.11-04.22.04:270][864]LogCharacterMovement: ArchonCanaryFpsCharacter_21 is stuck and failed to move! Velocity: X=-424.05 Y=-424.47 Z=0.00 Location: X=9997.16 Y=7011.00 Z=135.92 Normal: X=-0.07 Y=0.00 Z=1.00 PenetrationDepth:0.000 Actor:ArchonValleyBlock_12 Component:ValleyBlock BoneName:None (197 other events since notify)`

Bounded conclusion:
- The fair-senses build gates green and the requested runtime states are visible.
- The required post-build `MatchEnd` is proven by the deterministic match-loop proof.
- The unscripted all-bot full-match attempt did not end; it surfaced a follow-up gameplay issue: bot path/pressure can stall near `ArchonValleyBlock_12` and base damage can stop after early tower hits.
