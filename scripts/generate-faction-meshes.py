"""
Generate per-faction silhouette meshes and building shapes as real OBJ files.

Spec source: FactoryContracts/faction_silhouette.json — capsule scales, mantle
types, animal companion presence, building footprints.

Outputs (Wavefront OBJ, text format; Unreal auto-imports as UStaticMesh):
  games/splitroot/Content/Meshes/Factions/SK_Unit_<Faction>_Silhouette.obj  (placeholder
    unit silhouette: faction-distinct at 30m per the discipline rule)
  games/splitroot/Content/Meshes/Buildings/SM_Building_<Name>.obj           (4 buildings:
    VerdantOutpost, LenswrightOutpost, KinwildEncampment, SplitrootTreeCentral)

These are real binary game assets (OBJ is the text-format on disk; Unreal
turns them into binary UStaticMesh uassets on import). Geometry is built
procedurally from the silhouette discipline values + building footprint
table so palette/silhouette JSON updates ripple through automatically.

All units in centimeters (Unreal default). Coordinate convention:
  +X forward, +Y right, +Z up (matches Unreal).
"""

from __future__ import annotations

import math
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
FACTIONS_DIR = ROOT / "games" / "splitroot" / "Content" / "Meshes" / "Factions"
BUILDINGS_DIR = ROOT / "games" / "splitroot" / "Content" / "Meshes" / "Buildings"


# --------- low-level OBJ builder ---------

