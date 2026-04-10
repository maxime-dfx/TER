import os
import re
import subprocess
import numpy as np
import matplotlib.pyplot as plt
import toml
import gmsh
import math
import random

# --- CONFIGURATION HARMONISÉE ---
SOLVER_BIN = "./core/bin/solver_fe"
OUTPUT_DIR = "../data/outputs/ver_analysis"
RESULTS_DIR = "../results"

NB_SAMPLES = 100
VF_CIBLE = 0.58
L, R = 100.0, 3.5
ID_MAT, ID_FIB = 101, 102

os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(RESULTS_DIR, exist_ok=True)

def setup_config(mesh_path, index):
    """Crée un fichier config spécifique dans le dossier outputs"""
    config_path = os.path.join(OUTPUT_DIR, f"config_ver_{index}.toml")
    config = {
        "io": {
            "mesh_file": mesh_path,
            "output_dir": OUTPUT_DIR,
            "output_vtk": f"res_ver_{index}.vtk"
        },
        "simulation": {
            "strain_target": 0.001, 
            "plane_strain": True, 
            "delta_T": 0.0 # T majuscule pour correspondre au solver
        },
        "materiaux": {
            "matrice": {"label": ID_MAT, "E": 3500.0, "nu": 0.35, "alpha": 60e-6},
            "fibre":   {"label": ID_FIB, "E": 280000.0, "nu": 0.20, "alpha": -1e-6}
        }
    }
    with open(config_path, 'w') as f:
        toml.dump(config, f)
    return config_path

def generate_ver(index):
    """Génère le maillage dans outputs et répare les labels"""
    mesh_path = os.path.join(OUTPUT_DIR, f"ver_{index}.mesh")
    
    gmsh.initialize()
    gmsh.option.setNumber("General.Terminal", 0)
    gmsh.model.add(f"VER_{index}")
    
    centres = []
    nb_fibres = int((VF_CIBLE * L**2) / (math.pi * R**2))
    for _ in range(nb_fibres):
        for _ in range(1000):
            cx, cy = random.uniform(R+0.2, L-R-0.2), random.uniform(R+0.2, L-R-0.2)
            if all(math.hypot(cx-ox, cy-oy) > (2*R + 0.1) for ox, oy in centres):
                centres.append((cx, cy))
                break

    rect = gmsh.model.occ.addRectangle(0, 0, 0, L, L)
    for c in centres:
        gmsh.model.occ.addDisk(c[0], c[1], 0, R, R)
    gmsh.model.occ.fragment([(2, rect)], gmsh.model.occ.getEntities(2)[1:])
    gmsh.model.occ.synchronize()
    
    gmsh.option.setNumber("Mesh.CharacteristicLengthMin", 2.0)
    gmsh.option.setNumber("Mesh.CharacteristicLengthMax", 4.0)
    gmsh.model.mesh.generate(2)
    gmsh.write(mesh_path)
    gmsh.finalize()

    # REPARATION DES LABELS
    with open(mesh_path, 'r') as f:
        lines = f.readlines()

    new_lines = []; verts = []; sec = None
    for l in lines:
        p = l.split()
        if not p: new_lines.append(l); continue
        if p[0] in ["Vertices", "Triangles", "End"]:
            sec = p[0]; new_lines.append(l); continue
        if len(p) == 1: new_lines.append(l); continue

        if sec == "Vertices":
            verts.append((float(p[0]), float(p[1])))
            new_lines.append(l)
        elif sec == "Triangles":
            v1, v2, v3 = int(p[0])-1, int(p[1])-1, int(p[2])-1
            cx = (verts[v1][0]+verts[v2][0]+verts[v3][0])/3.0
            cy = (verts[v1][1]+verts[v2][1]+verts[v3][1])/3.0
            est_fib = any(math.hypot(cx-fx, cy-fy) < (R+0.05) for fx, fy in centres)
            new_lines.append(f"{p[0]} {p[1]} {p[2]} {ID_FIB if est_fib else ID_MAT}\n")
        else:
            new_lines.append(l)

    with open(mesh_path, 'w') as f:
        f.writelines(new_lines)
    return mesh_path

def run_sim(c_path, mode):
    res = subprocess.run([SOLVER_BIN, c_path, mode], capture_output=True, text=True)
    if res.returncode != 0:
        return None
    
    label = "E1" if mode == "--tx" else "E2"
    match = re.search(rf'{label}.*?([0-9.]+)\s+GPa', res.stdout)
    return float(match.group(1)) if match else None

# --- BOUCLE PRINCIPALE ---
results_ex = []
results_ey = []

print(f"=== ANALYSE STATISTIQUE DU VER ===")
print(f"Lancement sur {NB_SAMPLES} échantillons (Vf cible = {VF_CIBLE*100:.1f}%)")

for i in range(1, NB_SAMPLES + 1):
    print(f"Échantillon {i}/{NB_SAMPLES}...", end=" ", flush=True)
    m_path = generate_ver(i)
    c_path = setup_config(m_path, i)
    
    ex = run_sim(c_path, "--tx")
    ey = run_sim(c_path, "--ty")
    
    if ex and ey:
        results_ex.append(ex)
        results_ey.append(ey)
        print(f"OK (Ex={ex:.2f}, Ey={ey:.2f})")
    else:
        print("ERREUR (Solveur)")

# --- STATS ET GRAPHIQUES ---
if results_ex:
    ex_arr = np.array(results_ex)
    ey_arr = np.array(results_ey)
    all_data = np.concatenate([ex_arr, ey_arr])
    
    mean_val = np.mean(all_data)
    std_val = np.std(all_data)
    
    print(f"\n--- SYNTHÈSE (Sur {len(all_data)} mesures) ---")
    print(f"Moyenne    : {mean_val:.3f} GPa")
    print(f"Écart-type : {std_val:.3f} GPa")
    print(f"CV         : {(std_val/mean_val)*100:.2f} %")

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))

    # 1. Histogramme
    ax1.hist(all_data, bins=15, density=True, color='skyblue', edgecolor='black', alpha=0.7)
    x = np.linspace(mean_val - 3*std_val, mean_val + 3*std_val, 100)
    ax1.plot(x, 1/(std_val * np.sqrt(2 * np.pi)) * np.exp( - (x - mean_val)**2 / (2 * std_val**2) ), 
             linewidth=2, color='red', label='Loi Normale')
    
    ax1.set_title(f'Distribution des Modules (Vf={VF_CIBLE*100:.1f}%)')
    ax1.set_xlabel('E_eff (GPa)')
    ax1.set_ylabel('Fréquence')
    ax1.legend(); ax1.grid(alpha=0.3)

    # 2. Boxplot (Correction tick_labels)
    ax2.boxplot([ex_arr, ey_arr], tick_labels=['Ex', 'Ey'], patch_artist=True,
                boxprops=dict(facecolor='lightgreen', color='black'))
    ax2.set_title('Comparaison Isotropie')
    ax2.set_ylabel('E (GPa)')
    ax2.grid(axis='y', alpha=0.3)

    plt.tight_layout()
    # Sauvegarde dans le dossier results/
    graph_path = os.path.join(RESULTS_DIR, "bilan_statistique_ver.png")
    plt.savefig(graph_path, dpi=300)
    print(f"\n>> Graphique bilan sauvegardé dans : {graph_path}")
    plt.show()
else:
    print("\nAucune donnée collectée.")