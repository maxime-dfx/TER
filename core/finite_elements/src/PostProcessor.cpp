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

PostProcessor::PostProcessor(const Mesh& m, const MaterialManager& mat, const VectorXd& u, bool planeStress, double dT)
    : mesh(m), matMgr(mat), U(u), isPlaneStress(planeStress), deltaT(dT) {}

DetailedResults PostProcessor::runAnalysis(double totalArea, double imposedStrain, const BoundingBox& bb, int labelFibre, int labelMatrice, const std::string& loadCase) {
    
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

    // --- ON DÉCLARE 'res' ICI ---
    DetailedResults res;
    res.Vf = area_f / (area_f + area_m);
    res.E_eff = 0.0; res.nu_eff = 0.0; res.G_eff = 0.0;
    res.alpha_x = 0.0; res.alpha_y = 0.0; // Initialisation par défaut
    
    // Calcul mécanique classique
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
    
    // --- CALCUL THERMIQUE (La partie qui manquait) ---
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

    file << "# vtk DataFile Version 3.0\nResults\nBINARY\nDATASET UNSTRUCTURED_GRID\n";

    file << "POINTS " << vertices.size() << " double\n";
    for (const auto& v : vertices) {
        writeBin(file, (double)v.x);
        writeBin(file, (double)v.y);
        writeBin(file, (double)0.0);
    }

    file << "\nCELLS " << triangles.size() << " " << triangles.size() * 4 << "\n";
    for (const auto& t : triangles) {
        writeBin(file, (int)3);
        writeBin(file, (int)t.v[0]);
        writeBin(file, (int)t.v[1]);
        writeBin(file, (int)t.v[2]);
    }

    file << "\nCELL_TYPES " << triangles.size() << "\n";
    for (size_t i=0; i<triangles.size(); ++i) writeBin(file, (int)5); 

    file << "\nPOINT_DATA " << vertices.size() << "\n";
    file << "VECTORS Deplacement double\n";
    for (int i=0; i<(int)vertices.size(); ++i) {
        writeBin(file, (double)U(2*i));
        writeBin(file, (double)U(2*i+1));
        writeBin(file, (double)0.0);
    }
    
    file << "\nCELL_DATA " << triangles.size() << "\n";
    file << "SCALARS MateriauID int 1\nLOOKUP_TABLE default\n";
    for (const auto& t : triangles) writeBin(file, (int)t.label);

    vector<double> sxx(triangles.size()), syy(triangles.size()), txy(triangles.size()), v_mises(triangles.size());
    
    for(int t=0; t < (int)triangles.size(); ++t) {
        const Triangle& tri = triangles[t];
        Matrix3d D = matMgr.getHookeMatrix(tri.label, isPlaneStress);
        Vector3d s_th = matMgr.getThermalStressVector(tri.label, deltaT, isPlaneStress);
        double area;
        Matrix<double, 3, 6> B = mesh.computeBMatrix(t, area);
        Matrix<double, 6, 1> u_el;
        for(int i=0; i<3; ++i) { u_el(2*i)=U(2*tri.v[i]); u_el(2*i+1)=U(2*tri.v[i]+1); }
        
        Vector3d sigma = D * (B * u_el) - s_th;
        sxx[t] = sigma(0); syy[t] = sigma(1); txy[t] = sigma(2);
        
        double sigma_zz = 0.0;
        if (!isPlaneStress) {
            Prop p = matMgr.getProperties(tri.label);
            sigma_zz = p.nu * (sigma(0) + sigma(1)) - p.E * p.alpha * deltaT;
        }

        v_mises[t] = sqrt(0.5 * (pow(sigma(0) - sigma(1), 2) + 
                                 pow(sigma(1) - sigma_zz, 2) + 
                                 pow(sigma_zz - sigma(0), 2)) + 
                          3.0 * pow(sigma(2), 2));
    }

    file << "SCALARS Sigma_XX double 1\nLOOKUP_TABLE default\n";
    for (double val : sxx) writeBin(file, val);

    file << "SCALARS Sigma_YY double 1\nLOOKUP_TABLE default\n";
    for (double val : syy) writeBin(file, val);

    file << "SCALARS Tau_XY double 1\nLOOKUP_TABLE default\n";
    for (double val : txy) writeBin(file, val);

    file << "SCALARS Von_Mises double 1\nLOOKUP_TABLE default\n";
    for (double val : v_mises) writeBin(file, val);
    
    cout << "   [IO] Export VTK Binaire OK : " << filename << endl;
}