import json
import os
import re
import sys
import traceback

import unreal


PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
RESULT_PATH = os.path.join(PROJECT_ROOT, "Saved", "Proof", "last-fab-library-import.json")
SELECTED_KEY = os.environ.get("FAB_LIBRARY_SELECTED_KEY", "").strip()
INVENTORY_ONLY = os.environ.get("FAB_LIBRARY_INVENTORY_ONLY", "").strip() == "1"

MESH_EXTENSIONS = {".fbx", ".glb", ".gltf", ".obj", ".usd", ".usda", ".usdc"}

KNOWN_READY_FOLDERS = {
    "Fab_52529a12_FANTASTIC_Village_Pack": {
        "destination": "/Game/Fab/FANTASTIC_Village_Pack",
        "note": "Curated subset is already imported; full pack is low-poly/stylized and not the target look.",
    },
    "Fab_8a517c31_3_English_Oak": {
        "destination": "/Game/Fab/English_Oak",
        "note": "Already imported by priority Fab conversion wave.",
    },
    "Fab_8b804edb_Large_Medieval_Building": {
        "destination": "/Game/Fab/Large_Medieval_Building",
        "note": "Already imported by priority Fab conversion wave; photogrammetry-heavy.",
    },
    "Fab_936eefbf_Castle_of_Rocca_Calascio": {
        "destination": "/Game/Fab/Castle_Rocca_Calascio",
        "note": "Already imported by priority Fab conversion wave.",
    },
    "Fab_f065c0cd_Fantasy_Game_Character": {
        "destination": "/Game/Fab/Fantasy_Game_Character",
        "note": "Already imported as a skeletal mesh; not wired into map dressing.",
    },
    "Fab_729cfcec_Forest_ground_soil_pine_free": {
        "destination": "/Game/Fab/Forest_Ground_Soil_Pine",
        "note": "Already imported by priority Fab conversion wave; high triangle ground mesh.",
    },
    "Fab_705ac651_FREE_Windmill_Village_Buildings_Pack": {
        "destination": "/Game/Fab/Free_Windmill_Village",
        "note": "Already imported, but source resolved to a simple plane.",
    },
    "Fab_f255de9f_free_fantasy_architecture_w2r_16": {
        "destination": "/Game/Fab/Fantasy_Architecture_W2R16",
        "note": "Already imported, but source resolved to a simple plane.",
    },
}

LOW_PRIORITY_STYLE_MARKERS = [
    "low_poly",
    "low-poly",
    "stylized",
    "cartoon",
]

SKELETAL_MARKERS = [
    "assassin",
    "character",
    "kin",
    "lizard",
    "rogue",
    "wolf",
]


def p(*parts):
    return os.path.join(PROJECT_ROOT, *parts)


def write_result(result):
    os.makedirs(os.path.dirname(RESULT_PATH), exist_ok=True)
    with open(RESULT_PATH, "w", encoding="utf-8") as handle:
        json.dump(result, handle, indent=2, sort_keys=True)


def sanitize_name(value):
    value = re.sub(r"[^A-Za-z0-9_]+", "_", value)
    value = re.sub(r"_+", "_", value).strip("_")
    return value or "asset"


def label_from_folder(folder_name):
    match = re.match(r"Fab_[0-9a-fA-F]+_(.+)$", folder_name)
    if match:
        return match.group(1).replace("_", " ")
    return folder_name.replace("_", " ")


def slug_from_folder(folder_name, mesh_files):
    match = re.match(r"Fab_[0-9a-fA-F]+_(.+)$", folder_name)
    if match:
        return sanitize_name(match.group(1))
    if mesh_files:
        return sanitize_name(os.path.splitext(os.path.basename(mesh_files[0]))[0])
    return sanitize_name(folder_name)


def source_kind(path, folder_name):
    ext = os.path.splitext(path)[1].lower()
    lowered = (folder_name + " " + path).lower()
    if ext == ".fbx":
        if any(marker in lowered for marker in SKELETAL_MARKERS):
            return "skeletal_fbx"
        return "static_fbx"
    return "generic"


def make_static_fbx_task(filename, destination, destination_name):
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)

    static_data = options.get_editor_property("static_mesh_import_data")
    static_data.set_editor_property("auto_generate_collision", True)
    static_data.set_editor_property("combine_meshes", True)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filename)
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    return task


def make_skeletal_fbx_task(filename, destination, destination_name):
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)

    skeletal_data = options.get_editor_property("skeletal_mesh_import_data")
    if skeletal_data:
        skeletal_data.set_editor_property("import_meshes_in_bone_hierarchy", True)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filename)
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    return task


def make_generic_task(filename, destination, destination_name):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", filename)
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    return task


