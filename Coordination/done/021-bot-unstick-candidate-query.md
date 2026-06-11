# 021 - Bot unstick candidate query

Status: staged_not_live

Completed by Vector, 2026-06-11.

Why:
- Live replay metadata showed the next AI canary problem after native
  perception was movement recovery, not target choice.
- `games/splitroot/Proof/live-bot-match-log-snapshot.ps1` reported
  `BotStuckRecoveryCount=550` in the running live loop at
  `2026-06-11T16:17:02Z`.
- Stuck logs showed repeated alternating targets for the same bots, which
  means the old two-point side/backstep escape can oscillate.

Changed files:
- `Source/ArchonFactoryCanary/Public/ArchonBotSteeringPolicyLibrary.h`
- `Source/ArchonFactoryCanary/Private/ArchonBotSteeringPolicyLibrary.cpp`
- `Source/ArchonFactoryCanary/Public/ArchonBotBrainComponent.h`
- `Source/ArchonFactoryCanary/Private/ArchonBotBrainComponent.cpp`
- `Source/ArchonFactoryCanary/Private/Tests/ArchonBotSteeringPolicyTests.cpp`
- `games/splitroot/Proof/live-bot-match-log-snapshot.ps1`

What shipped:
- Added `ComputeUnstickEscapeCandidates` to generate a small ordered set of
  escape targets instead of only one alternating side/backstep target.
- Preserved `ComputeUnstickEscapeTarget` as the legacy first candidate for
  existing callers/tests.
- Added repeated-attempt expansion and forward diagonals so later attempts can
  leave a side/back oscillation.
- `UArchonBotBrainComponent` now traces candidate escape targets and selects
  the first clear one before falling back.
- Added replay metadata:
  - `ArchonFactoryCanary: BotUnstickQuery ... selected=clear|fallback`
  - `BotFeatureUse feature=unstick_reposition ...`
- Extended the read-only live snapshot parser with:
  - `BotUnstickQueryCount`
  - `BotUnstickQueryClearCount`
  - `BotUnstickQueryFallbackCount`
  - `LatestUnstickQuery`

Proof status:
- Read-only live snapshot command ran successfully and did not touch the live
  Unreal process:
  - `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1`
  - exit code: 0
  - artifact: `Saved\Proof\last-live-bot-match-log-snapshot.json`
- Current live build has not loaded this new C++ yet, so
  `BotUnstickQueryCount=0` is expected until a host-safe refresh or explicit
  restart loads the new module.
- Build/automation was intentionally not run from this lane because Jonathan's
  visible live game is running and coordination task 020 says not to close it
  for fresh-build adoption until a host-safe supervisor exists.

Next evidence to watch:
- After the next host-safe fresh build loads, compare:
  - `BotStuckRecoveryCount` per minute before/after;
  - `BotUnstickQueryClearCount` vs `BotUnstickQueryFallbackCount`;
  - whether high-repeat bots stop climbing into attempt counts above 10.
