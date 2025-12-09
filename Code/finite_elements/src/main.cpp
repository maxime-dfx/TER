/**
 * ============================================================================
 * MAIN.CPP - SIMULATION FEM COMPOSITE (ANIMATION MULTI-FICHIERS)
 * ============================================================================
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <limits>
#include <string>
#include <algorithm>
#include <fstream> 
#include <locale>
#include <sys/stat.h>
#include <sys/types.h>

// Headers personnels
#include "Mesh.h"
#include "Material.h"
#include "Solver.h"

using namespace std;

// Fonction utilitaire pour créer un dossier (portable)
bool createDirectory(const string& path) {
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

int main(int argc, char** argv) {
    setlocale(LC_NUMERIC, "C");

    cout << "========================================" << endl;
    cout << "   SIMULATION FEM COMPOSITE (TER)       " << endl;
    cout << "========================================" << endl;

    string meshFile = (argc > 1) ? argv[1] : "data/maillage/maillage_17216.mesh";

    try {
        // [1] MAILLAGE
        cout << "\n[1] Chargement du Maillage..." << endl;
        Mesh mesh;
        mesh.read(meshFile);
        
        if(mesh.vertices.empty() || mesh.triangles.empty()) {
            throw runtime_error("Maillage vide ou invalide!");
        }
        
        // Calcul des bornes
        double xmin = numeric_limits<double>::max();
        double xmax = numeric_limits<double>::lowest();
        double ymin = numeric_limits<double>::max();
        double ymax = numeric_limits<double>::lowest();
        
        for (const auto& v : mesh.vertices) {
            xmin = min(xmin, v.x);
            xmax = max(xmax, v.x);
            ymin = min(ymin, v.y);
            ymax = max(ymax, v.y);
        }
        
        double L = xmax - xmin;
        
        if(L < 1e-10) {
            throw runtime_error("Dimension du maillage invalide (L ≈ 0)");
        }
        
        cout << "   Dimensions: [" << xmin << ", " << xmax << "] x [" 
             << ymin << ", " << ymax << "]" << endl;
        cout << "   Longueur caractéristique L = " << L << endl;

        // [2] MATERIAUX
        cout << "\n[2] Propriétés matériaux..." << endl;
        MaterialManager matMgr;
        matMgr.addMaterial(101, 3500.0, 0.35);    // Matrice
        matMgr.addMaterial(102, 230000.0, 0.20);  // Fibre

        // [3] ASSEMBLAGE
        cout << "\n[3] Assemblage..." << endl;
        Solver solver(mesh, matMgr);
        solver.assemble();
        solver.printSystemInfo();

        // [4] CONDITIONS AUX LIMITES
        cout << "\n[4] Conditions aux Limites..." << endl;
        double u_imposed = L * 0.01; 
        double eps_geom = L * 0.0001;

        cout << "   - Bord gauche (x=" << xmin << "): Ux = 0" << endl;
        cout << "   - Coin bas-gauche: Uy = 0" << endl;
        cout << "   - Bord droit (x=" << xmax << "): Ux = " << u_imposed << endl;

        solver.addBoundaryCondition(
            [xmin, eps_geom](double x, double y) { 
                return std::abs(x - xmin) < eps_geom; 
            }, 0, 0.0
        );
        
        solver.addBoundaryCondition(
            [xmin, ymin, eps_geom](double x, double y) { 
                return std::abs(x - xmin) < eps_geom && 
                       std::abs(y - ymin) < eps_geom; 
            }, 1, 0.0
        );
        
        solver.addBoundaryCondition(
            [xmax, eps_geom](double x, double y) { 
                return std::abs(x - xmax) < eps_geom; 
            }, 0, u_imposed
        );

        // [5] RESOLUTION
        cout << "\n[5] Résolution..." << endl;
        solver.solve();

        // [6] GENERATION FICHIERS ANIMATION
        cout << "\n[6] Génération des fichiers d'animation..." << endl;
        
        // Création du dossier de sortie
        string outputDir = "data/resultats/data";
        cout << "   Création du dossier: " << outputDir << endl;
        
        if(!createDirectory("data")) {
            cerr << "   Attention: Impossible de créer 'data/'" << endl;
        }
        if(!createDirectory("data/resultats")) {
            cerr << "   Attention: Impossible de créer 'data/resultats/'" << endl;
        }
        if(!createDirectory(outputDir)) {
            cerr << "   Attention: Impossible de créer '" << outputDir << "'" << endl;
        }
        
        const Eigen::VectorXd& U = solver.getSolution();
        
        int nb_frames = 50;
        double amp_visuelle = 1.0;
        
        cout << "   Paramètres: " << nb_frames << " frames, amplification = " 
             << amp_visuelle << endl;
        
        int success_count = 0;
        for (int frame = 0; frame <= nb_frames; ++frame) {
            string filename = outputDir + "/anim_" + to_string(frame) + ".txt";
            ofstream out(filename);
            
            if (!out.is_open()) {
                cerr << "   [ERREUR] Impossible de créer " << filename << endl;
                continue;
            }

            double t = (double)frame / nb_frames;

            for (const auto& tri : mesh.triangles) {
                int nodes[4] = {tri.v[0], tri.v[1], tri.v[2], tri.v[0]};
                
                for (int k = 0; k < 4; ++k) {
                    int idx = nodes[k];
                    
                    // Vérification de sécurité
                    if(idx < 0 || idx >= (int)mesh.vertices.size()) {
                        cerr << "   [ERREUR] Indice invalide: " << idx << endl;
                        continue;
                    }
                    
                    double x_def = mesh.vertices[idx].x + U(2*idx) * t * amp_visuelle;
                    double y_def = mesh.vertices[idx].y + U(2*idx+1) * t * amp_visuelle;
                    
                    out << x_def << " " << y_def << "\n";
                }
                out << "\n";
            }
            
            out.close();
            success_count++;
            
            if (frame % 10 == 0 || frame == nb_frames) {
                cout << "   -> Frame " << frame << "/" << nb_frames 
                     << " générée." << endl;
            }
        }
        
        cout << "   Total: " << success_count << " fichiers créés avec succès." << endl;

        cout << "\n========================================" << endl;
        cout << "   CALCUL TERMINE                       " << endl;
        cout << "========================================" << endl;

    } catch (const exception& e) {
        cerr << "\n[ERREUR CRITIQUE] " << e.what() << endl;
        return 1;
    }

    return 0;
}