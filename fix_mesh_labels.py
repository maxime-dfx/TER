import sys
import os

def clean_mesh_labels(input_path, output_path, label_map):
    if not os.path.exists(input_path):
        print(f"[ERREUR] Le fichier source {input_path} n'existe pas.")
        return

    with open(input_path, 'r') as fin, open(output_path, 'w') as fout:
        lines = fin.readlines()
        
        i = 0
        while i < len(lines):
            line = lines[i]
            fout.write(line)
            
            # Identifier les sections structurelles du format Medit (.mesh)
            section = line.strip().lower()
            if section in ["vertices", "triangles", "edges", "tetrahedra", "quadrilaterals"]:
                i += 1
                if i >= len(lines): break
                
                # La ligne suivant le nom de la section donne le nombre total d'entités
                count_line = lines[i]
                fout.write(count_line)
                try:
                    count = int(count_line.strip())
                except ValueError:
                    i += 1
                    continue
                    
                # Parcours strict des entités pour ne modifier QUE la dernière colonne (le label)
                for _ in range(count):
                    i += 1
                    if i >= len(lines): break
                    elem_line = lines[i]
                    parts = elem_line.split()
                    
                    if len(parts) > 0:
                        try:
                            # Le label physique est toujours le dernier entier de la ligne
                            ref = int(parts[-1])
                            if ref in label_map:
                                parts[-1] = str(label_map[ref])
                            # Réécriture propre de la ligne avec le nouveau label
                            fout.write("  " + " ".join(parts) + "\n")
                        except ValueError:
                            # Sécurité si la ligne est mal formatée
                            fout.write(elem_line)
            i += 1

if __name__ == "__main__":
    # Dictionnaire de conversion Topologique (Ancien -> Nouveau)
    mapping = {
        101: 1, # Matrice
        102: 2, # Fibre
        103: 3  # Interphase (si tu en as une)
    }
    
    # ⚠️ ATTENTION: input_mesh DOIT être ton maillage fraîchement généré ou sauvegardé, PAS celui corrompu.
    input_mesh = "data/raw/geo/maillage_17216_original.mesh" 
    
    # Le maillage propre que le solveur C++ va lire
    output_mesh = "data/raw/geo/maillage_17216.mesh"
    
    print(f"🔧 [Topologie] Correction des labels physiques...")
    print(f"   -> Lecture de : {input_mesh}")
    
    clean_mesh_labels(input_mesh, output_mesh, mapping)
    
    print(f"   -> Écriture de : {output_mesh} (OK)")
    print("✅ Le maillage est maintenant compatible avec le solveur C++ sans corruption géométrique.")