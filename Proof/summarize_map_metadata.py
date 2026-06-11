import argparse
import json
import re
import subprocess
from collections import Counter, deque
from datetime import datetime, timezone
from pathlib import Path


BUILD_RE = re.compile(r"ArchonFactoryCanary: BuildFingerprint moduleDllUtc=(\S+)")
PHASE_RE = re.compile(r"ArchonFactoryCanary: MatchPhase phase=(\w+)")
MATCH_END_RE = re.compile(
    r"ArchonFactoryCanary: MatchEnd winner=(\w+) reason=(\w+) "
    r"pointsA=(\d+) pointsB=(\d+) liveSeconds=([0-9.]+) sitesA=(\d+) sitesB=(\d+)"
)
TRAVEL_RE = re.compile(r"ArchonFactoryCanary: TravelingTo map=(\S+) url=(.+)$")
SPAWN_RE = re.compile(
    r"ArchonFactoryCanary: SplitrootValleySpawned placements=(\d+) blocks=(\d+) "
    r"stones=(\d+) bases=(\d+) cores=(\d+) cosmeticOnly=(\w+)"
)
MAP_PASS_RE = re.compile(
    r"ArchonFactoryCanary: ValleyMapPass id=(\S+) routeClearance=(\d+) hotZoneClearance=(\d+) "
    r"decorTrees=(\d+) decorRocks=(\d+) decorFoliage=(\d+) resourceCollision=(\w+)"
)
FAB_RE = re.compile(
    r"ArchonFactoryCanary: ValleyFabVisual piece=(?P<piece>\w+) id=(?P<id>\S+)"
    r"(?: owner=(?P<owner>\S+))? asset=(?P<asset>\S+)"
)
FAB_COLLISION_RE = re.compile(r"ArchonFactoryCanary: ValleyFabCollision piece=(\w+) id=(\S+) mode=(\w+)")
DECOR_RE = re.compile(r"ArchonFactoryCanary: ValleyDecorScale id=(\S+) mesh=(\S+)")
SITE_RE = re.compile(r"ArchonFactoryCanary: SiteCaptured site=(\S+) team=(\d+)")
SUPPLY_RE = re.compile(r"ArchonFactoryCanary: SupplyTick team=(\d+) sites=(\d+) gained=(\d+) total=(\d+)")
STUCK_RE = re.compile(
    r"ArchonFactoryCanary: BotStuckRecovery bot=(\d+) team=(\d+) attempt=(\d+) "
    r"target=\((-?[0-9.]+),(-?[0-9.]+),(-?[0-9.]+)\)"
)
MOVEMENT_STUCK_RE = re.compile(
    r"LogCharacterMovement: (?P<pawn>\S+) is stuck.*?Location: "
    r"X=(?P<x>-?[0-9.]+) Y=(?P<y>-?[0-9.]+) Z=(?P<z>-?[0-9.]+).*?"
    r"Actor:(?P<actor>\S+) Component:(?P<component>\S+)"
)
WAYPOINT_RE = re.compile(r"ArchonFactoryCanary: BotWaypointReached bot=(\d+) team=(\d+)")


def iso_utc_from_timestamp(timestamp: float) -> str:
    return datetime.fromtimestamp(timestamp, timezone.utc).isoformat().replace("+00:00", "Z")


def parse_utc(value: str):
    if not value:
        return None
    try:
        return datetime.fromisoformat(value.replace("Z", "+00:00")).astimezone(timezone.utc)
    except ValueError:
        return None


def top_counts(counter: Counter, limit: int):
    return [
        {"key": key, "count": count}
        for key, count in sorted(counter.items(), key=lambda item: (-item[1], item[0]))[:limit]
    ]


def zone_key(x_value: float, y_value: float, cell_size: float = 1000.0) -> str:
    zone_x = round(x_value / cell_size) * cell_size
    zone_y = round(y_value / cell_size) * cell_size
    return f"x={int(zone_x)};y={int(zone_y)}"


