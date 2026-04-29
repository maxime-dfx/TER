import os

def prepare_geometry_workspace(config, geom_name, project_root):
    """
    Crée l'arborescence : workspace/nom_etude/nom_geometrie/
    Retourne un dictionnaire de chemins absolus.
    """
    study_name = config["projet"]["nom_etude"]
    base_path = os.path.join(project_root, "workspace", study_name, geom_name)
    
    paths = {
        "root": base_path,
        "mesh": os.path.join(base_path, "maillages"),
        "results": os.path.join(base_path, "resultats")
    }
    
    for p in paths.values():
        os.makedirs(p, exist_ok=True)
        
    return paths