# Fix Adoption Brief Native Audit Staleness

Goal:
Make `live-canary-adoption-interpretation-brief.ps1` report native adoption
consistently when the native audit artifact is stale relative to the current
Vector diagnosis/revision.

Steps:
- Inspect `games/splitroot/Proof/live-canary-adoption-interpretation-brief.ps1`.
- Reproduce with the current proof artifacts:
  - `Saved/Proof/live-canary-adoption-interpretation-brief-latest.json`
  - `Saved/Proof/last-vector-bot-replay-diagnosis.json`
  - `Saved/Proof/native-module-adoption-requirement-audit.json`
  - `Saved/Proof/host-refresh-pending-reason-audit.json`
- Ensure the brief either reruns/refreshes native adoption audit for the current
  revision or clearly marks stale native-audit data as stale.
- Prefer the current diagnosis gate reason over stale native-audit success when
  they disagree.

Done-when:
- The brief no longer reports `NativeAudit.RequirementSatisfied=true` for an old
  requirement while also reporting `GateReasons=native_module_requirement_not_satisfied`
  for the current revision.
- The markdown/JSON brief names the current revision, native requirement UTC,
  module UTC, and whether pending source changes mean the next supervisor
  boundary is still required.
- The fix is non-disruptive and does not close, kill, restart, minimize, or
  focus-steal Jonathan's visible Unreal host.

Report-to:
Gauge/Rook coordination.

Evidence:
- At `2026-06-11T23:40:41Z`, the adoption brief reported current revision
  `vector-2026-06-11-earlier-core-breakthrough-v32`.
- The same brief reported `GateReasons=native_module_requirement_not_satisfied`.
- But it also embedded stale `NativeAudit.RequirementSatisfied=true` with
  `RequiredAfterUtc=2026-06-11T23:11:12Z` and
  `ModuleDllUtc=2026-06-11T23:19:21Z`.
- The current pending-reason audit reported
  `source_newer_than_module_boundary_build_pending`, with newest compile input
  at `2026-06-11T23:36:53Z` and runtime contract at
  `2026-06-11T23:37:17Z`.
