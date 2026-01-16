#include "PostProcessor.h"
#include <iostream>
#include <fstream>
#include <cmath>

using namespace std;
using namespace Eigen;

PostProcessor::PostProcessor(const Mesh& m, const MaterialManager& mat, const VectorXd& u)
    : mesh(m), matMgr(mat), U(u) {}

double PostProcessor::getTriangleArea(int tIdx) const {
    // Copie de la logique Mesh pour être autonome ou appel direct si Mesh le permet
    const Triangle& t = mesh.triangles[tIdx];
    const Vertex& p0 = mesh.vertices[t.v[0]];
    const Vertex& p1 = mesh.vertices[t.v[1]];
    const Vertex& p2 = mesh.vertices[t.v[2]];
    return 0.5 * std::abs((p1.x - p0.x)*(p2.y - p0.y) - (p2.x - p0.x)*(p1.y - p0.y));
}

DetailedResults PostProcessor::runAnalysis(double totalArea, double imposedStrain, double meshHeight, int labelFibre, int labelMatrice) {
    
    // Accumulateurs par phase
    double sum_sigma_f = 0.0, sum_sigma_m = 0.0;
    double sum_eps_f = 0.0,   sum_eps_m = 0.0;
    double area_f = 0.0,      area_m = 0.0;

    for(int t=0; t < (int)mesh.triangles.size(); ++t) {
        const Triangle& tri = mesh.triangles[t];
        double area = getTriangleArea(t);
        
        // 1. Matériau & Matrice D
        Matrix3d D = matMgr.getHookeMatrix(tri.label, false);

        // 2. Calcul B (Géométrie)
        const Vertex& p0 = mesh.vertices[tri.v[0]];
        const Vertex& p1 = mesh.vertices[tri.v[1]];
        const Vertex& p2 = mesh.vertices[tri.v[2]];
        
        double b[3] = {p1.y - p2.y, p2.y - p0.y, p0.y - p1.y};
        double c[3] = {p2.x - p1.x, p0.x - p2.x, p1.x - p0.x};
        
        Matrix<double, 3, 6> B = Matrix<double, 3, 6>::Zero();
        for(int i=0; i<3; ++i) {
            B(0, 2*i)=b[i]; B(1, 2*i+1)=c[i]; B(2, 2*i)=c[i]; B(2, 2*i+1)=b[i];
        }
        double detJ = (p1.x-p0.x)*(p2.y-p0.y) - (p2.x-p0.x)*(p1.y-p0.y);
        B *= (detJ > 0 ? 1.0 : -1.0) / (2.0 * area);

        // 3. Déplacements Élémentaires
        Matrix<double, 6, 1> u_el;
        for(int i=0; i<3; ++i) {
            u_el(2*i)   = U(2*tri.v[i]);
            u_el(2*i+1) = U(2*tri.v[i]+1);
        }

        // 4. Calculs Locaux
        Vector3d eps = B * u_el;    // Déformation locale
        Vector3d sigma = D * eps;   // Contrainte locale

        // 5. Tri par phase
        if (tri.label == labelFibre) {
            sum_sigma_f += sigma(0) * area; // On intègre Sigma_xx
            sum_eps_f   += eps(0) * area;   // On intègre Epsilon_xx
            area_f      += area;
        } else {
            sum_sigma_m += sigma(0) * area;
            sum_eps_m   += eps(0) * area;
            area_m      += area;
        }
    }

    // Calcul Poisson global (Contraction transverse moyenne)
    double sumUy = 0.0; int count = 0;
    for (const auto& v : mesh.vertices) {
        if (std::abs(v.y - meshHeight) < 1e-4) {
            int idx = &v - &mesh.vertices[0];
            sumUy += U(idx * 2 + 1);
            count++;
        }
    }
    double mean_eps_y = (count > 0 ? sumUy/count : 0.0) / meshHeight;

    // Assemblage des résultats détaillés
    DetailedResults res;
    res.Vf = area_f / (area_f + area_m);
    
    // Moyennes volumiques (Surfaciques en 2D)
    res.mean_sigma_f = (area_f > 0) ? sum_sigma_f / area_f : 0.0;
    res.mean_sigma_m = (area_m > 0) ? sum_sigma_m / area_m : 0.0;
    res.mean_eps_f   = (area_f > 0) ? sum_eps_f / area_f : 0.0;
    res.mean_eps_m   = (area_m > 0) ? sum_eps_m / area_m : 0.0;

    // Propriétés Effectives Globales
    double global_sigma = (sum_sigma_f + sum_sigma_m) / (area_f + area_m);
    res.E_eff  = global_sigma / imposedStrain;
    res.nu_eff = - (mean_eps_y / imposedStrain);

    return res;
}

void PostProcessor::exportToVTK(const string& filename) const {
    ofstream file(filename);
    if (!file) return;

    file << "# vtk DataFile Version 3.0\nResults\nASCII\nDATASET UNSTRUCTURED_GRID\n";
    file << "POINTS " << mesh.vertices.size() << " double\n";
    for (const auto& v : mesh.vertices) file << v.x << " " << v.y << " 0.0\n";

    file << "CELLS " << mesh.triangles.size() << " " << mesh.triangles.size() * 4 << "\n";
    for (const auto& t : mesh.triangles) file << "3 " << t.v[0] << " " << t.v[1] << " " << t.v[2] << "\n";

    file << "CELL_TYPES " << mesh.triangles.size() << "\n";
    for (size_t i=0; i<mesh.triangles.size(); ++i) file << "5\n"; 

    file << "POINT_DATA " << mesh.vertices.size() << "\nVECTORS Deplacement double\n";
    for (int i=0; i<(int)mesh.vertices.size(); ++i) file << U(2*i) << " " << U(2*i+1) << " 0.0\n";
    
    file << "CELL_DATA " << mesh.triangles.size() << "\nSCALARS MateriauID int 1\nLOOKUP_TABLE default\n";
    for (const auto& t : mesh.triangles) file << t.label << "\n";
    
    cout << "   [IO] Export VTK : " << filename << endl;
}

void PostProcessor::exportToGnuplot(const string& filename) const {
    // (Inchangé ou simplifié)
    ofstream file(filename);
    for (int i=0; i<(int)mesh.vertices.size(); ++i)
        file << mesh.vertices[i].x << " " << mesh.vertices[i].y << " " 
             << U(2*i) << " " << U(2*i+1) << "\n";
}