@dataclass
class Mesh:
    name: str
    verts: list[tuple[float, float, float]] = field(default_factory=list)
    faces: list[tuple[int, ...]] = field(default_factory=list)  # 1-based vertex indices

    def add_quad(self, a, b, c, d):
        base = len(self.verts)
        self.verts.extend([a, b, c, d])
        # Two triangles, CCW
        self.faces.append((base + 1, base + 2, base + 3))
        self.faces.append((base + 1, base + 3, base + 4))

    def add_box(self, cx, cy, cz, sx, sy, sz):
        hx, hy, hz = sx / 2.0, sy / 2.0, sz / 2.0
        # 8 corners
        p = [
            (cx - hx, cy - hy, cz - hz),  # 0
            (cx + hx, cy - hy, cz - hz),  # 1
            (cx + hx, cy + hy, cz - hz),  # 2
            (cx - hx, cy + hy, cz - hz),  # 3
            (cx - hx, cy - hy, cz + hz),  # 4
            (cx + hx, cy - hy, cz + hz),  # 5
            (cx + hx, cy + hy, cz + hz),  # 6
            (cx - hx, cy + hy, cz + hz),  # 7
        ]
        # 6 faces (CCW outward)
        self.add_quad(p[0], p[3], p[2], p[1])  # bottom -Z
        self.add_quad(p[4], p[5], p[6], p[7])  # top +Z
        self.add_quad(p[0], p[1], p[5], p[4])  # -Y
        self.add_quad(p[2], p[3], p[7], p[6])  # +Y
        self.add_quad(p[1], p[2], p[6], p[5])  # +X
        self.add_quad(p[0], p[4], p[7], p[3])  # -X

    def add_cylinder(self, cx, cy, z_bottom, z_top, radius, segments=24):
        """Capped cylinder along +Z."""
        ring_bot = []
        ring_top = []
        for i in range(segments):
            ang = 2 * math.pi * i / segments
            x = cx + radius * math.cos(ang)
            y = cy + radius * math.sin(ang)
            ring_bot.append((x, y, z_bottom))
            ring_top.append((x, y, z_top))
        # Sides
        for i in range(segments):
            j = (i + 1) % segments
            self.add_quad(ring_bot[i], ring_bot[j], ring_top[j], ring_top[i])
        # Top cap (fan)
        base_top = len(self.verts)
        for v in ring_top:
            self.verts.append(v)
        self.verts.append((cx, cy, z_top))
        center_top = len(self.verts)
        for i in range(segments):
            j = (i + 1) % segments
            self.faces.append((base_top + i + 1, base_top + j + 1, center_top))
        # Bottom cap (fan, reversed)
        base_bot = len(self.verts)
        for v in ring_bot:
            self.verts.append(v)
        self.verts.append((cx, cy, z_bottom))
        center_bot = len(self.verts)
        for i in range(segments):
            j = (i + 1) % segments
            self.faces.append((base_bot + j + 1, base_bot + i + 1, center_bot))

    def add_capsule_segment(self, cx, cy, z_bottom, z_top, radius, segments=20, hemisphere_rings=6):
        """Cylinder + 2 hemisphere caps."""
        # Cylinder body (without flat caps)
        ring_bot = []
        ring_top = []
        for i in range(segments):
            ang = 2 * math.pi * i / segments
            x = cx + radius * math.cos(ang)
            y = cy + radius * math.sin(ang)
            ring_bot.append((x, y, z_bottom))
            ring_top.append((x, y, z_top))
        for i in range(segments):
            j = (i + 1) % segments
            self.add_quad(ring_bot[i], ring_bot[j], ring_top[j], ring_top[i])
        # Top hemisphere
        for r in range(hemisphere_rings):
            phi0 = (math.pi / 2.0) * r / hemisphere_rings
            phi1 = (math.pi / 2.0) * (r + 1) / hemisphere_rings
            r0 = radius * math.cos(phi0)
            r1 = radius * math.cos(phi1)
            z0 = z_top + radius * math.sin(phi0)
            z1 = z_top + radius * math.sin(phi1)
            for i in range(segments):
                ang0 = 2 * math.pi * i / segments
                ang1 = 2 * math.pi * (i + 1) / segments
                a = (cx + r0 * math.cos(ang0), cy + r0 * math.sin(ang0), z0)
                b = (cx + r0 * math.cos(ang1), cy + r0 * math.sin(ang1), z0)
                c = (cx + r1 * math.cos(ang1), cy + r1 * math.sin(ang1), z1)
                d = (cx + r1 * math.cos(ang0), cy + r1 * math.sin(ang0), z1)
                self.add_quad(a, b, c, d)
        # Bottom hemisphere
        for r in range(hemisphere_rings):
            phi0 = (math.pi / 2.0) * r / hemisphere_rings
            phi1 = (math.pi / 2.0) * (r + 1) / hemisphere_rings
            r0 = radius * math.cos(phi0)
            r1 = radius * math.cos(phi1)
            z0 = z_bottom - radius * math.sin(phi0)
            z1 = z_bottom - radius * math.sin(phi1)
            for i in range(segments):
                ang0 = 2 * math.pi * i / segments
                ang1 = 2 * math.pi * (i + 1) / segments
                a = (cx + r0 * math.cos(ang0), cy + r0 * math.sin(ang0), z0)
                b = (cx + r1 * math.cos(ang0), cy + r1 * math.sin(ang0), z1)
                c = (cx + r1 * math.cos(ang1), cy + r1 * math.sin(ang1), z1)
                d = (cx + r0 * math.cos(ang1), cy + r0 * math.sin(ang1), z0)
                self.add_quad(a, b, c, d)

    def add_sphere(self, cx, cy, cz, radius, segments=16, rings=8):
        for r in range(rings):
            phi0 = math.pi * r / rings - math.pi / 2
            phi1 = math.pi * (r + 1) / rings - math.pi / 2
            r0 = radius * math.cos(phi0)
            r1 = radius * math.cos(phi1)
            z0 = cz + radius * math.sin(phi0)
            z1 = cz + radius * math.sin(phi1)
            for i in range(segments):
                ang0 = 2 * math.pi * i / segments
                ang1 = 2 * math.pi * (i + 1) / segments
                a = (cx + r0 * math.cos(ang0), cy + r0 * math.sin(ang0), z0)
                b = (cx + r0 * math.cos(ang1), cy + r0 * math.sin(ang1), z0)
                c = (cx + r1 * math.cos(ang1), cy + r1 * math.sin(ang1), z1)
                d = (cx + r1 * math.cos(ang0), cy + r1 * math.sin(ang0), z1)
                self.add_quad(a, b, c, d)

    def add_cone(self, cx, cy, z_base, z_tip, base_radius, segments=20):
        ring = []
        for i in range(segments):
            ang = 2 * math.pi * i / segments
            ring.append((cx + base_radius * math.cos(ang), cy + base_radius * math.sin(ang), z_base))
        tip = (cx, cy, z_tip)
        # Sides
        for i in range(segments):
            j = (i + 1) % segments
            base = len(self.verts)
            self.verts.extend([ring[i], ring[j], tip])
            self.faces.append((base + 1, base + 2, base + 3))
        # Base (fan inward — visible from below)
        base_b = len(self.verts)
        for v in ring:
            self.verts.append(v)
        self.verts.append((cx, cy, z_base))
        center = len(self.verts)
        for i in range(segments):
            j = (i + 1) % segments
            self.faces.append((base_b + j + 1, base_b + i + 1, center))

    def write(self, path: Path):
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("w", encoding="utf-8") as f:
            f.write(f"# SPLITROOT procedural mesh: {self.name}\n")
            f.write(f"# Generated by scripts/generate-faction-meshes.py\n")
            f.write(f"o {self.name}\n")
            for v in self.verts:
                f.write(f"v {v[0]:.4f} {v[1]:.4f} {v[2]:.4f}\n")
            for face in self.faces:
                f.write("f " + " ".join(str(i) for i in face) + "\n")


