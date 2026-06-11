window.SPLITROOT_UNIT_ROSTER = {
  schema: "tinyassets.splitroot.unit_roster_view.v1",
  updated_at_utc: "2026-06-11T19:10:00Z",
  source_contracts: [
    "unit_ledger.json",
    "approved_assets.data.js"
  ],
  roster_notes: [
    "This is the standing tactical roster view. It is for understanding faction roles, tech paths, and current selected assets.",
    "Prototype stats are iteration targets, not final balance.",
    "Current asset means selected or strongest current candidate, not necessarily in-game proof."
  ],
  factions: [
    {
      id: "VerdantChoir",
      label: "Verdant Choir",
      identity: "Terrain control, living arrows, bramble cover, grove sustain, agile defenders.",
      movement: "VerdantRootVault",
      tech_paths: [
        {
          id: "verdant_base",
          label: "Base / Always Available",
          role_summary: "Cheap frontline, basic ranged pressure, and comeback-safe reinforcement.",
          entries: [
            {
              id: "verdant_thornbound",
              type: "unit",
              name: "Thornbound",
              role: "Frontline melee screen",
              gate: "free / reinforcement",
              cost: "low supply / squad",
              stats: "150 HP, 0.95 speed",
              mechanics: "Melee escort, bramble guard read, always-available body lineage.",
              team_use: "Holds lanes and protects Sporecaster, Grovekeeper, or hero pushes.",
              asset_ids: ["infinity_blade_warriors", "infinity_blade_adversaries"],
              asset_status: "No selected final asset; debug/RTS squad placeholder now."
            },
            {
              id: "thornsprout_bow",
              type: "item",
              name: "Thornsprout Bow",
              role: "Basic weapon",
              gate: "free",
              cost: "0",
              stats: "999 quiver, 1.2s fire, 5600 speed, 8000 range, 35/80/22 damage",
              mechanics: "Living-arrow projectile. Fire cycle gates rhythm; ammo does not currently gate play.",
              team_use: "Keeps a razed team fighting.",
              asset_ids: ["infinity_blade_weapons"],
              asset_status: "Weapon mesh upgrade pool; current C++ weapon is functional.",
              visual_status: "No actual bow mesh thumbnail yet. Infinity Blade Weapons is approved but not local; current C++ weapon is behavior-only."
            },
            {
              id: "reinforcement_squad",
              type: "squad",
              name: "Reinforcement Squad",
              role: "Basic army fill",
              gate: "free",
              cost: "catalog 150 / proof override 5",
              stats: "runtime size 2",
              mechanics: "TryPurchaseReinforcement fields bots and logs purchase/field events.",
              team_use: "Comeback-proof bodies when advanced tech is gone.",
              asset_ids: [],
              asset_status: "Uses current bot placeholder; needs faction-aware target later."
            }
          ]
        },
        {
          id: "verdant_grove",
          label: "Grove",
          role_summary: "Sustain, organic control, and Verdant caster/scout identity.",
          entries: [
            {
              id: "verdant_temporary_runner",
              type: "unit",
              name: "Temporary Verdant Runner",
              role: "Agile placeholder body",
              gate: "Grove",
              cost: "100",
              stats: "100 HP target, 1.05 speed target",
              mechanics: "Proof row for tech-gated Verdant unit purchases.",
              team_use: "Temporary Grove payoff until real Verdant body assets land.",
              asset_ids: ["fab_f065c0cd", "paragon_the_fey", "paragon_morigesh"],
              asset_status: "Selected: Fantasy Game Character, UE imported placeholder only.",
              visual_status: "Local source preview available; AI-generated placeholder should be replaced before faction identity locks."
            },
            {
              id: "verdant_sporecaster",
              type: "unit",
              name: "Sporecaster",
              role: "Ranged support",
              gate: "Grove",
              cost: "medium supply target",
              stats: "90 HP, 1.0 speed",
              mechanics: "Spore bolt and spore bloom.",
              team_use: "Backline area pressure behind Thornbound.",
              asset_ids: ["paragon_morigesh", "paragon_the_fey", "infinity_blade_effects"],
              asset_status: "Unselected; Morigesh is current best approved direction."
            },
            {
              id: "verdant_grovekeeper",
              type: "unit",
              name: "Grovekeeper",
              role: "Support / cover control",
              gate: "Grove",
              cost: "medium-high supply target",
              stats: "110 HP, 1.0 speed",
              mechanics: "Vine lash, growth pulse, grow cover patch.",
              team_use: "Sustain and terrain-control anchor.",
              asset_ids: ["paragon_the_fey", "infinity_blade_effects"],
              asset_status: "Unselected; The Fey is current best approved direction."
            }
          ]
        },
        {
          id: "verdant_hero",
          label: "High Supply / Hero",
          role_summary: "Late control hero and horizontal-only paid hero slot.",
          entries: [
            {
              id: "hero_briar_saint",
              type: "hero",
              name: "Briar Saint",
              role: "Terrain-control sustain hero",
              gate: "high supply unlock",
              cost: "high unlock",
              stats: "350 HP, 1.15 speed, 1.3 weapon multiplier",
              mechanics: "BriarWall, ChoirCircle, thorn/root control.",
              team_use: "Decisive cover and sustain anchor without paid raw power.",
              asset_ids: ["paragon_the_fey", "infinity_blade_warriors", "infinity_blade_effects"],
              asset_status: "Unselected; The Fey is current best approved source candidate."
            }
          ]
        }
      ]
    },
    {
      id: "KinwildCovenant",
      label: "Kinwild Covenant",
      identity: "Pack pressure, bonded beasts, scouting, shock assaults, and bound-leap mobility.",
      movement: "KinwildBoundLeap",
      tech_paths: [
        {
          id: "kinwild_base",
          label: "Base / Gap",
          role_summary: "Kinwild still needs a free/basic identity row beyond generic reinforcement.",
          entries: [
            {
              id: "kinwild_base_body_tbd",
              type: "open",
              name: "Base Hunter TBD",
              role: "Basic body gap",
              gate: "free",
              cost: "tbd",
              stats: "tbd",
              mechanics: "Should establish pack identity before Beast Den.",
              team_use: "Needed so Kinwild has a faction read before tech investment.",
              asset_ids: ["paragon_khaimera", "infinity_blade_adversaries"],
              asset_status: "Open design slot."
            }
          ]
        },
        {
          id: "kinwild_beast_den",
          label: "Beast Den",
          role_summary: "First real Kinwild tech path: fast hunter plus companion pressure.",
          entries: [
            {
              id: "kinwild_spear_hunter",
              type: "unit",
              name: "Spear Hunter",
              role: "Melee shock hunter",
              gate: "Beast Den",
              cost: "160",
              stats: "130 HP target, 1.10 speed target",
              mechanics: "Bound-leap, spear pressure, flank and tech-building punishment.",
              team_use: "First real Kinwild body row.",
              asset_ids: ["fab_bf70b6c1", "paragon_khaimera", "infinity_blade_adversaries"],
              asset_status: "Selected: Lizard Kin Warrior, raw source present/import pending.",
              visual_status: "Local source preview available; final proof needs UE import and in-project render."
            },
            {
              id: "kinwild_pack_companion",
              type: "unit",
              name: "Pack Companion",
              role: "Scout / chase companion",
              gate: "Beast Den",
              cost: "80",
              stats: "70 HP target, 1.25 speed target",
              mechanics: "Scout, reveal, chase, bite-assist direction.",
              team_use: "Cheap harassment and pack silhouette.",
              asset_ids: ["fab_c5747f8f", "infinity_blade_adversaries"],
              asset_status: "Selected: Low Poly wolf, raw source present/import pending.",
              visual_status: "Local cropped preview available; final proof needs UE import and in-project render."
            },
            {
              id: "kinwild_pack_pair",
              type: "squad",
              name: "Pack Pair",
              role: "Flank squad",
              gate: "Beast Den",
              cost: "tbd",
              stats: "2 Spear Hunter, 2 Pack Companion target",
              mechanics: "Fast flank, scout, chase, and tech-building harassment package.",
              team_use: "First Kinwild RTS package proposal.",
              asset_ids: ["fab_bf70b6c1", "fab_c5747f8f"],
              asset_status: "Ledger package; implementation pending."
            }
          ]
        },
        {
          id: "kinwild_support",
          label: "Support / Hero Open",
          role_summary: "Pack aura and hunt-call slot needed to finish team identity.",
          entries: [
            {
              id: "kinwild_packcaller_tbd",
              type: "hero",
              name: "Pack Caller",
              role: "Aura support / hero",
              gate: "Beast Den or high supply",
              cost: "tbd",
              stats: "300 HP target, 1.05 speed target",
              mechanics: "Hunt horn, scent reveal, pack aura, beast-call direction.",
              team_use: "Gives Kinwild support identity beyond melee pressure.",
              asset_ids: ["paragon_narbash", "paragon_khaimera", "paragon_grux", "paragon_rampage"],
              asset_status: "Open design slot; Narbash is current best approved support candidate."
            }
          ]
        }
      ]
    },
    {
      id: "LenswrightCompact",
      label: "Lenswright Compact",
      identity: "Pre-gunpowder science: pressure crossbows, clockwork, optics, field works, alchemy, siege tools.",
      movement: "LenswrightPressureThrust",
      tech_paths: [
        {
          id: "lenswright_base",
          label: "Base / Line",
          role_summary: "Shield line and disciplined bodyguard identity.",
          entries: [
            {
              id: "lenswright_gearguard",
              type: "unit",
              name: "Gearguard",
              role: "Frontline shield",
              gate: "free / base",
              cost: "low supply target",
              stats: "140 HP, 0.95 speed",
              mechanics: "Shield, impact baton, pressure punch.",
              team_use: "Bodyguard screen for ranged units and field works.",
              asset_ids: ["paragon_greystone", "paragon_terra", "infinity_blade_warriors"],
              asset_status: "Unselected; Greystone/Terra are current best approved candidates."
            }
          ]
        },
        {
          id: "lenswright_armory",
          label: "Armory",
          role_summary: "Pressure-bolt weapons, scouting, and first purchasable Lenswright body.",
          entries: [
            {
              id: "pressure_bolt_crossbow",
              type: "item",
              name: "Pressure-Bolt Crossbow",
              role: "Ranged weapon upgrade",
              gate: "Armory",
              cost: "100",
              stats: "1 quiver, 1.5s fire, 2.2s reload, 6000 speed, 6000 range, 40/90/25 damage",
              mechanics: "Single pressure bolt, crank reload, no gunpowder.",
              team_use: "Armory weapon payoff for disciplined ranged play.",
              asset_ids: ["infinity_blade_weapons"],
              asset_status: "Current C++ weapon works; mesh upgrade pool pending.",
              visual_status: "No actual crossbow mesh thumbnail yet. Infinity Blade Weapons is approved but not local; current C++ weapon is behavior-only."
            },
            {
              id: "lenswright_pressure_runner",
              type: "unit",
              name: "Pressure Runner",
              role: "Skirmisher / scout",
              gate: "Armory",
              cost: "120",
              stats: "100 HP target, 1.15 speed target",
              mechanics: "Prototype Lenswright shop body; future pressure-thrust expression.",
              team_use: "Fast Armory body row.",
              asset_ids: ["fab_15e2afd1", "paragon_kwang", "infinity_blade_warriors"],
              asset_status: "Selected: Rogue Character Model, UE imported.",
              visual_status: "Generated local preview available; needs clean in-engine render for final visual proof."
            },
            {
              id: "lenswright_bracewright",
              type: "unit",
              name: "Bracewright",
              role: "Defensive ranged",
              gate: "Armory",
              cost: "100 weapon row / unit cost tbd",
              stats: "80 HP, 360 walk speed",
              mechanics: "Defensive ranged AI, pressure-bolt crossbow, cover behavior.",
              team_use: "Holds lanes with pressure-bolt fire.",
              asset_ids: ["fab_15e2afd1", "paragon_greystone", "paragon_terra"],
              asset_status: "C++ debug actor now; Rogue selected for first visual swap.",
              visual_status: "Generated Rogue preview is a temporary body reference; final Bracewright needs in-engine render with weapon/tool silhouette."
            },
            {
              id: "lenswright_sundial_optic",
              type: "unit",
              name: "Sundial Optic",
              role: "Scout / vision",
              gate: "Armory",
              cost: "tbd",
              stats: "60 HP, 420 walk speed, 4500 vision",
              mechanics: "ScoutNoWeapon, long-vision optic.",
              team_use: "Map-table information advantage and approach warning.",
              asset_ids: ["fab_15e2afd1", "infinity_blade_warriors"],
              asset_status: "Debug cylinder; body/optic prop not selected."
            },
            {
              id: "lenswright_lens_arbalist",
              type: "unit",
              name: "Lens Arbalist",
              role: "Ranged damage",
              gate: "Armory",
              cost: "medium supply target",
              stats: "85 HP, 1.0 speed",
              mechanics: "Lens-guided bolt and focus shot.",
              team_use: "Damage counterpart if kept separate from Bracewright.",
              asset_ids: ["fab_15e2afd1", "paragon_kwang", "infinity_blade_weapons"],
              asset_status: "Open decision: keep separate or fold into Bracewright."
            },
            {
              id: "lenswright_fieldwright",
              type: "unit",
              name: "Fieldwright",
              role: "Engineer support",
              gate: "Armory",
              cost: "medium-high supply target",
              stats: "100 HP, 1.0 speed",
              mechanics: "Repair tool, deploy pressure node, field-work support.",
              team_use: "Keeps Lenswright structures and siege tools online.",
              asset_ids: ["fab_15e2afd1", "infinity_blade_warriors"],
              asset_status: "Needs tool/pack silhouette, not generic body only."
            }
          ]
        },
        {
          id: "lenswright_hero",
          label: "High Supply / Hero",
          role_summary: "Siege/control hero with no-gunpowder audit.",
          entries: [
            {
              id: "hero_master_artificer",
              type: "hero",
              name: "Master Artificer",
              role: "Device artillery hero",
              gate: "high supply unlock",
              cost: "high unlock",
              stats: "280 HP retained pending reconciliation",
              mechanics: "PressureGate, OpticBarrage, repair/device lineage.",
              team_use: "Late siege/control anchor for field works.",
              asset_ids: ["paragon_greystone", "paragon_terra", "infinity_blade_warriors", "infinity_blade_effects"],
              asset_status: "Unselected; needs heavy artificer silhouette and no-gunpowder VFX audit."
            }
          ]
        }
      ]
    }
  ],
  open_decisions: [
    "Decide whether Lens Arbalist remains separate or folds into Bracewright.",
    "Replace or retire Verdant Temporary Runner before its AI-generated placeholder becomes faction identity.",
    "Author Kinwild Pack Caller support/hero spec.",
    "Make reinforcement training faction-aware instead of always targeting verdant_reinforcement_squad."
  ]
};
