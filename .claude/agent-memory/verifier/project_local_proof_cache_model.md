---
name: project-local-proof-cache-model
description: How Proof/local-proof-checks.ps1 derives its ProofClaimState (cached, not live) and how to judge whether those claims are still valid
metadata:
  type: project
---

`Proof/local-proof-checks.ps1` is the cheap, no-Unreal-launch check. It does two things:

1. File-existence checks (~150 paths) — these ARE live (Test-Path at run time). `Missing` empty + engine/editor/shortcut valid => exit 0.
2. `ProofClaimState` — derived by reading cached JSON written by the *other* proof scripts: `Saved/Proof/last-policy-build-and-test.json`, `last-unreal-map-smoke.json`, `last-first-60-seconds-smoke.json`, `last-playtest-render.json`. It does NOT re-run build/automation/smoke.

**Why:** It is intentionally a file-evidence check so connector evidence can be gathered without a 5.7 editor launch. The tradeoff is that ProofClaimState reflects *the last time the expensive scripts were run*, not current source.

**How to apply:** When asked whether claims (171 tests, smoke, first60, playtest) are currently true, do NOT just trust ProofClaimState=true. Compare the mtime of `Saved/Proof/last-policy-build-and-test.json` against the newest file under `Source/` (and the Proof scripts). If any source/test file is newer than the cache, the cached green claims are stale and a fresh `build-and-test-policy.ps1` / smoke run is required before restating them as current. As of the 2026-06-09 review the caches were 2026-05-24 and NO source postdated them, so the green claims were still valid — old but not stale.

Exit-code gotcha: `$LASTEXITCODE` from the .ps1 does not propagate cleanly through nested `powershell -Command` or the Bash-tool pipeline. To read it reliably: `powershell -File '.\Proof\local-proof-checks.ps1' > $null 2>&1; exit $LASTEXITCODE` and read Bash `$?`.

Related: [[feedback-proof-ladder]] (never claim past current evidence).