def new_stats(path: str, kind: str) -> dict:
    file_path = Path(path)
    exists = file_path.is_file()
    return {
        "path": str(file_path) if kind != "aggregate" else path,
        "kind": kind,
        "exists": exists,
        "lastWriteTimeUtc": iso_utc_from_timestamp(file_path.stat().st_mtime) if exists else None,
        "sampledLineCount": 0,
        "matchPhaseCounts": Counter(),
        "matchEnds": [],
        "travelCount": 0,
        "latestTravel": None,
        "latestBuildFingerprint": None,
        "latestModuleDllUtc": None,
        "splitrootSpawnCount": 0,
        "latestSplitrootSpawn": None,
        "mapPassCount": 0,
        "latestMapPass": None,
        "fabVisualCount": 0,
        "fabVisualByPiece": Counter(),
        "fabAssets": Counter(),
        "fabCollisionCount": 0,
        "fabCollisionByPiece": Counter(),
        "fabCollisionModes": Counter(),
        "decorScaleCount": 0,
        "decorAssets": Counter(),
        "siteCaptureCount": 0,
        "siteCaptures": Counter(),
        "supplyTicks": {},
        "botStuckRecoveryCount": 0,
        "botStuckByTeam": Counter(),
        "botStuckZones": Counter(),
        "movementStuckCount": 0,
        "movementStuckActors": Counter(),
        "movementStuckComponents": Counter(),
        "movementStuckZones": Counter(),
        "latestMovementStuck": None,
        "characterStuckCount": 0,
        "characterStuckActors": Counter(),
        "characterStuckComponents": Counter(),
        "characterStuckZones": Counter(),
        "latestCharacterStuck": None,
        "botWaypointReachedCount": 0,
        "botWaypointReachedByTeam": Counter(),
        "weaponFiredCount": 0,
        "respawnCount": 0,
    }


def compact_stats(stats: dict) -> dict:
    return {
        "path": stats["path"],
        "kind": stats["kind"],
        "exists": stats["exists"],
        "lastWriteTimeUtc": stats["lastWriteTimeUtc"],
        "sampledLineCount": stats["sampledLineCount"],
        "matchPhaseCounts": dict(stats["matchPhaseCounts"]),
        "matchEndCount": len(stats["matchEnds"]),
        "latestMatchEnd": stats["matchEnds"][-1] if stats["matchEnds"] else None,
        "travelCount": stats["travelCount"],
        "latestTravel": stats["latestTravel"],
        "latestBuildFingerprint": stats["latestBuildFingerprint"],
        "splitrootSpawnCount": stats["splitrootSpawnCount"],
        "latestSplitrootSpawn": stats["latestSplitrootSpawn"],
        "mapPassCount": stats["mapPassCount"],
        "latestMapPass": stats["latestMapPass"],
        "fabVisualCount": stats["fabVisualCount"],
        "fabVisualByPiece": dict(stats["fabVisualByPiece"]),
        "topFabAssets": top_counts(stats["fabAssets"], 8),
        "fabCollisionCount": stats["fabCollisionCount"],
        "fabCollisionByPiece": dict(stats["fabCollisionByPiece"]),
        "topFabCollisionModes": top_counts(stats["fabCollisionModes"], 8),
        "decorScaleCount": stats["decorScaleCount"],
        "topDecorAssets": top_counts(stats["decorAssets"], 8),
        "siteCaptureCount": stats["siteCaptureCount"],
        "topSiteCaptures": top_counts(stats["siteCaptures"], 8),
        "latestSupplyTicks": stats["supplyTicks"],
        "botStuckRecoveryCount": stats["botStuckRecoveryCount"],
        "botStuckByTeam": dict(stats["botStuckByTeam"]),
        "topBotStuckZones": top_counts(stats["botStuckZones"], 8),
        "movementStuckCount": stats["movementStuckCount"],
        "topMovementStuckActors": top_counts(stats["movementStuckActors"], 8),
        "topMovementStuckComponents": top_counts(stats["movementStuckComponents"], 8),
        "topMovementStuckZones": top_counts(stats["movementStuckZones"], 8),
        "latestMovementStuck": stats["latestMovementStuck"],
        "characterStuckCount": stats["characterStuckCount"],
        "topCharacterStuckActors": top_counts(stats["characterStuckActors"], 8),
        "topCharacterStuckComponents": top_counts(stats["characterStuckComponents"], 8),
        "topCharacterStuckZones": top_counts(stats["characterStuckZones"], 8),
        "latestCharacterStuck": stats["latestCharacterStuck"],
        "botWaypointReachedCount": stats["botWaypointReachedCount"],
        "botWaypointReachedByTeam": dict(stats["botWaypointReachedByTeam"]),
        "weaponFiredCount": stats["weaponFiredCount"],
        "respawnCount": stats["respawnCount"],
    }


