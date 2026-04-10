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
OUTPUT_DIR = "../data/outputs/parametrics"
RESULTS_DIR = "../results"

# Fichiers de travail
MESH_FILE = os.path.join(OUTPUT_DIR, "ver_param.mesh")
CONFIG_FILE = os.path.join(OUTPUT_DIR, "config_param.toml")

# Plage de test
VF_RANGE = [0.0, 0.05, 0.15, 0.25, 0.35, 0.45, 1.0] 
L, H, R = 100.0, 100.0, 3.5
ID_MATRICE, ID_FIBRE = 101, 102

os.makedirs(OUTPUT_DIR, exist_ok=True)

# --- CORRECTEUR DE LABELS ---
def reparer_labels_mesh(filepath, centres_fibres, force_label=None):
    """ 
    Remplace les labels par 101/102. 
    force_label permet de traiter les cas 0% (tout 101) ou 100% (tout 102).
    """
    with open(filepath, 'r') as f:
        lines = f.readlines()

    new_lines, vertices = [], []
    in_vertices, in_triangles = False, False

    for line in lines:
        parts = line.split()
        if not parts:
            new_lines.append(line); continue

        if parts[0] == "Vertices":
            in_vertices = True; new_lines.append(line); continue
        if parts[0] == "Triangles":
            in_vertices = False; in_triangles = True; new_lines.append(line); continue
        if parts[0] in ["End", "Edges"]:
            in_triangles = False; new_lines.append(line); continue

        if in_vertices and len(parts) >= 3:
            vertices.append((float(parts[0]), float(parts[1])))
            new_lines.append(line)
        elif in_triangles and len(parts) >= 4:
            if force_label:
                label_final = force_label
            else:
                v1, v2, v3 = int(parts[0])-1, int(parts[1])-1, int(parts[2])-1
                cx = (vertices[v1][0] + vertices[v2][0] + vertices[v3][0]) / 3.0
                cy = (vertices[v1][1] + vertices[v2][1] + vertices[v3][1]) / 3.0
                est_fibre = any(math.hypot(cx - fx, cy - fy) < (R + 0.05) for fx, fy in centres_fibres)
                label_final = ID_FIBRE if est_fibre else ID_MATRICE
            
            new_lines.append(f"{parts[0]} {parts[1]} {parts[2]} {label_final}\n")
        else:
            new_lines.append(line)

    with open(filepath, 'w') as f:
        f.writelines(new_lines)

# --- MODÈLES ANALYTIQUES ---
def get_analytical(vf):
    vm = 1.0 - vf
    # Déformations planes
    Ef_ps = E_F / (1.0 - NU_F**2)
    Em_ps = E_M / (1.0 - NU_M**2)
    
    voigt = vf * Ef_ps + vm * Em_ps
    reuss = 1.0 / (vf / Ef_ps + vm / Em_ps) if (vf > 0 and vm > 0) else (Ef_ps if vf == 1.0 else Em_ps)
    
    alpha_L = (E_F * ALPHA_F * vf + E_M * ALPHA_M * vm) / (E_F * vf + E_M * vm)
    nu_L = NU_F * vf + NU_M * vm
    alpha_T_schapery = (1.0 + NU_F) * ALPHA_F * vf + (1.0 + NU_M) * ALPHA_M * vm - alpha_L * nu_L
    
    return voigt, reuss, alpha_T_schapery

