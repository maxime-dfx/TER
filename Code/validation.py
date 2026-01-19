import os
import subprocess
import numpy as np
import matplotlib.pyplot as plt
import toml

# --- PARAMETRES ---
SOLVER_BIN = "./bin/solver_fe"
# ATTENTION : Remettez ici votre chemin correct vers gmsh
GMSH_BIN = "/opt/gmsh/4.11.1/bin/gmsh"  
OUTPUT_DIR = "data/convergence"
GEO_FILE = f"{OUTPUT_DIR}/temp_mesh.geo"
MESH_FILE = f"{OUTPUT_DIR}/temp_mesh.mesh"
CONFIG_FILE = f"{OUTPUT_DIR}/temp_config.toml"

# Tailles de mailles à tester (du grossier au fin)
LC_VALUES = [80, 40, 20, 10, 5, 2] 

# Propriétés contrastées pour l'étude
E_m, nu_m = 3500.0, 0.35
E_f, nu_f = 280000.0, 0.20

def create_directory():
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

def generate_geo_composite(lc):
    """Génère un carré avec une fibre centrale (Identification Robuste)"""
    with open(GEO_FILE, 'w') as f:
        f.write('SetFactory("OpenCASCADE");\n')
        f.write(f'lc = {lc};\n')
        f.write('L = 100; R = 30;\n') 
        
        # 1. Géométrie
        f.write('Rectangle(1) = {0, 0, 0, L, L};\n')
        f.write('Disk(2) = {L/2, L/2, 0, R};\n')
        
        # 2. Découpe
        f.write('v[] = BooleanFragments{ Surface{1}; Delete; }{ Surface{2}; Delete; };\n')
        
        # 3. Identification Robuste (Par taille de BoundingBox)
        f.write('surf_mat[] = {};\n')
        f.write('surf_fib[] = {};\n')
        f.write('eps = 1e-3;\n')
        
        f.write('For k In {0 : #v[]-1}\n')
        f.write('  bb[] = BoundingBox Surface{ v[k] };\n')
        f.write('  width = bb[3] - bb[0];\n') # xmax - xmin
        
        # Si la largeur est proche de L (100), c'est la matrice
        f.write('  If ( width > L - 1 )\n') 
        f.write('    surf_mat[] += v[k];\n')
        f.write('  Else\n')
        f.write('    surf_fib[] += v[k];\n')
        f.write('  EndIf\n')
        f.write('EndFor\n')
        
        # 4. Physical Groups (Pour avoir les bons labels 101/102)
        f.write('Physical Surface("Matrice", 101) = surf_mat[];\n')
        f.write('Physical Surface("Fibre", 102) = surf_fib[];\n')
        
        # 5. Maillage
        f.write('Mesh.CharacteristicLengthMin = lc;\n')
        f.write('Mesh.CharacteristicLengthMax = lc;\n')

def generate_toml():
    """Crée le fichier de config"""
    config = {
        "io": {
            "mesh_file": MESH_FILE,
            "output_dir": OUTPUT_DIR,
            "output_vtk": "debug.vtk"
        },
        "simulation": {
            "strain_target": 0.001,
            "plane_strain": True
        },
        "materiaux": {
            "matrice": {"label": 101, "E": E_m, "nu": nu_m},
            "fibre":   {"label": 102, "E": E_f, "nu": nu_f}
        }
    }
    with open(CONFIG_FILE, 'w') as f:
        toml.dump(config, f)

def parse_result(output):
    for line in output.split('\n'):
        if "FEM (Calcul)" in line:
            parts = line.split()
            for p in parts:
                try:
                    val = float(p)
                    # Filtre simple pour prendre la bonne valeur (souvent > 0.1 GPa)
                    if val > 0.001: 
                        # Si le code sort des GPa, on convertit en MPa pour la précision
                        # (Ajustez selon si votre code sort déjà des MPa ou GPa)
                        # Supposons ici que le code sort des GPa comme vu précédemment
                        return val * 1000.0 
                except: continue
    return None

def run_study():
    create_directory()
    generate_toml()
    
    results = {}
    
    print(f"--- ETUDE DE CONVERGENCE (Composite) ---")
    print(f"{'LC':<10} {'E_FEM (MPa)':<15}")

    for lc in LC_VALUES:
        generate_geo_composite(lc)
        # Génération Maillage (Silence -v 0)
        subprocess.run([GMSH_BIN, "-2", GEO_FILE, "-o", MESH_FILE, "-v", "0"], check=True)
        
        # Solveur
        res_proc = subprocess.run([SOLVER_BIN, CONFIG_FILE], capture_output=True, text=True)
        
        if res_proc.returncode != 0:
            print(f"CRASH pour lc={lc} !")
            print(res_proc.stderr) # Affiche l'erreur si crash
            continue

        E_fem = parse_result(res_proc.stdout)
        
        if E_fem:
            results[lc] = E_fem
            print(f"{lc:<10} {E_fem:<15.4f}")
        else:
            print(f"Pas de resultat lu pour lc={lc}")

    return results

def plot_convergence(results):
    if not results:
        print("Aucun résultat à tracer.")
        return

    # La référence est le maillage le plus fin (le plus petit LC)
    ref_lc = min(results.keys())
    E_ref = results[ref_lc]
    
    print(f"\n>> Reference prise à LC={ref_lc} : E = {E_ref:.4f} MPa")
    
    h_vals = []
    err_vals = []
    
    for lc, E in results.items():
        if lc == ref_lc: continue 
        
        # Erreur relative
        err = abs(E - E_ref) / E_ref
        h_vals.append(lc)
        err_vals.append(err)
        
    h = np.array(h_vals)
    err = np.array(err_vals)
    
    # Régression pour la pente (Ordre de convergence)
    if len(h) > 1:
        p = np.polyfit(np.log(h), np.log(err), 1)
        slope = p[0]
    else:
        slope = 0
    
    plt.figure(figsize=(8, 6))
    plt.loglog(h, err, 'o-', label=f'Erreur (Pente = {slope:.2f})')
    
    # Guides visuels
    if len(h) > 0:
        plt.loglog(h, err[0]*(h/h[0])**1, '--', color='gray', label='O(h)')
        plt.loglog(h, err[0]*(h/h[0])**2, ':', color='red', label='O(h^2)')
    
    plt.xlabel('Taille de maille (h)')
    plt.ylabel('Erreur Relative vs Mesh Fin')
    plt.title(f'Convergence FEM (Cas Composite)\nPente = {slope:.2f}')
    plt.grid(True, which="both", ls="-")
    plt.legend()
    plt.gca().invert_xaxis()
    plt.savefig(f"{OUTPUT_DIR}/convergence_composite.png")
    print(f">> Graphique sauvegarde : {OUTPUT_DIR}/convergence_composite.png")
    # plt.show() # Décommentez si vous avez un écran graphique

if __name__ == "__main__":
    res = run_study()
    plot_convergence(res)