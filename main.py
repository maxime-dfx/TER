# main.py
import os
import sys
import toml
import shutil

PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(PROJECT_ROOT, "src"))

from utils.env_manager import setup_mingw_paths
from utils.workspace import prepare_geometry_workspace
from physics.vision_orchestrator import run_image_processing
from solver.mission_control import execute_missions
import meshing.transverse as mesh_trans

def run_branch(config, geom_name, env):
    """Exécute le pipeline complet pour une branche donnée."""
    print(f"\n⚙️  TRAITEMENT DE LA BRANCHE : {geom_name.upper()}")
    paths = prepare_geometry_workspace(config, geom_name, PROJECT_ROOT)
    
    # 1. Vision (si transverse)
    if geom_name == "transverse":
        csv = run_image_processing(config, paths, env, PROJECT_ROOT)
        if csv: config["io"]["nom_fichier_csv"] = csv

    # 2. Maillage
    print(f"   ➤ [MAILLAGE] Génération Gmsh...")
    mesh_trans.generer(config, paths["root"], PROJECT_ROOT)
    
    # Rangement du maillage
    m_src = os.path.join(paths["root"], "ver_transverse.mesh")
    m_dst = os.path.join(paths["mesh"], "ver_transverse.mesh")
    if os.path.exists(m_src):
        shutil.move(m_src, m_dst)
        config["io"]["mesh_file"] = m_dst.replace("\\", "/")

    # 3. Solver
    if config["plan_experiences"].get("run_mecanique"):
        print(f"   ➤ [SOLVER] Exécution des missions...")
        execute_missions(config, paths, PROJECT_ROOT)

def main():
    config_master = toml.load(os.path.join(PROJECT_ROOT, "etude_master.toml"))
    env = setup_mingw_paths()
    
    # Détection automatique des branches à lancer
    plan = config_master["plan_experiences"]
    branches = []
    if plan.get("generer_maillage_transverse"):  branches.append("transverse")
    if plan.get("generer_maillage_longitudinal"): branches.append("longitudinal")
    if config_master["io"].get("utiliser_maillage_manuel"): branches = ["manuel"]

    for b in branches:
        run_branch(config_master, b, env)

    print("\n" + "="*60 + f"\n FIN DE L'ÉTUDE : {config_master['projet']['nom_etude']}\n" + "="*60)

if __name__ == "__main__":
    main()