import os
import sys
import matplotlib.pyplot as plt
import toml

PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.append(PROJECT_ROOT)
from src.stats.common import generer_maillage, ecrire_config, lancer_solveur

SOLVER_BIN = "./core/bin/solver_fe.exe"
OUTPUT_DIR = "data/outputs/parametrics"
RESULTS_DIR = "results"
os.makedirs(OUTPUT_DIR, exist_ok=True)
os.makedirs(RESULTS_DIR, exist_ok=True)

AVEC_INTERPHASE = "--interphase" in sys.argv

toml_path = os.path.join(PROJECT_ROOT, "etude_master.toml")
config = toml.load(toml_path)

geom = config.get("geometrie", {})
R_FIBRE = geom.get("rayon_fibre", 3.5)
E_INT = geom.get("epaisseur_interphase", 0.5)

MATERIAUX = config.get("materiaux", {})
if not AVEC_INTERPHASE and "interphase" in MATERIAUX:
    del MATERIAUX["interphase"]

R_INT_ACTUEL = (R_FIBRE + E_INT) if AVEC_INTERPHASE else None
VF_RANGE = [0.0, 0.05, 0.15, 0.25, 0.35, 0.45, 0.55, 0.65, 0.75, 0.85, 1.0] 

# Modèle analytique 
def get_analytical(vf):
    Em, Ef = MATERIAUX["matrice"]["E"], MATERIAUX["fibre"]["E"]
    num, nuf = MATERIAUX["matrice"]["nu"], MATERIAUX["fibre"]["nu"]
    am, af = MATERIAUX["matrice"]["alpha"], MATERIAUX["fibre"]["alpha"]
    vm = 1.0 - vf
    Ef_ps, Em_ps = Ef / (1.0 - nuf**2), Em / (1.0 - num**2)
    voigt = vf * Ef_ps + vm * Em_ps
    reuss = 1.0 / (vf / Ef_ps + vm / Em_ps) if (vf > 0 and vm > 0) else (Ef_ps if vf == 1.0 else Em_ps)
    a_L = (Ef * af * vf + Em * am * vm) / (Ef * vf + Em * vm)
    nu_L = nuf * vf + num * vm
    schapery = (1.0 + nuf) * af * vf + (1.0 + num) * am * vm - a_L * nu_L
    return voigt, reuss, schapery

vfs, e1_list, e2_list, ax_list, ay_list = [], [], [], [], []
voigt_list, reuss_list, schapery_list = [], [], []

titre = "AVEC Interphase" if AVEC_INTERPHASE else "SANS Interphase"
print(f"=== ÉTUDE PARAMÉTRIQUE (0% -> 100%) {titre} ===")

for target in VF_RANGE:
    print(f"Simulation Vf = {target*100:.1f}%...", end=" ", flush=True)
    suffixe = "int" if AVEC_INTERPHASE else "noint"
    mesh_path = os.path.join(OUTPUT_DIR, f"ver_param_{suffixe}.mesh")
    config_path = os.path.join(OUTPUT_DIR, f"config_param_{suffixe}.toml")
    
    force_lbl = 1 if target == 0.0 else (2 if target == 1.0 else None)
    generer_maillage(mesh_path, 100.0, 100.0, R_FIBRE, target, r_int=R_INT_ACTUEL, force_label=force_lbl)
    ecrire_config(config_path, mesh_path, OUTPUT_DIR, dt=-100.0, materiaux=MATERIAUX)
    
    res = lancer_solveur(SOLVER_BIN, config_path, mode="--all")
    
    if res.get("E1") is not None:
        vf_reel = res["Vf_reel"] if res.get("Vf_reel") else target
        vfs.append(vf_reel); e1_list.append(res["E1"]); e2_list.append(res["E2"])
        ax_list.append(res["AlphaX"]); ay_list.append(res["AlphaY"])
        v, r, s = get_analytical(vf_reel)
        voigt_list.append(v/1000.0); reuss_list.append(r/1000.0); schapery_list.append(s)
        print("OK")
    else:
        print("ERREUR")

if vfs:
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
    
    ax1.plot(vfs, voigt_list, 'g--', label='Voigt (Sup - 2 phases)'); ax1.plot(vfs, reuss_list, 'r--', label='Reuss (Inf - 2 phases)')
    ax1.scatter(vfs, e1_list, color='blue', label='E1 FEM'); ax1.scatter(vfs, e2_list, color='orange', label='E2 FEM')
    ax1.set_xlabel('Fraction Volumique Vf'); ax1.set_ylabel('E (GPa)'); ax1.set_title(titre); ax1.legend(); ax1.grid(True, alpha=0.3)

    ax2.plot(vfs, schapery_list, 'k--', label='Schapery (2 phases)'); ax2.scatter(vfs, ax_list, color='blue', label='AlphaX FEM')
    ax2.scatter(vfs, ay_list, color='orange', label='AlphaY FEM')
    ax2.set_xlabel('Fraction Volumique Vf'); ax2.set_ylabel('Alpha (°C^-1)'); ax2.set_title(titre); ax2.legend(); ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, f"courbes_homogeneisation_limites_{suffixe}.png"))