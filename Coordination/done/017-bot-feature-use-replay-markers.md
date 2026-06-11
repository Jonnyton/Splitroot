# 017 - Add bot feature-use replay markers

Goal: make live all-AI matches prove whether bot players are using available
game features, not merely spawning, fighting, and respawning.

Vector context, 2026-06-11:
- Bots are already framed as real player bodies with programmed behavior:
  standard bodies, faction loadouts, fair senses, respawn, objective marching,
  tower/core pressure, and human takeover support.
- Replay/log metadata is now the design-truth instrument for this lane.
- Current bot proof gates answer broad questions such as spawned, fought,
  fired, respawned, reached objective pressure. They do not yet answer which
  player-facing features bots attempted, succeeded at, skipped, or could not
  reach.
- As map-table build verbs, tech/shop loops, equipment choices, purchases,
  rally/production, and richer RTS commands land, the AI players should become
  canaries for those features.

Recommended implementation:
1. Add stable replay markers for bot feature decisions, for example:
   `ArchonFactoryCanary: BotFeatureUse bot=3 team=0 feature=attack_tower decision=attempt result=success reason=in_range`.
2. Define initial feature keys for the current and near-term game surface:
   `march_objective`, `perceive_enemy`, `attack_tower`, `attack_core`,
   `respawn`, `capture_site`, `react_to_base_attack`,
   `purchase_reinforcement`, `use_shop`, `use_map_table_order`, `build_tech`.
3. Emit markers only where the feature exists today; future/locked features
   may be reported as unavailable in a non-gating summary, not faked.
4. Add a per-match summary marker such as:
   `ArchonFactoryCanary: BotFeatureCoverage feature=attack_tower attempted=4 succeeded=2 skipped=0 unavailable=0`.
5. Extend `games/splitroot/Proof/bot-match-smoke.ps1` or add a focused proof
   that parses the feature-use markers and records them in JSON.
6. Keep the fair-senses doctrine intact: feature-use success must not come from
   hidden map knowledge or authority a human player would not have.

Done-when:
- A live replay log can answer which currently available features bots used,
  which they ignored, and which are unavailable because the game feature is not
  built yet.
- The proof JSON includes a feature-coverage object suitable for Vector replay
  audits.
- At least the current core bot features (`march_objective`, `perceive_enemy`,
  `attack_tower` or `attack_core`, `respawn`, `capture_site`) are covered.

Proof:
- Run `powershell -ExecutionPolicy Bypass -File .\Proof\build-locked.ps1`.
- Run `powershell -ExecutionPolicy Bypass -File .\games\splitroot\Proof\bot-match-smoke.ps1`.
- Include proof JSON/log paths and sample `BotFeatureUse` /
  `BotFeatureCoverage` lines in the result.

Report-to:
- Move this file to `Coordination\done\` with `## Result`.
- Include changed files, commands, exit codes, proof artifact paths, and any
  feature keys intentionally left non-gating because the player-facing feature
  does not exist yet.

## Result

Completed by Hex on 2026-06-11 using the always-running live canary as the proof source. No Unreal process was stopped, relaunched, or replaced for this result.

Changed files observed for this slice:
- `Source/ArchonFactoryCanary/Private/ArchonBotFeatureTelemetry.h`
- `Source/ArchonFactoryCanary/Private/ArchonBotFeatureTelemetry.cpp`
- `Source/ArchonFactoryCanary/Private/ArchonBotBrainComponent.cpp`
- `Source/ArchonFactoryCanary/Private/ArchonMatchStateActor.cpp`
- `games/splitroot/Proof/bot-match-smoke.ps1`

Proof outcomes:
- `Saved/Proof/last-policy-build-and-test.json`: `BuildExitCode=0`, `AutomationExitCode=0`, `AutomationSucceededCount=214`, `AutomationFailedCount=0`.
- `Saved/Proof/last-live-bot-match-log-snapshot.json`: `EvidenceReady=true`, `LogExists=true`, live `UnrealEditor.exe` PID `26024`, `MatchLoopActive=true`, `BotMatchSpawned=true`.
- `Saved/Proof/last-live-bot-match-log-snapshot.json`: `LatestFeatureCoverage` includes `perceive_enemy`, `march_objective`, `react_to_base_attack`, `respawn`, `capture_site`, `attack_tower`, and `attack_core`.
- `Saved/Logs/ArchonFactoryCanary.log`: live log continued writing after the snapshot and showed rolling `BotFeatureUse` / `BotFeatureCoverage` markers, including `perceive_enemy`, `march_objective`, `respawn`, and `react_to_base_attack`.

Sample live markers:
- `ArchonFactoryCanary: BotFeatureUse bot=0 team=0 feature=perceive_enemy decision=attempt result=success reason=fair_senses`
- `ArchonFactoryCanary: BotFeatureCoverage feature=perceive_enemy attempted=84 succeeded=84 skipped=0 unavailable=0`
- `ArchonFactoryCanary: BotFeatureUse bot=10 team=1 feature=respawn decision=attempt result=success reason=respawn_timer`
- `ArchonFactoryCanary: BotFeatureCoverage feature=march_objective attempted=43 succeeded=43 skipped=0 unavailable=0`

Non-gating feature keys intentionally left out of live bot coverage:
- `purchase_reinforcement`
- `use_shop`
- `use_map_table_order`
- `build_tech`

Bounded caveat:
- `Saved/Proof/last-bot-match-smoke.json` from the standalone hidden smoke did not pass: it reports no feature markers and no combat progression in that run. Because Jonathan's visible canary is now the always-on telemetry source, this task is closed against the live log snapshot instead of launching another disruptive or misleading standalone proof. The standalone bot smoke should be fixed separately so it matches live-canary behavior without touching the visible game.
