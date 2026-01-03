/**
 * ============================================================================
 * MAIN.CPP - ESSAI DE TRACTION VIRTUEL (FEM COMPOSITES)
 * ============================================================================
 * Description : Simulation d'un essai de traction sur un VER composite.
 * Calcul E, Nu, Export Heatmap Gnuplot et Export VTK (Paraview).
 * Ajout : Calcul automatique des bornes de Voigt et Reuss.
 * ============================================================================
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <string>
#include <sys/stat.h>
#include <fstream>  
#include <functional>

// --- INCLUSION EIGEN ---
#include <Eigen/Dense>
using namespace Eigen;

// --- INCLUSION LOCALE ---
#include "Mesh.h"
#include "Material.h"
#include "Solver.h"

using namespace std;

// ============================================================================
// CONFIGURATION & CONSTANTES
// ============================================================================
const int LABEL_MATRICE = 101;
const int LABEL_FIBRE   = 102;

// Propriétés Matériaux (En MPa)
const double E_MATRICE  = 257000.0;  // 257 GPa
const double NU_MATRICE = 0.33;
const double E_FIBRE    = 311000.0;  // 311 GPa
const double NU_FIBRE   = 0.19;

// Dossiers de sortie
const string OUTPUT_DIR = "data/resultats/data";

// ============================================================================
// STRUCTURES & HELPERS
// ============================================================================

// --- Gestion des dossiers (Compatible Windows/Linux) ---
void ensureDir(const string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == -1) {
        #if defined(_WIN32)
            mkdir(path.c_str());
        #else 
            mkdir(path.c_str(), 0755);
        #endif
    }
}

// --- Infos Géométriques (Bounding Box) ---
struct BoundingBox {
    double xmin=1e9, xmax=-1e9, ymin=1e9, ymax=-1e9;
    double L, H;

    void compute(const Mesh& mesh) {
        for (const auto& v : mesh.vertices) {
            if(v.x < xmin) xmin = v.x;
            if(v.x > xmax) xmax = v.x;
            if(v.y < ymin) ymin = v.y;
            if(v.y > ymax) ymax = v.y;
        }
        L = xmax - xmin;
        H = ymax - ymin;
    }
};

// --- Calcul d'aire Triangle ---
double getArea(const Triangle& tri, const Mesh& mesh) {
    const Vertex& p0 = mesh.vertices[tri.v[0]];
    const Vertex& p1 = mesh.vertices[tri.v[1]];
    const Vertex& p2 = mesh.vertices[tri.v[2]];
    return 0.5 * std::abs((p1.x - p0.x)*(p2.y - p0.y) - (p2.x - p0.x)*(p1.y - p0.y));
}

// --- Analyse Taux de Fibre (Vf) - Modifié pour retourner la valeur ---
double analyzeMicrostructure(const Mesh& mesh) {
    double aire_totale = 0.0;
    double aire_fibre = 0.0;
    
    for(const auto& tri : mesh.triangles) {
        double a = getArea(tri, mesh);
        aire_totale += a;
        if(tri.label == LABEL_FIBRE) aire_fibre += a;
    }

    double Vf = (aire_fibre / aire_totale);

    cout << " [MICROSTRUCTURE]" << endl;
    cout << "  -> Aire Totale : " << aire_totale << endl;
    cout << "  -> Taux de fibre (Vf) : " << fixed << setprecision(2) 
         << Vf * 100.0 << " %" << endl;
    cout << "---------------------------------------------------" << endl;

    return Vf;
}

// ============================================================================
// EXPORT VTK (PARAVIEW)
// ============================================================================
void exportToVTK(const string& filename, const Mesh& mesh, const VectorXd& U) {
    ofstream file(filename);
    if (!file.is_open()) { cerr << "Erreur: Impossible de creer " << filename << endl; return; }

    file << "# vtk DataFile Version 3.0\nResultats FEM Traction\nASCII\nDATASET UNSTRUCTURED_GRID\n";

    // Noeuds
    file << "POINTS " << mesh.vertices.size() << " double\n";
    for (const auto& node : mesh.vertices) file << node.x << " " << node.y << " 0.0\n";

    // Éléments
    file << "CELLS " << mesh.triangles.size() << " " << mesh.triangles.size() * 4 << "\n";
    for (const auto& tri : mesh.triangles) file << "3 " << tri.v[0] << " " << tri.v[1] << " " << tri.v[2] << "\n";

    // Types
    file << "CELL_TYPES " << mesh.triangles.size() << "\n";
    for (size_t i = 0; i < mesh.triangles.size(); ++i) file << "5\n"; 

    // Déplacement
    file << "POINT_DATA " << mesh.vertices.size() << "\n";
    file << "VECTORS Deplacement double\n";
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        file << U(2 * i) << " " << U(2 * i + 1) << " 0.0\n";
    }

    // Matériaux
    file << "CELL_DATA " << mesh.triangles.size() << "\n";
    file << "SCALARS MaterialID int 1\nLOOKUP_TABLE default\n";
    for (const auto& tri : mesh.triangles) file << tri.label << "\n";

    file.close();
    cout << "Fichier VTK genere : " << filename << endl;
}

// ============================================================================
// EXPORT POUR GNUPLOT
// ============================================================================
void exportFieldData(const string& filename, const Mesh& mesh, const VectorXd& U) {
    ofstream file(filename);
    if (!file.is_open()) return;
    file << "# X Y Ux Uy Magnitude\n";
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        double ux = U(2 * i);
        double uy = U(2 * i + 1);
        file << mesh.vertices[i].x << " " << mesh.vertices[i].y << " " << ux << " " << uy << " " << sqrt(ux*ux+uy*uy) << "\n";
    }
    file.close();
}

// ============================================================================
// CALCULS PHYSIQUES (Post-Processing)
// ============================================================================
double computeHomogenizedStress(const Mesh& mesh, const VectorXd& U, const MaterialManager& matMgr) {
    double sigma_integral = 0.0;
    double volume_total = 0.0;

    for(const auto& tri : mesh.triangles) {
        double D_arr[3][3];
        matMgr.getHookeMatrix(tri.label, D_arr, false); // false = Plane Strain
        Matrix3d D; for(int i=0;i<3;i++) for(int j=0;j<3;j++) D(i,j)=D_arr[i][j];

        const Vertex& p0 = mesh.vertices[tri.v[0]];
        const Vertex& p1 = mesh.vertices[tri.v[1]];
        const Vertex& p2 = mesh.vertices[tri.v[2]];
        double area = getArea(tri, mesh);
        
        double b[3] = {p1.y - p2.y, p2.y - p0.y, p0.y - p1.y};
        double c[3] = {p2.x - p1.x, p0.x - p2.x, p1.x - p0.x};

        Matrix<double, 3, 6> B; B.setZero();
        for(int i=0; i<3; ++i) {
            B(0, 2*i)=b[i]; B(1, 2*i+1)=c[i]; B(2, 2*i)=c[i]; B(2, 2*i+1)=b[i];
        }
        double detJ = (p1.x - p0.x)*(p2.y - p0.y) - (p2.x - p0.x)*(p1.y - p0.y);
        B *= ((detJ > 0 ? 1.0 : -1.0) / (2.0 * area));

        Matrix<double, 6, 1> u_el;
        for(int i=0; i<3; ++i) { u_el(2*i)=U(2*tri.v[i]); u_el(2*i+1)=U(2*tri.v[i]+1); }

        Vector3d sigma = D * B * u_el;
        sigma_integral += sigma(0) * area; 
        volume_total += area;
    }
    return sigma_integral / volume_total;
}

double computeTransverseStrain(const Mesh& mesh, const VectorXd& U, double meshHeight) {
    double sumUy = 0.0;
    int count = 0;
    const double eps = 1e-4;
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        if (std::abs(mesh.vertices[i].y - meshHeight) < eps) {
            sumUy += U(i * 2 + 1); 
            count++;
        }
    }
    return (count == 0) ? 0.0 : (sumUy / count) / meshHeight; 
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    setlocale(LC_NUMERIC, "C"); 
    cout << "\n===================================================" << endl;
    cout << "   ESSAI DE TRACTION VIRTUEL (E & NU EFFECTIFS)    " << endl;
    cout << "===================================================" << endl;

    string meshFile = (argc > 1) ? argv[1] : "data/maillage/maillage_17216.mesh";
    
    ensureDir("data"); ensureDir("data/resultats"); ensureDir("data/resultats/data"); ensureDir("data/resultats/graphique");

    try {
        // [1] INITIALISATION
        Mesh mesh;
        mesh.read(meshFile);
        BoundingBox bbox; bbox.compute(mesh);
        
        // Calcul du taux de fibre pour Voigt/Reuss
        double Vf = analyzeMicrostructure(mesh);

        MaterialManager matMgr;
        matMgr.addMaterial(LABEL_MATRICE, E_MATRICE, NU_MATRICE);    
        matMgr.addMaterial(LABEL_FIBRE,   E_FIBRE,   NU_FIBRE);   

        // [2] SIMULATION FEM
        int n_steps = 1; // Un seul pas suffit pour l'elasticite lineaire
        double max_strain_x = 0.001; // 0.1% strain
        
        Solver solver(mesh, matMgr);
        double imposed_ux = bbox.L * max_strain_x;

        // CL: Bloqué X à gauche
        solver.addBoundaryCondition([&bbox](double x, double){ return abs(x - bbox.xmin) < 1e-6; }, 0, 0.0);
        // CL: Traction X à droite
        solver.addBoundaryCondition([&bbox](double x, double){ return abs(x - bbox.xmax) < 1e-6; }, 0, imposed_ux);
        // CL: Pivot
        solver.addBoundaryCondition([&bbox](double x, double y){ return abs(x - bbox.xmin) < 1e-6 && abs(y - bbox.ymin) < 1e-6; }, 1, 0.0);

        cout << "Calcul FEM en cours..." << endl;
        solver.assemble();
        solver.solve();

        double mean_stress_x = computeHomogenizedStress(mesh, solver.getSolution(), matMgr);
        double mean_strain_y = computeTransverseStrain(mesh, solver.getSolution(), bbox.H);

        // Résultats FEM
        double E_eff_FEM  = mean_stress_x / max_strain_x;
        double Nu_eff_FEM = - (mean_strain_y / max_strain_x);

        // [3] CALCULS ANALYTIQUES (VOIGT & REUSS)
        double Vm = 1.0 - Vf;
        // Voigt (Borne Supérieure - Parallèle) : E_v = Vf*Ef + Vm*Em
        double E_Voigt = Vf * E_FIBRE + Vm * E_MATRICE;
        
        // Reuss (Borne Inférieure - Série) : 1/E_r = Vf/Ef + Vm/Em
        double E_Reuss = 1.0 / ( (Vf / E_FIBRE) + (Vm / E_MATRICE) );

        // [4] AFFICHAGE COMPARATIF
        cout << "\n---------------------------------------------------" << endl;
        cout << "RESULTATS COMPARATIFS (En GPa)" << endl;
        cout << "---------------------------------------------------" << endl;
        cout << left << setw(20) << "Modele" << setw(15) << "Young (GPa)" << setw(15) << "Ecart / FEM" << endl;
        cout << "---------------------------------------------------" << endl;
        
        cout << left << setw(20) << "Reuss (Borne Inf)" 
             << setw(15) << (E_Reuss / 1000.0) 
             << ((E_Reuss - E_eff_FEM)/E_eff_FEM * 100.0) << " %" << endl;

        cout << left << setw(20) << "FEM (Calcul)"      
             << setw(15) << (E_eff_FEM / 1000.0) 
             << "-" << endl;

        cout << left << setw(20) << "Voigt (Borne Sup)" 
             << setw(15) << (E_Voigt / 1000.0) 
             << ((E_Voigt - E_eff_FEM)/E_eff_FEM * 100.0) << " %" << endl;
        
        cout << "---------------------------------------------------" << endl;
        cout << "Poisson Effectif (FEM) : " << Nu_eff_FEM << endl;
        cout << "---------------------------------------------------" << endl;

        // [5] EXPORTS
        exportToVTK("data/resultats/traction_resultat.vtu", mesh, solver.getSolution());
        exportFieldData("data/resultats/data/champ_complet.dat", mesh, solver.getSolution());

    } catch (const exception& e) {
        cerr << "\nERREUR : " << e.what() << endl;
        return 1;
    }

    return 0;
}