# Archon Adapter Contract

This contract is the reusable local handoff from the tinyassets workflow branch to an Unreal implementation.

## Scope

The canary game is a fantasy Archon RTS/FPS hybrid. Players fight in first person as regular units or available heroes. Base map tables expose RTS mode. Any teammate at a valid map table can input normal RTS commands into the shared team state.

## Non-Goals

- No command tokens.
- No commander elections.
- No voting system.
- No soft locks.
- No anti-grief governance layer.
- No claim that this local shell is a completed game, packaged build, Steam build, AAA product, or sell-ready release.

## Reusable Modules

- `ArchonSessionRoute`: route classification and debug visibility.
- `ArchonEntitlementPolicy`: route-first access checks.
- `ArchonMapTable`: inspect, select, preview, and execute/no-op command surface.
- `ArchonTeamRtsState`: authoritative team command state.
- `ArchonPossession`: first-person possession, death, respawn, regular-unit choice, and hero choice.
- `ArchonHeroCatalog`: default heroes, horizontal paid heroes, cooldowns, and availability.
- `ArchonProofHarness`: local evidence collection and forbidden-claim checks.

## Required Future Proof

The project cannot claim `local_playable_prototype` until there is evidence for:

- Unreal Editor UI opens the project.
- A playable map loads outside headless smoke mode.
- First-person movement works.
- Death and respawn can be demonstrated.
- A map table can be entered.
- Map table preview works.
- Shared RTS state mutates only after authorized execution.
- SteamOnline free map-table execution is preview/no-op.
- OfflineSkirmish, LANHosted, and PrivateHost grant full free gameplay.
- `C:\Users\Jonathan\Desktop\SPLITROOT Current Build.lnk` targets
  `Launchers/Play-CurrentBuild.cmd`, and that launcher starts the
  current merged playable build (editor canary today, packaged build
  once packaging becomes the current surface).
