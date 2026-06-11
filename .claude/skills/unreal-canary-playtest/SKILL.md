---
name: unreal-canary-playtest
description: Use when a Rook session needs to actually SEE the SPLITROOT canary instead of trusting headless log flags. Launches the Unreal canary with rendering, drives the first-60-seconds scripted arc, captures screenshots, and surfaces PNG paths so the session can Read them as images and evaluate the build visually. Grows a learning log over time so future sessions know what good frames look like vs. what's known-broken.
---

# Unreal Canary Playtest

## Why this skill exists

The headless smoke (`Proof/unreal-map-smoke.ps1`, `games/splitroot/Proof/first-60-seconds-smoke.ps1`)
proves structural correctness via log flags. It cannot see whether the
table looks like a table, whether the squad cube is where it should be,
whether the cover stones are in a line, or whether the Lenswright
silhouette reads as a ghost. Jonathan shouldn't have to look at every
half-baked frame either — when an interface is obviously basic the
session should be able to see that itself and iterate.

This skill gives a Rook (or any Claude Code session) eyes on the
canary without needing a human in the loop.

## What it does

1. Launches the canary with real rendering (`-RenderOffScreen`,
   no `-NullRHI`) at a known resolution (default 1280×720).
2. Sets the `-ArchonRunFirst60SecondsProof` and
   `-ArchonPlayTestScreenshots` command-line flags so
   `UArchonCanaryWorldSubsystem` runs the first-60-seconds proof arc
   and, after the arc completes, frames the camera toward the canary
   blockout, requests a PNG screenshot 2.5s later, then quits 2.5s
   after that.
3. Wipes prior `Saved/Screenshots/Windows/*.png` and `Saved/AutoScreenshot.png`
   so the post-run listing reflects only this playtest.
4. Returns the captured PNG paths as JSON.
5. The session reads each PNG with the Read tool — Claude is
   multimodal and gets the image content directly.

## How to use it from a session

```powershell
powershell -ExecutionPolicy Bypass -File .\Proof\playtest-render.ps1
```

Reads JSON `{ Screenshots: [<absolute paths>] }`. Read each PNG path
with the Read tool. Then evaluate against the checklist below.

## Visual-correctness checklist (current iteration)

Update this each session based on what was broken/right.

### What the first-60-seconds frame SHOULD show

- A framed rendered camera view of the `Lvl_FirstPerson` canary corner.
- The canary `BP_FirstPersonGameMode` HUD if visible.
- Somewhere in view (or near it):
  - The runtime map table actor (`AArchonMapTableActor`) — a placeholder mesh with `TextRenderComponent` saying `RTS SURFACE CLOSED\nFPS CONTROL ACTIVE` after the arc.
  - The canary RTS squad actor (`AArchonCanaryRtsSquadActor`) — a small cube with `SQUAD MOVING` / `SQUAD HOLDING` text.
  - The blockout actors: VerdantOutpost (large mossy cube), SplitrootTreeCentral (tall brown cylinder), LenswrightOutpostGhost (brass-oxblood cube), 12 CoverStoneRootVault (small grey stones in a row).

### Known limitations (do NOT report as bugs, they're scoped out at this iteration)

- No production art. Everything is engine BasicShapes (cubes, cylinders).
- No UMG widget visible at the moment of screenshot — the arc has CLOSED the table before the screenshot.
- No squad-arrow visuals.
- No fog-of-war shader in 3D view (only data-layer).
- HUD strings on the map table actor are placeholder.
- The screenshot is taken after the player has done all 3 root-vaults via direct component drive — the pawn may have moved.
- The screenshot camera is auto-framed for inspection; it is not manual spawn-perspective feel evidence.

### Triggers for surfacing to Jonathan

Report to Jonathan only if:
- The screenshot is fully black / fully white / engine crash splash.
- The blockout actors are missing entirely (visual contradiction with `BlockoutActor` log flags).
- A choice was made that violates a hill (e.g., Lenswright with muzzle flash; that would be a real hill bug worth interrupting for).

