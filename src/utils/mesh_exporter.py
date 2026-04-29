# src/utils/mesh_exporter.py
import gmsh

def export_medit_correctly(filename):
    """
    Exporte le maillage actif de Gmsh au format .mesh en forçant les Physical Tags.
    """
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