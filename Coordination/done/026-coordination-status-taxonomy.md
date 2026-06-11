# 026 - Coordination status taxonomy for blocked vs complete work

Goal: make the coordination queue show the true state of work so a future pass
does not treat blocked or proof-only results as live gameplay adoption.

Context:
- `Coordination/done/` is currently the only acknowledgement lane, so it holds
  completed tasks, blocked tasks, staged work, and proof-only work.
- That is workable as an ack protocol, but it is misleading for live adoption.
  Examples:
  - `021-bot-objective-firing-position-adoption.md` is in `done/`, but its
    result says blocked by required live-session handoff.
  - `022-rally-point-reinforcement-adoption.md` is in `done/`, but its result
    says no build/smoke/relaunch was run and no live claim is valid.
  - `024-vector-bot-strategy-tuning-contract.md` completed a contract/loader
    slice, while live metadata still shows `BotStrategyTuningLoadedCount=0`.
- The live game must remain always running unless Jonathan explicitly hands it
  over, so "blocked by live-session handoff" will be a normal state.

Steps:
1. Update the coordination protocol to require a status line in every result:
   `Status: completed`, `Status: blocked_requires_handoff`,
   `Status: staged_not_live`, `Status: proof_only`, or
   `Status: superseded`.
2. Either add a `Coordination/blocked/` or `Coordination/pending-adoption/`
   folder, or keep the single `done/` folder but make a summary script/report
   count blocked/staged tasks explicitly.
3. Backfill status lines for the existing ambiguous records:
   - `021-bot-objective-firing-position-adoption.md`
   - `021-bot-unstick-candidate-query.md`
   - `022-rally-point-reinforcement-adoption.md`
   - `024-vector-bot-strategy-tuning-contract.md`
4. Teach the live watch/adoption summary to report:
   - active inbox count;
   - blocked handoff count;
   - staged-not-live count;
   - proof-only count;
   - completed-and-live count.
5. Keep the current simple Hex move-to-done ack behavior if Rook prefers it,
   but make the status machine-readable enough that Gauge/Vector/Grid/Ledger
   cannot accidentally overclaim live adoption.

Done-when:
- A future agent can scan coordination state and immediately see what is
  complete versus blocked/staged/proof-only.
- Existing blocked adoption tasks are no longer invisible just because they
  live under `Coordination/done/`.
- No live gameplay success is implied unless a proof artifact or live marker
  supports it.

Report-to:
- Move this file to `Coordination/done/` with `## Result`.
- Include changed docs/scripts, any folder created, and the counted status
  summary after backfill.

## Result

Status: completed

Changed docs/scripts:

- `Coordination/README.md`
  - Added the machine-readable result status taxonomy:
    `completed`, `blocked_requires_handoff`, `staged_not_live`,
    `proof_only`, and `superseded`.
  - Kept the single `Coordination/done/` ack lane; no new coordination folder
    was created.
- `Coordination/status-summary.ps1`
  - New read-only summary script.
  - Writes `Saved/Proof/coordination-status-summary.json`.
- `games/splitroot/Proof/live-canary-watch-note.ps1`
  - Now includes `CoordinationStatus` counts when
    `Saved/Proof/coordination-status-summary.json` exists.

Backfilled exact status lines:

- `Coordination/done/021-bot-objective-firing-position-adoption.md`
  - `Status: blocked_requires_handoff`
- `Coordination/done/021-bot-unstick-candidate-query.md`
  - `Status: staged_not_live`
- `Coordination/done/022-rally-point-reinforcement-adoption.md`
  - `Status: blocked_requires_handoff`
- `Coordination/done/023-adopt-vector-bot-strategy-tuning-loader.md`
  - `Status: blocked_requires_handoff`
- `Coordination/done/024-vector-bot-strategy-tuning-contract.md`
  - `Status: staged_not_live`

Proof commands:

- `powershell -ExecutionPolicy Bypass -File .\Coordination\status-summary.ps1`
  - exit `0`
  - artifact: `Saved/Proof/coordination-status-summary.json`
  - observed at `2026-06-11T16:59:57.7163805Z`
  - `DoneCount=25`, `ActiveInboxCount=2`
  - `BlockedRequiresHandoffCount=3`
  - `StagedNotLiveCount=2`
  - `ProofOnlyCount=0`
  - `CompletedAndLiveCount=0`
  - `LegacyUnknownCount=20`
- `python .\scripts\check_live_canary_process_safety.py`
  - exit `0`
  - output: `OK - no broad live-canary process stop patterns found`

Live process safety:

- No Unreal stop/restart/minimize/focus action was run.
- Current visible `UnrealEditor.exe` observed after the proof: PID `7680`,
  desktop current-build command line.
