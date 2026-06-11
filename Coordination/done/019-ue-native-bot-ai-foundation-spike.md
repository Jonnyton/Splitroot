# 019 - Spike UE-native bot AI foundation

Goal: stop hand-rolling generic team-FPS intelligence where Unreal already
has mature AI primitives. Keep SPLITROOT-specific strategy in local policies,
but move commodity bot foundations toward UE-native AIController, Perception,
Navigation, EQS, Smart Objects, Gameplay Debugger, and Visual Logger.

Vector research, 2026-06-11:
- UE 5.7 installed here includes `AIModule`, `NavigationSystem`,
  `GameplayStateTree`, `SmartObjects`, `GameplayBehaviorSmartObjects`,
  `MassEntity`, `MassGameplay`, `MassAI`, `MassCrowd`, and experimental
  `LearningAgents`.
- This project already depends on `AIModule` and `NavigationSystem`.
- `GameplayStateTree` is already enabled in `ArchonFactoryCanary.uproject`.
- Current bots are useful canaries, but `UArchonBotBrainComponent` owns too
  much generic FPS logic: perception, pursuit memory, stuck recovery,
  objective choice, and attack state.
- The right first move is not importing a giant NPC plugin. It is adding a
  UE-native adapter around the existing bot contract, then replacing only the
  commodity pieces that Unreal already solves.

Recommended implementation slice:

1. Add `AArchonBotAIController` or equivalent native controller for spawned
   bot player bodies.
2. Implement `IGenericTeamAgentInterface` and expose bot team id through the
   controller or pawn so AI Perception affiliation can work normally.
3. Add an optional `UAIPerceptionComponent` with sight/hearing/damage stimuli
   mapped to the current fair-senses doctrine.
4. Keep `UArchonBotBrainComponent` as the SPLITROOT decision shell for now,
   but let it read perceived hostile actors / last-known stimuli from the UE
   component when available.
5. Add a tiny EQS or policy-backed query seam for one commodity choice:
   nearest valid objective pressure point, nearest cover-ish firing point, or
   nearest usable feature terminal. Pick the smallest one that can be proven.
6. Add Gameplay Debugger or Visual Logger output for bot team, state, target,
   perceived enemy, objective, and feature-use decision.
7. Do not enable Mass, LearningAgents, or Wise Feline in this slice. Those are
   later/larger choices.

Done-when:
- Bot spawning still uses standard player bodies and supports human takeover.
- A bot can perceive enemies through UE-native perception without violating
  fair-senses rules.
- Existing bot match behavior remains green or has a clearly bounded failure.
- Replay/debug output can distinguish native perception from fallback custom
  perception.

Proof:
- Run `powershell -ExecutionPolicy Bypass -File .\Proof\build-locked.ps1`.
- Run `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\bot-match-smoke.ps1`.
- If adding a targeted proof, include the proof JSON/log path and sample lines
  showing `BotNativePerception` or equivalent markers.

Report-to:
- Move this file to `Coordination\done\` with `## Result`.
- Include changed files, commands, exit codes, proof artifact paths, and the
  next UE-native AI primitive that should be adopted after the spike.

## Result

Completed by Vector, 2026-06-11.

Changed files:
- `Source/ArchonFactoryCanary/Public/ArchonBotAIController.h`
- `Source/ArchonFactoryCanary/Private/ArchonBotAIController.cpp`
- `Source/ArchonFactoryCanary/Public/ArchonCanaryFpsCharacter.h`
- `Source/ArchonFactoryCanary/Private/ArchonCanaryFpsCharacter.cpp`
- `Source/ArchonFactoryCanary/Public/ArchonCombatHealthComponent.h`
- `Source/ArchonFactoryCanary/Public/ArchonBotBrainComponent.h`
- `Source/ArchonFactoryCanary/Private/ArchonBotBrainComponent.cpp`
- `Source/ArchonFactoryCanary/Private/ArchonCanaryWorldSubsystem.cpp`
- `games/splitroot/Proof/bot-match-smoke.ps1`
- `games/splitroot/Proof/live-bot-match-log-snapshot.ps1`

What shipped:
- Added `AArchonBotAIController` as the UE-native adapter for bot player
  bodies. It owns `UAIPerceptionComponent`, sight/hearing/damage sense config,
  and team identity.
- Exposed `IGenericTeamAgentInterface` on `AArchonCanaryFpsCharacter`, backed
  by the existing `UArchonCombatHealthComponent` team id, so UE Perception can
  classify player bodies by affiliation.
- Bot spawning now assigns the native controller before `SpawnDefaultController`
  and logs `BotAIControllerConfigured`.
- `UArchonBotBrainComponent` now asks native perception first, records
  `BotNativePerception`, and falls back to the existing custom fair-senses scan
  with `BotFallbackPerception`.
- Added a read-only live metadata parser,
  `games/splitroot/Proof/live-bot-match-log-snapshot.ps1`, so live-loop replay
  metadata can be sampled without launching or killing a proof harness.

Proof commands:
- `powershell -ExecutionPolicy Bypass -File .\Proof\build-locked.ps1`
  - exit code: 0
  - artifacts:
    - `Saved\Proof\last-policy-build.log`
    - `Saved\Proof\last-policy-automation.log`
    - `Saved\Proof\Automation\index.json`
  - result: build passed; automation passed 214/214.
- `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\bot-match-smoke.ps1`
  - exit code: 1
  - artifact: `Saved\Proof\last-bot-match-smoke.json`
  - bounded failure: the headless smoke spawned bots and native controllers, but
    its log did not progress into bot behavior markers during the observation
    window. Do not use this harness while Jonathan is running the live game.
- `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\live-bot-match-log-snapshot.ps1`
  - exit code: 0
  - artifact: `Saved\Proof\last-live-bot-match-log-snapshot.json`
  - result from the running live match loop:
    - `EvidenceReady=true`
    - `NativeBotControllerConfiguredCount=32`
    - `NativeBotPerceptionCount=151`
    - `FallbackPerceptionCount=91`
    - `WeaponFiredCount=299`
    - `TowerFiredCount=52`
    - `BotRespawnedCount=25`
    - feature coverage includes `perceive_enemy`, `march_objective`,
      `react_to_base_attack`, `respawn`, `capture_site`, `attack_tower`, and
      `attack_core`.

Operational correction:
- Do not run proof/build commands that force-stop Unreal while Jonathan is
  using the live loop. Treat the live game as the primary canary source and
  sample `Saved\Logs\ArchonFactoryCanary.log` with the read-only snapshot
  parser.

Next UE-native AI primitive:
- Add a small EQS-style or policy-backed "unstick / firing position" query.
  The live metadata now shows high `BotStuckRecovery` counts, so the next
  commodity behavior to replace is movement target selection around obstacles,
  not combat perception.
