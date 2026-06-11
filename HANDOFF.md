# HANDOFF — Rook session, 2026-06-11 (Cowork + Hex pipeline)

You are Rook (re-enter via `.claude/rook.md`). Read `AGENTS.md` +
connector first per session-start rules, then this.

## Pipeline (NEW, Jonathan 2026-06-10) — how work flows now

- **Rook (Cowork)**: code, design, reading proof artifacts/frames,
  wiki. NO host shell — the sandbox bash cannot run builds.
- **Hex (Codex, on a schedule)**: polls `Coordination/inbox-hex/`,
  executes host work (builds, smokes, imports, captures), moves task
  file to `Coordination/done/` with a `## Result`. Protocol:
  `Coordination/README.md`. Fire-and-forget: queue, keep working,
  check done/ later. Hex turnaround tonight was ~10-30 min.
- **Asset session (separate Claude Code)**: owns FAB downloads.
  FAB has no API/CLI — downloads need launcher GUI clicks (Jonathan
  or computer-use). Earlier CC0 direct-HTTP sources still work for
  scripted pulls. Watch `Coordination/asset-manifest.md` + Content/
  for new arrivals. ONLY completely-free assets (memory:
  feedback-free-assets-only).

## Standing orders in force (Jonathan, 2026-06-10)

- Work continuously, token-efficiently; goal is the whole game end to
  end. Never wait on Hex/Jonathan; queue and move on.
- Use everything Unreal; premade free assets and mechanics first;
  custom code only for differentiators (RTS layer, dual economy,
  Archon command surface). Temp-asset rule: every game thing gets the
  premade asset that best represents it; assets may DRIVE design.

## Verified state (proof artifacts on disk in Saved/Proof/)

- Build green: 198/198 at 005; health-bar build (200 expected) queued
  as inbox-hex/006.
