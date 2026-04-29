import os
import re
import math
import random
import subprocess
import toml
import gmsh

def distance_periodique(x1, y1, x2, y2, L, H):
    """Calcule la plus courte distance sur un tore topologique (domaine replié)."""
    dx = abs(x1 - x2)
    dy = abs(y1 - y2)
    dx = min(dx, L - dx)
    dy = min(dy, H - dy)
    return math.hypot(dx, dy)

def generer_maillage(mesh_path, L, H, r_fibre, vf_cible, r_int=None, force_label=None, id_mat=1, id_fib=2, id_int=3):
    """
    Génère un VER stochastique avec conditions périodiques géométriques strictes.
    Utilise un filtrage topologique pour la labellisation des éléments.
    """
    gmsh.initialize()
    gmsh.option.setNumber("General.Terminal", 0)
    gmsh.model.add("VER_Periodique")
    
    # ==========================================
    # 1. Algorithme RSA Périodique
    # ==========================================
    centres = []
    r_collision = r_int if r_int else r_fibre
    
    if 0.0 < vf_cible < 1.0:
        nb_fibres_cible = int((vf_cible * L * H) / (math.pi * (r_fibre**2)))
        tentatives_max = 50000 # Sécurité anti-boucle infinie
        tentatives = 0
        
        while len(centres) < nb_fibres_cible and tentatives < tentatives_max:
            cx = random.uniform(0, L)
            cy = random.uniform(0, H)
            
            # Vérification des collisions avec distance périodique !
            if all(distance_periodique(cx, cy, ox, oy, L, H) > (2 * r_collision + 0.1) for ox, oy in centres):
                centres.append((cx, cy))
            tentatives += 1
            
        if len(centres) < nb_fibres_cible:
            vf_atteint = len(centres)*math.pi*(r_fibre**2)/(L*H)
            print(f"\n      [!] AVERTISSEMENT : Limite de blocage atteinte à Vf = {vf_atteint:.2%}")
            print(f"      [!] L'algorithme RSA strict ne peut pas atteindre {vf_cible:.2%} sans relaxation mécanique.")

    # ==========================================
    # 2. Topologie : Rectangle et Fibres Fantômes
    # ==========================================
    rect_tag = gmsh.model.occ.addRectangle(0, 0, 0, L, H)
    tools = []
    
    if len(centres) > 0:
        for cx, cy in centres:
            # Génération de la fibre principale et de ses fantômes (9 positions possibles)
            for dx in [-L, 0, L]:
                for dy in [-H, 0, H]:
                    nx, ny = cx + dx, cy + dy
                    
                    # Optimisation : on ne crée le fantôme que s'il intersecte la boîte [0,L]x[0,H]
                    if -r_collision <= nx <= L + r_collision and -r_collision <= ny <= H + r_collision:
                        if r_int:
                            tools.append((2, gmsh.model.occ.addDisk(nx, ny, 0, r_int, r_int)))
                        tools.append((2, gmsh.model.occ.addDisk(nx, ny, 0, r_fibre, r_fibre)))
    
    # ==========================================
    # 3. Fragmentation et Filtrage Topologique
    # ==========================================
    if tools:
        gmsh.model.occ.fragment([(2, rect_tag)], tools)
        gmsh.model.occ.synchronize()
        
        surfaces = gmsh.model.getEntities(2)
        mat_tags, fib_tags, int_tags = [], [], []
        eps = 1e-4 # Tolérance numérique
        
        for dim, tag in surfaces:
            bbox = gmsh.model.occ.getBoundingBox(dim, tag)
            # bbox = (xmin, ymin, zmin, xmax, ymax, zmax)
            
            # Le fragment est-il strictement DANS la boîte [0, L] x [0, H] ?
            if bbox[0] >= -eps and bbox[1] >= -eps and bbox[3] <= L + eps and bbox[4] <= H + eps:
                
                # Identification de la nature physique du fragment via son centre de masse
                com = gmsh.model.occ.getCenterOfMass(dim, tag)
                
                # On cherche la distance au centre de fibre originel le plus proche (via Tore)
                dist_min = min([distance_periodique(com[0], com[1], cx, cy, L, H) for cx, cy in centres])
                
                if dist_min <= r_fibre + eps:
                    fib_tags.append(tag)
                elif r_int and dist_min <= r_int + eps:
                    int_tags.append(tag)
                else:
                    mat_tags.append(tag)
            else:
                # Destruction des résidus géométriques extérieurs générés par les fantômes
                gmsh.model.removeEntities([(dim, tag)], recursive=True)
        
        # Application des Physical Groups
        if mat_tags: gmsh.model.addPhysicalGroup(2, mat_tags, id_mat)
        if fib_tags: gmsh.model.addPhysicalGroup(2, fib_tags, id_fib)
        if int_tags: gmsh.model.addPhysicalGroup(2, int_tags, id_int)
    else:
        gmsh.model.occ.synchronize()
        gmsh.model.addPhysicalGroup(2, [rect_tag], force_label if force_label else id_mat)

    # ==========================================
    # 4. Paramétrage et Export du Maillage
    # ==========================================
    gmsh.option.setNumber("Mesh.SaveAll", 0) 
    gmsh.option.setNumber("Mesh.SaveElementTagType", 2) 
    gmsh.option.setNumber("Mesh.CharacteristicLengthMin", 1.5)
    gmsh.option.setNumber("Mesh.CharacteristicLengthMax", 4.0)
    
    gmsh.model.mesh.generate(2)
    gmsh.write(mesh_path)
    gmsh.finalize()


