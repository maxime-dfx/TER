import os
import sys
import numpy as np
import matplotlib.pyplot as plt
import toml

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.append(PROJECT_ROOT)
from src.stats.common import generer_maillage, ecrire_config, lancer_solveur

SOLVER_BIN = "./core/bin/solver_fe.exe"
OUTPUT_DIR = "data/outputs/stats_thermique"
RESULTS_DIR = "results"
os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(RESULTS_DIR, exist_ok=True)

# ==========================================
# LECTURE DYNAMIQUE DE LA CONFIGURATION
# ==========================================
AVEC_INTERPHASE = "--interphase" in sys.argv

toml_path = os.path.join(PROJECT_ROOT, "etude_master.toml")
config = toml.load(toml_path)

geom = config.get("geometrie", {})
simu = config.get("simulation", {})

VF_CIBLE = geom.get("fraction_volumique", 0.58)
R_FIBRE = geom.get("rayon_fibre", 3.5)
E_INT = geom.get("epaisseur_interphase", 0.5)

NB_SAMPLES = simu.get("nb_echantillons", 20)
DELTA_T = simu.get("delta_T", -100.0)

MATERIAUX = config.get("materiaux", {})
if not AVEC_INTERPHASE and "interphase" in MATERIAUX:
    del MATERIAUX["interphase"]

R_INT_ACTUEL = (R_FIBRE + E_INT) if AVEC_INTERPHASE else None

titre = "AVEC Interphase" if AVEC_INTERPHASE else "SANS Interphase"
print(f"=== ÉTUDE THERMIQUE STATISTIQUE {titre} ({NB_SAMPLES} échantillons) ===")

results_x, results_y = [], []

for i in range(NB_SAMPLES):
    suffixe = "int" if AVEC_INTERPHASE else "noint"
    mesh_path = os.path.join(OUTPUT_DIR, f"ver_th_{suffixe}_{i}.mesh")
    config_path = os.path.join(OUTPUT_DIR, f"config_th_{suffixe}_{i}.toml")
    
    # On force L et H à 100.0 µm pour les études statistiques (VER)
    generer_maillage(mesh_path, 100.0, 100.0, R_FIBRE, VF_CIBLE, r_int=R_INT_ACTUEL)
    ecrire_config(config_path, mesh_path, OUTPUT_DIR, dt=DELTA_T, materiaux=MATERIAUX)
    
    res = lancer_solveur(SOLVER_BIN, config_path, mode="--thermo")
    
    if res.get("AlphaX") is not None:
        results_x.append(res["AlphaX"]); results_y.append(res["AlphaY"])
        print(f" Sample {i+1:02d}/{NB_SAMPLES} : AlphaX = {res['AlphaX']:.2e} | AlphaY = {res['AlphaY']:.2e}")
    else:
        print(f" Sample {i+1:02d}/{NB_SAMPLES} : ERREUR Solveur")

if results_x:
    plt.figure(figsize=(12, 5))
    plt.subplot(1, 2, 1)
    plt.hist(results_x, bins=10, alpha=0.5, label='Alpha X', color='blue')
    plt.hist(results_y, bins=10, alpha=0.5, label='Alpha Y', color='red')
    plt.title(f"Distribution des Alphas ({titre})"); plt.legend()

    plt.subplot(1, 2, 2)
    plt.boxplot([results_x, results_y], tick_labels=['Alpha X', 'Alpha Y'])
    plt.title("Dispersion (Boxplot)")

    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, f"bilan_thermique_stats_{suffixe}.png"))