def collision_counts(mesh):
    try:
        body_setup = mesh.get_editor_property("body_setup")
    except Exception:
        try:
            body_setup = mesh.get_body_setup()
        except Exception:
            body_setup = None

    if not body_setup:
        return {"total": 0, "error": "missing_body_setup"}

    try:
        agg_geom = body_setup.get_editor_property("agg_geom")
    except Exception as exc:
        return {"total": 0, "error": "missing_agg_geom", "message": str(exc)}

    counts = {}
    total = 0
    for prop in [
        "box_elems",
        "sphere_elems",
        "sphyl_elems",
        "convex_elems",
        "tapered_capsule_elems",
        "level_set_elems",
        "skinned_level_set_elems",
    ]:
        try:
            value = agg_geom.get_editor_property(prop)
            count = len(value) if value is not None else 0
        except Exception:
            count = 0
        counts[prop] = count
        total += count
    counts["total"] = total
    return counts


def triangle_count(mesh):
    attempts = []
    method = getattr(mesh, "get_num_triangles", None)
    if method:
        try:
            return int(method(0)), attempts
        except Exception as exc:
            attempts.append("get_num_triangles: %s" % exc)

    static_lib = getattr(unreal, "EditorStaticMeshLibrary", None)
    if static_lib:
        for method_name in ["get_number_triangles", "get_num_triangles"]:
            method = getattr(static_lib, method_name, None)
            if method:
                try:
                    return int(method(mesh, 0)), attempts
                except Exception as exc:
                    attempts.append("EditorStaticMeshLibrary.%s: %s" % (method_name, exc))

    subsystem_class = getattr(unreal, "StaticMeshEditorSubsystem", None)
    if subsystem_class:
        try:
            subsystem = unreal.get_editor_subsystem(subsystem_class)
            for method_name in ["get_number_triangles", "get_num_triangles"]:
                method = getattr(subsystem, method_name, None)
                if method:
                    try:
                        return int(method(mesh, 0)), attempts
                    except Exception as exc:
                        attempts.append("StaticMeshEditorSubsystem.%s: %s" % (method_name, exc))
        except Exception as exc:
            attempts.append("StaticMeshEditorSubsystem: %s" % exc)

    return None, attempts


def add_collision_if_missing(mesh):
    before = collision_counts(mesh)
    if before.get("total", 0) > 0:
        return {"attempted": False, "before": before, "after": before}

    static_lib = getattr(unreal, "EditorStaticMeshLibrary", None)
    shape_enum = getattr(unreal, "ScriptingCollisionShapeType", None)
    shape = getattr(shape_enum, "NDOP10_X", None) if shape_enum else None
    if static_lib and hasattr(static_lib, "add_simple_collisions") and shape is not None:
        try:
            static_lib.add_simple_collisions(mesh, shape)
            unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        except Exception as exc:
            return {"attempted": True, "before": before, "after": collision_counts(mesh), "error": str(exc)}

    return {"attempted": bool(static_lib and shape is not None), "before": before, "after": collision_counts(mesh)}


def mesh_source_files(root):
    sources = []
    for dirpath, _, filenames in os.walk(root):
        for filename in filenames:
            ext = os.path.splitext(filename)[1].lower()
            if ext in MESH_EXTENSIONS:
                sources.append(os.path.join(dirpath, filename))
    return sorted(sources)


def discover_packs():
    content_root = p("Content")
    packs = []
    for folder_name in sorted(os.listdir(content_root)):
        if not folder_name.startswith("Fab_"):
            continue
        folder_path = os.path.join(content_root, folder_name)
        if not os.path.isdir(folder_path):
            continue

        mesh_files = mesh_source_files(folder_path)
        known = KNOWN_READY_FOLDERS.get(folder_name)
        slug = slug_from_folder(folder_name, mesh_files)
        pack = {
            "folder": folder_name,
            "label": label_from_folder(folder_name),
            "slug": slug,
            "source_root": folder_path,
            "mesh_source_count": len(mesh_files),
            "destination": known["destination"] if known else "/Game/Fab/Library/%s" % slug,
            "known_ready": bool(known),
            "note": known["note"] if known else "",
            "tasks": [],
            "skipped_sources": [],
        }

        if known:
            pack["status"] = "already_ready"
            packs.append(pack)
            continue

        if not mesh_files:
            pack["status"] = "blocked_no_mesh_source"
            packs.append(pack)
            continue

        if len(mesh_files) > 50:
            pack["status"] = "blocked_bulk_stylized_source"
            pack["skipped_sources"].append({
                "count": len(mesh_files),
                "reason": "Bulk source set is too large for blind import and conflicts with the high-quality non-cartoon map direction.",
            })
            packs.append(pack)
            continue

        used_names = set()
        for source in mesh_files:
            base_name = sanitize_name(os.path.splitext(os.path.basename(source))[0])
            asset_name = base_name
            index = 2
            while asset_name.lower() in used_names:
                asset_name = "%s_%d" % (base_name, index)
                index += 1
            used_names.add(asset_name.lower())
            lowered = (folder_name + " " + source).lower()
            style_flags = [marker for marker in LOW_PRIORITY_STYLE_MARKERS if marker in lowered]
            pack["tasks"].append({
                "task_key": "%s|%s" % (folder_name, asset_name),
                "kind": source_kind(source, folder_name),
                "asset_name": asset_name,
                "source": source,
                "destination": pack["destination"],
                "style_flags": style_flags,
            })

        pack["status"] = "pending_import"
        packs.append(pack)
    return packs


