# 027 - Vector bot replay diagnosis

Result: completed by Vector on 2026-06-11 as a read-only replay-analysis
iteration. No Unreal process was stopped, relaunched, minimized, focus-stolen,
or replaced by this session.

Changed files:
- `games/splitroot/Proof/vector-bot-replay-diagnosis.ps1`
- `Coordination/done/024-vector-bot-strategy-tuning-contract.md`

Behavior added:
- New Vector replay lens writes
  `Saved\Proof\last-vector-bot-replay-diagnosis.json`.
- The script reads the live log tail and reports:
  - tuning JSON revision and whether the running DLL logged
    `BotStrategyTuningLoaded`;
  - combat-window readiness;
  - stuck recovery by bot, team, role, and 500uu map zone;
  - max stuck attempt per bot;
  - perception, respawn, state churn, structure-hit, and feature coverage
    counts;
  - recommendation gates that refuse JSON tuning while the loader is absent.

Live evidence:
- Initial read-only snapshot still showed older live PID `26024` with
  `BotStrategyTuningLoadedCount=0`.
- During this pass the visible process changed to PID `7680` outside Vector's
  actions. The new process loaded module DLL timestamp
  `2026-06-11T15:59:42.000Z`, so the strategy loader was still not active.
- `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\vector-bot-replay-diagnosis.ps1`: exit `0`;
  `Saved\Proof\last-vector-bot-replay-diagnosis.json` reported
  `HasCombatWindow=true`, `WeaponFiredCount=73`, `TowerFiredCount=9`,
  `BotStuckRecoveryCount=79`, `MaxObservedStuckAttempt=14`,
  `BotUnstickQueryCount=0`, `BotFiringPositionCount=0`,
  `NativePerceptionCount=48`, `FallbackPerceptionCount=26`, and
  `BotStrategyTuningLoadedCount=0`.
- Top stuck zones in that tail were `x=-5500;y=5500`,
  `x=-2500;y=-5000`, `x=-1500;y=-6000`, `x=-5500;y=6500`, and
  `x=-6000;y=6500`.
- Parser check for the new script passed.
- `python scripts\check_live_canary_process_safety.py`: exit `0`;
  no broad live-canary stop patterns found.

Bounded conclusion:
- Do not tune `bot_strategy_tuning.json` yet; the live DLL has not loaded the
  strategy tuning contract.
- The next useful proof after hot reload/adoption is not just "game runs"; it
  is `BotStrategyTuningLoadedCount > 0` plus a new Vector diagnosis showing
  whether unstick-query/firing-position adoption reduced repeated stuck zones.
