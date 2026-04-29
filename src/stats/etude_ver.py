import os
import sys
import numpy as np
import matplotlib.pyplot as plt
import toml

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.append(PROJECT_ROOT)
from src.stats.common import generer_maillage, ecrire_config, lancer_solveur

SOLVER_BIN = "./core/bin/solver_fe.exe"
OUTPUT_DIR = "data/outputs/ver_analysis"
RESULTS_DIR = "results"
os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(RESULTS_DIR, exist_ok=True)

AVEC_INTERPHASE = "--interphase" in sys.argv

toml_path = os.path.join(PROJECT_ROOT, "etude_master.toml")
config = toml.load(toml_path)

geom = config.get("geometrie", {})
simu = config.get("simulation", {})

VF_CIBLE = geom.get("fraction_volumique", 0.58)
R_FIBRE = geom.get("rayon_fibre", 3.5)
E_INT = geom.get("epaisseur_interphase", 0.5)
NB_SAMPLES = simu.get("nb_echantillons", 100)

MATERIAUX = config.get("materiaux", {})
if not AVEC_INTERPHASE and "interphase" in MATERIAUX:
    del MATERIAUX["interphase"]

R_INT_ACTUEL = (R_FIBRE + E_INT) if AVEC_INTERPHASE else None

titre = "AVEC Interphase" if AVEC_INTERPHASE else "SANS Interphase"
print(f"=== STATISTIQUE VER MÉCANIQUE {titre} ({NB_SAMPLES} échantillons) ===")

results_ex, results_ey = [], []

for i in range(1, NB_SAMPLES + 1):
    print(f"Échantillon {i:03d}/{NB_SAMPLES}...", end=" ", flush=True)
    suffixe = "int" if AVEC_INTERPHASE else "noint"
    mesh_path = os.path.join(OUTPUT_DIR, f"ver_{suffixe}_{i}.mesh")
    config_path = os.path.join(OUTPUT_DIR, f"config_{suffixe}_{i}.toml")
    
    generer_maillage(mesh_path, 100.0, 100.0, R_FIBRE, VF_CIBLE, r_int=R_INT_ACTUEL)
    ecrire_config(config_path, mesh_path, OUTPUT_DIR, dt=0.0, materiaux=MATERIAUX)
    
    res_x = lancer_solveur(SOLVER_BIN, config_path, mode="--tx")
    res_y = lancer_solveur(SOLVER_BIN, config_path, mode="--ty")
    
    if res_x.get("E1") and res_y.get("E2"):
        results_ex.append(res_x["E1"]); results_ey.append(res_y["E2"])
        print(f"OK (Ex={res_x['E1']:.2f}, Ey={res_y['E2']:.2f})")
    else:
        print("ERREUR")

if results_ex:
    all_data = np.concatenate([results_ex, results_ey])
    mean_val, std_val = np.mean(all_data), np.std(all_data)
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
    ax1.hist(all_data, bins=15, density=True, color='skyblue', edgecolor='black', alpha=0.7)
    x = np.linspace(mean_val - 3*std_val, mean_val + 3*std_val, 100)
    ax1.plot(x, 1/(std_val * np.sqrt(2 * np.pi)) * np.exp(-(x - mean_val)**2 / (2 * std_val**2)), color='red')
    ax1.set_title(f'Distribution (Vf={VF_CIBLE*100:.1f}% {titre})'); ax1.grid(alpha=0.3)

    ax2.boxplot([results_ex, results_ey], tick_labels=['Ex', 'Ey'], patch_artist=True, boxprops=dict(facecolor='lightgreen'))
    ax2.set_title('Isotropie transverse (X vs Y)'); ax2.grid(axis='y', alpha=0.3)

    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, f"bilan_stat_ver_{suffixe}.png"))