Otherwise, log findings into "Learning log" below and iterate the skill.

## Learning log (grows each session)

Future sessions: append. Don't delete prior entries unless they're
factually wrong; otherwise add a follow-up note.

### 2026-05-24 — first iteration

- Skill created.
- Pipeline added: `Proof/playtest-render.ps1` + `-ArchonPlayTestScreenshots` flag in `UArchonCanaryWorldSubsystem`.
- First playtest result: the arc completed and Unreal logged a screenshot, but
  the script looked only for `HighresScreenshot*.png`; Unreal wrote a different
  path, so `ScreenshotCount=0`.
- Follow-up fix: the script now collects `Saved/Screenshots/Windows/*.png` and
  `Saved/AutoScreenshot.png`, and the runtime frames the camera toward the
  first-60 blockout before capture.
- Verified follow-up result: `Proof/playtest-render.ps1` returned
  `ExitCode=0`, `PlayTestCameraFramed=true`, `ScreenshotCount=1`, and
  `Saved/Screenshots/Windows/Playtest_0001.png`.
- Visual finding from the first readable frame: blockout labels were mirrored
  from the inspection camera/player side. Fixed by rotating the native
  blockout `TextRenderComponent` labels to face the spawn-side view.

### 2026-05-24 — second iteration (post-label-fix visual audit)

After the text-rotation fix shipped, re-ran `Proof/playtest-render.ps1`.
Read `Saved/Screenshots/Windows/Playtest_0001.png`. The labels are
now legible: "CENTRAL SPLITROOT" top-left, "VERDANT OUTPOST" mid-left
(partial occlusion), "LENSWRIGHT GHOST" deep in frame (small, but the
ghost-keep actor is identifiable for the first time).

Visual issues that remain (queued for iteration 3+, none worth surfacing to Jonathan yet):

1. **Scene visually mixes with `Lvl_FirstPerson` starter template clutter** (yellow cubes, orange pillars, grey parkour blocks). Can't tell SPLITROOT actors apart from template props at a glance. Fixes available:
   - Spawn the canary on `Lvl_Empty` instead of `Lvl_FirstPerson`.
   - OR move the spawn anchor 5000+uu away from the template course.
   - OR add a debug skybox/floor tint when the playtest flag is set.
2. **No faction color distinction.** All blockout meshes use default basic-shape grey. The fix in C++ (per actor constructor):
   - `UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mesh->GetMaterial(0), this);`
   - `MID->SetVectorParameterValue(TEXT("BaseColor"), FactionColor);`
   - `Mesh->SetMaterial(0, MID);`
   - Verdant: `FLinearColor(0.30f, 0.55f, 0.25f)`; Lenswright: `FLinearColor(0.55f, 0.30f, 0.15f)`; Splitroot trunk: `FLinearColor(0.45f, 0.30f, 0.15f)`; Cover stones: neutral grey is OK.
3. **Squad cube + map table actor not in framing camera's view.** Squad at (640,-260,120); table at (350,0,120). Either widen the camera frame or move spawn anchors.
4. **Lenswright ghost is full-opaque** — should be semitransparent to read as a ghost. Apply translucent material with `Opacity ~0.5`.

Iteration 3 target: pick ONE of these fixes (the faction-color one is highest-value-per-effort) and re-run. Update this log after.

Text-orientation caveat: the static 180° yaw fix in iteration-2 is correct for the current framing camera vantage. From the opposite side the labels will mirror again. The fully-correct fix is a billboard tick that aims at the active player camera each frame.

### 2026-05-24 - third iteration (faction-color pass)

Added native debug colors for the blockout actors and exposed them through
`GetDebugColor()` so automation can prove the visual contract:

- Verdant outpost: green-dominant.
- Splitroot central tree: warm brown.
- Lenswright ghost: brass/oxblood.
- Cover stones: neutral grey.

`Proof/build-and-test-policy.ps1` now reports 73 passing automation tests,
including `ArchonFactory.Blockout.DebugColorsDistinguishFactionActors` and
`ArchonFactory.Blockout.CoverStoneDebugColorNeutral`.

