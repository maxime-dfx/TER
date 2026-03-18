#include "PostProcessor.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm> // NOUVEAU : Pour std::reverse et std::copy

using namespace std;
using namespace Eigen;

// =========================================================================
// UTILITAIRE POUR L'EXPORT BINAIRE
// =========================================================================
namespace {
    // Le format VTK Binaire exige des données en "Big-Endian". 
    // Nos ordinateurs sont en "Little-Endian", on doit donc inverser les octets.
    template <typename T>
    void writeBin(std::ofstream& out, T val) {
        char bytes[sizeof(T)];
        std::copy(reinterpret_cast<const char*>(&val), reinterpret_cast<const char*>(&val) + sizeof(T), bytes);
        std::reverse(bytes, bytes + sizeof(T)); // Inversion Little-Endian -> Big-Endian
        out.write(bytes, sizeof(T));
    }
}
// =========================================================================

PostProcessor::PostProcessor(const Mesh& m, const MaterialManager& mat, const VectorXd& u, bool planeStress)
    : mesh(m), matMgr(mat), U(u), isPlaneStress(planeStress) {}

DetailedResults PostProcessor::runAnalysis(double totalArea, double imposedStrain, const BoundingBox& bb, int labelFibre, int labelMatrice, const std::string& loadCase) {
    
    double sig_xx_tot = 0.0, sig_yy_tot = 0.0, tau_xy_tot = 0.0;
    double area_f = 0.0, area_m = 0.0;

    const auto& triangles = mesh.getTriangles();
    const auto& vertices = mesh.getVertices();

    for(int t=0; t < (int)triangles.size(); ++t) {
        const Triangle& tri = triangles[t];
        
        Matrix3d D = matMgr.getHookeMatrix(tri.label, isPlaneStress);

        double area = 0.0;
        Matrix<double, 3, 6> B = mesh.computeBMatrix(t, area);

        Matrix<double, 6, 1> u_el;
        for(int i=0; i<3; ++i) {
            u_el(2*i)   = U(2*tri.v[i]);
            u_el(2*i+1) = U(2*tri.v[i]+1);
        }

        Vector3d eps = B * u_el;
        Vector3d sigma = D * eps;

        sig_xx_tot += sigma(0) * area;
        sig_yy_tot += sigma(1) * area;
        tau_xy_tot += sigma(2) * area;

        if (tri.label == labelFibre) area_f += area;
        else area_m += area;
    }

    DetailedResults res;
    res.Vf = area_f / (area_f + area_m);
    res.E_eff  = 0.0; 
    res.nu_eff = 0.0; 
    res.G_eff  = 0.0;
    
    if (loadCase == "traction_x") {
        res.E_eff = (sig_xx_tot / totalArea) / imposedStrain;
        
        double sumUy_top = 0.0, sumUy_bottom = 0.0; 
        int count_top = 0, count_bottom = 0;
        double tol = 1e-5 * bb.H;

        for (size_t i = 0; i < vertices.size(); ++i) {
            if (std::abs(vertices[i].y - bb.ymax) < tol) { sumUy_top += U(i * 2 + 1); count_top++; }
            if (std::abs(vertices[i].y - bb.ymin) < tol) { sumUy_bottom += U(i * 2 + 1); count_bottom++; }
        }
        
        double mean_Uy_top = (count_top > 0) ? (sumUy_top / count_top) : 0.0;
        double mean_Uy_bottom = (count_bottom > 0) ? (sumUy_bottom / count_bottom) : 0.0;
        double mean_eps_y = (mean_Uy_top - mean_Uy_bottom) / bb.H;
        res.nu_eff = -mean_eps_y / imposedStrain;
    } 
    else if (loadCase == "traction_y") {
        res.E_eff = (sig_yy_tot / totalArea) / imposedStrain;
        
        double sumUx_right = 0.0, sumUx_left = 0.0; 
        int count_right = 0, count_left = 0;
        double tol = 1e-5 * bb.L;

        for (size_t i = 0; i < vertices.size(); ++i) {
            if (std::abs(vertices[i].x - bb.xmax) < tol) { sumUx_right += U(i * 2); count_right++; }
            if (std::abs(vertices[i].x - bb.xmin) < tol) { sumUx_left += U(i * 2); count_left++; }
        }
        
        double mean_Ux_right = (count_right > 0) ? (sumUx_right / count_right) : 0.0;
        double mean_Ux_left = (count_left > 0) ? (sumUx_left / count_left) : 0.0;
        double mean_eps_x = (mean_Ux_right - mean_Ux_left) / bb.L;
        res.nu_eff = -mean_eps_x / imposedStrain;
    } 
    else if (loadCase == "cisaillement") {
        res.G_eff = (tau_xy_tot / totalArea) / imposedStrain;
    }

    return res;
}