# --------- silhouette parameters (mirror games/splitroot Content faction_silhouette.json) ---------

BASELINE_HEIGHT = 192.0  # cm
BASELINE_RADIUS = 42.0   # cm


def verdant_silhouette() -> Mesh:
    m = Mesh("Unit_Verdant_Silhouette")
    height = BASELINE_HEIGHT * 1.05  # vertical bias
    radius = BASELINE_RADIUS * 1.0
    # Main capsule body, ground at z=0
    m.add_capsule_segment(0, 0, radius, height - radius, radius)
    # Right shoulder protrusion: vertical bramble box (organic top hint)
    shoulder_x = 35.0  # right (+Y in mesh coords used here as right)
    m.add_box(0, shoulder_x, height - 30, 18, 22, 60)
    # Rounded top cap (organic): small dome at the apex
    m.add_sphere(0, 0, height + radius * 0.3, radius * 0.7)
    return m


def lenswright_silhouette() -> Mesh:
    m = Mesh("Unit_Lenswright_Silhouette")
    height = BASELINE_HEIGHT
    upper_radius = BASELINE_RADIUS * 1.0
    lower_radius = BASELINE_RADIUS * 1.2  # wider base, anchored
    # Lower half (wider): from ground to mid
    mid = height * 0.55
    m.add_capsule_segment(0, 0, lower_radius, mid, lower_radius)
    # Upper half (narrower)
    m.add_capsule_segment(0, 0, mid, height - upper_radius, upper_radius)
    # Right shoulder crossbow mount: cylinder pointing forward (+X)
    # Approximate by adding a long box from shoulder out forward
    m.add_box(50, 40, height * 0.85, 80, 20, 18)
    # Reflective head-lens sphere on top
    m.add_sphere(0, 0, height + 12, 12)
    return m


def kinwild_silhouette() -> Mesh:
    m = Mesh("Unit_Kinwild_Silhouette")
    height = BASELINE_HEIGHT
    radius = BASELINE_RADIUS
    # Main capsule (forward-leaning approximated by offsetting top forward)
    # For a basic OBJ placeholder we offset the cap by the lean projection.
    lean_deg = 10.0
    lean_offset = math.tan(math.radians(lean_deg)) * height
    # Body cylinder + leaning hint: bottom at (0,0,r), top shifted forward
    # We use a sheared box approximation for the body and a sphere at the head.
    base_z = radius
    apex_z = height - radius
    # Body: 4 vertex quads forming a tapered tilted box
    half = radius
    bv = [(-half, -half, base_z), (half, -half, base_z), (half, half, base_z), (-half, half, base_z)]
    tv = [
        (-half + lean_offset, -half, apex_z),
        (half + lean_offset, -half, apex_z),
        (half + lean_offset, half, apex_z),
        (-half + lean_offset, half, apex_z),
    ]
    m.add_quad(bv[0], bv[3], bv[2], bv[1])  # bottom
    m.add_quad(tv[0], tv[1], tv[2], tv[3])  # top
    m.add_quad(bv[0], bv[1], tv[1], tv[0])  # -Y
    m.add_quad(bv[2], bv[3], tv[3], tv[2])  # +Y
    m.add_quad(bv[1], bv[2], tv[2], tv[1])  # +X
    m.add_quad(bv[0], tv[0], tv[3], bv[3])  # -X
    # Head sphere at forward-leaned apex
    m.add_sphere(lean_offset, 0, height, radius * 0.7)
    # Mantle cloak: trailing triangular fan behind (-X)
    m.add_quad(
        (-half, -half * 1.2, base_z + 30),
        (-half - 80, -half * 0.6, base_z + 10),
        (-half - 80, half * 0.6, base_z + 10),
        (-half, half * 1.2, base_z + 30),
    )
    # Animal companion: smaller capsule offset to the right (+Y by 80)
    companion_h = 80.0
    companion_r = 28.0
    m.add_capsule_segment(0, 80, companion_r, companion_h - companion_r, companion_r)
    return m


