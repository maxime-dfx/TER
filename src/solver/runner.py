# src/solver/runner.py
import subprocess
import sys
import logging
import os
import json
import toml 

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO) # On bloque les DEBUG pour la clarté
formatter = logging.Formatter('%(asctime)s | %(levelname)-7s | %(message)s', datefmt='%H:%M:%S')

console_handler = logging.StreamHandler(sys.stdout)
console_handler.setFormatter(formatter)
logger.addHandler(console_handler)

def run_cpp_solver(executable_path: str, config_file: str):
    if not os.path.exists(executable_path) or not os.path.exists(config_file):
        raise FileNotFoundError("Exécutable ou Fichier introuvable.")

    config = toml.load(config_file)
    output_dir = config.get("io", {}).get("output_dir", "workspace")

    custom_env = os.environ.copy()
    for p in [r"C:\msys64\mingw64\bin", r"C:\msys64\ucrt64\bin", r"C:\MinGW\bin", r"C:\Strawberry\c\bin"]:
        if os.path.exists(p):
            custom_env["PATH"] = p + os.pathsep + custom_env.get("PATH", "")
            break 

    process = subprocess.Popen(
        [executable_path, config_file],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, bufsize=1, env=custom_env 
    )

    for line in process.stdout:
        line = line.strip()
        if not line: continue
        
        if line.startswith("[INFO]"): logger.info(line.split("]", 1)[-1].strip())
        elif line.startswith("[WARN]"): logger.warning(line.split("]", 1)[-1].strip())
        elif line.startswith("[ERROR]"): logger.error(line.split("]", 1)[-1].strip())
        elif line.startswith("[PROG]"):
            sys.stdout.write(f"\r\033[94m[PROG]   \033[0m | {line.split(']', 1)[-1].strip()}")
            sys.stdout.flush()
        elif line.startswith("[DEBUG]"):
            pass # On ignore les debugs internes
        else:
            # === LA CORRECTION EST ICI ===
            # Tout ce qui n'a pas d'étiquette, c'est le std::cout du C++ (Les résultats !)
            clean_line = line.replace("[C++ RAW]", "").strip()
            if clean_line:
                print(f"      {clean_line}")

    process.wait()
    sys.stdout.write("\n")

    if process.returncode != 0:
        raise RuntimeError(f"Échec de la simulation (Code {process.returncode})")

    # -- RECUPERATION DU JSON --
    results_data = {}
    if os.path.exists(output_dir):
        for filename in os.listdir(output_dir):
            if filename.endswith(".json"):
                try:
                    with open(os.path.join(output_dir, filename), 'r', encoding='utf-8') as f:
                        results_data[filename] = json.load(f)
                except Exception:
                    pass
    return results_data