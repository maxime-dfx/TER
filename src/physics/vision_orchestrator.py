# src/physics/vision_orchestrator.py
import os
import subprocess

def run_image_processing(config, paths, env, project_root):
    """
    Lance l'outil C++ de sélection de fibres. 
    Retourne le chemin vers le fichier CSV si succès, sinon None.
    """
    if not config["io"].get("utiliser_traitement_image"):
        return None

    img_exe = os.path.join(project_root, "core", "bin", "traitement_img.exe")
    img_src = os.path.join(project_root, config["io"]["chemin_image_microscope"])
    csv_dest = os.path.join(paths["root"], "fibres_reelles.csv")

    print(f"   ➤ [VISION] Traitement de {os.path.basename(img_src)}...")
    
    try:
        subprocess.run([img_exe, img_src, "manuel", csv_dest], check=True, env=env)
        return csv_dest
    except Exception as e:
        print(f"   [!] Erreur Vision : {e}")
        return None