# SPLITROOT Unit Ledger

Updated: 2026-06-11
Owner lane: Ledger
Machine-readable source: `unit_ledger.json`
Tactical roster view: `unit_roster.html` / `unit_roster.data.js`
Approved asset registry: `approved_assets.html` / `approved_assets.data.js`

This ledger lays out the current unit roster, prototype stats, mechanics,
team use, tech-building gates, and selected visual assets. It does not claim
final balance or production art. A unit asset is not "in-game" until a local
status, proof artifact, load log, or captured frame proves it.

## Shop Tech

| Tech | Current build target | Unlocks | Team use |
| --- | --- | --- | --- |
| `armory` | `verdant_armory_slot` | `pressure_bolt_crossbow`, `lenswright_pressure_runner` | Ranged pressure and Lenswright body row. |
| `beast_den` | `kinwild_beast_den_slot` | `kinwild_spear_hunter`, `kinwild_pack_companion` | Kinwild hunter plus companion/scout row. |
| `grove` | `verdant_grove_slot` | `verdant_temporary_runner` | Verdant growth/body row; placeholder until stronger organic assets land. |

## Verdant Choir

| Unit | Status | Stats | Mechanics | Team use | Current asset |
| --- | --- | --- | --- | --- | --- |
| Thornbound | RTS squad presence | 150 HP, 0.95 speed, low supply | Melee escort, bramble guard, basic always-available body | Cheap lane holder and frontline screen | No selected final asset; current debug/RTS squad placeholder |
| Sporecaster | Spec drafted | 90 HP, 1.0 speed, medium supply | Spore bolt, spore bloom | Backline area pressure | No selected asset; approved pool includes Morigesh/The Fey |
| Grovekeeper | Spec drafted | 110 HP, 1.0 speed, medium-high supply | Vine lash, growth pulse, grow cover patch | Sustain and terrain control after Grove | No selected asset; The Fey is best current approved candidate |
| Temporary Verdant Runner | Shop row | 100 HP target, 1.05 speed target, cost 100, Grove | Placeholder agile body | Proves Grove can unlock a unit row | `fab_fantasy_game_character`, UE imported placeholder only |
| Briar Saint | Contracted hero | 350 HP, 1.15 speed, 1.3 weapon multiplier | BriarWall, ChoirCircle, thorn/root control | Verdant terrain-control hero | No selected asset; The Fey/Infinity Blade pool approved |

## Kinwild Covenant

| Unit | Status | Stats | Mechanics | Team use | Current asset |
| --- | --- | --- | --- | --- | --- |
| Spear Hunter | Shop row, import pending | 130 HP target, 1.10 speed target, cost 160, Beast Den | Bound-leap, spear pressure, shock assault | First real Kinwild body row | `fab_lizard_kin_warrior`, raw GLB present/import pending |
| Pack Companion | Shop row, import pending | 70 HP target, 1.25 speed target, cost 80, Beast Den | Scout/reveal/chase/bite-assist direction | Cheap pack pressure and scouting | `fab_low_poly_wolf`, raw FBX present/import pending |
| Pack Caller | Open hero/support slot | 300 HP target, 1.05 speed target | Hunt horn, aura, scent/reveal, beast-call TBD | Needed support identity beyond melee pressure | No selected asset; Narbash/Khaimera/Grux/Rampage approved pool |

## Lenswright Compact

| Unit | Status | Stats | Mechanics | Team use | Current asset |
| --- | --- | --- | --- | --- | --- |
| Gearguard | Spec drafted | 140 HP, 0.95 speed, low supply | Shield, impact baton/pressure punch | Frontline holder for ranged units | No selected asset; Greystone/Terra approved pool |
| Bracewright | Implemented debug actor | 80 HP, 360 walk speed | Defensive ranged AI, pressure-bolt crossbow | Armory ranged pressure and cover defense | C++ debug mesh; `fab_rogue_character_model` selected for first visual swap |
| Sundial Optic | Implemented debug actor | 60 HP, 420 walk speed, 4500 vision | ScoutNoWeapon, long-vision optic | Map-table information advantage | Debug cylinder; no body selected yet |
| Lens Arbalist | Legacy archetype | 85 HP, 1.0 speed, medium supply | Lens-guided bolt, focus shot | Damage counterpart if kept separate from Bracewright | No selected asset; may merge with Bracewright |
| Fieldwright | Spec drafted | 100 HP, 1.0 speed, medium-high supply | Repair tool, deploy pressure node | Keeps buildings/siege alive | No selected asset; needs engineer/tool silhouette |
| Pressure Runner | Shop row | 100 HP target, 1.15 speed target, cost 120, Armory | Prototype skirmisher/scout body | Fast Armory body row | `fab_rogue_character_model`, UE imported |
| Master Artificer | Contracted hero | 280 HP retained from v0 sheet pending reconciliation | PressureGate, OpticBarrage, repair/device lineage | Lenswright siege/control hero | No selected asset; Greystone/Terra/Infinity Blade pool approved |

## Loadouts

| Item | Status | Gate | Stats | Notes |
| --- | --- | --- | --- | --- |
| `thornsprout_bow` | Shipped | Free | 999 quiver, 1.2s fire, 1.8s reload, 5600 speed, 8000 range, 35/80/22 damage | Current basic Verdant weapon. |
| `pressure_bolt_crossbow` | Shipped | Armory | 1 quiver, 1.5s fire, 2.2s reload, 6000 speed, 6000 range, 40/90/25 damage | Lenswright pressure weapon. No gunpowder. |

## Squad Packages

| Squad | Composition | Status | Use |
| --- | --- | --- | --- |
| `reinforcement_squad` | Current runtime size 2 | Implemented purchase/fielding | Comeback-proof army fill; catalog cost 150, proof override cost 5. |
| `verdant_thornbound_squad` | 5 Thornbound | v0 sheet | Cheap line holder. |
| `verdant_sporecaster_squad` | 3 Sporecaster | v0 sheet | Small ranged support group. |
| `verdant_mixed_grove_squad` | 3 Thornbound, 2 Sporecaster, 1 Grovekeeper | v0 sheet | Self-sustaining Verdant push. |
| `lenswright_gearguard_squad` | 5 Gearguard | v0 sheet | Shield line. |
| `lenswright_ranged_squad` | 3 Lens Arbalist | v0 sheet | Ranged Lenswright pressure; may remap to Bracewright. |
| `lenswright_field_column` | 3 Gearguard, 2 Lens Arbalist, 1 Fieldwright | v0 sheet | Durable repair/support column. |
| `kinwild_pack_pair` | 2 Spear Hunter, 2 Pack Companion | Ledger new package | First Kinwild flank/scout harassment package. |

## Open Decisions

- Decide whether Lens Arbalist remains separate or folds into Bracewright.
- Replace or retire `verdant_temporary_runner` before its AI-generated placeholder becomes identity by inertia.
- Author the Kinwild Pack Caller support/hero spec.
- Make reinforcement training faction-aware; the current train intent still targets `verdant_reinforcement_squad`.