def ecrire_config(config_path, mesh_path, output_dir, dt, materiaux):
    """Écrit le fichier TOML de configuration pour le solveur."""
    config = {
        "io": {"mesh_file": mesh_path, "output_dir": output_dir, "output_vtk": "stats_out.vtk"},
        "simulation": {"strain_target": 0.005, "plane_strain": True, "delta_T": dt},
        "materiaux": materiaux
    }
    with open(config_path, 'w') as f:
        toml.dump(config, f)


def lancer_solveur(solver_bin, config_path, mode="--all"):
    """Exécute le solveur C++ et extrait les résultats via le JSON généré."""
    solver_exe = os.path.abspath(solver_bin)
    config_abs = os.path.abspath(config_path)
    
    my_env = os.environ.copy()
    # Ajout du chemin MinGW si nécessaire sous Windows (à adapter selon ta config)
    my_env["PATH"] += os.pathsep + r"C:\msys64\mingw64\bin"
    
    res = subprocess.run([solver_exe, config_abs, mode], capture_output=True, text=True, env=my_env)
    
    if res.returncode != 0:
        print(f"\n[CRASH SOLVEUR]\n{res.stderr.strip() if res.stderr else res.stdout.strip()}")
        return {}

    import json
    config = toml.load(config_path)
    out_dir = config.get("io", {}).get("output_dir", ".")
    json_path = os.path.join(out_dir, "results.json")
    
    if os.path.exists(json_path):
        with open(json_path, 'r') as f:
            data = json.load(f)
        
        if mode == "--thermo" and data.get("AlphaX", 0.0) == 0.0:
            print(f"\n[ATTENTION] Le calcul a tourné, mais AlphaX vaut 0.\nConsole C++ :\n{res.stdout.strip()}")
            
        return data
    else:
        print(f"\n[ERREUR LECTURE JSON] Le fichier est introuvable.\nConsole C++ :\n{res.stdout.strip()}")
        return {}

    def extract(pattern, text):
        m = re.search(pattern, text)
        return float(m.group(1)) if m else None

    # Fallback par RegEx sur la console si le JSON est corrompu
    results = {
        "E1": extract(r'E1.*?([\d\.]+)\s+GPa', res.stdout),
        "E2": extract(r'E2.*?([\d\.]+)\s+GPa', res.stdout),
        "G12": extract(r'G12.*?([\d\.]+)\s+GPa', res.stdout),
        "AlphaX": extract(r'AlphaX.*?\s+([-+]?[\d\.eE+-]+)\s+°C', res.stdout),
        "AlphaY": extract(r'AlphaY.*?\s+([-+]?[\d\.eE+-]+)\s+°C', res.stdout),
        "Vf_reel": extract(r'Fraction Volumique.*?([\d\.]+)', res.stdout)
    }
    if results["Vf_reel"] is not None:
        results["Vf_reel"] /= 100.0
        
    return results

def get_materiaux_dict(avec_interphase):
    """Retourne le dictionnaire des matériaux avec propriétés mécaniques de base."""
    mats = {
        "matrice": {"label": 1, "E": 3500.0,   "nu": 0.35, "alpha": 60e-6, "Xt": 80.0, "Xc": 150.0},
        "fibre":   {"label": 2, "E": 280000.0, "nu": 0.20, "alpha": -1e-6, "Xt": 3000.0, "Xc": 2000.0}
    }
    if avec_interphase:
        mats["interphase"] = {"label": 3, "E": 2000.0, "nu": 0.38, "alpha": 70e-6, "Xt": 40.0, "Xc": 100.0}
    return mats