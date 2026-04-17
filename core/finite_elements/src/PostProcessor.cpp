#include "PostProcessor.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm> 

using namespace std;
using namespace Eigen;

namespace {
    template <typename T>
    void writeBin(std::ofstream& out, T val) {
        char bytes[sizeof(T)];
        std::copy(reinterpret_cast<const char*>(&val), reinterpret_cast<const char*>(&val) + sizeof(T), bytes);
        std::reverse(bytes, bytes + sizeof(T)); 
        out.write(bytes, sizeof(T));
    }
}

// MISE À JOUR : Initialisation de isAntiPlane
PostProcessor::PostProcessor(const Mesh& m, const MaterialManager& mat, const VectorXd& u, bool planeStress, double dT, bool antiPlane)
    : mesh(m), matMgr(mat), U(u), isPlaneStress(planeStress), deltaT(dT), isAntiPlane(antiPlane) {}

DetailedResults PostProcessor::runAnalysis(double totalArea, double imposedStrain, const BoundingBox& bb, int labelFibre, int labelMatrice, const std::string& loadCase) {
    
    // Si on est en anti-plan, l'analyse des contraintes in-plane n'a pas de sens
    if (isAntiPlane) {
        DetailedResults res;
        // L'énergie et G12 sont déjà calculés directement dans HomogenizationManager
        return res; 
    }

    double sig_xx_tot = 0.0, sig_yy_tot = 0.0, tau_xy_tot = 0.0;
    double eps_xx_tot = 0.0, eps_yy_tot = 0.0; 
    double area_f = 0.0, area_m = 0.0;

    const auto& triangles = mesh.getTriangles();

    for(int t=0; t < (int)triangles.size(); ++t) {
        const Triangle& tri = triangles[t];
        Matrix3d D = matMgr.getHookeMatrix(tri.label, isPlaneStress);
        Vector3d sigma_th = matMgr.getThermalStressVector(tri.label, deltaT, isPlaneStress);

        double area = 0.0;
        Matrix<double, 3, 6> B = mesh.computeBMatrix(t, area);

        Matrix<double, 6, 1> u_el;
        for(int i=0; i<3; ++i) {
            u_el(2*i)   = U(2*tri.v[i]);
            u_el(2*i+1) = U(2*tri.v[i]+1);
        }

        Vector3d eps = B * u_el;
        Vector3d sigma = D * eps - sigma_th;

        sig_xx_tot += sigma(0) * area;
        sig_yy_tot += sigma(1) * area;
        tau_xy_tot += sigma(2) * area;
        
        eps_xx_tot += eps(0) * area;
        eps_yy_tot += eps(1) * area;

        if (tri.label == labelFibre) area_f += area;
        else area_m += area;
    }

    DetailedResults res;
    res.Vf = area_f / (area_f + area_m);
    res.E_eff = 0.0; res.nu_eff = 0.0; res.G_eff = 0.0;
    res.alpha_x = 0.0; res.alpha_y = 0.0; 
    
    if (std::abs(imposedStrain) > 1e-12) {
        if (loadCase == "traction_x") {
            res.E_eff = (sig_xx_tot / totalArea) / imposedStrain;
            res.nu_eff = -(eps_yy_tot / totalArea) / imposedStrain;
        } 
        else if (loadCase == "traction_y") {
            res.E_eff = (sig_yy_tot / totalArea) / imposedStrain;
            res.nu_eff = -(eps_xx_tot / totalArea) / imposedStrain;
        } 
        else if (loadCase == "cisaillement") {
            res.G_eff = (tau_xy_tot / totalArea) / imposedStrain;
        }
    }
    
    if (loadCase == "thermique" && std::abs(deltaT) > 1e-9) {
        res.alpha_x = (eps_xx_tot / totalArea) / deltaT;
        res.alpha_y = (eps_yy_tot / totalArea) / deltaT;
    }

    res.macro_sig_xx = sig_xx_tot / totalArea;
    res.macro_sig_yy = sig_yy_tot / totalArea;
    res.macro_tau_xy = tau_xy_tot / totalArea;

    return res;
}