Re-ran `Proof/playtest-render.ps1` and read
`Saved/Screenshots/Windows/Playtest_0001.png`. The Verdant outpost now
visibly reads pale green, and the Lenswright/Splitroot placeholders are warmer
than the default BasicShapes grey. This does not make the scene production
readable yet, but it removes the "all grey placeholder" failure from
iteration 2.

### 2026-05-24 — fourth iteration (Rook re-read after C2 bow + faction-color landings)

Re-ran the skill from a different Rook session after:
- Hex shipped C2 Verdant bow wired into `AArchonCanaryFpsCharacter` ctor.
- Hex's iteration-3 faction-color materials are in.
- Combat contracts C1-C6 + Kinwild + art/audio/HUD/MP/hero plans all
  authored on the connector (Rook's parallel lane this session).

Read iteration-4 `Playtest_0001.png`. The framed shot shows:
- "CENTRAL SPLITROOT" label readable top-center.
- "VERDA..." label mid-left (Verdant Outpost).
- **A green cube center-frame** — the Verdant Outpost's debug-color
  material is doing real visual work. First time the Verdant building
  pops as Verdant in the screenshot without needing the label.
- "LENSWRIGH..." label deep in the scene (smaller, far).
- FirstPerson template orange/yellow content still cluttering the
  background and foreground — the SPLITROOT actors are now
  identifiable BY COLOR but they share the frame with template props
  that share similar hues.

Real progress: iteration 1 (totally invisible) → iteration 2 (text
readable, no faction visual ID) → iteration 4 (color identity emerging).

Issues remaining (rung-2 polish slices already plan-spec'd for these):
1. **Scene clutter still mixes template props with canary actors.**
   Fix: art-direction polish §"P4 — Splitroot Valley lighting +
   atmosphere" + Splitroot Valley v1 blockout. Once a non-template
   map ships, this resolves.
2. **Splitroot tree (brown cylinder) not clearly visible in frame.**
   May be behind the Verdant Outpost; camera framing needs to
   include it explicitly.
3. **Lenswright ghost too small/distant** at current framing — its
   brass-oxblood doesn't read as ghost-translucent yet (still
   full-opaque; art-direction P3 will fix).
4. **The first-person camera doesn't visibly show the Verdant bow**
   in this capture — bow is constructed on the FPS character but
   not rendered in the framing camera's vantage (camera is set
   third-person-style by the proof runner's `FramePlayTestCameraForScreenshot`).
   Substrate idea: alternate "first-person vantage" screenshot mode
   that captures from the FPS pawn's camera component (would show
   the bow). Filed for next iteration.

Iteration 5 target (when Hex's rung-2 polish art-direction
`UArchonFactionMaterialBuilder` lands): re-shoot; expect Verdant
green to deepen + Splitroot brown to pop + Lenswright to read
brass-oxblood-with-some-translucency.

This iteration confirms the playtest skill is doing exactly what
it was built for: surfacing real visual progress between sessions
without Jonathan needing to look at every half-baked frame.

Remaining visual issues, now highest value:

1. Starter-template geometry still dominates the frame; SPLITROOT blockout
   actors are mixed into yellow/orange/grey FirstPerson course props.
2. The `CENTRAL SPLITROOT` label is too large and clipped at the top of the
   screenshot.
3. Verdant label still partially occludes the actor from this camera angle.
4. Squad cube and map table actor are still outside the screenshot framing.

Iteration 4 target: isolate the canary blockout from `Lvl_FirstPerson`
template clutter or move the playtest framing to a dedicated clean anchor.

### 2026-06-09 — fifth iteration (FIRST SPLITROOT VALLEY RENDER — off the template at last)

Context: Fable 5 Rook session, solo-dev mode. C6 second-60 smoke landed
green (combat anchor complete). Valley shipped as code:
`UArchonSplitrootValleyLayoutLibrary` (pure placements, 6 tests) +
`AArchonValleyBlockActor` + runtime lighting from `GetLightingAnchor()`,
spawned behind `-ArchonSplitrootValley`. Rendered on `/Engine/Maps/Entry`
via `playtest-render.ps1 -Map /Engine/Maps/Entry -ExtraArgs "-ArchonSplitrootValley"`
(script now takes -Map and -ExtraArgs).

