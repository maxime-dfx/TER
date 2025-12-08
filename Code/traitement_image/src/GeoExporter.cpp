#include "GeoExporter.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

using namespace std;

GeoExporter::GeoExporter(int width, int height, double meshSize)
    : m_width(width), m_height(height), m_lc(meshSize) {}

bool GeoExporter::save(const string& filename, const vector<Fibre>& fibres) {
    ofstream f(filename);
    if (!f.is_open()) {
        cerr << "Erreur : Impossible d'ouvrir " << filename << endl;
        return false;
    }

    cout << "Génération du .geo (Méthode Robuste : Tri par BoundingBox) ..." << endl;

    f << "// --- Fichier généré : Matrice + Fibres (Tri Automatique) ---\n";
    f << "SetFactory(\"OpenCASCADE\");\n\n";

    // --- Paramètres ---
    f << "lc_mat = " << m_lc << ";\n";
    f << "lc_fib = " << (m_lc / 4.0) << ";\n\n";
    
    // Nettoyage géométrique
    f << "Geometry.Tolerance = 1e-6;\n";
    f << "Mesh.CharacteristicLengthMin = 0;\n\n";

    // --- Géométrie de base ---
    // 1. La Matrice Finale
    f << "Rectangle(1) = {0, 0, 0, " << m_width << ", " << m_height << "};\n";
    // 2. L'Outil de Coupe
    f << "Rectangle(2) = {0, 0, 0, " << m_width << ", " << m_height << "};\n\n";

    f << "fibres_finales[] = {};\n\n";

    int start_id = 1000;

    // --- BOUCLE DE DÉCOUPE INDIVIDUELLE ---
    for (size_t i = 0; i < fibres.size(); ++i) {
        double fy = m_height - fibres[i].center.y; 
        int disk_id = start_id + i;

        f << "Disk(" << disk_id << ") = {" 
          << fibres[i].center.x << ", " << fy << ", 0, " 
          << fibres[i].radius << "};\n";
        
        // Coupe sans supprimer l'outil
        f << "temp_cut[] = BooleanIntersection{ Surface{" << disk_id << "}; Delete; }{ Surface{2}; };\n";
        f << "fibres_finales[] += temp_cut[];\n";
    }

    // Suppression de l'outil
    f << "\nRecursive Delete { Surface{2}; }\n";

    // Fusion Finale
    f << "\n// Fusion Finale\n";
    f << "parts[] = BooleanFragments{ Surface{1}; Delete; }{ Surface{fibres_finales[]}; Delete; };\n";
    f << "\nCoherence; RemoveAllDuplicates;\n";

    // =========================================================
    // CORRECTION ICI : TRI INTELLIGENT MATRICE vs FIBRES
    // =========================================================
    f << "\n// --- Identification Automatique (Matrice vs Fibres) ---\n";
    f << "surf_matrice[] = {};\n";
    f << "surf_fibres[] = {};\n\n";
    
    f << "eps = 1e-3; // Tolérance\n";
    f << "W_tot = " << m_width << ";\n";
    f << "H_tot = " << m_height << ";\n\n";

    // On boucle sur toutes les surfaces résultantes
    f << "For k In {0 : #parts[]-1}\n";
    f << "  // On récupère la boite englobante de la surface k\n";
    f << "  bb[] = BoundingBox Surface{ parts[k] };\n";
    f << "  xmin = bb[0]; ymin = bb[1];\n";
    f << "  xmax = bb[3]; ymax = bb[4];\n\n";

    f << "  // Si la surface fait la taille de l'image, c'est la Matrice\n";
    f << "  If ( Abs(xmax - xmin - W_tot) < eps && Abs(ymax - ymin - H_tot) < eps )\n";
    f << "    surf_matrice[] += parts[k];\n";
    f << "  Else\n";
    f << "    surf_fibres[] += parts[k];\n";
    f << "  EndIf\n";
    f << "EndFor\n";

    // --- MAILLAGE & PHYSIQUE (Utilisant les listes triées) ---
    
    f << "\n// Maillage Adaptatif\n";
    f << "MeshSize{ PointsOf{ Surface{surf_matrice[]} } } = lc_mat;\n";
    f << "MeshSize{ PointsOf{ Surface{surf_fibres[]} } } = lc_fib;\n";

    f << "\n// Physical Groups\n";
    f << "Physical Surface(\"Matrice\", 101) = surf_matrice[];\n";
    
    f << "If (#surf_fibres[] > 0)\n";
    f << "  Physical Surface(\"Fibres\", 102) = surf_fibres[];\n";
    f << "  Color Red { Surface{ surf_fibres[] }; }\n";
    f << "EndIf\n";

    f << "Color Blue { Surface{ surf_matrice[] }; }\n";

    f.close();
    return true;
}