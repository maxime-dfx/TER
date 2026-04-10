import os
import re
import subprocess
import numpy as np
import toml
import gmsh
import math
import random

# --- CONFIGURATION GÉOMÉTRIQUE ---
L, H, R = 100.0, 100.0, 3.5
VF_CIBLE = 0.58
ID_MATRICE = 101
ID_FIBRE = 102

# --- CHEMINS ET BINAIRES ---
SOLVER_BIN = "./core/bin/solver_fe"        
DATA_DIR = "../data/outputs/etude_thermique" 
MESH_FILE = os.path.join(DATA_DIR, "ver_thermique.mesh")
CONFIG_FILE = os.path.join(DATA_DIR, "config_thermique.toml")

# --- PROPRIÉTÉS THERMO-ÉLASTIQUES (Epoxy / Carbone) ---
MATERIAUX = {
    "matrice": {"label": ID_MATRICE, "E": 3500.0, "nu": 0.35, "alpha": 60e-6},
    "fibre":   {"label": ID_FIBRE,   "E": 280000.0, "nu": 0.20, "alpha": -1e-6}
}

# --- RÉPARATION DES LABELS DU MAILLAGE ---
def reparer_labels_mesh(filepath, centres_fibres):
    """ Force les IDs 101/102 selon la position géométrique réelle pour le solver C++ """
    with open(filepath, 'r') as f: 
        lines = f.readlines()

    new_lines = []; vertices = []; sec = None
    for line in lines:
        parts = line.split()
        if not parts: 
            new_lines.append(line); continue
        
        if parts[0] in ["Vertices", "Triangles", "End"]: 
            sec = parts[0]; new_lines.append(line); continue
        
        if len(parts) == 1: 
            new_lines.append(line); continue

        if sec == "Vertices" and len(parts) >= 3:
            vertices.append((float(parts[0]), float(parts[1])))
            new_lines.append(line)
        elif sec == "Triangles" and len(parts) >= 4:
            v1, v2, v3 = int(parts[0])-1, int(parts[1])-1, int(parts[2])-1
            # Centre de gravité du triangle
            cx = (vertices[v1][0] + vertices[v2][0] + vertices[v3][0]) / 3.0
            cy = (vertices[v1][1] + vertices[v2][1] + vertices[v3][1]) / 3.0
            
            # Détection de l'appartenance à une fibre
            est_fib = any(math.hypot(cx-fx, cy-fy) < (R+0.1) for fx, fy in centres_fibres)
            label = ID_FIBRE if est_fib else ID_MATRICE
            new_lines.append(f"{parts[0]} {parts[1]} {parts[2]} {label}\n")
        else:
            new_lines.append(line)

    with open(filepath, 'w') as f: 
        f.writelines(new_lines)

# --- GÉNÉRATION DU VER ---
def generer_ver_thermique():
    """ Génère un maillage RSA périodique via Gmsh """
    gmsh.initialize(); gmsh.option.setNumber("General.Terminal", 0)
    gmsh.model.add("VER_THERMIQUE")
    
    centres = []
    nb_fibres = int((VF_CIBLE * L * H) / (math.pi * R**2))
    
    # Algorithme RSA simple
    for _ in range(nb_fibres):
        for _ in range(500):
            cx, cy = random.uniform(R+0.5, L-R-0.5), random.uniform(R+0.5, H-R-0.5)
            if all(math.hypot(cx-ox, cy-oy) > (2*R+0.3) for ox, oy in centres):
                centres.append((cx, cy)); break
                
    rect = gmsh.model.occ.addRectangle(0, 0, 0, L, H)
    for c in centres: 
        gmsh.model.occ.addDisk(c[0], c[1], 0, R, R)
    
    gmsh.model.occ.fragment([(2, rect)], gmsh.model.occ.getEntities(2)[1:])
    gmsh.model.occ.synchronize()
    
    gmsh.option.setNumber("Mesh.CharacteristicLengthMin", 2.5)
    gmsh.option.setNumber("Mesh.CharacteristicLengthMax", 4.0)
    gmsh.model.mesh.generate(2)
    gmsh.write(MESH_FILE)
    gmsh.finalize()
    
    reparer_labels_mesh(MESH_FILE, centres)

# --- CONFIGURATION DU SOLVEUR ---
def setup_config(dt):
    """ Prépare le fichier TOML pour le solveur C++ """
    config = {
        "io": {
            "mesh_file": MESH_FILE, 
            "output_dir": DATA_DIR, 
            "output_vtk": "thermique.vtk"
        },
        "simulation": {
            "strain_target": 0.001, 
            "plane_strain": True, 
            "delta_T": dt  # Indispensable : T Majuscule pour Datafile.cpp
        },
        "materiaux": MATERIAUX
    }
    with open(CONFIG_FILE, 'w') as f: 
        toml.dump(config, f)

# --- APPEL DU SOLVEUR ---
def run_full_characterization():
    """ Lance le solveur C++ avec tous les cas de charge nécessaires """
    print(" -> Lancement du solveur (TX + TY + SHEAR + THERMO)...")
    
    # Utilisation du flag --all pour une caractérisation complète
    # Ou les flags individuels : "--tx", "--ty", "--shear", "--thermo"
    cmd = [SOLVER_BIN, CONFIG_FILE, "--all"] 
    
    res = subprocess.run(cmd, capture_output=True, text=True)
    print("\n--- SORTIE DU SOLVEUR ---")
    print(res.stdout)
    
    if res.returncode != 0:
        print(f"Erreur Solveur :\n{res.stderr}")
        return None

    # Extraction du Alpha effectif dans la console (Regex mise à jour)
    # Cherche 'AlphaX', 'Alpha1' ou 'Alpha_eff'
    match = re.search(r'Alpha[XY].*?\s+([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\s+°C', res.stdout)    
    return float(match.group(1)) if match else None

# --- BOUCLE D'EXÉCUTION ---
if __name__ == "__main__":
    os.makedirs(DATA_DIR, exist_ok=True)
    
    print("=== DÉBUT DE L'ANALYSE THERMO-MÉCANIQUE ===")
    
    # 1. Maillage
    print("Étape 1 : Génération du VER...")
    generer_ver_thermique()
    
    # 2. Configuration
    # Le refroidissement de -100°C simule le retour à température ambiante après cuisson
    print("Étape 2 : Configuration du refroidissement (-100°C)...")
    setup_config(-100.0)
    
    # 3. Calcul
    print("Étape 3 : Calcul des propriétés homogénéisées...")
    alpha_final = run_full_characterization()
    
    # 4. Conclusion
    if alpha_final:
        print("\n" + "="*50)
        print(f"RÉSULTAT FINAL : Alpha_eff = {alpha_final:.2e} °C^-1")
        print("="*50)
    else:
        print("\n[ÉCHEC] Le solveur n'a pas pu extraire Alpha_eff.")
        print("Vérifie les flags et l'affichage dans le code C++.")