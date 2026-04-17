import gmsh
import os

def generer(config, run_dir):
    gmsh.initialize()
    gmsh.option.setNumber("General.Terminal", 0)
    gmsh.model.add("Transverse")

    geom = config.get("geometrie", {})
    L = geom.get("taille_ver_transverse", 20.0)
    rf = geom.get("rayon_fibre", 3.5)
    
    # SÉCURITÉ : Retourne 0.0 si la ligne n'existe pas dans le TOML
    ei = geom.get("epaisseur_interphase", 0.0) 
    c = L / 2.0

    mats = config.get("materiaux", {})
    has_interphase = "interphase" in mats and ei > 0.0

    rect = gmsh.model.occ.addRectangle(0, 0, 0, L, L)

    disks = []
    if has_interphase:
        disks.append((2, gmsh.model.occ.addDisk(c, c, 0, rf + ei, rf + ei)))
    disks.append((2, gmsh.model.occ.addDisk(c, c, 0, rf, rf)))

    outDimTags, outDimTagsMap = gmsh.model.occ.fragment([(2, rect)], disks)
    gmsh.model.occ.synchronize()

    rect_frags = set(tag for dim, tag in outDimTagsMap[0])

    if has_interphase:
        int_frags = set(tag for dim, tag in outDimTagsMap[1])
        fib_frags = set(tag for dim, tag in outDimTagsMap[2])
    else:
        int_frags = set()
        fib_frags = set(tag for dim, tag in outDimTagsMap[1])

    final_fib = fib_frags
    final_int = int_frags - final_fib
    final_mat = rect_frags - final_int - final_fib

    if final_mat and "matrice" in mats: 
        gmsh.model.addPhysicalGroup(2, list(final_mat), mats["matrice"]["label"])
    if final_fib and "fibre" in mats: 
        gmsh.model.addPhysicalGroup(2, list(final_fib), mats["fibre"]["label"])
    if final_int and has_interphase: 
        gmsh.model.addPhysicalGroup(2, list(final_int), mats["interphase"]["label"])

    gmsh.option.setNumber("Mesh.SaveAll", 0)
    gmsh.option.setNumber("Mesh.CharacteristicLengthMin", L / 60)
    gmsh.option.setNumber("Mesh.CharacteristicLengthMax", L / 20)

    gmsh.model.mesh.generate(2)
    output_path = os.path.join(run_dir, "ver_transverse.mesh")
    gmsh.write(output_path)
    gmsh.finalize()
    print(f"      [Gmsh] Maillage Transverse généré (OK)")