# --- SIMULATION ---
def run_simulation(target_vf):
    gmsh.initialize(); gmsh.option.setNumber("General.Terminal", 0)
    gmsh.model.add("PARAM")
    centres = []
    
    # Géométrie de base
    rect = gmsh.model.occ.addRectangle(0, 0, 0, L, H)
    
    # Cas intermédiaires : placement RSA
    if 0.0 < target_vf < 1.0:
        nb_f = int((target_vf * L * H) / (math.pi * R**2))
        for _ in range(nb_f):
            for _ in range(500):
                cx, cy = random.uniform(R+0.5, L-R-0.5), random.uniform(R+0.5, H-R-0.5)
                if all(math.hypot(cx-ox, cy-oy) > (2*R+0.3) for ox, oy in centres):
                    centres.append((cx, cy)); break
        for c in centres: gmsh.model.occ.addDisk(c[0], c[1], 0, R, R)
        gmsh.model.occ.fragment([(2, rect)], gmsh.model.occ.getEntities(2)[1:])
    
    gmsh.model.occ.synchronize()
    gmsh.model.mesh.generate(2)
    gmsh.write(MESH_FILE)
    gmsh.finalize()
    
    # Réparation des labels selon le cas
    f_label = ID_MATRICE if target_vf == 0.0 else (ID_FIBRE if target_vf == 1.0 else None)
    reparer_labels_mesh(MESH_FILE, centres, force_label=f_label)
    
    config = {
        "io": {"mesh_file": MESH_FILE, "output_dir": DATA_DIR, "output_vtk": "tmp.vtk"},
        "simulation": {"strain_target": 0.001, "plane_strain": True, "delta_T": -100.0},
        "materiaux": {
            "matrice": {"label": ID_MATRICE, "E": E_M, "nu": NU_M, "alpha": ALPHA_M},
            "fibre": {"label": ID_FIBRE, "E": E_F, "nu": NU_F, "alpha": ALPHA_F}
        }
    }
    with open(CONFIG_FILE, 'w') as f: toml.dump(config, f)
    
    res = subprocess.run([SOLVER_BIN, CONFIG_FILE, "--all"], capture_output=True, text=True)
    
    if res.returncode != 0:
        return None, None, None, None, None

    def extract(pattern, text):
        m = re.search(pattern, text)
        return float(m.group(1)) if m else None

    e1 = extract(r'E1.*?([\d\.]+)\s+GPa', res.stdout)
    e2 = extract(r'E2.*?([\d\.]+)\s+GPa', res.stdout)
    ax = extract(r'AlphaX.*?\s+([-+]?[\d\.eE+-]+)\s+°C', res.stdout)
    ay = extract(r'AlphaY.*?\s+([-+]?[\d\.eE+-]+)\s+°C', res.stdout)
    
    # Calcul de la fraction réelle (ou forcée pour les bornes)
    if target_vf == 0.0 or target_vf == 1.0:
        vf_real = target_vf
    else:
        v_raw = extract(r'Fraction Volumique.*?([\d\.]+)', res.stdout)
        vf_real = v_raw / 100.0 if v_raw is not None else target_vf
    
    return vf_real, e1, e2, ax, ay

# --- BOUCLE DE CALCUL ---
vfs, e1_list, e2_list, ax_list, ay_list = [], [], [], [], []
voigt_list, reuss_list, schapery_list = [], [], []

print("=== ÉTUDE PARAMÉTRIQUE (0% -> 100%) ===")

for target in VF_RANGE:
    print(f"Simulation Vf = {target*100:.1f}%...", end=" ", flush=True)
    vr, e1, e2, ax, ay = run_simulation(target)
    
    if e1 is not None:
        vfs.append(vr); e1_list.append(e1); e2_list.append(e2)
        ax_list.append(ax); ay_list.append(ay)
        v, r, s = get_analytical(vr)
        voigt_list.append(v/1000.0); reuss_list.append(r/1000.0); schapery_list.append(s)
        print("OK")
    else:
        print("ERREUR")

# --- TRACÉ ---
if vfs:
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
    
    # Graphe 1 : Modules d'Young
    ax1.plot(vfs, voigt_list, 'g--', label='Borne Sup (Voigt)')
    ax1.plot(vfs, reuss_list, 'r--', label='Borne Inf (Reuss)')
    ax1.scatter(vfs, e1_list, color='blue', marker='o', label='E1 FEM (Longitudinal)')
    ax1.scatter(vfs, e2_list, color='orange', marker='s', label='E2 FEM (Transverse)')
    ax1.set_xlabel('Fraction Volumique Vf'); ax1.set_ylabel('E (GPa)')
    ax1.set_title('Validation des Bornes de Voigt/Reuss')
    ax1.legend(); ax1.grid(True, alpha=0.3)

    # Graphe 2 : Alpha
    ax2.plot(vfs, schapery_list, 'k--', label='Modèle Schapery')
    ax2.scatter(vfs, ax_list, color='blue', label='AlphaX FEM')
    ax2.scatter(vfs, ay_list, color='orange', label='AlphaY FEM')
    ax2.set_xlabel('Fraction Volumique Vf'); ax2.set_ylabel('Alpha (°C^-1)')
    ax2.set_title('Évolution de la dilatation thermique')
    ax2.legend(); ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, "courbes_homogeneisation_limites.png"), dpi=300)
    print(f"\n>> Graphique sauvegardé dans : {RESULTS_DIR}/courbes_homogeneisation_limites.png")
    plt.show()