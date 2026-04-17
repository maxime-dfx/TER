import os
import subprocess
import json
import toml

def run_sim(solver_exe, master_config, run_dir, mode, nom_analyse):
    # Formatage aligné pour un rendu console propre
    print(f"      [C++] Lancement {nom_analyse:25} | Mode: {mode:10}...", end="", flush=True)

    json_path = os.path.join(run_dir, "results.json")
    if os.path.exists(json_path):
        os.remove(json_path)

    config_path = _generate_temp_config(master_config, run_dir, mode)

    my_env = os.environ.copy()
    msys_path = r"C:\msys64\mingw64\bin"
    if msys_path not in my_env.get("PATH", ""):
        my_env["PATH"] = my_env.get("PATH", "") + os.pathsep + msys_path

    try:
        res = subprocess.run(
            [os.path.abspath(solver_exe), os.path.abspath(config_path), mode],
            capture_output=True, text=True, env=my_env, cwd=run_dir
        )
        
        if not os.path.exists(json_path):
            print(f" [!] ERREUR")
            if res.stdout: print(f"      --- SORTIE C++ ---\n{res.stdout.strip()}")
            if res.stderr: print(f"      --- ERREUR C++ ---\n{res.stderr.strip()}")
            return None, None

        with open(json_path, 'r') as f:
            data = json.load(f)
        
        module_val = data.get('module', 0.0)
        print(f" OK | Res: {module_val:.2f} GPa")
        return module_val, data.get('poisson', 0.0)

    except Exception as e:
        print(f" [!] Erreur système : {e}")
        return None, None

def _generate_temp_config(master_config, run_dir, mode):
    # Les analyses transverses et cisaillées s'appliquent sur le modèle circulaire
    if mode in ["--ty", "--shear", "--long-shear"]:
        mesh_name = "ver_transverse.mesh"
    else:
        mesh_name = "ver_longitudinal.mesh"
    
    mesh_path = os.path.abspath(os.path.join(run_dir, mesh_name)).replace("\\", "/")
    
    temp_dict = {
        "io": {
            "mesh_file": mesh_path,
            "output_dir": ".", 
            "output_vtk": f"res_{mode.strip('-')}.vtk"
        },
        "simulation": {
            "strain_target": 0.005, 
            "plane_strain": True, 
            "delta_T": 0.0
        },
        "materiaux": master_config.get("materiaux", {})
    }
    
    path = os.path.join(run_dir, f"config_{mode.strip('-')}.toml")
    with open(path, 'w') as f:
        toml.dump(temp_dict, f)
    return path