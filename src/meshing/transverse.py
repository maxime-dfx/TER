# src/meshing/transverse.py
import os
import gmsh
from utils.mesh_exporter import export_medit_correctly
from utils.gmsh_helpers import appliquer_guillotine, assign_physical_groups
from physics.geometry_utils import read_image_and_csv, generer_positions_aleatoires

def generer(config, run_dir, project_root):
    geom = config.get("geometrie", {})
    L = geom.get("taille_ver_transverse", 45.0)
    ep = geom.get("epaisseur_interphase", 0.5)
    
    # 1. ACQUISITION DES DONNÉES
    if config["io"].get("utiliser_traitement_image"):
        csv_path = os.path.join(run_dir, config["io"].get("nom_fichier_csv", "fibres_reelles.csv"))
        img_path = os.path.join(project_root, config["io"].get("chemin_image_microscope", ""))
        positions, rayons = read_image_and_csv(csv_path, img_path, L)
    else:
        rf = geom.get("rayon_fibre", 3.5)
        vf = geom.get("fraction_volumique", 0.58)
        positions = generer_positions_aleatoires(L, rf + ep, vf)
        rayons = [(rf, rf)] * len(positions)

    # 2. INITIALISATION GMSH ET GÉOMÉTRIE DE BASE
    gmsh.initialize()
    gmsh.option.setNumber("General.Terminal", 0)
    gmsh.model.add("VER_Transverse")
    
    rect = gmsh.model.occ.addRectangle(-L/2, -L/2, 0, L, L)
    
    disks = []
    for i, (px, py) in enumerate(positions):
        rx, ry = rayons[i]
        if ep > 0.0: 
            disks.append((2, gmsh.model.occ.addDisk(px, py, 0, rx + ep, ry + ep)))
        disks.append((2, gmsh.model.occ.addDisk(px, py, 0, rx, ry)))

    # 3. TOPOLOGIE BOOLÉENNE ET NETTOYAGE
    gmsh.model.occ.fragment([(2, rect)], disks)
    
    # On coupe ce qui dépasse du carré
    appliquer_guillotine(L)
    gmsh.model.occ.synchronize()
    
    # 4. ASSIGNATION DES MATÉRIAUX (1: Matrice, 2: Fibre, 3: Interphase)
    assign_physical_groups(positions, rayons, ep)
    
    # 5. CONFIGURATION DE LA FINESSE DU MAILLAGE
    finesse = geom.get("facteur_finesse", 1.0)
    
    # Adaptation dynamique de la taille des éléments selon la taille de la boîte (L)
    # et le facteur de finesse demandé par l'utilisateur.
    gmsh.option.setNumber("Mesh.MeshSizeMin", max(0.01, (L / 150.0) / finesse))
    gmsh.option.setNumber("Mesh.MeshSizeMax", max(0.1, (L / 20.0) / finesse))
    
    # Force Gmsh à raffiner automatiquement autour des courbures (les fibres)
    gmsh.option.setNumber("Mesh.MeshSizeFromCurvature", 1)
    
    # 6. GÉNÉRATION ET EXPORT
    gmsh.model.mesh.generate(2)
    
    output_file = os.path.join(run_dir, "ver_transverse.mesh")
    export_medit_correctly(output_file)
    
    gmsh.finalize()