void PostProcessor::exportToVTK(const string& filename) const {
    ofstream file(filename, ios::out | ios::binary);
    if (!file) return;

    const auto& vertices = mesh.getVertices();
    const auto& triangles = mesh.getTriangles();

    // En-tête du fichier VTK
    file << "# vtk DataFile Version 3.0\nResults\nBINARY\nDATASET UNSTRUCTURED_GRID\n";

    // 1. Écriture des Nœuds (Points)
    file << "POINTS " << vertices.size() << " double\n";
    for (const auto& v : vertices) {
        writeBin(file, (double)v.x);
        writeBin(file, (double)v.y);
        writeBin(file, (double)0.0); // Z = 0 en 2D
    }

    // 2. Écriture de la connectivité (Triangles)
    file << "\nCELLS " << triangles.size() << " " << triangles.size() * 4 << "\n";
    for (const auto& t : triangles) {
        writeBin(file, (int)3); // 3 sommets par triangle
        writeBin(file, (int)t.v[0]);
        writeBin(file, (int)t.v[1]);
        writeBin(file, (int)t.v[2]);
    }

    file << "\nCELL_TYPES " << triangles.size() << "\n";
    for (size_t i = 0; i < triangles.size(); ++i) writeBin(file, (int)5); // Type 5 = Triangle VTK

    // 3. Écriture des Déplacements aux nœuds (POINT_DATA)
    file << "\nPOINT_DATA " << vertices.size() << "\n";
    file << "VECTORS Deplacement double\n";
    
    // --- MODE ANTI-PLAN ---
    if (isAntiPlane) {
        for (int i = 0; i < (int)vertices.size(); ++i) {
            writeBin(file, (double)0.0);       // u = 0
            writeBin(file, (double)0.0);       // v = 0
            writeBin(file, (double)U(i));      // w != 0 (Cisaillement hors plan)
        }
        
        file << "\nCELL_DATA " << triangles.size() << "\n";
        file << "SCALARS MateriauID int 1\nLOOKUP_TABLE default\n";
        for (const auto& t : triangles) writeBin(file, (int)t.label);
        
        cout << "   [IO] Export VTK Binaire OK (Anti-Plan) : " << filename << endl;
        return; // Arrêt prématuré, pas de contraintes planes à afficher
    }
    
    // --- MODE IN-PLANE (Classique) ---
    for (int i = 0; i < (int)vertices.size(); ++i) {
        writeBin(file, (double)U(2*i));
        writeBin(file, (double)U(2*i+1));
        writeBin(file, (double)0.0);
    }
    
    // 4. Écriture des Données aux Cellules (CELL_DATA)
    file << "\nCELL_DATA " << triangles.size() << "\n";
    
    // ID des matériaux
    file << "SCALARS MateriauID int 1\nLOOKUP_TABLE default\n";
    for (const auto& t : triangles) writeBin(file, (int)t.label);

    // Initialisation des vecteurs de stockage
    vector<double> sxx(triangles.size()), syy(triangles.size()), txy(triangles.size()), v_mises(triangles.size());
    vector<double> sigma_max_vec(triangles.size()), sigma_min_vec(triangles.size()), failure_index(triangles.size());
    
    // NOUVEAU : Vérification de la présence du Cache CPU
    bool useCache = !cached_Sxx.empty();

    // BOUCLE OPTIMISÉE SUR LES ÉLÉMENTS
    for(int t = 0; t < (int)triangles.size(); ++t) {
        const Triangle& tri = triangles[t];
        Vector3d sigma;
        double fi_val = 0.0;

        if (useCache) {
            // LECTURE DIRECTE EN RAM (O(1), aucun calcul matriciel)
            sigma(0) = cached_Sxx[t];
            sigma(1) = cached_Syy[t];
            sigma(2) = cached_Txy[t];
            fi_val = cached_FI[t];
        } else {
            // FALLBACK : Méthode matricielle (pour runMechanicalCase)
            Matrix3d D = matMgr.getHookeMatrix(tri.label, isPlaneStress);
            Vector3d s_th = matMgr.getThermalStressVector(tri.label, deltaT, isPlaneStress);
            double area;
            Matrix<double, 3, 6> B = mesh.computeBMatrix(t, area);
            Matrix<double, 6, 1> u_el;
            for(int i = 0; i < 3; ++i) { 
                u_el(2*i) = U(2*tri.v[i]); 
                u_el(2*i+1) = U(2*tri.v[i]+1); 
            }
            sigma = D * (B * u_el) - s_th;
            
            // Calcul classique de Tsai-Wu au vol
            double center = (sigma(0) + sigma(1)) / 2.0;
            double radius = sqrt(pow((sigma(0) - sigma(1)) / 2.0, 2) + pow(sigma(2), 2));
            double sigma_max = center + radius; 
            double sigma_min = center - radius; 

            Prop p = matMgr.getProperties(tri.label);
            double fi_t = 0.0, fi_c = 0.0;
            if (sigma_max > 0) fi_t = sigma_max / p.Xt;
            if (sigma_min < 0) fi_c = std::abs(sigma_min) / p.Xc;
            fi_val = std::max(fi_t, fi_c);
        }

        // Assignation des contraintes primaires
        sxx[t] = sigma(0); 
        syy[t] = sigma(1); 
        txy[t] = sigma(2);
        failure_index[t] = fi_val;
        
        // Calcul rapide de Von Mises et Contraintes Principales (Scalaires pure)
        double sigma_zz = 0.0;
        if (!isPlaneStress) {
            Prop p = matMgr.getProperties(tri.label);
            sigma_zz = p.nu * (sigma(0) + sigma(1)) - p.E * p.alpha * deltaT;
        }

        v_mises[t] = sqrt(0.5 * (pow(sigma(0) - sigma(1), 2) + 
                                 pow(sigma(1) - sigma_zz, 2) + 
                                 pow(sigma_zz - sigma(0), 2)) + 
                          3.0 * pow(sigma(2), 2));

        double center = (sigma(0) + sigma(1)) / 2.0;
        double radius = sqrt(pow((sigma(0) - sigma(1)) / 2.0, 2) + pow(sigma(2), 2));
        sigma_max_vec[t] = center + radius; 
        sigma_min_vec[t] = center - radius; 
    }

    // 5. Écriture binaire de tous les champs sur le disque
    file << "SCALARS Sigma_XX double 1\nLOOKUP_TABLE default\n";
    for (double val : sxx) writeBin(file, val);
    
    file << "SCALARS Sigma_YY double 1\nLOOKUP_TABLE default\n";
    for (double val : syy) writeBin(file, val);
    
    file << "SCALARS Tau_XY double 1\nLOOKUP_TABLE default\n";
    for (double val : txy) writeBin(file, val);
    
    file << "SCALARS Von_Mises double 1\nLOOKUP_TABLE default\n";
    for (double val : v_mises) writeBin(file, val);
    
    file << "SCALARS Sigma_Max double 1\nLOOKUP_TABLE default\n";
    for (double val : sigma_max_vec) writeBin(file, val);
    
    file << "SCALARS Sigma_Min double 1\nLOOKUP_TABLE default\n";
    for (double val : sigma_min_vec) writeBin(file, val);
    
    file << "SCALARS Indice_Rupture double 1\nLOOKUP_TABLE default\n";
    for (double val : failure_index) writeBin(file, val);
    
    // NOUVEAU : EXPORT DE LA VARIABLE D'ENDOMMAGEMENT (FISSURES)
    if (!damageState.empty()) {
        file << "SCALARS Element_Damage double 1\nLOOKUP_TABLE default\n";
        for (double val : damageState) {
            writeBin(file, val);
        }
    }
    
    if (useCache) {
        cout << "   [IO] Export VTK Binaire OK (Optimise/Cache) : " << filename << endl;
    } else {
        cout << "   [IO] Export VTK Binaire OK (Fallback matriciel) : " << filename << endl;
    }
}