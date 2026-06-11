# SPLITROOT asset source approvals

Canonical standing registry: `games/splitroot/FactoryContracts/approved_assets.html`
and `games/splitroot/FactoryContracts/approved_assets.data.js`.
Add newly approved URLs there first; this coordination note is historical
context for the initial approval pass.

Updated: 2026-06-11
Owner lane: Ledger (units/items)

This is the local operational source list for free unit/item art candidates.
Project design authority still lives on the connector wiki; this file exists
so asset download/import sessions can act without re-reading chat history.

## Immediate add

- Infinity Blade Collection
  - URL: https://www.unrealengine.com/blog/free-infinity-blade-collection-marketplace-release
  - Status: approved by Jonathan; add/import ASAP.
  - Local check: not found in `Content/` or local Epic VaultCache on
    2026-06-11. Legendary CLI is authenticated, but
    `Saved/Proof/legendary-list-include-ue.json` only exposed UE engines,
    Fab UE Plugin, Quixel Bridge, Fortnite, and Fortnite STW content; no
    Infinity Blade or approved Paragon packs. The pack still needs an Epic/Fab
    library entitlement or a safe non-GUI downloader path before local import
    can execute.
  - Intended use: item-shop weapons, armor pieces, adversaries, effects, and
    faction-flavored loadout visuals. Good first source for shop rows unlocked
    by tech buildings.
  - Pack priority:
    1. Infinity Blade: Weapons
       - URL: https://www.unrealengine.com/marketplace/en-US/product/infinity-blade-weapons
       - Use: first-pass item-shop weapon mesh pool.
    2. Infinity Blade: Warriors
       - URL: https://www.unrealengine.com/marketplace/en-US/product/infinity-blade-characters
       - Use: heroic humanoid armor/body options.
    3. Infinity Blade: Adversaries
       - URL: https://www.unrealengine.com/marketplace/en-US/product/infinity-blade-enemies
       - Use: enemy/monster/creature body options, especially Kinwild and neutral mobs.
    4. Infinity Blade: Effects
       - URL: https://www.unrealengine.com/marketplace/en-US/product/infinity-blade-effects
       - Use: spell, hit, shop-item, beacon, and tech-building VFX.
    5. Infinity Blade: Grass Lands
       - URL: https://www.unrealengine.com/marketplace/en-US/product/infinity-blade-plain-lands
       - Use: Verdant/Lenswright set dressing if still available.
    6. Infinity Blade: Fire Lands
       - URL: https://www.unrealengine.com/marketplace/en-US/product/infinity-blade-fire-lands
       - Use: forge, siege, molten tech-building dressing if still available.
    7. Infinity Blade: Ice Lands
       - URL: https://www.unrealengine.com/marketplace/en-US/product/infinity-blade-ice-lands
       - Use: stone/ice ruins, neutral encounter dressing if still available.
  - Do not chase Infinity Blade: Sounds yet. Epic's announcement says the
    Sounds pack was removed from Marketplace after rights review.

## Approved, bring in selectively

Do not download every pack just because it is approved. Bring these in when a
specific faction, shop row, unit stand-in, hero presentation, or tech-building
unlock needs the asset.

## Local unit/body candidates

Structured inventory: `games/splitroot/FactoryContracts/unit_asset_candidates.json`.

- Rogue Character Model
  - Status: UE imported under `/Game/Fab/Library/Rogue_Character_Model`.
  - Best first use: Lenswright pressure-runner/scout placeholder; possible
    Verdant agile body if needed.
  - Notes: non-AI, rigged, Epic Skeleton-compatible listing text; needs frame
    verification before claiming in-game.
- Lizard Kin Warrior Character Rig
  - Status: raw GLB present; import pending while live canary stays running.
  - Best first use: Kinwild spear hunter / creature infantry.
- Low Poly wolf
  - Status: raw FBX present; import pending while live canary stays running.
  - Best first use: Kinwild companion/scout/beast-den shop row.
- Fantasy Game Character
  - Status: UE imported under `/Game/Fab/Fantasy_Game_Character`.
  - Best first use: low-priority Verdant placeholder only.
  - Notes: AI-generated, zero reviews; do not use as faction identity anchor.
- Free Stylized Assassin
  - Status: raw GLB present; low-priority import pending.
  - Best first use: Verdant shadow-runner placeholder only if needed.
  - Notes: AI-generated; avoid as final unit identity.

### Verdant Choir

- Paragon: The Fey
  - URL: https://www.fab.com/listings/9afbcde6-4a14-4018-95c3-2f3a2e1da858
  - Use: Briar Saint, grove caster, Verdant magic/nature silhouette.
- Paragon: Morigesh
  - URL: https://www.fab.com/listings/29e67175-fa08-448f-822b-37f411530749
  - Use: darker Verdant witch/sporecaster direction.

### Lenswright Compact

- Paragon: Greystone
  - URL: https://www.fab.com/listings/122fd7bf-6f12-4304-a930-cccbbacdaebc
  - Use: Gearguard/heavy armored kit.
- Paragon: Terra
  - URL: https://www.fab.com/listings/5ea6bcb6-e43e-4bbe-813f-c19d8c907565
  - Use: shield/tank armor kit.
- Paragon: Kwang
  - URL: https://www.fab.com/listings/f4c67e92-b976-4b5b-ab9f-4c25b010f6f3
  - Use: disciplined melee body if Lenswright needs a non-firearm close kit.

### Kinwild Covenant

- Paragon: Khaimera
  - URL: https://www.fab.com/listings/e7c665c1-8c13-42f0-9152-0753008853d7
  - Use: primary Kinwild hunter/beast-body candidate.
- Paragon: Grux
  - URL: https://www.fab.com/listings/8c4bac2c-f7f7-4632-a644-47f4e104f5d8
  - Use: heavy brawler creature kit.
- Paragon: Rampage
  - URL: https://www.fab.com/listings/0807cf74-08fd-4a33-8c8d-f33c9439fb1f
  - Use: late-tech creature tank or beast-form item.
- Paragon: Narbash
  - URL: https://www.fab.com/listings/d8904a0e-9169-4763-b82b-5fcf864235a4
  - Use: support/drummer/aura kit.

## Not approved

Do not download or import these for SPLITROOT unless Jonathan reverses this
decision later.

- FREE Starter Pack - Sidekick Modular Characters by Synty
  - URL: https://www.fab.com/listings/8d8e9639-d93f-4f1d-8332-32ae0ef14bca
- Free Rigged Knights - Game Ready Characters with Animations
  - URL: https://www.fab.com/listings/ada783ab-ca49-4b44-a555-0e51807e8fce
- Cursed Knight - UE5 Character on Sketchfab
  - URL: https://sketchfab.com/3d-models/cursed-knight-ue5-character-game-ready-f204e0ad3af645bcb0ecffd9e9ae9a15

## Rules

- Only completely free assets are eligible.
- Do not use Paragon trademarks to advertise or name the game.
- Lenswright assets must preserve the no-gunpowder rule.
- An asset is not "in-game" until there is a load log line, proof artifact, or
  captured frame showing it.
