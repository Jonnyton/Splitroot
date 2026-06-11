# 034 - Quarry valley triangle layout test

Goal: hand the remaining map-layout automation failure to the map lane without
blocking hot-reload adoption.

Observed failure:
- `Proof/build-and-test-policy.ps1` compiles successfully, but automation has
  one failure.
- Failing test: `ArchonFactory.Valley.ThreeFactionBasesFormTriangle`.
- Error: `Expected 'Lenswright and Kinwild far apart' to be true.`
- Source: `Source/ArchonFactoryCanary/Private/Tests/ArchonSplitrootValleyLayoutTests.cpp:177`.
- Latest proof: `Saved/Proof/last-policy-build-and-test.json` has
  `BuildExitCode=0`, `AutomationExitCode=0`, `AutomationFailedCount=1`.

Steps:
1. Quarry should inspect the current valley base positions and the test's
   distance threshold.
2. Decide whether the authored map layout is wrong or the old triangle
   invariant is stale after recent map work.
3. Update the layout or test contract, then run
   `powershell -ExecutionPolicy Bypass -File .\Proof\build-and-test-policy.ps1`.
4. Keep the always-running canary rule: do not close Jonathan's live game
   outside the supervisor boundary handoff.

Done-when:
- `ArchonFactory.Valley.ThreeFactionBasesFormTriangle` passes or is replaced
  by a correct current-map invariant.
- `Saved/Proof/last-policy-build-and-test.json` reports
  `AutomationFailedCount=0`.

Report-to:
- Move this file to `Coordination/done/` with `Status: completed`,
  `Status: staged_not_live`, or `Status: blocked_requires_handoff`.

## Result

Status: blocked_requires_handoff

Hex result time: 2026-06-11T18:28Z.
Gauge follow-up proof time: 2026-06-11T18:39Z.

Read:
- `Saved/Proof/last-policy-build-and-test.json`.
- `Saved/Proof/last-policy-automation.log`.
- `Saved/Proof/live-adoption-ledger.json`.
- `Saved/Proof/host-refresh-pending.json`.

Bounded proof:
- Latest completed policy proof still fails one automation test:
  `Saved/Proof/last-policy-build-and-test.json`.
  - `BuildExitCode=0`.
  - `AutomationExitCode=0`.
  - `AutomationSucceeded=false`.
  - `AutomationSucceededCount=235`.
  - `AutomationFailedCount=1`.
- Failure location remains:
  `Saved/Proof/last-policy-automation.log`.
  - `ArchonFactory.Valley.ThreeFactionBasesFormTriangle`.
  - `Expected 'Lenswright and Kinwild far apart' to be true.`
  - `Source/ArchonFactoryCanary/Private/Tests/ArchonSplitrootValleyLayoutTests.cpp(177)`.
- Source is now staged newer than the adopted DLL:
  `Saved/Proof/live-adoption-ledger.json`.
  - `SourceFilesNewerThanDllCount=3`.
  - Pending source files:
    `Source/ArchonFactoryCanary/Private/Tests/ArchonSplitrootValleyLayoutTests.cpp`,
    `Source/ArchonFactoryCanary/Private/ArchonSplitrootValleyLayoutLibrary.cpp`,
    `Source/ArchonFactoryCanary/Private/ArchonBlockoutActors.cpp`.
- Host is waiting for a supervisor-owned boundary refresh:
  `Saved/Proof/host-refresh-pending.json`.
  - `Pending=true`.
  - `Reason=source_newer_than_module_boundary_build_pending`.
  - `CurrentProcessId=7128`.
  - `SupervisorOwnsHost=true`.

Outcome:
- Hex did not run `Proof/build-and-test-policy.ps1` manually and did not stop
  or relaunch the live Unreal process.
- Gauge observed the supervisor adopt the staged source at the next match
  boundary. The current live module is newer than the staged source, but
  `ArchonFactory.Valley.ThreeFactionBasesFormTriangle` still fails.

Gauge follow-up:
- `Saved/Proof/last-host-supervisor-refresh.json` now shows `BeforePid=15248`,
  `AfterPid=25396`, `BuildAttempted=true`, `PolicyBuildExitCode=0`,
  `BuildSucceeded=true`, and `ModuleTimestampAdvanced=true`.
- `Saved/Proof/live-adoption-ledger.json` now shows
  `SourceFilesNewerThanDllCount=0`, `ContractFilesNewerThanDllCount=0`, and
  `HostRefresh.Pending=false`.
- `Saved/Proof/Automation/index.json` still reports
  `ArchonFactory.Valley.ThreeFactionBasesFormTriangle` as `Fail`, with
  `Expected 'Lenswright and Kinwild far apart' to be true.` at
  `Source/ArchonFactoryCanary/Private/Tests/ArchonSplitrootValleyLayoutTests.cpp:177`.
- Latest failure timestamp is `2026.06.11-18.36.08`, after the adopted
  `v20260611-183550` build.

Next:
- Quarry should continue owning the map-layout invariant decision. The failure
  is live-adopted now, not merely staged.
