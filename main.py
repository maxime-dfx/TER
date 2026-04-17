import os
import sys
import toml

# --- CONFIGURATION DYNAMIQUE DU PATH ---
PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(PROJECT_ROOT, "src"))

import meshing.transverse as mesh_trans
import meshing.longitudinal as mesh_longi
import solver.runner as runner
import physics.homogenisation as physics

def setup_workspace(config):
    """Crée le dossier de travail dynamiquement."""
    run_dir = os.path.join(PROJECT_ROOT, config["projet"]["dossier_sortie"], config["projet"]["nom_etude"])
    os.makedirs(run_dir, exist_ok=True)
    return run_dir

def main():
    print("======================================================")
    print("   ORCHESTRATEUR D'HOMOGÉNÉISATION MULTI-ÉCHELLES 3D  ")
    print("   Convention : Fibres orientées selon l'axe Z        ")
    print("======================================================")

    # 1. Chargement de la configuration
    toml_path = os.path.join(PROJECT_ROOT, "etude_master.toml")
    if not os.path.exists(toml_path):
        print(f"\n[!] Erreur : {toml_path} introuvable.")
        return
    config = toml.load(toml_path)

    # 2. Setup Environnement
    run_dir = setup_workspace(config)
    solver_exe = os.path.join(PROJECT_ROOT, "core", "bin", "solver_fe.exe")

    print(f"-> Espace d'étude   : {run_dir}")
    print(f"-> Solveur C++      : {solver_exe}")
    print("------------------------------------------------------\n")

    plan = config.get("plan_experiences", {})

    # 3. ÉTAPE MAILLAGE
    if plan.get("generer_maillage_transverse", False):
        mesh_trans.generer(config, run_dir)

    if plan.get("generer_maillage_longitudinal", False):
        mesh_longi.generer(config, run_dir)

    # 4. ÉTAPE RÉSOLUTION (Stratégie de capture des constantes)
    if plan.get("run_mecanique", False):
        print("\n➤ [ÉTAPE] Lancement des solveurs FEA...")

        # --- A. PROPRIÉTÉS LONGITUDINALES (Sur le modèle en BANDES) ---
        # Ici, le solver tire sur les bandes : on récupère E longitudinal et le Poisson associé
        E1, nu12 = runner.run_sim(solver_exe, config, run_dir, "--tx", "Longitudinal (E1, nu12)")

        # --- B. PROPRIÉTÉS TRANSVERSES (Sur le modèle à CERCLES) ---
        # On tire perpendiculairement à la fibre : on récupère E transverse et Poisson transverse
        E2, nu23 = runner.run_sim(solver_exe, config, run_dir, "--ty", "Transverse (E2, nu23)")

        # --- C. CISAILLEMENT LONGITUDINAL (Sur le modèle à CERCLES) ---
        print("\n➤ [ÉTAPE] Calcul du module de cisaillement longitudinal G12 (Anti-Plan)...")
        
        # 1. On s'assure que la section 'io' existe pour éviter le KeyError
        if "io" not in config:
            config["io"] = {}
        
        # 2. On force le solveur à utiliser le maillage TRANSVERSE (cercles) 
        #    généré précédemment dans le dossier de l'étude.
        config["io"]["mesh_file"] = os.path.join(run_dir, "ver_transverse.mesh")

        # 3. Lancement de la simulation avec le nouveau flag
        G12, _ = runner.run_sim(solver_exe, config, run_dir, "--long-shear", "Transverse (G12)")

        # 5. SYNTHÈSE ET CALCUL TENSORIEL
        print("\n➤ [ÉTAPE] Synthèse tensorielle (Convention Fibres en Z)...")
        C_eff = physics.calculer_matrice_rigidite(E1, E2, nu12, nu23, G12)
        
        if C_eff is not None:
            physics.formater_resultats(C_eff)

    print("\n======================================================")
    print("                    FIN DE L'ÉTUDE                    ")
    print("======================================================")

if __name__ == "__main__":
    main()