# 032 - Vector fighter-lane v4 and trend lens

Goal:
Stage and verify the next AI strategy iteration without interrupting the
always-running SPLITROOT current-build loop.

## Result

Status: completed

Vector result time: 2026-06-11T18:25Z.

Changes:
- Updated `games/splitroot/FactoryContracts/bot_strategy_tuning.json` to
  `revision=vector-2026-06-11-fighter-lane-v4`.
- Kept breacher focus unchanged:
  - `breacher_pursuit_memory_window_seconds=2.5`
  - `breacher_threat_window_seconds=3.5`
- Tuned movement only:
  - `route_stall_max_unstick_attempts=6`
  - `objective_stall_lane_shift_attempts=4`
  - `objective_stall_max_lane_shift=3`
- Extended `games/splitroot/Proof/vector-bot-replay-diagnosis.ps1` with:
  - local-vs-loaded strategy revision adoption fields;
  - `BotUnstickQueryClearCount`;
  - `BotUnstickQueryFallbackCount`;
  - `BotUnstickQueryClearRate`;
  - by-bot and by-role unstick selection rows.
- Extended `games/splitroot/Proof/live-bot-match-log-snapshot.ps1` with the
  same revision-adoption and clear-rate fields.
- Added `games/splitroot/Proof/vector-bot-trend-summary.ps1` for cross-window
  trend analysis from `Saved\Proof\vector-bot-replay-diagnosis-history.jsonl`.

Live proof:
- `Saved\Proof\last-live-bot-match-log-snapshot.json`
  - `BotStrategyTuningRevision=vector-2026-06-11-fighter-lane-v4`
  - `LatestBotStrategyTuningLoadedRevision=vector-2026-06-11-fighter-lane-v4`
  - `BotStrategyTuningRevisionAdopted=true`
  - `BotUnstickQueryClearRate=1`
  - `BotStuckRecoveryCount=38`
  - `BotRouteWaypointAbandonedCount=1`
  - `BotObjectiveLaneShiftCount=2`
  - `BotFiringPositionCount=20`
- `Saved\Proof\last-vector-bot-replay-diagnosis.json`
  - `BotStrategyTuningRevision=vector-2026-06-11-fighter-lane-v4`
  - `LatestBotStrategyTuningLoadedRevision=vector-2026-06-11-fighter-lane-v4`
  - `BotStrategyTuningRevisionAdopted=true`
  - `VectorTuningGate.Status=ready_for_tuning_review`
  - `BotUnstickQueryClearCount=40`
  - `BotUnstickQueryFallbackCount=0`
  - `BotUnstickQueryClearRate=1`
  - `BotRouteWaypointAbandonedCount=1`
  - `BotObjectiveLaneShiftCount=2`
- `Saved\Proof\last-vector-bot-trend-summary-v4.json`
  - `LatestRevision=vector-2026-06-11-fighter-lane-v4`
  - `LatestLoadedRevision=vector-2026-06-11-fighter-lane-v4`
  - `RecommendedLens=track_or_route_stable_hotspot`
  - v4 aggregate: `75/75` unstick queries selected `clear`, `2` route
    abandons, `4` objective lane shifts, and `38` firing positions across
    the sampled v4 rows.
  - v4 stable stuck zones: `x=7000;y=5500`, `x=12500;y=5500`,
    `x=-15500;y=-1000`, and `x=7000;y=4500`.
- `python .\scripts\check_live_canary_process_safety.py`
  - exit `0`
  - `OK - no broad live-canary process stop patterns found`

Live-session boundary:
- Vector did not close, kill, restart, minimize, focus-steal from, or replace
  Jonathan's visible current-build game.
- Hot-load/supervisor behavior adopted the JSON revision into a live match.

Bounded conclusion:
- v4 is live and should be watched for whether earlier route and lane
  thresholds reduce repeated clear-unstick stalls.
- The stable stuck zones are now specific enough to route a terrain/route probe
  task without guessing from a single match tail.