What the frame showed (Playtest_0001.png, iteration 5):

**Confirmed working — the big wins:**
- ZERO template clutter. The whole frame is SPLITROOT for the first time.
- The Central Splitroot reads as a LANDMARK: trunk + wide flat canopy
  silhouette dominates the skyline from across the valley. The scale
  decision (45m trunk, 55m canopy) is right.
- Ridge walls frame the valley edges; floor reads as one coherent place.
- Verdant base pad pops green in the corner; lane stones visible.
- Runtime SkyAtmosphere + SkyLight + DirectionalLight + fog all work
  spawned at runtime on an empty engine map — no authored .umap needed.

**Issues found (iteration 6 targets):**
1. **Whole scene bleached sand-beige.** Root cause: lighting anchor's
   `ExponentialFogDensity = 0.05` authored-on-paper was never rendered;
   at 400m valley scale it washes everything to fog color. Retuned to
   0.008 (types header + faction_palette.json mirror updated; the
   LightingAnchor test pins pitch/yaw/skylight/bloom, NOT fog — no test
   change needed). LESSON: art-direction numbers authored without a
   render are hypotheses, not constants.
2. Camera too far back: base in corner, stones tiny. Valley framing
   moved (-20000,-7000,3200)→(-13000,-5200,1900), target z 2000→1500.
3. Wood browns / canopy green barely read through the wash — re-judge
   after the fog fix before touching palette values.
4. Buttress roots invisible at distance — may need bigger scale.
5. Bases need massing (walls/structures), not just pad + small cube.

### 2026-06-09 — sixth iteration (valley visual pass COMPLETE: exposure, canopy, loam)

Three more render-judge-fix loops after the fog fix, one variable each:

1. **Exposure (renders 3-4):** post-fog frame still bleached cream.
   Root cause: auto-exposure normalizing a dim warm scene (6-lux sun).
   Fix: runtime `APostProcessVolume` (bUnbound) with EV100 locked
   min=max. First guess 1.0 overexposed — for ~6 lux × 0.35 albedo the
   correct lock is ~EV 2.6. Verified: sky reads blue, shadows hold.
   LESSON: never trust auto-exposure in a procedurally-lit blockout;
   lock EV100 and derive the value from sun lux × albedo.
2. **Canopy (render 5):** canopy was authored `VerdantSecondary` —
   that's the CREAM slot of green-cream. Read as a mushroom-table.
   Layout now uses `VerdantPrimary`; tree reads as foliage landmark.
   LESSON: palette SLOT names lie; check the actual RGB when a piece
   reads wrong. Layout tests pin counts/heights, not colors, so color
   retunes are test-safe.
3. **Ground (render 6):** `GroundEarth 0.35/0.27/0.19` lifted to
   desert sand under warm sun + EV lock. Retuned to loam
   `0.16/0.13/0.08` (palette lib + faction_palette.json mirror).
   Valley now reads dark borderland earth; green pads/canopy pop.

Banked state after this pass: fog 0.008, EV100 2.6, canopy
VerdantPrimary, ground loam. Camera (-16800,-6200,2300)→(0,0,1300).
Buttresses scaled (22,4,4.4) — still subtle at distance, acceptable.

Known clutter (parked): old first-60 blockout cluster (CENTRAL
SPLITROOT label + ghost outposts + 12 stones) spawns at (800,0,100)
regardless of valley mode and lands at the valley trunk. Rebase its
anchor when bValleyActive — folded into the match-framework task.

New since render 6 (not yet rendered): valley layout adds 2 `BaseCore`
pieces (faction-primary pillars inside each base corner) + 3 neutral
`ResourceSite` pads (central-south of tree, north flank, south flank)
for the playable match loop. Next render should verify they read.

