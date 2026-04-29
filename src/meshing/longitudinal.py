import gmsh
import os
import csv

def export_medit_correctly(filename):
    """Bypass le bug de Gmsh en forçant l'écriture des Physical Tags dans le .mesh"""
    nodeTags, nodeCoords, _ = gmsh.model.mesh.getNodes()
    if not len(nodeTags): return
    
    node_map = {tag: i+1 for i, tag in enumerate(nodeTags)}
    triangles = []
    phys_groups = gmsh.model.getPhysicalGroups(2)
    
    for dim, p_tag in phys_groups:
        entities = gmsh.model.getEntitiesForPhysicalGroup(dim, p_tag)
        for ent in entities:
            elemTypes, elemTags, elemNodeTags = gmsh.model.mesh.getElements(dim, ent)
            for i, eType in enumerate(elemTypes):
                if eType == 2:
                    nodes = elemNodeTags[i]
                    for j in range(0, len(nodes), 3):
                        n1, n2, n3 = node_map[nodes[j]], node_map[nodes[j+1]], node_map[nodes[j+2]]
                        triangles.append(f"{n1} {n2} {n3} {p_tag}")
                        
    with open(filename, "w") as f:
        f.write("MeshVersionFormatted 1\nDimension 2\n")
        f.write(f"Vertices\n{len(nodeTags)}\n")
        for i in range(len(nodeTags)):
            f.write(f"{nodeCoords[i*3]} {nodeCoords[i*3+1]} 0\n")
        f.write(f"Triangles\n{len(triangles)}\n")
        f.write("\n".join(triangles) + "\n")
        f.write("End\n")

def generer(config, run_dir):
    geom = config.get("geometrie", {})
    L = geom.get("taille_ver_longitudinal", 100.0)
    H = 20.0
    vf = geom.get("fraction_volumique", 0.58)
    ei = geom.get("epaisseur_interphase", 0.0)

    hf = H * vf
    hi = hf + 2 * ei

    geo_path = os.path.join(run_dir, "ver_longitudinal.geo")
    geo = ['SetFactory("Built-in");']

    if ei > 0.0:
        y_vals = [0, (H-hi)/2, (H-hf)/2, (H+hf)/2, (H+hi)/2, H]
        mats = [1, 3, 2, 3, 1]
    else:
        y_vals = [0, (H-hf)/2, (H+hf)/2, H]
        mats = [1, 2, 1]

    p_id = 1; l_id = 1; s_id = 1
    horiz_lines = []

    for y in y_vals:
        pL = p_id; pR = p_id+1; p_id+=2
        geo.append(f'Point({pL}) = {{0, {y}, 0}};')
        geo.append(f'Point({pR}) = {{{L}, {y}, 0}};')
        lH = l_id; l_id+=1
        geo.append(f'Line({lH}) = {{{pL}, {pR}}};')
        horiz_lines.append((pL, pR, lH))

    surfaces_by_mat = {1: [], 2: [], 3: []}

    for i in range(len(y_vals)-1):
        pL_bot, pR_bot, lH_bot = horiz_lines[i]
        pL_top, pR_top, lH_top = horiz_lines[i+1]

        lV_R = l_id; l_id+=1
        geo.append(f'Line({lV_R}) = {{{pR_bot}, {pR_top}}};')
        lV_L = l_id; l_id+=1
        geo.append(f'Line({lV_L}) = {{{pL_top}, {pL_bot}}};')

        loop = l_id; l_id+=1
        geo.append(f'Curve Loop({loop}) = {{{lH_bot}, {lV_R}, -{lH_top}, {lV_L}}};')

        surf = s_id; s_id+=1
        geo.append(f'Plane Surface({surf}) = {{{loop}}};')
        surfaces_by_mat[mats[i]].append(surf)

    for mat_id, surfs in surfaces_by_mat.items():
        if surfs:
            geo.append(f'Physical Surface({mat_id}) = {{{", ".join(map(str, surfs))}}};')

    geo.append(f'Mesh.MeshSizeMin = {H/40.0};')
    geo.append(f'Mesh.MeshSizeMax = {H/15.0};')

    with open(geo_path, "w") as f:
        f.write("\n".join(geo))

    gmsh.initialize()
    gmsh.option.setNumber("General.Terminal", 0)
    gmsh.open(geo_path)
    gmsh.model.mesh.generate(2)
    
    # APPEL DE L'EXPORTATEUR CORRECTIF
    export_medit_correctly(os.path.join(run_dir, "ver_longitudinal.mesh"))
    
    gmsh.finalize()
    print("      [Gmsh] Maillage Longitudinal (.geo) généré avec labels stricts (1,2,3).")