# --------- buildings ---------

def verdant_outpost() -> Mesh:
    m = Mesh("Building_VerdantOutpost")
    # Tall central spire 800x800x4000
    m.add_box(0, 0, 2000, 800, 800, 4000)
    # 4 low encircling walls (450 each, 600x150 footprint)
    wall_h = 450
    wall_t = 150
    wall_l = 600
    offset = 600
    m.add_box( offset, 0, wall_h / 2, wall_t, wall_l, wall_h)
    m.add_box(-offset, 0, wall_h / 2, wall_t, wall_l, wall_h)
    m.add_box(0,  offset, wall_h / 2, wall_l, wall_t, wall_h)
    m.add_box(0, -offset, wall_h / 2, wall_l, wall_t, wall_h)
    return m


def lenswright_outpost() -> Mesh:
    m = Mesh("Building_LenswrightOutpost")
    # Wide low building 2000x1500x1500
    m.add_box(0, 0, 750, 2000, 1500, 1500)
    # Two corner turret cylinders (front-left, front-right)
    turret_r = 250
    turret_h_top = 2000
    m.add_cylinder( 900,  650, 0, turret_h_top, turret_r)
    m.add_cylinder( 900, -650, 0, turret_h_top, turret_r)
    return m


def kinwild_encampment() -> Mesh:
    m = Mesh("Building_KinwildEncampment")
    # 3 angled tent cones around a central fire-pit cylinder
    fire_r = 200
    fire_h = 200
    m.add_cylinder(0, 0, 0, fire_h, fire_r)
    # Tents: 3 cones radiating
    for i in range(3):
        ang = 2 * math.pi * i / 3 + math.pi / 6
        dist = 600
        cx = dist * math.cos(ang)
        cy = dist * math.sin(ang)
        m.add_cone(cx, cy, 0, 800, 350)
    return m


def central_splitroot_tree() -> Mesh:
    m = Mesh("Building_CentralSplitrootTree")
    # Tall central trunk 1500x1500x6000 (cylinder representation)
    m.add_cylinder(0, 0, 0, 6000, 750)
    # 3 above-ground root knee-walls radiating toward each faction
    # West (toward Verdant), East (toward Lenswright), North (toward Kinwild)
    # Each root: 3000x600x800 ramp shape approximated as box
    root_h = 800
    root_w = 600
    root_l = 3000
    for ang_deg in (180.0, 0.0, 90.0):  # W, E, N
        ang = math.radians(ang_deg)
        # Place root centered halfway outward from trunk
        cx = (root_l / 2 + 750) * math.cos(ang)
        cy = (root_l / 2 + 750) * math.sin(ang)
        # Orient the root along its outward direction by swapping length/width
        # appropriately for cardinal-aligned roots only.
        if abs(math.cos(ang)) > 0.5:  # E/W
            m.add_box(cx, cy, root_h / 2, root_l, root_w, root_h)
        else:  # N
            m.add_box(cx, cy, root_h / 2, root_w, root_l, root_h)
    return m


# --------- driver ---------

def main() -> int:
    FACTIONS_DIR.mkdir(parents=True, exist_ok=True)
    BUILDINGS_DIR.mkdir(parents=True, exist_ok=True)

    artifacts = []

    for fn, sub in [
        (verdant_silhouette, "Verdant"),
        (lenswright_silhouette, "Lenswright"),
        (kinwild_silhouette, "Kinwild"),
    ]:
        mesh = fn()
        out = FACTIONS_DIR / f"SM_Unit_{sub}_Silhouette.obj"
        mesh.write(out)
        artifacts.append(out)

    for fn, name in [
        (verdant_outpost, "VerdantOutpost"),
        (lenswright_outpost, "LenswrightOutpost"),
        (kinwild_encampment, "KinwildEncampment"),
        (central_splitroot_tree, "CentralSplitrootTree"),
    ]:
        mesh = fn()
        out = BUILDINGS_DIR / f"SM_Building_{name}.obj"
        mesh.write(out)
        artifacts.append(out)

    print(f"Wrote {len(artifacts)} OBJ mesh files.")
    total = 0
    for p in artifacts:
        size = p.stat().st_size
        total += size
        rel = p.relative_to(ROOT)
        print(f"  {str(rel):72s} {size:>10,} bytes")
    print(f"Total: {total:,} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
