import os
import re
import subprocess
import numpy as np
import matplotlib.pyplot as plt
import toml
import gmsh
import math

# --- CONFIGURATION HARMONISÉE ---
SOLVER_BIN = "./core/bin/solver_fe"
OUTPUT_DIR = "../data/outputs/validation"
RESULTS_DIR = "../results"

MESH_FILE = os.path.join(OUTPUT_DIR, "val_temp.mesh")
CONFIG_FILE = os.path.join(OUTPUT_DIR, "val_temp.toml")
ID_MAT, ID_FIB = 101, 102

os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(RESULTS_DIR, exist_ok=True)

def setup_config(E_m, E_f, plane_strain=False):
    """Prépare la config pour le test"""
    config = {
        "io": {"mesh_file": MESH_FILE, "output_dir": OUTPUT_DIR, "output_vtk": "validation.vtk"},
        "simulation": {"strain_target": 0.001, "plane_strain": plane_strain, "delta_T": 0.0},
        "materiaux": {
            "matrice": {"label": ID_MAT, "E": E_m, "nu": 0.35, "alpha": 60e-6},
            "fibre":   {"label": ID_FIB, "E": E_f, "nu": 0.35, "alpha": -1e-6}
        }
    }
    with open(CONFIG_FILE, 'w') as f: toml.dump(config, f)

def generate_mesh(lc, R=30.0):
    """Génère une inclusion centrale unique pour la validation"""
    gmsh.initialize(); gmsh.option.setNumber("General.Terminal", 0)
    gmsh.model.add("ValidationMesh")
    L = 100.0
    rect = gmsh.model.occ.addRectangle(0, 0, 0, L, L)
    circ = gmsh.model.occ.addDisk(L/2, L/2, 0, R, R)
    gmsh.model.occ.fragment([(2, rect)], [(2, circ)])
    gmsh.model.occ.synchronize()
    gmsh.option.setNumber("Mesh.CharacteristicLengthMin", lc)
    gmsh.option.setNumber("Mesh.CharacteristicLengthMax", lc)
    gmsh.model.mesh.generate(2); gmsh.write(MESH_FILE); gmsh.finalize()

    # Réparation des labels
    with open(MESH_FILE, 'r') as f: lines = f.readlines()
    new_lines = []; vertices = []; section = None
    for l in lines:
        p = l.split()
        if not p: new_lines.append(l); continue
        if p[0] in ["Vertices", "Triangles", "End"]: section = p[0]; new_lines.append(l); continue
        if section == "Vertices" and len(p) >= 3: 
            vertices.append((float(p[0]), float(p[1]))); new_lines.append(l)
        elif section == "Triangles" and len(p) >= 4:
            v1, v2, v3 = int(p[0])-1, int(p[1])-1, int(p[2])-1
            cx = (vertices[v1][0] + vertices[v2][0] + vertices[v3][0]) / 3.0
            cy = (vertices[v1][1] + vertices[v2][1] + vertices[v3][1]) / 3.0
            label = ID_FIB if math.hypot(cx-50, cy-50) < (R+0.1) else ID_MAT
            new_lines.append(f"{p[0]} {p[1]} {p[2]} {label}\n")
        else: new_lines.append(l)
    with open(MESH_FILE, 'w') as f: f.writelines(new_lines)

def run_solver(mode="--tx"):
    res = subprocess.run([SOLVER_BIN, CONFIG_FILE, mode], capture_output=True, text=True)
    label = "E1" if mode == "--tx" else "E2"
    match = re.search(rf'{label}.*?([0-9.]+)\s+GPa', res.stdout)
    return float(match.group(1)) * 1000.0 if match else None

# --- SUITE DE TESTS ---

def test_1_homogene():
    print("--- [TEST 1] MATÉRIAU HOMOGÈNE ---")
    setup_config(3500.0, 3500.0, False)
    generate_mesh(10.0)
    val = run_solver("--tx")
    print(f" -> E_eff: {val:.2f} MPa | Attendu: 3500.00")
    print(" ✅ VALIDE" if abs(val-3500)<1e-2 else " ❌ ÉCHEC")

def test_2_convergence():
    print("\n--- [TEST 2] ANALYSE DE CONVERGENCE ---")
    lc_list = [20, 10, 5, 2.5]
    results = []
    setup_config(3500.0, 280000.0, False)
    for lc in lc_list:
        generate_mesh(lc)
        results.append(run_solver("--tx"))
        print(f"  h={lc:<5} -> E={results[-1]:.2f} MPa")
    
    ref = results[-1]
    errors = [abs(r - ref)/ref for r in results[:-1]]
    print(f" -> Erreur finale relative: {errors[-1]*100:.4f}%")
    print(" ✅ VALIDE" if errors[-1] < 0.01 else " ⚠️ PRÉCISION FAIBLE")

def test_3_bornes():
    print("\n--- [TEST 3] PHYSIQUE : BORNES DE VOIGT/REUSS ---")
    Em, Ef = 3500.0, 280000.0
    R, L = 30.0, 100.0
    f = (math.pi * R**2) / (L**2)
    # On compare aux bornes simples (Loi des mélanges)
    E_voigt = f * Ef + (1-f) * Em
    E_reuss = 1 / (f/Ef + (1-f)/Em)
    setup_config(Em, Ef, False)
    generate_mesh(2.5)
    val = run_solver("--tx")
    print(f" -> Bornes: {E_reuss:.1f} < {val:.1f} < {E_voigt:.1f}")
    print(" ✅ VALIDE" if E_reuss < val < E_voigt else " ❌ PHYSIQUE INCOHÉRENTE")

if __name__ == "__main__":
    print("🚀 DÉMARRAGE DE LA VALIDATION DU SOLVEUR\n")
    test_1_homogene()
    test_2_convergence()
    test_3_bornes()
    print("\n🏁 TOUS LES TESTS SONT TERMINÉS.")