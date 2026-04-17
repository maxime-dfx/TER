import gmsh
import os

def generer(config, run_dir):
    gmsh.initialize()
    gmsh.option.setNumber("General.Terminal", 0)
    gmsh.model.add("Longitudinal")

    geom = config.get("geometrie", {})
    L = geom.get("taille_ver_longitudinal", 100.0)
    H = 20.0 
    vf = geom.get("fraction_volumique", 0.58)
    
    # SÉCURITÉ : Retourne 0.0 si la ligne n'existe pas
    ei = geom.get("epaisseur_interphase", 0.0)

    mats = config.get("materiaux", {})
    has_interphase = "interphase" in mats and ei > 0.0

    hf = H * vf
    hi = hf + 2 * ei

    rect = gmsh.model.occ.addRectangle(0, 0, 0, L, H)

    bands = []
    if has_interphase:
        bands.append((2, gmsh.model.occ.addRectangle(0, (H - hi)/2, 0, L, hi)))
    bands.append((2, gmsh.model.occ.addRectangle(0, (H - hf)/2, 0, L, hf)))

    outDimTags, outDimTagsMap = gmsh.model.occ.fragment([(2, rect)], bands)
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
    output_path = os.path.join(run_dir, "ver_longitudinal.mesh")
    gmsh.write(output_path)
    gmsh.finalize()
    print(f"      [Gmsh] Maillage Longitudinal généré (OK)")