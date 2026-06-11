# 035 - Vector route-hotspot probe

Goal:
Build or extend proof around the stable route/stuck hotspots Vector found in
the live bot replay trend. Do not change gameplay balance unless the proof
shows the hotspot is caused by route geometry, nav, or waypoint placement.

Context:
- Vector v4 is live:
  - `BotStrategyTuningRevision=vector-2026-06-11-fighter-lane-v4`
  - `LatestBotStrategyTuningLoadedRevision=vector-2026-06-11-fighter-lane-v4`
  - `BotStrategyTuningRevisionAdopted=true`
- v4 trend proof path:
  - `Saved\Proof\last-vector-bot-trend-summary-v4.json`
- Stable v4 stuck zones:
  - `x=7000;y=5500`
  - `x=12500;y=5500`
  - `x=-15500;y=-1000`
  - `x=7000;y=4500`
- The latest unstick metadata says candidates are clear, not blocked:
  - `BotUnstickQueryClearCount=75`
  - `BotUnstickQueryFallbackCount=0`
  - `BotUnstickQueryClearRate=1`

Steps:
1. Re-enter through `.agents/hex.md`, `AGENTS.md`, and `Coordination/README.md`.
2. Keep Jonathan's visible current-build game running. Do not close, kill,
   restart, minimize, focus-steal from, or replace it.
3. Inspect the route/waypoint/nav evidence around the stable zones above.
4. Prefer a proof/readout improvement first if the cause is not obvious:
   identify whether these are terrain geometry, waypoint placement, objective
   standoff, nav projection, or combat-position issues.
5. If a small safe local fix is obvious and can hot-load/adopt without
   interrupting the visible loop, make it; otherwise record the blocker and the
   exact next evidence needed.
6. Run:
   - `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\vector-bot-trend-summary.ps1 -RecentRows 20`
   - `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1 -TailLines 12000`
   - `python .\scripts\check_live_canary_process_safety.py`

Done-when:
- Move this file to `Coordination/done/` with a `## Result` section.
- Include `Status: completed`, `Status: proof_only`, or
  `Status: blocked_requires_handoff`.
- Attach the relevant proof JSON fields, especially whether the stable zones
  persist after the next one or two v4 match windows.

Report-to:
Vector and Rook via `Coordination/done/`.

## Result

Status: proof_only

Hex completed a non-disruptive proof pass at 2026-06-11T18:33Z. I did not close,
kill, restart, minimize, focus-steal from, or replace Jonathan's visible Unreal
session, and I did not change gameplay balance or source code for this task.

Proof commands and outputs:
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\vector-bot-trend-summary.ps1 -RecentRows 20`
  wrote `Saved\Proof\last-vector-bot-trend-summary.json`.
  Latest revision: `vector-2026-06-11-route-abandon-v5`; latest loaded
  revision: `vector-2026-06-11-route-abandon-v5`; adopted: `true`; gate:
  `observe_more`; recommended lens: `track_or_route_stable_hotspot`.
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\vector-bot-trend-summary.ps1 -RecentRows 20 -Revision vector-2026-06-11-route-abandon-v5 -OutputPath .\Saved\Proof\last-vector-bot-trend-summary-v5.json`
  wrote `Saved\Proof\last-vector-bot-trend-summary-v5.json`.
  v5 rows analyzed: 4; adopted windows: 3; ready windows: 0; observe-more
  windows: 4; weapon fires: 363; tower fires: 10; stuck recoveries: 83;
  unstick queries: 83 clear / 0 fallback; route waypoint abandons: 4;
  objective lane shifts: 6.
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1 -TailLines 12000`
  wrote `Saved\Proof\last-live-bot-match-log-snapshot.json`.
  Snapshot `2026-06-11T18:31:40.4848455Z` showed match loop active, PID 29092,
  contract revision `vector-2026-06-11-route-abandon-v5`, latest loaded
  revision `vector-2026-06-11-route-abandon-v5`, adopted `true`, and
  `EvidenceReady=false` because the window was still early.
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\games\splitroot\Proof\vector-bot-replay-diagnosis.ps1 -TailLines 12000`
  wrote `Saved\Proof\last-vector-bot-replay-diagnosis.json` and appended
  `Saved\Proof\vector-bot-replay-diagnosis-history.jsonl`.
  Latest diagnosis showed v5 adopted, `HasCombatWindow=true`, weapon fires 2,
  stuck recoveries 7, max stuck attempt 3, unstick queries 7 clear / 0 fallback,
  route waypoint abandons 0, objective lane shifts 0, firing positions 3, and
  `VectorTuningGate.Status=observe_more` with reason `early_match_window`.
- `powershell -NoProfile -ExecutionPolicy Bypass -File .\Proof\host-refresh-pending-state.ps1 -Action snapshot -ObservedLogPath .\Saved\Proof\playtest-drive.log`
  returned `Pending=false` at `2026-06-11T18:32:29.0989015Z` with current PID
  29092 and build fingerprint `version=v20260611-183057`.
- `python .\scripts\check_live_canary_process_safety.py` returned
  `OK - no broad live-canary process stop patterns found`.

Hotspot readout:
- The refreshed v4-filtered proof, `Saved\Proof\last-vector-bot-trend-summary-v4.json`,
  had five v4 rows and preserved the four zones from the task context:
  `x=12500;y=5500`, `x=7000;y=5500`, `x=-15500;y=-1000`, and
  `x=7000;y=4500`.
- The v5-filtered proof now points at a different early cluster:
  `x=10000;y=7500` seen in 3 windows, `x=-9000;y=-3500` seen in 3 windows,
  `x=12500;y=7000` seen in 3 windows, and `x=10000;y=8000` seen in 2 windows.
- All v5 unstick selections in the trend were clear, with no fallback
  selections. That supports "route hotspot is still observable", but it does
  not prove terrain geometry, nav projection, waypoint placement, or balance
  as the cause.

Next evidence needed:
Collect at least two v5 `ready_for_tuning_review` windows, then compare stable
zones against `BotRouteWaypointAbandoned`, `BotObjectiveLaneShift`, and
`BotFiringPosition` rows. If the same v5 zones persist with route-abandon
events, the next slice should map those zone keys back to the route waypoint or
objective standoff that produced them. If they do not persist, keep v5 and
avoid geometry/balance edits.
