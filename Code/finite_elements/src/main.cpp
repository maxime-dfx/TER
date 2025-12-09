/**
 * ============================================================================
 * MAIN.CPP - ESSAI DE TRACTION VIRTUEL (FEM COMPOSITES)
 * ============================================================================
 * Description : Simulation d'un essai de traction sur un VER composite.
 *               Calcul E, Nu, Export Heatmap Gnuplot et Export VTK (Paraview).
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
// #include "PostProcessor.h" // Plus nécessaire si on met la fonction locale ici

using namespace std;

// ============================================================================
// CONFIGURATION & CONSTANTES
// ============================================================================
const int LABEL_MATRICE = 101;
const int LABEL_FIBRE   = 102;

// Propriétés Matériaux
const double E_MATRICE  = 3500.0;  // MPa
const double NU_MATRICE = 0.35;
const double E_FIBRE    = 72000.0; // MPa
const double NU_FIBRE   = 0.22;

// Dossiers
const string OUTPUT_DIR = "data/resultats/data";

// ============================================================================
// STRUCTURES & HELPERS
// ============================================================================

// --- Gestion des dossiers ---
void ensureDir(const string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == -1) mkdir(path.c_str(), 0755);
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

// --- Analyse Taux de Fibre (Vf) ---
void analyzeMicrostructure(const Mesh& mesh) {
    double aire_totale = 0.0;
    double aire_fibre = 0.0;
    
    for(const auto& tri : mesh.triangles) {
        double a = getArea(tri, mesh);
        aire_totale += a;
        if(tri.label == LABEL_FIBRE) aire_fibre += a;
    }
    cout << " [MICROSTRUCTURE]" << endl;
    cout << "  -> Aire Totale : " << aire_totale << endl;
    cout << "  -> Taux de fibre (Vf) : " << fixed << setprecision(2) 
         << (aire_fibre / aire_totale) * 100.0 << " %" << endl;
    cout << "---------------------------------------------------" << endl;
}

// ============================================================================
// EXPORT VTK (PARAVIEW) -- POUR LA DEFORMEE
// ============================================================================
void exportToVTK(const string& filename, const Mesh& mesh, const VectorXd& U) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Erreur: Impossible de creer " << filename << endl;
        return;
    }

    // 1. En-tête VTK
    file << "# vtk DataFile Version 3.0\n";
    file << "Resultats FEM Traction Composite\n";
    file << "ASCII\n";
    file << "DATASET UNSTRUCTURED_GRID\n";

    // 2. Les Noeuds (Géométrie initiale)
    file << "POINTS " << mesh.vertices.size() << " double\n";
    for (const auto& node : mesh.vertices) {
        file << node.x << " " << node.y << " 0.0\n"; // z=0
    }

    // 3. Les Éléments (Triangles)
    file << "CELLS " << mesh.triangles.size() << " " << mesh.triangles.size() * 4 << "\n";
    for (const auto& tri : mesh.triangles) {
        file << "3 " << tri.v[0] << " " << tri.v[1] << " " << tri.v[2] << "\n";
    }

    // 4. Types de cellules (5 = Triangle)
    file << "CELL_TYPES " << mesh.triangles.size() << "\n";
    for (size_t i = 0; i < mesh.triangles.size(); ++i) {
        file << "5\n"; 
    }

    // 5. Données vectorielles (Déplacement) aux noeuds
    file << "POINT_DATA " << mesh.vertices.size() << "\n";
    file << "VECTORS Deplacement double\n";
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        double ux = U(2 * i);
        double uy = U(2 * i + 1);
        file << ux << " " << uy << " 0.0\n";
    }

    // 6. Données scalaires (Matériaux) aux éléments (Pour voir fibres vs matrice)
    file << "CELL_DATA " << mesh.triangles.size() << "\n";
    file << "SCALARS MaterialID int 1\n";
    file << "LOOKUP_TABLE default\n";
    for (const auto& tri : mesh.triangles) {
        file << tri.label << "\n";
    }

    file.close();
    cout << "Fichier VTK genere : " << filename << " (Pret pour ParaView)" << endl;
}

// ============================================================================
// EXPORT POUR GNUPLOT (HEATMAP)
// ============================================================================
void exportFieldData(const string& filename, const Mesh& mesh, const VectorXd& U) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Erreur: Impossible de creer " << filename << endl;
        return;
    }

    // En-tête commenté
    file << "# X Y Ux Uy Magnitude\n";

    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        double x = mesh.vertices[i].x;
        double y = mesh.vertices[i].y;
        
        // Récupération déplacement
        double ux = U(2 * i);
        double uy = U(2 * i + 1);
        double mag = sqrt(ux*ux + uy*uy); 

        // Ecriture
        file << x << " " << y << " " << ux << " " << uy << " " << mag << "\n";
    }
    file.close();
    cout << "Donnees Gnuplot exportees : " << filename << endl;
}

// ============================================================================
// CALCULS PHYSIQUES (Post-Processing)
// ============================================================================

// 1. Calcul de la Contrainte Moyenne (Sigma_XX)
double computeHomogenizedStress(const Mesh& mesh, const VectorXd& U, const MaterialManager& matMgr) {
    double sigma_integral = 0.0;
    double volume_total = 0.0;

    for(const auto& tri : mesh.triangles) {
        // Matrice D
        double D_arr[3][3];
        matMgr.getHookeMatrix(tri.label, D_arr, false);
        Matrix3d D;
        for(int i=0; i<3; i++) for(int j=0; j<3; j++) D(i,j) = D_arr[i][j];

        // Matrice B
        const Vertex& p0 = mesh.vertices[tri.v[0]];
        const Vertex& p1 = mesh.vertices[tri.v[1]];
        const Vertex& p2 = mesh.vertices[tri.v[2]];
        double area = getArea(tri, mesh);
        
        double b[3] = {p1.y - p2.y, p2.y - p0.y, p0.y - p1.y};
        double c[3] = {p2.x - p1.x, p0.x - p2.x, p1.x - p0.x};

        Matrix<double, 3, 6> B; B.setZero();
        for(int i=0; i<3; ++i) {
            B(0, 2*i) = b[i]; B(1, 2*i+1) = c[i];
            B(2, 2*i) = c[i]; B(2, 2*i+1) = b[i];
        }
        double detJ = (p1.x - p0.x)*(p2.y - p0.y) - (p2.x - p0.x)*(p1.y - p0.y);
        B *= ((detJ > 0 ? 1.0 : -1.0) / (2.0 * area));

        // Vecteur u élémentaire
        Matrix<double, 6, 1> u_el;
        for(int i=0; i<3; ++i) {
            u_el(2*i) = U(2*tri.v[i]); 
            u_el(2*i+1) = U(2*tri.v[i]+1);
        }

        Vector3d sigma = D * B * u_el;
        sigma_integral += sigma(0) * area; 
        volume_total += area;
    }
    return sigma_integral / volume_total;
}

// 2. Calcul de la Déformation Transverse Moyenne (Epsilon_YY)
double computeTransverseStrain(const Mesh& mesh, const VectorXd& U, double meshHeight) {
    double sumUy = 0.0;
    int count = 0;
    const double eps = 1e-4;

    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        const auto& node = mesh.vertices[i];
        if (std::abs(node.y - meshHeight) < eps) {
            double uy = U(i * 2 + 1); 
            sumUy += uy;
            count++;
        }
    }
    if (count == 0) return 0.0;
    return (sumUy / count) / meshHeight; 
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
    
    // Création récursive des dossiers
    ensureDir("data");
    ensureDir("data/resultats");
    ensureDir("data/resultats/data");
    ensureDir("data/resultats/graphique");

    try {
        // [1] INITIALISATION
        Mesh mesh;
        mesh.read(meshFile);
        
        BoundingBox bbox; 
        bbox.compute(mesh);
        analyzeMicrostructure(mesh);

        MaterialManager matMgr;
        matMgr.addMaterial(LABEL_MATRICE, E_MATRICE, NU_MATRICE);    
        matMgr.addMaterial(LABEL_FIBRE,   E_FIBRE,   NU_FIBRE);  

        // [2] CONFIGURATION SIMULATION
        int n_steps = 5;
        double max_strain_x = 0.005; // 0.5% d'allongement
        
        vector<double> eps_x_list, eps_y_list, sigma_x_list;

        cout << " Step | Strain X (%) | Stress (MPa) | Strain Y (%) " << endl;
        cout << "------+--------------+--------------+--------------" << endl;

        // [3] BOUCLE DE CHARGEMENT
        for(int step = 1; step <= n_steps; ++step) {
            
            double current_eps_x = (max_strain_x * step) / n_steps;
            double imposed_ux    = bbox.L * current_eps_x;

            Solver solver(mesh, matMgr); 
            
            // CL: Bloqué en X à gauche
            solver.addBoundaryCondition([&bbox](double x, double){ 
                return abs(x - bbox.xmin) < 1e-6; 
            }, 0, 0.0);
            
            // CL: Traction imposée à droite
            solver.addBoundaryCondition([&bbox](double x, double){ 
                return abs(x - bbox.xmax) < 1e-6; 
            }, 0, imposed_ux);
            
            // CL: Pivot (un seul point bloqué en Y)
            solver.addBoundaryCondition([&bbox](double x, double y){ 
                return abs(x - bbox.xmin) < 1e-6 && abs(y - bbox.ymin) < 1e-6; 
            }, 1, 0.0);

            solver.assemble();
            solver.solve();

            double mean_stress_x = computeHomogenizedStress(mesh, solver.getSolution(), matMgr);
            double mean_strain_y = computeTransverseStrain(mesh, solver.getSolution(), bbox.H);

            eps_x_list.push_back(current_eps_x);
            eps_y_list.push_back(mean_strain_y);
            sigma_x_list.push_back(mean_stress_x);

            cout << setw(5) << step << " | " 
                 << setw(12) << (current_eps_x*100.0) << " | "
                 << setw(12) << mean_stress_x << " | " 
                 << setw(12) << (mean_strain_y*100.0) << endl;

            // --- EXPORTS AU DERNIER PAS ---
            if(step == n_steps) {
                // 1. Export VTK (Pour ParaView -> Déformée)
                string vtkPath = "data/resultats/traction_resultat.vtu";
                exportToVTK(vtkPath, mesh, solver.getSolution());

                // 2. Export DAT (Pour Gnuplot -> Heatmap)
                string fieldPath = "data/resultats/data/champ_complet.dat";
                exportFieldData(fieldPath, mesh, solver.getSolution());
            }
        }
        
        // [4] ANALYSE DES RESULTATS
        double sum_xx = 0, sum_xy_stress = 0, sum_xy_strain = 0;
        for(size_t i=0; i<eps_x_list.size(); ++i) {
            sum_xx          += eps_x_list[i] * eps_x_list[i];
            sum_xy_stress   += eps_x_list[i] * sigma_x_list[i];
            sum_xy_strain   += eps_x_list[i] * eps_y_list[i];
        }
        
        double E_eff  = sum_xy_stress / sum_xx;
        double Nu_eff = - (sum_xy_strain / sum_xx);

        // [5] EXPORT COURBE TRACTION
        ofstream datFile("data/resultats/data/courbe_traction.dat");
        datFile << "# StrainX Stress StrainY\n";
        for(size_t i=0; i<eps_x_list.size(); ++i) {
            datFile << eps_x_list[i] << " " << sigma_x_list[i] << " " << eps_y_list[i] << "\n";
        }
        datFile.close();

        cout << "---------------------------------------------------" << endl;
        cout << "[RESULTATS HOMOGENEISATION]" << endl;
        cout << " > Module d'Young (E_eff)      : " << E_eff << " MPa" << endl;
        cout << " > Coefficient Poisson (Nu_eff): " << Nu_eff << endl;
        cout << "---------------------------------------------------" << endl;

    } catch (const exception& e) {
        cerr << "\nERREUR : " << e.what() << endl;
        return 1;
    }

    return 0;
}