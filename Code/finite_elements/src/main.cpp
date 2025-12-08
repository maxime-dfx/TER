#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <limits>
#include <string>
#include <algorithm> // Pour std::max

// Tes headers
#include "Mesh.h"
#include "Material.h"
#include "Solver.h"

using namespace std;

int main(int argc, char** argv) {
    cout << "========================================" << endl;
    cout << "   SIMULATION FEM COMPOSITE (TER)       " << endl;
    cout << "   Traction Uniaxiale 2D (Plane Strain) " << endl;
    cout << "========================================" << endl;

    // Gestion du fichier en argument ou par défaut
    string meshFile = (argc > 1) ? argv[1] : "data/maillage/maillage_17216.mesh";

    try {
        // -------------------------------------------------
        // ETAPE 1 : MAILLAGE & GEOMETRIE
        // -------------------------------------------------
        cout << "\n[1] Chargement du Maillage..." << endl;
        
        Mesh mesh;
        mesh.read(meshFile);
        mesh.printStats();

        // Calcul de la Bounding Box pour identifier les bords
        double xmin = numeric_limits<double>::max();
        double xmax = numeric_limits<double>::lowest();
        double ymin = numeric_limits<double>::max();
        double ymax = numeric_limits<double>::lowest();

        for (const auto& v : mesh.vertices) {
            if (v.x < xmin) xmin = v.x;
            if (v.x > xmax) xmax = v.x;
            if (v.y < ymin) ymin = v.y;
            if (v.y > ymax) ymax = v.y;
        }
        
        double L = xmax - xmin;
        double H = ymax - ymin;
        
        cout << "    -> Dimensions Geometrie : [" << L << " x " << H << "]" << endl;
        cout << "    -> X range : [" << xmin << ", " << xmax << "]" << endl;
        cout << "    -> Y range : [" << ymin << ", " << ymax << "]" << endl;


        // -------------------------------------------------
        // ETAPE 2 : MATERIAUX
        // -------------------------------------------------
        cout << "\n[2] Configuration des Matériaux..." << endl;

        MaterialManager matMgr;
        
        // --- DEFINITION DES PROPRIETES ---
        // Label 101 = Matrice (ex: Epoxy)
        // Label 102 = Fibres  (ex: Carbone)
        // Unités : MPa (N/mm^2)
        
        double E_mat = 3500.0;   double nu_mat = 0.35;
        double E_fib = 230000.0; double nu_fib = 0.20;

        matMgr.addMaterial(101, E_mat, nu_mat);
        matMgr.addMaterial(102, E_fib, nu_fib);
        
        cout << "    -> Matrice (101) : E=" << E_mat << ", nu=" << nu_mat << endl;
        cout << "    -> Fibres  (102) : E=" << E_fib << ", nu=" << nu_fib << endl;


        // -------------------------------------------------
        // ETAPE 3 : INITIALISATION & ASSEMBLAGE
        // -------------------------------------------------
        cout << "\n[3] Initialisation du Solveur..." << endl;
        
        if (mesh.triangles.empty()) throw runtime_error("Maillage vide !");
        
        Solver solver(mesh, matMgr);

        // Lancement de l'assemblage de la matrice K
        solver.assemble();
        
        // Vérification technique
        solver.printSystemInfo();


        // -------------------------------------------------
        // ETAPE 4 : CONDITIONS AUX LIMITES (CL)
        // -------------------------------------------------
        cout << "\n[4] Definition des Conditions aux Limites..." << endl;

        // Paramètres de l'essai de traction
        double epsilon_target = 0.01; // 1% de déformation
        double u_imposed = L * epsilon_target;
        double eps_geom = L * 0.0001; // Tolérance pour attraper les noeuds sur les bords

        // A. Encastrement Glissant à GAUCHE (x = xmin) -> Ux = 0
        // On utilise une expression Lambda [...] pour capturer les variables
        solver.addBoundaryCondition(
            [xmin, eps_geom](double x, double y) { 
                return std::abs(x - xmin) < eps_geom; 
            }, 
            0,   // Direction X (0)
            0.0  // Valeur 0
        );

        // B. Point Pivot Coin BAS-GAUCHE (x = xmin, y = ymin) -> Uy = 0
        // Empêche le mouvement de corps rigide en Y
        solver.addBoundaryCondition(
            [xmin, ymin, eps_geom](double x, double y) { 
                return std::abs(x - xmin) < eps_geom && std::abs(y - ymin) < eps_geom; 
            }, 
            1,   // Direction Y (1)
            0.0  // Valeur 0
        );

        // C. Traction Imposée à DROITE (x = xmax) -> Ux = u_imposed
        solver.addBoundaryCondition(
            [xmax, eps_geom](double x, double y) { 
                return std::abs(x - xmax) < eps_geom; 
            }, 
            0,         // Direction X (0)
            u_imposed  // Valeur imposée
        );

        cout << "    -> CL 1 : Blocage X sur bord gauche." << endl;
        cout << "    -> CL 2 : Blocage Y sur coin bas-gauche." << endl;
        cout << "    -> CL 3 : Traction X = " << u_imposed << " sur bord droit." << endl;


        // -------------------------------------------------
        // ETAPE 5 : RESOLUTION
        // -------------------------------------------------
        cout << "\n[5] Résolution du système K.U = F..." << endl;
        
        solver.solve();


        // -------------------------------------------------
        // ETAPE 6 : RESULTATS SOMMAIRES
        // -------------------------------------------------
        cout << "\n[6] Résultats..." << endl;
        const Eigen::VectorXd& U = solver.getSolution();
        
        double max_ux = -1e9;
        double max_uy = -1e9;

        // On parcourt le vecteur solution pour extraire le max par composante
        for(int i=0; i < (int)mesh.vertices.size(); ++i) {
            double ux = U(2*i);
            double uy = U(2*i+1);
            if(std::abs(ux) > max_ux) max_ux = std::abs(ux);
            if(std::abs(uy) > max_uy) max_uy = std::abs(uy);
        }

        cout << "    -> Déplacement Max |Ux| : " << max_ux << " (Cible: " << u_imposed << ")" << endl;
        cout << "    -> Déplacement Max |Uy| : " << max_uy << " (Effet de Poisson)" << endl;

        cout << "\n========================================" << endl;
        cout << "   CALCUL TERMINE AVEC SUCCES           " << endl;
        cout << "========================================" << endl;

    } catch (const exception& e) {
        cerr << "\n[ERREUR CRITIQUE] " << e.what() << endl;
        return 1;
    }

    return 0;
}