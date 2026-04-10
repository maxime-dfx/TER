import os
import re
import subprocess
import numpy as np
import toml
import gmsh
import math
import random
import matplotlib.pyplot as plt

# --- CONFIGURATION HARMONISÉE ---
SOLVER_BIN = "./core/bin/solver_fe"
OUTPUT_DIR = "../data/outputs/stats_thermique"
RESULTS_DIR = "../results"

NB_SAMPLES = 20
VF_CIBLE = 0.58
L, H, R = 100.0, 100.0, 3.5  #
ID_MATRICE, ID_FIBRE = 101, 102

# Pour le solveur
MESH_BASE = os.path.join(OUTPUT_DIR, "ver_sample")
CONFIG_FILE = os.path.join(OUTPUT_DIR, "config_stats.toml")

# Création des répertoires
os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(RESULTS_DIR, exist_ok=True)

MATERIAUX = {
    "matrice": {"label": ID_MATRICE, "E": 3500.0, "nu": 0.35, "alpha": 60e-6},
    "fibre":   {"label": ID_FIBRE,   "E": 280000.0, "nu": 0.20, "alpha": -1e-6}
}

# --- FONCTIONS UTILES ---
def reparer_labels(filepath, centres):
    with open(filepath, 'r') as f: lines = f.readlines()
    new_lines = []; vertices = []; sec = None
    for line in lines:
        parts = line.split()
        if not parts: new_lines.append(line); continue
        if parts[0] in ["Vertices", "Triangles", "End"]: sec = parts[0]; new_lines.append(line); continue
        if len(parts) == 1: new_lines.append(line); continue
        if sec == "Vertices": vertices.append((float(parts[0]), float(parts[1]))); new_lines.append(line)
        elif sec == "Triangles":
            v1, v2, v3 = int(parts[0])-1, int(parts[1])-1, int(parts[2])-1
            cx = (vertices[v1][0] + vertices[v2][0] + vertices[v3][0]) / 3.0
            cy = (vertices[v1][1] + vertices[v2][1] + vertices[v3][1]) / 3.0
            is_f = any(math.hypot(cx-fx, cy-fy) < (R+0.1) for fx, fy in centres)
            new_lines.append(f"{parts[0]} {parts[1]} {parts[2]} {ID_FIBRE if is_f else ID_MATRICE}\n")
        else: new_lines.append(line)
    with open(filepath, 'w') as f: f.writelines(new_lines)

def generer_mesh(index):
    mesh_path = f"{MESH_BASE}_{index}.mesh"
    gmsh.initialize(); gmsh.option.setNumber("General.Terminal", 0)
    gmsh.model.add(f"VER_{index}")
    centres = []
    nb_f = int((VF_CIBLE * L * H) / (math.pi * R**2))
    for _ in range(nb_f):
        for _ in range(500):
            cx, cy = random.uniform(R+0.5, L-R-0.5), random.uniform(R+0.5, H-R-0.5)
            if all(math.hypot(cx-ox, cy-oy) > (2*R+0.3) for ox, oy in centres):
                centres.append((cx, cy)); break
    r = gmsh.model.occ.addRectangle(0, 0, 0, L, H)
    for c in centres: gmsh.model.occ.addDisk(c[0], c[1], 0, R, R)
    gmsh.model.occ.fragment([(2, r)], gmsh.model.occ.getEntities(2)[1:])
    gmsh.model.occ.synchronize()
    gmsh.option.setNumber("Mesh.CharacteristicLengthMax", 4.0)
    gmsh.model.mesh.generate(2); gmsh.write(mesh_path); gmsh.finalize()
    reparer_labels(mesh_path, centres)
    return mesh_path

def run_sim(mesh_path):
    # Correction : Utilisation de OUTPUT_DIR au lieu de DATA_DIR
    config = {
        "io": {"mesh_file": mesh_path, "output_dir": OUTPUT_DIR, "output_vtk": "last.vtk"},
        "simulation": {"strain_target": 0.001, "plane_strain": True, "delta_T": -100.0},
        "materiaux": MATERIAUX
    }
    with open(CONFIG_FILE, 'w') as f: 
        toml.dump(config, f)
    
    res = subprocess.run([SOLVER_BIN, CONFIG_FILE, "--all"], capture_output=True, text=True)
    
    m_ax = re.search(r'AlphaX.*?\s+([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\s+°C', res.stdout)
    m_ay = re.search(r'AlphaY.*?\s+([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\s+°C', res.stdout)
    
    if m_ax and m_ay:
        return float(m_ax.group(1)), float(m_ay.group(1))
    else:
        print(f"Erreur d'extraction sur {mesh_path}")
        return None, None

# --- BOUCLE PRINCIPALE ---
results_x, results_y = [], []
print(f"Lancement de l'étude sur {NB_SAMPLES} échantillons...")

for i in range(NB_SAMPLES):
    path = generer_mesh(i)
    ax, ay = run_sim(path)
    if ax is not None:
        results_x.append(ax); results_y.append(ay)
        print(f" Sample {i+1}/{NB_SAMPLES} : AlphaX = {ax:.2e}")

# --- GRAPHIQUES ---
if results_x:
    plt.figure(figsize=(12, 5))

    plt.subplot(1, 2, 1)
    plt.hist(results_x, bins=10, alpha=0.5, label='Alpha X', color='blue')
    plt.hist(results_y, bins=10, alpha=0.5, label='Alpha Y', color='red')
    plt.title("Distribution des Alphas")
    plt.legend()

    plt.subplot(1, 2, 2)
    plt.boxplot([results_x, results_y], tick_labels=['Alpha X', 'Alpha Y'])
    plt.title("Dispersion (Boxplot)")

    plt.tight_layout()
    # On sauvegarde l'image finale dans RESULTS_DIR pour le rapport
    plt.savefig(os.path.join(RESULTS_DIR, "bilan_thermique_stats.png"))
    plt.show()

    print(f"\nMoyenne AlphaX : {np.mean(results_x):.2e}")
    print(f"Écart-type : {np.std(results_x):.2e}")
else:
    print("Aucun résultat collecté.")