def iter_tail(path: Path, max_lines: int):
    if max_lines <= 0:
        with path.open("r", encoding="utf-8", errors="replace") as handle:
            yield from handle
        return

    lines = deque(maxlen=max_lines)
    with path.open("r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            lines.append(line)
    yield from lines


def apply_line(stats: dict, aggregate: dict, line: str, source: str):
    for target in (stats, aggregate):
        target["sampledLineCount"] += 1

    if match := BUILD_RE.search(line):
        for target in (stats, aggregate):
            target["latestBuildFingerprint"] = line.strip()
            target["latestModuleDllUtc"] = match.group(1)

    if match := PHASE_RE.search(line):
        for target in (stats, aggregate):
            target["matchPhaseCounts"][match.group(1)] += 1

    if match := MATCH_END_RE.search(line):
        entry = {
            "winner": match.group(1),
            "reason": match.group(2),
            "pointsA": int(match.group(3)),
            "pointsB": int(match.group(4)),
            "liveSeconds": float(match.group(5)),
            "sitesA": int(match.group(6)),
            "sitesB": int(match.group(7)),
            "source": source,
        }
        stats["matchEnds"].append(entry)
        aggregate["matchEnds"].append(entry)

    if match := TRAVEL_RE.search(line):
        travel = {"map": match.group(1), "url": match.group(2)}
        for target in (stats, aggregate):
            target["travelCount"] += 1
            target["latestTravel"] = travel

    if match := SPAWN_RE.search(line):
        spawn = {
            "placements": int(match.group(1)),
            "blocks": int(match.group(2)),
            "stones": int(match.group(3)),
            "bases": int(match.group(4)),
            "cores": int(match.group(5)),
            "cosmeticOnly": match.group(6),
        }
        for target in (stats, aggregate):
            target["splitrootSpawnCount"] += 1
            target["latestSplitrootSpawn"] = spawn

    if match := MAP_PASS_RE.search(line):
        map_pass = {
            "id": match.group(1),
            "routeClearance": int(match.group(2)),
            "hotZoneClearance": int(match.group(3)),
            "decorTrees": int(match.group(4)),
            "decorRocks": int(match.group(5)),
            "decorFoliage": int(match.group(6)),
            "resourceCollision": match.group(7),
        }
        for target in (stats, aggregate):
            target["mapPassCount"] += 1
            target["latestMapPass"] = map_pass

    if match := FAB_RE.search(line):
        for target in (stats, aggregate):
            target["fabVisualCount"] += 1
            target["fabVisualByPiece"][match.group("piece")] += 1
            target["fabAssets"][match.group("asset")] += 1

    if match := FAB_COLLISION_RE.search(line):
        mode_key = f"{match.group(1)}:{match.group(3)}"
        for target in (stats, aggregate):
            target["fabCollisionCount"] += 1
            target["fabCollisionByPiece"][match.group(1)] += 1
            target["fabCollisionModes"][mode_key] += 1

    if match := DECOR_RE.search(line):
        for target in (stats, aggregate):
            target["decorScaleCount"] += 1
            target["decorAssets"][match.group(2)] += 1

    if match := SITE_RE.search(line):
        key = f"{match.group(1)}:team{match.group(2)}"
        for target in (stats, aggregate):
            target["siteCaptureCount"] += 1
            target["siteCaptures"][key] += 1

    if match := SUPPLY_RE.search(line):
        team_key = f"team{match.group(1)}"
        tick = {
            "team": int(match.group(1)),
            "sites": int(match.group(2)),
            "gained": int(match.group(3)),
            "total": int(match.group(4)),
        }
        for target in (stats, aggregate):
            target["supplyTicks"][team_key] = tick

    if match := STUCK_RE.search(line):
        team_key = f"team{match.group(2)}"
        zone = zone_key(float(match.group(4)), float(match.group(5)))
        for target in (stats, aggregate):
            target["botStuckRecoveryCount"] += 1
            target["botStuckByTeam"][team_key] += 1
            target["botStuckZones"][zone] += 1

    if match := MOVEMENT_STUCK_RE.search(line):
        zone = zone_key(float(match.group("x")), float(match.group("y")))
        entry = {
            "pawn": match.group("pawn"),
            "actor": match.group("actor"),
            "component": match.group("component"),
            "x": float(match.group("x")),
            "y": float(match.group("y")),
            "z": float(match.group("z")),
            "zone": zone,
            "source": source,
        }
        for target in (stats, aggregate):
            target["movementStuckCount"] += 1
            target["movementStuckActors"][match.group("actor")] += 1
            target["movementStuckComponents"][match.group("component")] += 1
            target["movementStuckZones"][zone] += 1
            target["latestMovementStuck"] = entry
            if match.group("pawn").startswith("ArchonCanaryFpsCharacter"):
                target["characterStuckCount"] += 1
                target["characterStuckActors"][match.group("actor")] += 1
                target["characterStuckComponents"][match.group("component")] += 1
                target["characterStuckZones"][zone] += 1
                target["latestCharacterStuck"] = entry

    if match := WAYPOINT_RE.search(line):
        team_key = f"team{match.group(2)}"
        for target in (stats, aggregate):
            target["botWaypointReachedCount"] += 1
            target["botWaypointReachedByTeam"][team_key] += 1

    if "ArchonFactoryCanary: WeaponFired " in line:
        for target in (stats, aggregate):
            target["weaponFiredCount"] += 1

    if "ArchonFactoryCanary: BotRespawned " in line:
        for target in (stats, aggregate):
            target["respawnCount"] += 1


def active_unreal_processes(project_root: Path):
    command = (
        "Get-CimInstance Win32_Process -Filter \"Name = 'UnrealEditor.exe'\" | "
        "Where-Object { $_.CommandLine -match 'ArchonFactoryCanary\\.uproject' } | "
        "Select-Object ProcessId,CommandLine | ConvertTo-Json -Depth 3"
    )
    try:
        completed = subprocess.run(
            ["powershell", "-NoProfile", "-Command", command],
            cwd=str(project_root),
            text=True,
            capture_output=True,
            timeout=3,
            check=False,
        )
    except Exception:
        return []
    text = completed.stdout.strip()
    if not text:
        return []
    try:
        parsed = json.loads(text)
    except json.JSONDecodeError:
        return []
    if isinstance(parsed, dict):
        parsed = [parsed]
    return parsed


def load_map_workbench(proof_dir: Path) -> dict:
    workbench_dir = proof_dir / "MapWorkbench"
    variants = []
    if workbench_dir.is_dir():
        for json_path in sorted(workbench_dir.glob("splitroot-valley-*.json")):
            try:
                data = json.loads(json_path.read_text(encoding="utf-8"))
            except (OSError, json.JSONDecodeError):
                continue

            score = data.get("score", {})
            variant = data.get("variant", json_path.stem.replace("splitroot-valley-", ""))
            png_path = workbench_dir / f"splitroot-valley-{variant}.png"
            variants.append(
                {
                    "variant": variant,
                    "jsonPath": str(json_path),
                    "pngPath": str(png_path),
                    "pngExists": png_path.is_file(),
                    "lastWriteTimeUtc": iso_utc_from_timestamp(json_path.stat().st_mtime),
                    "layoutCount": data.get("layoutCount"),
                    "score": score.get("score"),
                    "routeViolationCount": len(score.get("routeViolations", [])),
                    "routeHotZoneViolationCount": len(score.get("routeHotZoneViolations", [])),
                    "hotZoneViolationCount": len(score.get("hotZoneViolations", [])),
                    "lensBaseDistanceToNearestObservedHotZone": score.get(
                        "lensBaseDistanceToNearestObservedHotZone"
                    ),
                    "counts": score.get("counts", {}),
                    "read": score.get("read", []),
                }
            )

    variants.sort(key=lambda item: item["lastWriteTimeUtc"] or "", reverse=True)
    best = None
    if variants:
        best = max(
            variants,
            key=lambda item: (
                item["score"] if isinstance(item.get("score"), (int, float)) else -1,
                item["lastWriteTimeUtc"] or "",
            ),
        )
    return {
        "directory": str(workbench_dir),
        "latest": variants[0] if variants else None,
        "best": best,
        "variants": variants,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-root", default=str(Path(__file__).resolve().parents[1]))
    parser.add_argument("--recent-archived-logs", type=int, default=5)
    parser.add_argument("--max-lines-per-log", type=int, default=60000)
    parser.add_argument("--output-path", default="")
    args = parser.parse_args()

    project_root = Path(args.project_root).resolve()
    proof_dir = project_root / "Saved" / "Proof"
    live_log = project_root / "Saved" / "Logs" / "ArchonFactoryCanary.log"
    playtest_drive_log = proof_dir / "playtest-drive.log"
    match_logs_dir = proof_dir / "MatchLogs"
    map_workbench = load_map_workbench(proof_dir)
    module_dll = project_root / "Binaries" / "Win64" / "UnrealEditor-ArchonFactoryCanary.dll"
    output_path = Path(args.output_path) if args.output_path else proof_dir / "last-map-metadata-summary.json"

    log_inputs = []
    if live_log.is_file():
        log_inputs.append({"path": str(live_log), "kind": "live"})
    if playtest_drive_log.is_file():
        log_inputs.append({"path": str(playtest_drive_log), "kind": "playtest_drive"})
    if match_logs_dir.is_dir() and args.recent_archived_logs > 0:
        archived = sorted(match_logs_dir.glob("*.log"), key=lambda path: path.stat().st_mtime, reverse=True)
        for log_path in archived[: args.recent_archived_logs]:
            log_inputs.append({"path": str(log_path), "kind": "archived"})

    aggregate = new_stats("<aggregate>", "aggregate")
    summaries = []
    for input_item in log_inputs:
        stats = new_stats(input_item["path"], input_item["kind"])
        path = Path(input_item["path"])
        if stats["exists"]:
            for line in iter_tail(path, args.max_lines_per_log):
                apply_line(stats, aggregate, line, str(path))
        summaries.append(compact_stats(stats))

    active_dll_utc = None
    if module_dll.is_file():
        active_dll_utc = datetime.fromtimestamp(module_dll.stat().st_mtime, timezone.utc)
    latest_observed_build_utc = parse_utc(aggregate["latestModuleDllUtc"])
    live_build_needs_refresh = bool(
        active_dll_utc and latest_observed_build_utc and active_dll_utc > latest_observed_build_utc
    )

    map_read = []
    if aggregate["characterStuckCount"] > 0:
        top_character_actors = top_counts(aggregate["characterStuckActors"], 8)
        top_actor = next(
            (
                item["key"]
                for item in top_character_actors
                if not item["key"].startswith("ArchonCanaryFpsCharacter")
                and not item["key"].startswith("ArchonArrowProjectile")
            ),
            top_character_actors[0]["key"],
        )
        map_read.append(
            f"Character movement stuck events found; top external blocker is {top_actor}."
        )
    elif aggregate["movementStuckCount"] > 0:
        top_actor = top_counts(aggregate["movementStuckActors"], 1)[0]["key"]
        map_read.append(f"Movement stuck events found; top moving-actor blocker is {top_actor}.")
    if aggregate["botStuckRecoveryCount"] > 0:
        map_read.append("BotStuckRecovery is active; inspect top stuck zones before adding more blockers near those routes.")
    if aggregate["siteCaptureCount"] == 0:
        map_read.append("No site captures found in sampled logs; objective pull may be too weak or route access may be blocked.")
    if aggregate["decorScaleCount"] == 0:
        map_read.append("No ValleyDecorScale lines yet; current live session has not adopted the latest map metadata logging.")
    if aggregate["mapPassCount"] == 0:
        map_read.append("No ValleyMapPass marker yet; current live session has not adopted the latest map-pass logging.")
    if aggregate["fabCollisionCount"] == 0:
        map_read.append("No ValleyFabCollision lines yet; current live session has not adopted the latest collision-mode logging.")
    if live_build_needs_refresh:
        map_read.append("Current DLL is newer than the latest observed live build; wait for the host-safe between-match refresh before judging new map changes.")
    latest_workbench = map_workbench["latest"]
    if latest_workbench:
        if latest_workbench["routeHotZoneViolationCount"] > 0:
            map_read.append(
                f"Latest workbench variant {latest_workbench['variant']} still crosses replay hot zones; do not port it."
            )
        elif latest_workbench["routeViolationCount"] > 0 or latest_workbench["hotZoneViolationCount"] > 0:
            map_read.append(
                f"Latest workbench variant {latest_workbench['variant']} has placement violations; inspect its JSON before porting."
            )
        else:
            map_read.append(
                f"Latest workbench variant {latest_workbench['variant']} score={latest_workbench['score']} has clean route/decor hot-zone checks."
            )

    result = {
        "projectRoot": str(project_root),
        "snapshotUtc": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "inputs": log_inputs,
        "outputPath": str(output_path),
        "activeUnrealProcesses": active_unreal_processes(project_root),
        "moduleDllLastWriteUtc": active_dll_utc.isoformat().replace("+00:00", "Z") if active_dll_utc else None,
        "latestObservedBuildUtc": latest_observed_build_utc.isoformat().replace("+00:00", "Z") if latest_observed_build_utc else None,
        "liveBuildNeedsHostSafeRefresh": live_build_needs_refresh,
        "aggregate": compact_stats(aggregate),
        "logs": summaries,
        "mapWorkbench": map_workbench,
        "mapRead": map_read,
    }

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