// =========================================================================
// EXPORT VTK BINAIRE
// =========================================================================
void PostProcessor::exportToVTK(const string& filename) const {
    // Ouverture du fichier en mode binaire
    ofstream file(filename, ios::out | ios::binary);
    if (!file) {
        cerr << "   [Erreur] Impossible de creer le fichier " << filename << endl;
        return;
    }

    const auto& vertices = mesh.getVertices();
    const auto& triangles = mesh.getTriangles();

    // 1. En-tête ASCII (Même dans un fichier binaire, l'en-tête doit être lisible en texte)
    file << "# vtk DataFile Version 3.0\n";
    file << "Results\n";
    file << "BINARY\n"; // C'est ce mot-clé qui dit à ParaView de s'attendre à des octets bruts
    file << "DATASET UNSTRUCTURED_GRID\n";

    // 2. Coordonnées des points
    file << "POINTS " << vertices.size() << " double\n";
    for (const auto& v : vertices) {
        writeBin(file, (double)v.x);
        writeBin(file, (double)v.y);
        writeBin(file, (double)0.0); // 3D forcé par VTK
    }

    // 3. Connectivité (Triangles)
    file << "\nCELLS " << triangles.size() << " " << triangles.size() * 4 << "\n";
    for (const auto& t : triangles) {
        writeBin(file, (int)3); // Nombre de sommets pour ce polygone
        writeBin(file, (int)t.v[0]);
        writeBin(file, (int)t.v[1]);
        writeBin(file, (int)t.v[2]);
    }

    // 4. Type de cellules (5 = Triangle en VTK)
    file << "\nCELL_TYPES " << triangles.size() << "\n";
    for (size_t i=0; i<triangles.size(); ++i) {
        writeBin(file, (int)5); 
    }

    // --- DONNÉES AUX NOEUDS ---
    file << "\nPOINT_DATA " << vertices.size() << "\n";
    file << "VECTORS Deplacement double\n";
    for (int i=0; i<(int)vertices.size(); ++i) {
        writeBin(file, (double)U(2*i));
        writeBin(file, (double)U(2*i+1));
        writeBin(file, (double)0.0);
    }
    
    // --- DONNÉES AUX ÉLÉMENTS ---
    file << "\nCELL_DATA " << triangles.size() << "\n";
    
    file << "SCALARS MateriauID int 1\nLOOKUP_TABLE default\n";
    for (const auto& t : triangles) {
        writeBin(file, (int)t.label);
    }

    // Pré-calcul des contraintes
    vector<Vector3d> sigmas(triangles.size());
    for(int t=0; t < (int)triangles.size(); ++t) {
        const Triangle& tri = triangles[t];
        Matrix3d D = matMgr.getHookeMatrix(tri.label, isPlaneStress);
        double area = 0.0;
        Matrix<double, 3, 6> B = mesh.computeBMatrix(t, area);

        Matrix<double, 6, 1> u_el;
        for(int i=0; i<3; ++i) {
            u_el(2*i)   = U(2*tri.v[i]);
            u_el(2*i+1) = U(2*tri.v[i]+1);
        }
        sigmas[t] = D * (B * u_el); 
    }

    file << "\nSCALARS Sigma_XX double 1\nLOOKUP_TABLE default\n";
    for (const auto& s : sigmas) writeBin(file, (double)s(0));

    file << "\nSCALARS Sigma_YY double 1\nLOOKUP_TABLE default\n";
    for (const auto& s : sigmas) writeBin(file, (double)s(1));

    file << "\nSCALARS Tau_XY double 1\nLOOKUP_TABLE default\n";
    for (const auto& s : sigmas) writeBin(file, (double)s(2));
    
    cout << "   [IO] Export VTK Binaire effectue : " << filename << endl;
}