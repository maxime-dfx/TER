# src/solver/mission_control.py
import os
import toml
import copy
from solver.runner import run_cpp_solver
from physics.homogenisation import calculer_matrice_rigidite

def afficher_rapport_analytique(config):
    """Génère le rapport avec ton module d'homogénéisation analytique !"""
    try:
        # 1. Extraction et conversion MPa -> GPa
        E_f = config["materiaux"]["fibre"]["E"] / 1000.0
        E_m = config["materiaux"]["matrice"]["E"] / 1000.0
        nu_f = config["materiaux"]["fibre"]["nu"]
        nu_m = config["materiaux"]["matrice"]["nu"]
        Vf = config["geometrie"]["fraction_volumique"]
        Vm = 1.0 - Vf

        # 2. Lois de comportement de base
        G_f = E_f / (2.0 * (1.0 + nu_f))
        G_m = E_m / (2.0 * (1.0 + nu_m))

        # 3. Micro-mécanique (Voigt & Reuss)
        E1 = Vf * E_f + Vm * E_m
        E2 = 1.0 / (Vf / E_f + Vm / E_m)
        nu12 = Vf * nu_f + Vm * nu_m
        nu23 = Vf * nu_f + Vm * nu_m # Approx isostrain
        G12 = 1.0 / (Vf / G_f + Vm / G_m)

        print("\n" + "="*65)
        print(" 📊 RAPPORT DE MICRO-MÉCANIQUE ANALYTIQUE")
        print("="*65)
        print(" Propriétés des constituants :")
        print(f"  - Fibre   (Vf = {Vf*100:.1f}%) : E = {E_f:6.2f} GPa | nu = {nu_f}")
        print(f"  - Matrice (Vm = {Vm*100:.1f}%) : E = {E_m:6.2f} GPa | nu = {nu_m}")
        print("-" * 65)
        print(" Modèles d'homogénéisation théoriques :")
        print(f"  ➤ E1  (Longitudinal - Voigt) : {E1:6.2f} GPa")
        print(f"  ➤ E2  (Transverse - Reuss)   : {E2:6.2f} GPa")
        print(f"  ➤ G12 (Cisaillement - Reuss) : {G12:6.2f} GPa")
        print(f"  ➤ nu12 (Poisson - Voigt)     : {nu12:6.2f}")
        
        # 4. TON CODE EN ACTION !
        C_th = calculer_matrice_rigidite(E1, E2, nu12, nu23, G12)
        if C_th is not None:
            print("\n  ➤ TENSEUR DE RIGIDITÉ ANALYTIQUE C_th (en GPa)")
            for i in range(6):
                row = "      [ " + "  ".join([f"{val:8.2f}" for val in C_th[i]]) + " ]"
                print(row)
        print("="*65 + "\n")
        
    except KeyError as e:
        print(f"\n [!] Données incomplètes pour l'analytique : {e}\n")


def execute_missions(config, paths, project_root):
    solver_exe = os.path.join(project_root, "core", "bin", "solver_fe.exe")
    analyses = config.get("analyses", {})
    
    active_missions = []
    if analyses.get("calculer_rigidite_elastique"): active_missions.append("elasticite")
    if analyses.get("calculer_thermique"):           active_missions.append("thermique")
    if analyses.get("calculer_pda"):                active_missions.append(f"pda_{analyses.get('pda_load_case', 'traction_y')}")

    for name in active_missions:
        print(f"      -> [SOLVER] Lancement : {name.upper()}")
        
        out_dir = os.path.join(paths["results"], name)
        os.makedirs(out_dir, exist_ok=True)
        
        run_conf = copy.deepcopy(config)
        run_conf["io"]["output_dir"] = out_dir.replace("\\", "/")
        run_conf["analyses"] = {
            "calculer_rigidite_elastique": (name == "elasticite"),
            "calculer_thermique": (name == "thermique"),
            "calculer_pda": name.startswith("pda"),
            "pda_load_case": analyses.get("pda_load_case")
        }

        tmp_toml = os.path.join(paths["root"], f"config_{name}.toml")
        with open(tmp_toml, "w") as f:
            toml.dump(run_conf, f)

        # Le C++ va parler ici !
        results_json = run_cpp_solver(solver_exe, tmp_toml)
        
        # Une fois fini, on affiche le comparatif
        if name == "elasticite":
            afficher_rapport_analytique(config)
            
            if results_json:
                print(" 💻 SYNTHÈSE DES RÉSULTATS FEA (JSON)")
                print("-" * 65)
                for file, data in results_json.items():
                    for key, val in data.items():
                        if isinstance(val, list):
                            print(f"  ➤ {key} :")
                            for row in val:
                                if isinstance(row, list):
                                    print("      [ " + "  ".join([f"{v:8.2f}" for v in row]) + " ]")
                        elif isinstance(val, float):
                            print(f"  ➤ {key:25} : {val:10.2f}")
                        else:
                            print(f"  ➤ {key:25} : {val}")
                print("="*65 + "\n")