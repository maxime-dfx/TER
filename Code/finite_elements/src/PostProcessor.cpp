#include <fstream>
#include <vector>
#include <string>
#include <iomanip>

// Structures supposées (adapte selon ton code)
struct Node { double x, y; };
struct Element { int n1, n2, n3; }; // Indices des noeuds

void exportToVTK(const std::string& filename, 
                 const std::vector<Node>& nodes, 
                 const std::vector<Element>& elements, 
                 const std::vector<double>& U_global) {
    
    std::ofstream file(filename);

    if (!file.is_open()) {
        // Gestion d'erreur simple
        return; 
    }

    // --- 1. En-tête VTK ---
    file << "# vtk DataFile Version 3.0\n";
    file << "Resultats FEM Traction\n";
    file << "ASCII\n";
    file << "DATASET UNSTRUCTURED_GRID\n";

    // --- 2. Les Noeuds (Géométrie initiale) ---
    file << "POINTS " << nodes.size() << " double\n";
    for (const auto& node : nodes) {
        // On écrit x, y, z (z=0 pour la 2D)
        file << node.x << " " << node.y << " 0.0\n";
    }

    // --- 3. Les Éléments (Connectivité) ---
    // Format : CELLS n_cells size_list
    // size_list = n_cells * (1 chiffre pour le type + 3 indices de noeuds) = n_cells * 4
    file << "CELLS " << elements.size() << " " << elements.size() * 4 << "\n";
    for (const auto& elem : elements) {
        // "3" indique qu'il y a 3 noeuds qui suivent
        file << "3 " << elem.n1 << " " << elem.n2 << " " << elem.n3 << "\n";
    }

    // --- 4. Types de cellules ---
    file << "CELL_TYPES " << elements.size() << "\n";
    for (size_t i = 0; i < elements.size(); ++i) {
        file << "5\n"; // 5 est le code VTK pour un TRIANGLE
    }

    // --- 5. Données aux noeuds (Le résultat) ---
    file << "POINT_DATA " << nodes.size() << "\n";
    file << "VECTORS Deplacement double\n"; // On définit un vecteur 3D

    // On parcourt le vecteur solution U_global [u0, v0, u1, v1...]
    for (size_t i = 0; i < nodes.size(); ++i) {
        double u = U_global[2 * i];     // Déplacement X
        double v = U_global[2 * i + 1]; // Déplacement Y
        // On écrit u, v, w (w=0)
        file << u << " " << v << " 0.0\n";
    }

    file.close();
    // Petit message dans la console pour confirmer
    // std::cout << "Fichier " << filename << " généré avec succès !" << std::endl;
}