# src/utils/gmsh_helpers.py
import gmsh
import math

def appliquer_guillotine(L):
    """Supprime proprement tout ce qui dépasse du carré L x L."""
    surfaces = gmsh.model.occ.getEntities(2)
    to_remove = [
        (dim, tag) for dim, tag in surfaces
        if abs(gmsh.model.occ.getCenterOfMass(dim, tag)[0]) > L/2 + 1e-4 or 
           abs(gmsh.model.occ.getCenterOfMass(dim, tag)[1]) > L/2 + 1e-4
    ]
    if to_remove:
        gmsh.model.occ.remove(to_remove, recursive=True)

def assign_physical_groups(positions, rayons, ep):
    """Détecte et assigne les tags (1:Matrice, 2:Fibre, 3:Interphase)."""
    surfaces = gmsh.model.getEntities(2)
    tags = {1: [], 2: [], 3: []}

    # Calcul du diamètre maximal d'une fibre pour notre filtre de sécurité
    max_diam = 0
    if rayons:
        max_diam = max(max(rx, ry) for rx, ry in rayons) * 2.0

    for _, tag in surfaces:
        com = gmsh.model.occ.getCenterOfMass(2, tag)
        label = 1 # Matrice par défaut
        
        # 1. Test classique du centre de gravité
        for i, (px, py) in enumerate(positions):
            dist = math.hypot(com[0] - px, com[1] - py)
            r_max = max(rayons[i])
            if dist <= r_max + 1e-4:
                label = 2
                break
            elif dist <= (r_max + ep) + 1e-4:
                label = 3
                break
        
        # 2. FILTRE DE SÉCURITÉ : La Bounding Box
        # On calcule l'encombrement (largeur/hauteur) de la surface
        xmin, ymin, zmin, xmax, ymax, zmax = gmsh.model.occ.getBoundingBox(2, tag)
        width = xmax - xmin
        height = ymax - ymin
        
        # Si la surface est plus large qu'une fibre (+ l'interphase et une marge),
        # c'est physiquement impossible que ce soit une fibre. C'est la matrice !
        if width > (max_diam + 2.0 * ep + 1.0) or height > (max_diam + 2.0 * ep + 1.0):
            label = 1
            
        tags[label].append(tag)

    # Assigner les Physical Groups finaux
    for label_id, tag_list in tags.items():
        if tag_list:
            gmsh.model.addPhysicalGroup(2, tag_list, label_id)