def make_task(task_spec):
    kind = task_spec["kind"]
    if kind == "static_fbx":
        return make_static_fbx_task(task_spec["source"], task_spec["destination"], task_spec["asset_name"])
    if kind == "skeletal_fbx":
        return make_skeletal_fbx_task(task_spec["source"], task_spec["destination"], task_spec["asset_name"])
    return make_generic_task(task_spec["source"], task_spec["destination"], task_spec["asset_name"])


def inventory_destination(destination):
    inventory = {
        "destination": destination,
        "assets": [],
        "meshes": [],
        "mesh_count": 0,
        "static_mesh_count": 0,
        "skeletal_mesh_count": 0,
    }
    if not unreal.EditorAssetLibrary.does_directory_exist(destination):
        return inventory

    asset_paths = unreal.EditorAssetLibrary.list_assets(destination, True, False)
    for asset_path in sorted(asset_paths):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not asset:
            continue
        class_name = asset.get_class().get_name()
        inventory["assets"].append({"asset": asset_path, "class": class_name})
        if class_name == "StaticMesh":
            collision = add_collision_if_missing(asset)
            tris, tri_attempts = triangle_count(asset)
            inventory["meshes"].append({
                "asset": asset_path,
                "class": class_name,
                "triangles_lod0": tris,
                "triangle_count_attempts": tri_attempts,
                "photogrammetry_heavy": bool(tris is not None and tris > 500000),
                "collision": collision.get("after", {}),
                "collision_generation": collision,
            })
            inventory["static_mesh_count"] += 1
        elif class_name == "SkeletalMesh":
            inventory["meshes"].append({
                "asset": asset_path,
                "class": class_name,
                "triangles_lod0": None,
                "photogrammetry_heavy": False,
                "collision": {},
            })
            inventory["skeletal_mesh_count"] += 1
    inventory["mesh_count"] = len(inventory["meshes"])
    return inventory


def main():
    packs = discover_packs()
    import_queue = []
    for pack in packs:
        for task_spec in pack["tasks"]:
            import_queue.append(task_spec)

    result = {
        "success": False,
        "project_root": PROJECT_ROOT,
        "selected_key": SELECTED_KEY,
        "inventory_only": INVENTORY_ONLY,
        "one_import_asset_tasks_call": True,
        "packs": packs,
        "import_queue": import_queue,
        "task_count": len(import_queue),
        "imported_object_paths": [],
        "selected_task": None,
        "inventory": {},
        "notes": [
            "Imports one selected source per commandlet run; final inventory validates saved assets.",
            "FANTASTIC Village bulk is deliberately not imported because the current art direction rejects cartoon/boxy map dressing.",
        ],
    }

    task_lookup = {task_spec["task_key"]: task_spec for task_spec in import_queue}
    if SELECTED_KEY and SELECTED_KEY not in task_lookup:
        result["error"] = "selected_key_not_found"
        write_result(result)
        return 2

    if SELECTED_KEY and not INVENTORY_ONLY:
        task_spec = task_lookup[SELECTED_KEY]
        result["selected_task"] = task_spec
        task = make_task(task_spec)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        try:
            imported = task.get_editor_property("imported_object_paths")
            result["imported_object_paths"].append({
                "task_key": task_spec["task_key"],
                "paths": list(imported) if imported else [],
            })
        except Exception as exc:
            result["imported_object_paths"].append({
                "task_key": task_spec["task_key"],
                "error": str(exc),
            })

    destinations = sorted(set(pack["destination"] for pack in packs))
    for destination in destinations:
        result["inventory"][destination] = inventory_destination(destination)

    failed_known = []
    for pack in packs:
        if pack["known_ready"] and result["inventory"].get(pack["destination"], {}).get("mesh_count", 0) <= 0:
            failed_known.append(pack["folder"])

    result["failed_known_ready_folders"] = failed_known
    result["success"] = len(failed_known) == 0
    write_result(result)
    return 0 if result["success"] else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        write_result({
            "success": False,
            "error": str(exc),
            "traceback": traceback.format_exc(),
            "selected_key": SELECTED_KEY,
            "inventory_only": INVENTORY_ONLY,
        })
        raise
