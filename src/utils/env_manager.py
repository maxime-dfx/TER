import os

def setup_mingw_paths():
    """
    Configure l'environnement système pour inclure les binaires MinGW/MSYS2.
    Retourne un dictionnaire d'environnement prêt pour subprocess.
    """
    custom_env = os.environ.copy()
    
    # Liste ordonnée des chemins prioritaires pour les DLL (OpenCV, etc.)
    possible_paths = [
        r"C:\msys64\mingw64\bin",
        r"C:\msys64\ucrt64\bin",
        r"C:\MinGW\bin",
        r"C:\Strawberry\c\bin"
    ]
    
    for path in possible_paths:
        if os.path.exists(path):
            # On injecte au DÉBUT du PATH pour être prioritaire
            custom_env["PATH"] = path + os.pathsep + custom_env.get("PATH", "")
            return custom_env
            
    return custom_env