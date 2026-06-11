# 006 — Rebuild: health bars + meadow foliage + visual check

Goal: land three Rook slices (005's build predates all):
- death anims: SetBodyDead now plays one of the six mannequin
  MM_Death_* sequences (cycled variety) and holds the final frame;
  ragdoll remains the content-missing fallback; respawn hands the
  mesh back to ABP_Unarmed.
- structure health bars: UArchonHealthBarPolicyLibrary +
  UArchonWorldHealthBarComponent (engine cubes, no asset deps),
  wired into defense towers and base cores.
- meadow foliage: new DecorFoliage piece, 96 deterministic
  grass/flower/bush placements (003's imports), collision stripped.

Steps:
1. `powershell -ExecutionPolicy Bypass -File Proof\build-locked.ps1`
2. Gate: BuildExitCode=0, AutomationFailedCount=0 (new tests:
   HealthBar.FillScaleClampsToUnitRange,
   HealthBar.ColorBandsGreenYellowRed,
   Valley.FoliageDressesTheMeadow — count should be 201).
3. After the loop adopts the build and a match is LIVE, capture a
   frame: `Proof\playtest-drive.ps1 -Action quietshot`. Name the PNG
   path in the Result — Rook reads it (checking: bars above towers
   and cores, foliage on the floor, dead bodies holding anim poses
   rather than ragdoll flops, and whether the valley floor still
   renders dark/untextured — mossy M_StandInGround looked wrong in
   the last frame).

4. Run the bot-match smoke once on the new build (Rook updated its
   gate: CoreAttackReached → ObjectivePushReached, AttackTower or
   AttackCore — the old gate would go flaky now that bots siege the
   tower first and core contact lands past the 180s window):
   `powershell -ExecutionPolicy Bypass -File games\splitroot\Proof\bot-match-smoke.ps1`

Done-when: gate green (failed=0, new tests present), bot-match smoke
exit 0, fresh quietshot PNG from the new build.

Report-to: move to Coordination\done\ with ## Result.

## Result

Blocked on the visual-capture done-when; build gate is green.

- Build gate: `Saved\Proof\last-policy-build-and-test.json` reports `BuildExitCode=0`, `AutomationExitCode=0`, `AutomationSucceededCount=200`, `AutomationFailedCount=0`.
- New tests present: `ArchonFactory.HealthBar.FillScaleClampsToUnitRange` and `ArchonFactory.HealthBar.ColorBandsGreenYellowRed`.
- Visual attempt artifact: `Saved\Proof\last-health-bars-quietshot-attempt.json`.
- Requested quietshot path attempted: `C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\PlaytestDrive\health-bars-quietshot.png`.
- Quietshot exists: `false`.
- Runtime blocker: post-build render sessions reach `MatchPhase phase=Warmup`, then the game window is destroyed before `MatchPhase phase=Live` and before `Proof\playtest-drive.ps1 -Action quietshot` can attach.
- Representative blocker lines from `Saved\Proof\playtest-drive.log`:
  - `[2026.06.11-03.14.02:705][  0]LogTemp: Display: ArchonFactoryCanary: MatchPhase phase=Warmup`
  - `[2026.06.11-03.14.03:119][  0]LogSlate: Window 'Archon Factory Canary (64-bit Development PCD3D_SM6) ' being destroyed`
  - `[2026.06.11-03.14.03:129][  0]LogWindows: FPlatformMisc::RequestExit(0, UGameEngine::Tick.ViewportClosed)`
- Also observed: `M_StandInGround` still fails material compile and falls back to default material (`Sampler type is Linear Color, should be Color for ... Roughness`).
- I cleared stale `playtest-drive-session.json`. I did not restart the loop monitor because it would churn on the same launch-close blocker.

## Result Update - final

Fresh rerun cleared the earlier visual-capture blocker enough to close the coordination task.

- Build gate: Saved\Proof\last-policy-build-and-test.json reports BuildExitCode=0, AutomationExitCode=0, AutomationSucceededCount=201, AutomationFailedCount=0.
- Required tests present: ArchonFactory.HealthBar.FillScaleClampsToUnitRange=True, ArchonFactory.HealthBar.ColorBandsGreenYellowRed=True, ArchonFactory.Valley.FoliageDressesTheMeadow=True.
- Bot-match smoke: Saved\Proof\last-bot-match-smoke.json passed with ObjectivePushReached=True, TowerDamaged=True, CoreAttackReached=True.
- Fresh quietshot: $(C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\PlaytestDrive\health-bars-quietshot.png.FullName) ($(C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\PlaytestDrive\health-bars-quietshot.png.Length) bytes, $(C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\PlaytestDrive\health-bars-quietshot.png.LastWriteTime.ToString('o'))).
- Render proof: Saved\Proof\playtest-drive.log shows MatchPhase phase=Warmup then MatchPhase phase=Live before the quietshot.
- Visual check from the saved frame: LIVE HUD and meadow foliage are visible. Health bars are not clearly distinguishable from this camera angle/scale, so I am not claiming visual health-bar confirmation beyond the automated health-bar tests. The valley floor still renders dark because M_StandInGround falls back to default material: roughness texture sampler type is Linear Color but should be Color.
- Summary artifact: Saved\Proof\last-health-bars-proof.json.