### 2026-06-10 — seventh iteration (PLAYER-STYLE TESTING: real window, real mouse — two launch-blocking bugs found)

Jonathan hit a launch-blocking bug the entire proof chain missed: the
main menu spawned with NO visible mouse cursor, so a human could not
click anything. His feedback (now a standing memory): "you should be
actually launching it and using it like a user to play test it."
Headless smokes drive HANDLERS (`HostMap()` via proof timer); they
prove nothing about whether a player can physically reach the button.

New methodology added this session — the player-style test:

1. Launch the real windowed build with the SAME map URL the desktop
   shortcut uses (`/Engine/Maps/Entry?ArchonMainMenu -game -windowed`)
   plus `-abslog` for verification.
2. Find the game window by PID (EnumWindows; VERIFY WindowTitle —
   `Process.MainWindowHandle` + `CopyFromScreen` happily captures
   whatever OTHER window overlaps that rect; the first capture grabbed
   a terminal and a blind click would have landed in it).
3. Bring it to front: `SetWindowPos(HWND_TOPMOST)` +
   `AttachThreadInput` before `SetForegroundWindow` (background
   processes are denied plain SetForegroundWindow by the
   foreground lock).
4. Park the OS cursor inside the window, capture with the hardware
   cursor DRAWN INTO the bitmap (`GetCursorInfo`+`DrawIcon` —
   CopyFromScreen omits the cursor), and READ the PNG. Cursor flags
   are also engine-independent truth: flags&1 == visible.
5. Click with real OS input (`SetCursorPos` + `mouse_event` down/up)
   at coordinates picked by LOOKING at the screenshot, then capture
   again and scan the -abslog markers.
6. Leave the session running afterward unless certain it is idle —
   Jonathan was walking around in the match when an earlier cleanup
   `Stop-Process` killed it under him.

Scripts: promoted to **`Proof/playtest-drive.ps1`** (Layer A) — the
player-style driver. Actions: `launch` (windowed, shortcut-equivalent
args + -abslog), `capture -Name x.png` (cursor drawn in), `key -Key
escape|tab|e|w|...` (SendInput SCANCODES), `hold -Key w -Ms 1200`
(movement), `click -X -Y` (screen coords), `look -Dx -Dy` (RELATIVE
mouse deltas in 30-count/10ms steps — this is how UE camera look is
driven through the user path), `markers -Pattern regex` (scan the
abslog), `status`, `stop`. Session state in
`Saved/Proof/playtest-drive-session.json`; captures in
`Saved/Proof/PlaytestDrive/`. Every action re-asserts foreground
(topmost + AttachThreadInput). Pass `-Name shot.png` to any input
action to capture after it (act -> settle -> capture -> verify).

Harness self-test results (2026-06-10, two live runs): ALL EIGHT
primitives proven through the user path — launch (lowered posture),
window-targeting (verify title contains 'Archon' — PID alone captures
overlapping windows), capture (restored ground truth), quietshot
(equivalence-proven), click→Slate (HOST button, sensitivity slider),
scancode key→game (Escape, multiple cycles), look (relative deltas
yawed the camera), hold (W walked the character). Harness bugs the
self-test caught: (1) `$PSScriptRoot` is empty in param-default
evaluation under `powershell -File` — resolve the project root in the
body; (2) clicks work without focus but KEYS need it, and
`SetForegroundWindow` is silently denied under the foreground lock —
fall back to clicking the title bar (user-path focus); (3) **NEVER
launch a UE 5.7 windowed game minimized** — the D3D12 swapchain never
initializes and the first restore crashes the render thread
(EXCEPTION_ACCESS_VIOLATION in D3D12RHI/SlateRHIRenderer).

The "good cheat" doctrine (Jonathan 2026-06-10): the harness may use
techniques a player couldn't (quiet capture of an occluded window) so
long as the SYSTEM UNDER TEST stays real — same rendered pixels a
player would see, same input path. Test the equivalence before
trusting the shortcut. Implemented as:
- **Lowered posture** (never minimized): window restored at bottom
  z-order, never activated — invisible under Jonathan's windows,
  taskbar-clickable for him, still rendered.
