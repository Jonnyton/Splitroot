# 005 — Rebuild for the stalemate fixes (scoring + bot siege)

Goal: land two Rook changes (001's build predates both):
- scoring: tower damage feeds points at a discount; sites-held
  tiebreak at the bell; MatchEnd logs sitesA/sitesB.
- bot siege focus: bots now target the gate tower (new AttackTower
  state) — towers out-ranged the core ring and were untargetable,
  which is why every archived match was a 0-0 Draw.

Steps:
1. `powershell -ExecutionPolicy Bypass -File Proof\build-locked.ps1`
2. Gate on Saved\Proof\last-policy-build-and-test.json:
   BuildExitCode=0, AutomationFailedCount=0, suite should now count
   198 (two new tests: Match.TimeLimitTiebreaksBySitesThenDraw,
   Match.TowerDamageFeedsPointsAtDiscount).
3. Let the loop adopt the build. After the next match completes, grep
   the active session log for `MatchEnd` — the line now carries
   `sitesA=`/`sitesB=`; paste the first post-build MatchEnd line into
   the Result. Also grep for `state=AttackTower` (new bot state) and
   note whether the match ended core_destroyed vs time_limit.

Done-when: gate green at 198, one post-build MatchEnd line captured,
AttackTower observed in the log.

Report-to: move to Coordination\done\ with ## Result.

## Result

Completed.

- Build gate: `Saved\Proof\last-policy-build-and-test.json` reports `BuildExitCode=0`, `AutomationExitCode=0`, `AutomationSucceededCount=198`, `AutomationFailedCount=0`.
- Post-build loop evidence artifact: `Saved\Proof\last-stalemate-fix-loop-observation.json`.
- First observed `AttackTower` line: `[2026.06.11-02.56.01:396][580]LogTemp: Display: ArchonFactoryCanary: BotState bot=7 team=0 state=AttackTower detail=ArchonValleyDefenseTower_1`
- First post-build match end line: `[2026.06.11-03.00.21:762][119]LogTemp: Display: ArchonFactoryCanary: MatchEnd winner=TeamB reason=core_destroyed pointsA=14 pointsB=2156 liveSeconds=328.3 sitesA=1 sitesB=2`
- End reason observed: `core_destroyed`.
- The active loop session used `Saved\Proof\playtest-drive.log`; game PID at observation time was `7792`.
