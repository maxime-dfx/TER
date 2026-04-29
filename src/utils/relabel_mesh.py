import sys
import os
import math

def relabel_mesh(input_file, fibres_csv, output_file, epaisseur_interphase=0.5):
    """
    Lit un fichier .mesh, calcule le barycentre de chaque élément,
    et lui assigne le bon matériau (1, 2 ou 3) selon sa position par rapport aux fibres.
    """
    # 1. Charger les fibres depuis le CSV (x, y, rayon)
    fibres = []
    if os.path.exists(fibres_csv):
        with open(fibres_csv, 'r') as f:
            for line in f:
                parts = line.strip().split(',')
                if len(parts) >= 3:
                    try:
                        fibres.append((float(parts[0]), float(parts[1]), float(parts[2])))
                    except ValueError:
                        pass
    else:
        print(f"    [!] Avertissement : Fichier {os.path.basename(fibres_csv)} introuvable.")
        print("        Tous les éléments seront assignés au matériau 1 (Matrice).")

    # 2. Lecture du maillage
    with open(input_file, 'r') as f:
        lines = f.readlines()

    vertices = {}
    out_lines = []
    i = 0
    n_lines = len(lines)

    while i < n_lines:
        line = lines[i]
        stripped = line.strip().lower()
        out_lines.append(line)
        
        # Enregistrement des coordonnées des nœuds pour le calcul des distances
        if stripped == 'vertices':
            i += 1
            num_v_line = lines[i]
            out_lines.append(num_v_line)
            num_v = int(num_v_line.strip())
            
            for v_idx in range(1, num_v + 1):
                i += 1
                parts = lines[i].strip().split()
                # Gmsh enregistre : x y z label
                vertices[v_idx] = (float(parts[0]), float(parts[1]))
                out_lines.append(lines[i])
        
        # Traitement des éléments (Triangles / Quads)
        elif stripped in ['triangles', 'quadrilaterals']:
            i += 1
            num_e_line = lines[i]
            out_lines.append(num_e_line)
            num_e = int(num_e_line.strip())
            
            for _ in range(num_e):
                i += 1
                parts = lines[i].strip().split()
                if not parts:
                    out_lines.append("\n")
                    continue
                
                try:
                    # Les N-1 premières valeurs sont les numéros des nœuds
                    node_indices = [int(idx) for idx in parts[:-1]]
                    
                    # Calcul du centre de gravité de l'élément (Barycentre)
                    x_c = sum(vertices[n][0] for n in node_indices) / len(node_indices)
                    y_c = sum(vertices[n][1] for n in node_indices) / len(node_indices)
                    
                    # --- MOTEUR DE LABELLISATION GÉOMÉTRIQUE ---
                    label = 1  # Matrice par défaut
                    
                    for (fx, fy, fr) in fibres:
                        dist = math.hypot(x_c - fx, y_c - fy)
                        if dist <= fr:
                            label = 2  # C'est une Fibre !
                            break
                        elif dist <= fr + epaisseur_interphase:
                            label = 3  # C'est l'Interphase !
                            break
                            
                    # On remplace l'ancien label par le nouveau (1, 2 ou 3)
                    parts[-1] = str(label)
                    
                except Exception:
                    pass
                    
                out_lines.append(" ".join(parts) + "\n")
        i += 1

    # 3. Écriture du nouveau maillage parfait
    with open(output_file, 'w') as f:
        f.writelines(out_lines)


if __name__ == "__main__":
    # Paramétrage exact selon l'erreur de ta console
    if len(sys.argv) < 4:
        print("Usage: python relabel_mesh.py <input.mesh> <fibres.csv> <output.mesh> [epaisseur_interphase]")
        sys.exit(1)
        
    in_mesh = sys.argv[1]
    f_csv = sys.argv[2]
    out_mesh = sys.argv[3]
    epaisseur = float(sys.argv[4]) if len(sys.argv) > 4 else 0.5
    
    try:
        relabel_mesh(in_mesh, f_csv, out_mesh, epaisseur)
    except Exception as e:
        print(f"Erreur critique de relabellisation : {e}")
        sys.exit(1)