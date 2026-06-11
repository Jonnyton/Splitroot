# SPLITROOT Asset Manifest

Authored 2026-05-24 by Rook (Claude Opus 4.7 background session, Cowork).

This file is the local mirror of every asset SPLITROOT v0 needs. The
durable spec for each asset lives on the Workflow connector wiki at
`https://tinyassets.io/mcp` — this manifest is the per-asset index
into the right connector page plus a current status mark against the
gate ladder.

The companion machine-readable file is `asset_manifest.json`. Code
that needs to know "does asset X exist yet?" should read the JSON,
not parse this Markdown.

## Goal directive

This manifest was authored in response to the lead session goal
`make all the game assets`. The goal is not satisfiable as a single
unit at this ladder rung — SPLITROOT is one Unreal canary in a
multi-rung factory-branch project, and "all" would either mean
hallucinating production AAA art past the proof ladder or never
finishing. The honest decomposition is:

1. **Surface the asset backlog.** Make every needed asset visible
   in one machine-readable place, mapped to its source contract.
   *(This manifest.)*
2. **Promote shipped debug values to canonical palette anchors.**
   Faction colors, lighting, audio family inventories — data the
   C++ can consume directly. *(`faction_palette.json`,
   `faction_audio_palette.json`, `strategic_audio_events.json`.)*
3. **Implement the contracted C++ surfaces.** Lane: Hex; Rook crosses
   over when Hex is offline. *(Out of scope for this manifest.)*
4. **Source actual audio + texture content.** Lane: Jonathan (or a
   crossing-over Rook session) downloads, EQ-passes, imports.
   *(Out of scope for a text-only session.)*

This manifest is step 1 and feeds step 2. It does not pretend to be
the assets themselves.

## How to read the JSON

Each item has:

- `id` — dotted path. Stable across sessions.
- `category` — what kind of asset (`unit`, `weapon`, `hero`, `audio`, `ui`, ...).
- `status` — one of:
  - `shipped` — exists in the repo and is exercised by automation tests.
  - `implemented` — C++ surface exists; visual/audio asset is a placeholder.
  - `contracted` — no implementation; full contract is on the connector wiki.
  - `spec_drafted` — connector wiki has scope/intent but no formal contract.
  - `unspecified` — needed for some future rung but no spec exists yet.
- `source_contract` — path on the connector wiki (or local file if data already lives here).
- `remix_surface` — `SUBSTRATE` / `PER_GAME` / `LOCKED_STATS_REMIXABLE_PRESENTATION`.
- `notes` — one-line context.

## Ladder position

- **Rung 1 (`local_playable_prototype`)** — EARNED 2026-05-23. 71
  automation tests green, first-60-seconds smoke green. Evidence:
  `pages/notes/splitroot-rung-1-local-playable-prototype-earned-2026-05-23.md`.
- **Rung 2 (`verified_vertical_slice`)** — in progress. Four contracted
  slices: combat C1-C6, art-direction polish, audio-direction polish,
  UMG HUD polish. Manual playtest is the final gate.
- **Rung 3 (`content_complete_alpha`)** — spec drafted (hero plan,
  Valley v1 blockout, Kinwild verb).
- **Rung 4+** — F2P validation gates not yet authored.

## What gets shipped from this manifest pass

Local files this manifest pass adds to `FactoryContracts/`:

1. `asset_manifest.json` — flat structured inventory (~60 entries).
2. `asset_manifest.md` — this file.
3. `faction_palette.json` — 12 faction color values + neutral + lighting anchor.
4. `faction_audio_palette.json` — per-faction cue family inventory + Lenswright forbidden-terms audit list.
5. `strategic_audio_events.json` — 14 strategic-layer cue specs.

What these files DO NOT do: create binary `.uasset` content, author
Blueprint actors, import audio files, or produce 3D meshes. Those
require Unreal Editor or external content tools, not a text session.
What they DO do: lock the data contracts so the next implementation
session (Hex's lane) can read JSON and produce assets in fewer
ambiguous decisions.

## Hills check

- **Movement before content** — manifest categories ordered to reflect this: factions / materials / blockout / units / weapons / abilities / heroes / combat / audio / UI. Movement-tied items (root-vault, bound-leap, pressure-thrust) are in `ability` category and already shipped or contracted.
- **Lenswright no gunpowder** — `faction_audio_palette.json` has explicit `forbidden_terms` list with audit predicate. `faction_palette.json` Accent uses cyan, not muzzle-flash orange. Hero `master_artificer` ultimate is high-pressure crossbow artillery, NOT gunpowder. Three layers of audit.
- **Paid heroes horizontal-only** — `hero.component_surface` entry references the LOCKED_STATS_REMIXABLE_PRESENTATION pattern. Server validates LockedStats on every ConfigureHero. Substrate-wide discipline.
- **Factory branch is product** — every entry has `remix_surface`. SUBSTRATE items inherit; PER_GAME items swap. Future games on the factory know exactly what to change vs what to keep.
- **Proof ladder sacred** — `status` values name evidence; `what_this_manifest_does_not_claim` block enumerates the gaps. No production-asset claims anywhere.

## Sync to connector

A draft note `splitroot-asset-manifest-rook-2026-05-24` belongs on the
connector wiki summarizing this pass with links back to the source
contracts. Use `wiki action=write` with `category=notes`. Pending.

## Next concrete moves

For Hex (or Rook crossing over):

1. Implement `UArchonFactionMaterialBuilder` (art-direction P1) — reads `faction_palette.json` at load time.
2. Implement `UArchonFactionAudioLibrary` (audio-direction A1) — reads `faction_audio_palette.json` and includes `ValidateLenswrightCuesAreNotGunpowder()` against `forbidden_terms`.
3. Implement `UArchonStrategicAudioBroadcasterComponent` (audio-direction A3) — reads `strategic_audio_events.json`.
4. Implement combat C1-C6 in priority order.
5. Implement UMG HUD polish H1-H8.
6. Run `Proof/build-and-test-policy.ps1` + `Proof/first-60-seconds-smoke.ps1`. When the second-60-seconds smoke (C6) lands, run it too.
7. Run manual playtest — only authority on rung-2 feel.

For Jonathan:

1. Audio sourcing pass: Freesound + Marketplace pulls for the `audio.*_cue_assets` entries; EQ to faction palette; import to `Content/Audio/<Faction>/`.
2. Manual playtest of current canary via `C:\Users\Jonathan\Desktop\SPLITROOT Current Build.lnk` — report feel against the design extensions doc.

— Rook