- **quietshot**: PrintWindow(PW_RENDERFULLCONTENT) of the lowered
  window — verified pixel-equivalent to the restored ground truth.
- **Input bursts**: restore+focus briefly, act, re-lower, hand focus
  back to Jonathan's previous app.
- **In-engine recorder**: any attended, rendered session (Rook OR
  Jonathan via the shortcut) logs `PlaytestRecorderStarted` and writes
  a screenshot every 5s to `Saved/PlaytestCaptures/<session>/` (one
  session per world; keeps recording while lowered) PLUS an immediate
  **event shot** on every meaningful beat — map_table_open/closed,
  rts_order, pause_open/close, weapon_fire, player_death,
  player_respawn, menu_host, menu_join — with the event in the
  filename (`shot_0009_map_table_open.png`), so a fast human sequence
  inside one periodic window is never lost (Jonathan's
  build-then-change-body-in-10s case). Rate limit is PER EVENT NAME
  (0.75s): spam shares one shot per beat, distinct beats never swallow
  each other. Headless smokes are excluded by the unattended/render
  gates. Ingest findings into this log, then `-Action clear`.

More self-test-caught bugs (2026-06-10, event-shot verification run):
(4) the title-bar focus click leaks into UE as a raw-input LMB and
FIRES THE BOW — focus fallback is now an ALT tap (maps to nothing
in-game) with the title-bar click as warned last resort; (5) a global
event rate limit swallowed pause_open behind a weapon_fire — per-event
keying fixed it. Lesson: verify the recorder by driving a real
multi-beat sequence and checking every beat got a named frame.

Operating etiquette (learned the hard way, both on 2026-06-10):
- A running session is a LISTEN SERVER on port 7777 after hosting and
  LOCKS the module DLL — `launch` refuses to start over a live session,
  and you must `stop` before any build. Smokes use dedicated ports
  (7878 two-player, 7879 front-end, 7880 bot-match) so they never
  collide with a real session.
- While the perpetual loop runs, build ONLY through
  `Proof/build-locked.ps1`: it raises `build-lock.flag`, WAITS for
  every editor process to exit (a 45s blind grace lost a link race on
  day one), builds, and releases. The loop relaunches on the new build
  by itself.
- Do NOT drive the screen while Jonathan is at the machine — injected
  input steals his mouse. If he interrupts a drive, stop driving and
  let him use the open session himself.

Bugs this found (both fixed + smoke-pinned same session):

1. **Invisible mouse on the menu.** `SpawnMainMenuIfRequested` set the
   cursor correctly, but `OnWorldBeginPlay` continued into
   `InstallRuntimePlayerBridge`, which possessed an FPS body and set
   `FInputModeGameOnly` + `SetShowMouseCursor(false)` over the menu.
   Fix: menu world is now menu-only (early return before map
   table/squad/bridge) + `ApplyMainMenuInputMode` re-assert with
   `MainMenuCursorShown cursor=true` marker. Front-end smoke asserts
   `MenuCursorShown` + `MenuWorldIsMenuOnly`.
2. **Menu stayed painted over the match after HOST.** The Slate panel
   was added to the game viewport, which SURVIVES ServerTravel.
   Fix: subsystem holds the widget ref and removes it in
   `Deinitialize()` (fires on world teardown for both host and join
   paths); marker `MainMenuPanelRemoved`, asserted as
   `MenuPanelRemoved`.

Verified the player way: menu renders with visible cursor (hover
highlight works), one real mouse click on HOST travels to the valley,
player spawns as FPS character, match loop active, and the after-click
frame is CLEAN (no menu overlay) showing the Verdant base corner.

LESSON: the proof ladder needs BOTH rails — headless markers for
regression-locking, and a player-style pass through the real entry
point for anything player-facing. A passing smoke is not evidence a
human can play it.

### 2026-06-10 — eighth iteration (E-interact slice playtest: the harness finds design truth)