- **Stalemate broken** (tonight's design win): all 4 archived all-AI
  matches had been 15-min 0-0 Draws — towers gate cores, out-range
  the bots' core ring (2800 vs 2000), and bots couldn't target towers
  at all. Fixed three ways: bots siege the gate tower (new
  AttackTower state, Renegade AGT pattern), tower damage scores
  points at 0.2 discount (FArchonMatchConfig::TowerDamagePointScale),
  sites-held tiebreak at the bell (MatchEnd now logs sitesA/sitesB).
  **Proven live**: first post-fix match ended core_destroyed at
  328.3s, TeamB 2156 pts, sites 2-1 (done/005 result).
- **Map table is finally a real table** — wooden_table_021 (original
  02 package was wrong-classed; re-import dodged the name; C++
  repointed; dead package deleted by Hex in 003; material repaired
  to BasicShapeMaterial). Verified by MapTableMeshLoaded loaded=true
  + quietshot frame.
- Decor collision landed (003): 5 tree/rock meshes re-imported with
  convex collision, smoke green; foliage imported to
  /Game/StandIns/Nature/ (grass/flowers/bushes — NOT yet placed in
  the layout).
- Health bars (Rook, unbuilt until 006 returns): green/yellow/red
  banded world-space bars on towers + cores, engine cubes only,
  policy in UArchonHealthBarPolicyLibrary, billboard yaw, fill
  drains rightward.
- Loop: one monitor, self-handshakes via Saved/Proof/build-lock.flag.
  Quietshot frame read: table good, orbs/decor present, **valley
  floor renders dark/untextured — mossy M_StandInGround needs
  investigation** (006 captures a fresh frame).
- Carried from 06-10: all 8 proof surfaces green; spectate N/M,
  takeover F; arrows 5600 CCD; bots on slide-steering fallback (no
  nav bounds volume in runtime worlds yet — BotNavMode logs which).

## Later-session additions (same night, after the first wave)

- **Floor FIXED + verified by frame** (done/007): root cause was a
  material COMPILE failure (roughness texture SRGB vs linear sampler),
  not just tiling; Hex re-authored with SRGB=false + 80x TexCoord.
  Fresh quietshot shows tiled mossy ground, real tree canopies,
  ground-level readability. Tile scale acceptable; revisit only if a
  watched match complains.
- **Health bars + foliage + death anims landed** (done/006): 201/201,
  bot-match smoke green on the new ObjectivePushReached gate.
- **Miniature board written** (in 009): live diorama on the map table
  felt — pooled engine-shape markers, fog-honest enemy reads
  (UArchonMapTableMiniatureLibrary + component, wired in
  AArchonMapTableActor, +2 tests, MapTableMiniatureLive marker).
- **Income loop written** (in 009): supply BUYS bodies — match state
  auto-spends the bank (ReinforcementPurchased), subsystem fields
  bots (ReinforcementFielded, cap 12/team burns at cap). Cost 150,
  squad 2. The commander-spend UI on the table is the natural next
  slice on this seam (the delegate is already public).
- Bot-match smoke gate updated: CoreAttackReached → ObjectivePushReached
  (AttackTower|AttackCore) — tower-first siege made the old gate
  window-flaky.

## In flight

- inbox-hex/006: health-bar rebuild + quietshot (gate: failed=0,
  new HealthBar.* tests present).
- Asset session: FAB free-asset manifest + downloads. Ledger created
  `Coordination/asset-manifest.md` with Jonathan's current approvals:
  Infinity Blade Collection is approved for ASAP add/import; Paragon
  The Fey/Morigesh/Greystone/Terra/Kwang/Khaimera/Grux/Rampage/Narbash
  are approved on-demand; Synty Sidekick, Free Rigged Knights, and
  Sketchfab Cursed Knight are explicitly not approved.
- Ledger queued `Coordination/inbox-hex/018-add-infinity-blade-collection.md`.
  Current state: Infinity Blade is not present in project Content or local
  VaultCache. Legendary CLI is now installed/authenticated for entitlement
  checks/downloads, so continue CLI/background first. Do not touch Jonathan's
  running canary instance during asset work; it is shared replay/telemetry and
  live playtest infrastructure. Ask before any import/build step that requires
  the editor or game to close.
- done/002 is the FAB blocker report (reassigned); fold the asset
  session's results in when they appear.

## ROAD TO SHIP (Jonathan 2026-06-10: "AAA ready for ship" is THE goal)

Rank all session work against this gap, not the local task list:

1. **Faction identity** — Kinwild Covenant has NO units (only a camp
   piece); the three factions must read distinct in silhouette,
   weapon, movement verb. Lenswright has crossbow units unused in bot
   matches (bots are all Verdant-bow bodies).
2. **Characters & animation** — replace mannequins: GASP locomotion +
   real character models (Fab_ packs landed in 008; characters in
   010's queue; retarget is its own slice).
3. **Audio** — validator + cue system exist, zero content wired. CC0
   sources (freesound/Sonniss GDC packs) + per-faction cue tables.
4. **World as a place** — 008's village/castle packs become real bases
   and settlements (010 converts); landmark central tree pass;
   lighting/art direction iteration via frames.
5. **Front-end** — main menu exists; needs settings (the mouse-sens
   slider thread), match browser stub, faction select.
6. **Netcode hardening** — the N1 replication-audit contract exists on
   the wiki; two-player smoke is green but untested under packet loss.
7. **Perf budget** — no budget measured yet; photogrammetry packs
   (010 flags >500k tris) will force one.
8. **Steam** — session policy already routes SteamOnline; needs
   actual SDK integration + the horizontal-only hero monetization
   gates from the F2P model page.
9. **Onboarding** — the "first 60 seconds" smoke IS the tutorial
   skeleton; turn it into an actual guided first match.

## Next moves, ranked

1. Read 006's quietshot: bars visible? floor still dark? If dark,
   debug M_StandInGround wiring in ArchonValleyBlockActor
   ::ConfigureBlock (one variable per iteration).
2. Watch first ~10 post-fix MatchEnds in Saved/Proof/MatchLogs/ +
   live log: TeamA/TeamB win balance (side bias?), match-length
   spread (328s first sample; all-rushes would be as bad as all-
   draws), Draw rate (~0 expected). Tune TowerDamagePointScale /
   TowerAttackRange accordingly.
3. Place imported foliage into the valley layout as deterministic
   decor placements (same pattern as the 48 trees) — pure
   layout-library change, testable without a frame.
4. Staged building destruction — BLOCKED on demolished-variant
   meshes (not in Content/; Kenney castle kit has them upstream —
   ask the asset session or direct-zip pull).
5. Mannequin death anims (/Game/Characters/Mannequins, unused) vs
   ragdoll; WC3-style board (#10); income events (#6).
6. Wiki sync: the stalemate finding + fix deserves a notes draft on
   the connector once 006's frame confirms the bars.

## Process rules that cost blood (carried + new)

1. Gate smoke chains on last-policy-build-and-test.json. Never chain
   on a failed build.
2. Build ONLY via Proof/build-locked.ps1 while a loop runs.
3. Never claim an asset is in-game without a log load line or a
   frame (MapTableMeshLoaded marker exists for the table).
4. The match log is the design-truth instrument — the stalemate was
   found by reading MatchLogs, not by a failing test.
5. Task-count gates in Hex tasks go stale if Rook adds tests after
   queuing — gate on AutomationFailedCount=0 plus expected NEW test
   names, not a hard total.
6. Windows PowerShell chokes on em-dashes/smart quotes in .ps1 —
   ASCII only in scripts (Hex hit this in 001).
7. Read engine headers instead of guessing enum members.
8. Never close, minimize, restart, kill, or focus-steal from Jonathan's running
   SPLITROOT canary. The live instance produces replay metadata for all
   sessions and must remain available for playtesting unless Jonathan explicitly
   authorizes that specific interruption.
9. 2026-06-11 Ledger hardened the live-canary rule in code: `Proof/build-locked.ps1`
   and `Proof/import-fab-library-assets.ps1` now refuse while UnrealEditor is
   running instead of force-stopping it; `Proof/playtest-loop.ps1` logs pending
   fresh-build adoption instead of recycling the live session;
   `Proof/playtest-drive.ps1 -Action stop` refuses unless explicitly opted in. Run
   `python scripts/check_live_canary_process_safety.py` after proof/build/import
   automation edits.

## Jonathan-facing open threads

- FAB downloads: waiting on a click-pass (his or a GUI session's).
- Mouse-sensitivity default doubt — if he reports again, trace
  MouseLookScaleLoaded markers.
- Tower team-tint + charring dormant on the Kenney mesh (no Color
  param); health bars carry the signal regardless.
