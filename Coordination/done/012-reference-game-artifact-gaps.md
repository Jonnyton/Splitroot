# 012 - Reference-game artifact gaps: Renegade covered, Savage 2 missing

Goal: make the C&C Renegade / Savage 2 gameplay-mechanics references durable enough that future SPLITROOT implementation work can cite them without guessing.

Context:
- Gauge audited the connector + local repo on 2026-06-11.
- C&C Renegade is already well represented:
  - `drafts/research/splitroot-renegade-match-flow-research-2026-06-09.md`
  - `pages/research/pages-research-splitroot-rts-fps-hybrid-lineage-research-2026-05-23.md`
  - local code comments across match flow, renown, base cores, base defense towers, map rotation, bot tower targeting, and item shop policy.
- Savage is not equally covered:
  - The promoted lineage page covers `Savage: The Battle for Newerth`.
  - The local item-shop code cites "Savage 2 tech-gating lineage".
  - `drafts/plans/splitroot-item-shop-tech-building-direction-2026-06-11.md` says subagent research confirmed Savage 2 mechanics.
  - Gauge did NOT find a dedicated `Savage 2: A Tortured Soul` mechanics research artifact.

Steps:
1. Re-check the connector first:
   - `wiki search` for `Savage 2`, `A Tortured Soul`, `tech gating`, `officer`, `item shop`, and `commander-built tech`.
   - If Gauge missed an existing dedicated Savage 2 page, do not duplicate it; patch/link the existing one.
2. If no dedicated page exists, write a source-backed connector draft under `drafts/research/`, suggested filename:
   - `splitroot-savage-2-mechanics-research-2026-06-11.md`
3. The Savage 2 page should extract only gameplay-mechanics implications:
   - commander-built tech/buildings and how they unlock player-facing kits/items;
   - officer/forward-spawn mechanics if confirmed by sources;
   - economy/resource loops and what players personally buy vs what commander/team spends;
   - melee/ranged/combat feel lessons worth stealing or rejecting;
   - what SPLITROOT should steal, reject, or modify;
   - exact implications for `UArchonItemShopPolicyLibrary`, tech-building actors, map-table build verbs, shop UI, officer portal, beacon/endgame, bots, and tests.
4. Patch `drafts/plans/splitroot-item-shop-tech-building-direction-2026-06-11.md` to link the Savage 2 research page once it exists.
5. Optional if time: promote or request review/promotion for the Renegade draft; do not block the Savage 2 gap on that.

Done-when:
- There is a dedicated, source-backed connector artifact for `Savage 2: A Tortured Soul` mechanics OR an existing one is identified and linked.
- `splitroot-item-shop-tech-building-direction-2026-06-11.md` links to the Savage 2 artifact instead of relying on an implicit "subagent research" claim.
- Any local source comments that cite the Savage 2 lineage still point to a discoverable wiki page.
- No local gameplay success is claimed unless a proof script actually ran.

Report-to:
- Move this file to `Coordination/done/` with `## Result`.
- Include the connector page path(s), whether a new page was written or an existing one was linked, and any source/proof limits.

## Result

Completed by Hex on 2026-06-11.

- Re-checked connector search for `Savage 2`, `A Tortured Soul`, and commander/tech/officer/shop terms. I did not find an existing dedicated `Savage 2: A Tortured Soul` mechanics research page; the relevant existing artifact was the item-shop direction draft that referenced Savage 2 implicitly.
- Wrote new connector draft: `drafts/research/splitroot-savage-2-mechanics-research-2026-06-11.md`.
- Patched existing connector draft: `drafts/plans/splitroot-item-shop-tech-building-direction-2026-06-11.md`.
  - Old plan SHA: `b24fae55ca8dc6be65b11f00569898a12f247b9c02d63480262da642570d8755`
  - New plan SHA: `c5f546032aa3f1c9fac816f36afe2685a95e3a948084555a4324a6f7c5333154`
- Source limits: no official manual was found in the quick pass. The draft is bounded to public secondary/player-authored sources: Wikipedia gameplay synthesis, Shacknews preview, GameFAQs player mini-guide, and GameFAQs user review. The draft marks GameFAQs as corroborating player-authored evidence.
- Proof limits: no local gameplay success is claimed for this research task. Connector read-back verified the new draft exists and the item-shop plan links `[[splitroot-savage-2-mechanics-research-2026-06-11]]`.