First slice playtested end-to-end THROUGH the new harness (E-prompt +
third-person toggle). The headless proof passed first run (walk-up arc
teleport-verified). The player-path test then produced findings no
headless proof could:

1. **No wayfinding.** Navigating eye-level from spawn to the team's own
   map table took MANY look/walk iterations — no compass, no markers,
   no landmark legibility for the most important object in the game.
   A new player hits this in minute one. (Queued: compass/objective
   markers in HUD slice; table beacon.)
2. **The map table is visually anonymous** — small unlabeled cube among
   cubes; its status text faces one direction and the table top reads
   as a separate object from a side approach.
3. **Jonathan hit the E-flow blind spot live**: pressed E at the prompt
   and "nothing happened" — the surface opened INVISIBLY (the logic
   widget has no rendering). Interim fix shipped same hour: a visible
   MAP TABLE overlay (live order count/sequence + key guide + TAB
   close). Real board UI is its own task.
4. **Prompt-as-proximity-detector**: while lost, the InteractPromptShown
   marker doubled as a range alarm — markers are navigation
   instruments during playtests, not just assertions.
5. Mouse-look calibration drifts with the sensitivity setting (it is a
   user config now) — calibrate per session from a landmark before
   navigating; ~10°/1100 counts at the 0.07 default.

Also hardened the human-respect protocol: ALL surface-touching actions
now refuse to run unless system input has been idle ≥45s
(`GetLastInputInfo`); `-Force` only when Jonathan explicitly hands the
machine over. He reported the window "popping up in front of what I'm
doing" — the gate makes that structurally impossible.

### 2026-06-10 — ninth iteration (BOT MATCHES: watch the replay metadata, not the screen)

Jonathan's reframe: LLMs are slow hands but fast readers — so don't
hand-play; run all-AI matches (bots = canaries exercising the human
verb set) and read the MARKER TIMELINE + recorder frames like replay
metadata. First sessions of this method immediately found:

1. **Arrow self-collision bug (launch-blocking for combat)**: the
   first 3v3 ran two minutes — both teams engaged, hundreds of shots,
   ZERO deaths. Arrows spawn at the camera INSIDE the shooter's
   capsule and detonated on the archer's own face. No earlier proof
   caught it because every prior damage path used synthetic origins
   away from a body. Fix: `IgnoreActorWhenMoving(Owner)` +
   owner-check in HandleHit.
2. **Meatgrinder match shape**: first watched 8v8 produced 899 events,
   heavy symmetric death churn (bots at 6-8 deaths by minute 3.5), and
   ZERO core damage — 2200uu aggro radius pulled every marcher into
   every fight (4+ bots piling one target). Fix: role mix — fighters
   at 1400uu aggro, every third bot a BREACHER at 500uu that pushes
   the core. Result: CoreDamaged + AttackCore flipped true, BOTH cores
   under asymmetric pressure (a real race).

Method notes: the marker timeline is queryable with one regex pass
(BotState/BotRespawned/BaseCoreDamaged/MatchPhase) — 899 events read
in seconds; per-bot death counts and pile-on patterns fall out of the
data; the screen capture is only needed when the numbers don't explain
the feel. Launch via
`playtest-drive.ps1 -Action launch -MapUrl '<valley match url>'
-ExtraArgs '-ArchonBotMatch -ArchonBotCountPerTeam=8'` (lowered,
recorder on), arm a Monitor on the MatchEnd marker, analyze after.

### 2026-06-10 — tenth iteration (the perpetual loop era begins)

The playtest stopped being an event: `Proof/playtest-loop.ps1` (run
under a persistent Monitor) keeps an all-AI 8v8 running match after
match — 15-min cap, valley self-cycling, every relaunch adopting the
newest DLL, `build-lock.flag` handshake with builds, human-idle gate
on every (re)launch. Match HUD + base defense towers shipped into the
loop the same night (bot-match-smoke: 14 gates green, full chain
green).

**Next session, FIRST MOVE: read the overnight data.** `-Action
sessions` for recorder folders; the loop's drive log marker timeline
(BotState/BotRespawned/BaseCoreDamaged/TowerFired/MatchPhase/
TravelingTo) per session; per-team death/damage tallies; did any match
reach MatchEnd with a WINNER? Did towers change who breaks through?
Ingest findings here, `-Action clear`, then tune ONE variable and let
the loop keep going. That is the standing loop: the matches play all
night; the morning reads the replays.

### 2026-06-10 — eleventh iteration (the dressed-world night: process rules)

Hard-won rules from the forest/asset/nav marathon:

1. **Gate smoke chains on the build JSON, not script flow.** A failed
   compile left the old DLL in place; the unconditionally-chained map
   smoke ran green on the STALE binary and the failure hid for a full
   cycle. Pattern now: read last-policy-build-and-test.json, run
   smokes only when BuildExitCode==0 AND AutomationFailedCount==0.
2. **One asset import per commandlet run.** The post-save Slate assert
   kills every queued task after the first; a 10-asset batch landed 1.
   The one-per-run loop (env-var script) lands all of them.
3. **Nav-data absence must degrade, not freeze.** MoveToLocation
   returns Failed when no RecastNavMesh exists (runtime worlds have no
   bounds volume); an unconditional `return` after it froze every bot
   — silent-match signature #2. Pattern: detect Failed → fall back to
   slide steering → log BotNavMode transitions.
4. **Engine headers: read the enum, don't guess.** Two compile cycles
   lost to guessing EPathFollowingRequestResult members; one grep of
   the engine source answers it (`Failed` needed
   Navigation/PathFollowingComponent.h, not the fwd-decl).
5. The bot smoke is now the FASTEST design-truth instrument: it caught
   the tower-gates-the-core siege arc (gates updated to
   structure-damage OR) and both silent-match freezes within minutes
   of their builds.

## Iteration notes for next session

When you use this skill:

1. Run `Proof/playtest-render.ps1`. If `ExitCode != 0` or `ScreenshotCount < 1`, diagnose (renderer may need different flags; on a true headless server `-RenderOffScreen` won't work without SwiftShader / virtual display).
2. Read each PNG path with `Read`. Look at the actual pixels.
3. Compare against the checklist. Add a Learning-log entry naming what was off OR what was confirmed working.
4. If something was clearly broken in a fixable way (camera at wrong location, missing actor, etc.), fix the source + re-run the headless smoke + re-run this playtest.
5. If the screenshot didn't show what you needed (e.g., wanted to see the table OPEN but the arc closed it before capture), modify `UArchonCanaryWorldSubsystem::SchedulePlayTestScreenshotsIfRequested` to take MULTIPLE shots at different phases, then update this skill's checklist accordingly.

## Substrate improvements to consider over time

- Multi-screenshot mode: one per arc phase (before-table-open, table-open, after-order, after-root-vault). Each screenshot named `HighresScreenshot_phaseN.png`.
- A debug-overlay flag that draws actor names + state on screen for the screenshot, so the image is legible even with placeholder art.
- A "compare to previous run" hash check that flags when the screenshot pixel diff exceeds a threshold (regression alarm).
- Eventually: drive real input via `FInputKeyParams` instead of direct component calls, so the screenshot truly reflects what a player would see when pressing keys.

## Failure modes seen so far

- 2026-05-24: screenshot path mismatch. The runtime logged screenshot capture,
  but `Proof/playtest-render.ps1` found zero PNGs because it searched only
  `HighresScreenshot*.png`. Fixed by collecting all playtest PNG outputs and
  adding `PlayTestCameraFramed` as a required log marker.

## Related skills + scripts

- `unreal-archon-game-factory` — canonical Unreal-canary skill; this is the visual-evidence companion to its proof scripts.
- `Proof/build-and-test-policy.ps1` — automation tests, run before playtest if source changed.
- `games/splitroot/Proof/first-60-seconds-smoke.ps1` — headless equivalent of this playtest; passes/fails on log flags, no PNG.
- `Proof/unreal-map-smoke.ps1` — canary smoke for the existing arc.
