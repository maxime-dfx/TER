#include "PostProcessor.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <cstring>
#include <omp.h>

using namespace std;
using namespace Eigen;

// ============================================================================
// BOÎTE À OUTILS HPC POUR L'I/O (Intrinsèques CPU & Écriture par Blocs)
// ============================================================================
namespace {
    // Instruction CPU native pour inverser 4 octets (int) en 1 cycle
    inline uint32_t bswap32(uint32_t x) {
#if defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap32(x);
#elif defined(_MSC_VER)
        return _byteswap_ulong(x);
#else
        return (x >> 24) | ((x >> 8) & 0x0000FF00) | ((x << 8) & 0x00FF0000) | (x << 24);
#endif
    }

    // Instruction CPU native pour inverser 8 octets (double) en 1 cycle
    inline uint64_t bswap64(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap64(x);
#elif defined(_MSC_VER)
        return _byteswap_uint64(x);
#else
        return (x >> 56) | ((x & 0x00FF000000000000ull) >> 40) |
               ((x & 0x0000FF0000000000ull) >> 24) | ((x & 0x000000FF00000000ull) >> 8) |
               ((x & 0x00000000FF000000ull) << 8) | ((x & 0x0000000000FF0000ull) << 24) |
               ((x & 0x000000000000FF00ull) << 40) | (x << 56);
#endif
    }

    // Écriture d'un vecteur complet de `double` en une seule opération disque
    void writeDoubleBlock(std::ofstream& out, const std::vector<double>& data) {
        std::vector<uint64_t> buffer(data.size());
        
        #pragma omp parallel for schedule(static)
        for (size_t i = 0; i < data.size(); ++i) {
            uint64_t bits;
            std::memcpy(&bits, &data[i], sizeof(double));
            buffer[i] = bswap64(bits); // SIMD Vectorized Swap
        }
        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(uint64_t));
    }

    // Écriture d'un vecteur complet de `int` en une seule opération disque
    void writeIntBlock(std::ofstream& out, const std::vector<int>& data) {
        std::vector<uint32_t> buffer(data.size());
        
        #pragma omp parallel for schedule(static)
        for (size_t i = 0; i < data.size(); ++i) {
            uint32_t bits;
            std::memcpy(&bits, &data[i], sizeof(int));
            buffer[i] = bswap32(bits);
        }
        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(uint32_t));
    }
}

PostProcessor::PostProcessor(const Mesh& m, const MaterialManager& mat, const VectorXd& u, bool planeStress, double dT, bool antiPlane)
    : mesh(m), matMgr(mat), U(u), isPlaneStress(planeStress), deltaT(dT), isAntiPlane(antiPlane) {}

DetailedResults PostProcessor::runAnalysis(double totalArea, double imposedStrain, const BoundingBox& bb, int labelFibre, int labelMatrice, const std::string& loadCase) {
    if (isAntiPlane) return DetailedResults(); 

    double sig_xx_tot = 0.0, sig_yy_tot = 0.0, tau_xy_tot = 0.0;
    double eps_xx_tot = 0.0, eps_yy_tot = 0.0; 
    double area_f = 0.0, area_m = 0.0;

    const auto& triangles = mesh.getTriangles();

    #pragma omp parallel for reduction(+:sig_xx_tot, sig_yy_tot, tau_xy_tot, eps_xx_tot, eps_yy_tot, area_f, area_m)
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

    // 1. Écriture des Nœuds (Points) - BLOCK WRITE
    file << "POINTS " << vertices.size() << " double\n";
    std::vector<double> points_buffer(vertices.size() * 3, 0.0);
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < vertices.size(); ++i) {
        points_buffer[3*i] = vertices[i].x;
        points_buffer[3*i+1] = vertices[i].y;
        points_buffer[3*i+2] = 0.0;
    }
    writeDoubleBlock(file, points_buffer);

    // 2. Écriture de la connectivité (Triangles) - BLOCK WRITE
    file << "\nCELLS " << triangles.size() << " " << triangles.size() * 4 << "\n";
    std::vector<int> cells_buffer(triangles.size() * 4);
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < triangles.size(); ++i) {
        cells_buffer[4*i] = 3;
        cells_buffer[4*i+1] = triangles[i].v[0];
        cells_buffer[4*i+2] = triangles[i].v[1];
        cells_buffer[4*i+3] = triangles[i].v[2];
    }
    writeIntBlock(file, cells_buffer);

    file << "\nCELL_TYPES " << triangles.size() << "\n";
    std::vector<int> cell_types_buffer(triangles.size(), 5); // 5 = Triangle
    writeIntBlock(file, cell_types_buffer);

    // 3. Écriture des Déplacements aux nœuds (POINT_DATA) - BLOCK WRITE
    file << "\nPOINT_DATA " << vertices.size() << "\n";
    file << "VECTORS Deplacement double\n";
    
    std::vector<double> u_buffer(vertices.size() * 3, 0.0);
    if (isAntiPlane) {
        #pragma omp parallel for schedule(static)
        for (size_t i = 0; i < vertices.size(); ++i) u_buffer[3*i+2] = U(i);
    } else {
        #pragma omp parallel for schedule(static)
        for (size_t i = 0; i < vertices.size(); ++i) {
            u_buffer[3*i] = U(2*i);
            u_buffer[3*i+1] = U(2*i+1);
        }
    }
    writeDoubleBlock(file, u_buffer);
    
    file << "\nCELL_DATA " << triangles.size() << "\n";
    
    // Matériaux
    file << "SCALARS MateriauID int 1\nLOOKUP_TABLE default\n";
    std::vector<int> mat_ids(triangles.size());
    #pragma omp parallel for schedule(static)
    for (size_t i = 0; i < triangles.size(); ++i) mat_ids[i] = triangles[i].label;
    writeIntBlock(file, mat_ids);

    if (isAntiPlane) return; // Arrêt prématuré
    
    // 4. Variables d'état
    vector<double> sxx(triangles.size()), syy(triangles.size()), txy(triangles.size()), v_mises(triangles.size());
    vector<double> sigma_max_vec(triangles.size()), sigma_min_vec(triangles.size()), failure_index(triangles.size());
    
    bool useCache = !cached_Sxx.empty();

    #pragma omp parallel for schedule(static)
    for(int t = 0; t < (int)triangles.size(); ++t) {
        Vector3d sigma;
        double fi_val = 0.0;
        const Triangle& tri = triangles[t];

        if (useCache) {
            sigma(0) = cached_Sxx[t]; sigma(1) = cached_Syy[t]; sigma(2) = cached_Txy[t];
            fi_val = cached_FI[t];
        } else {
            // ... (logique de repli)
        }

        sxx[t] = sigma(0); syy[t] = sigma(1); txy[t] = sigma(2); failure_index[t] = fi_val;
        
        double sigma_zz = 0.0;
        if (!isPlaneStress) {
            double nu = matMgr.getPoissonRatio(tri.label);
            double E = matMgr.getYoungModulus(tri.label);
            double alpha = matMgr.getThermalExpansion(tri.label);
            
            sigma_zz = nu * (sigma(0) + sigma(1)) - E * alpha * deltaT;
        }

        v_mises[t] = sqrt(0.5 * (pow(sigma(0) - sigma(1), 2) + pow(sigma(1) - sigma_zz, 2) + pow(sigma_zz - sigma(0), 2)) + 3.0 * pow(sigma(2), 2));

        double center = (sigma(0) + sigma(1)) / 2.0;
        double radius = sqrt(pow((sigma(0) - sigma(1)) / 2.0, 2) + pow(sigma(2), 2));
        sigma_max_vec[t] = center + radius; 
        sigma_min_vec[t] = center - radius; 
    }

    // BLOCK WRITES FINAUX
    file << "SCALARS Sigma_XX double 1\nLOOKUP_TABLE default\n"; writeDoubleBlock(file, sxx);
    file << "SCALARS Sigma_YY double 1\nLOOKUP_TABLE default\n"; writeDoubleBlock(file, syy);
    file << "SCALARS Tau_XY double 1\nLOOKUP_TABLE default\n"; writeDoubleBlock(file, txy);
    file << "SCALARS Von_Mises double 1\nLOOKUP_TABLE default\n"; writeDoubleBlock(file, v_mises);
    file << "SCALARS Sigma_Max double 1\nLOOKUP_TABLE default\n"; writeDoubleBlock(file, sigma_max_vec);
    file << "SCALARS Sigma_Min double 1\nLOOKUP_TABLE default\n"; writeDoubleBlock(file, sigma_min_vec);
    file << "SCALARS Indice_Rupture double 1\nLOOKUP_TABLE default\n"; writeDoubleBlock(file, failure_index);
    
    if (!damageState.empty()) {
        file << "SCALARS Element_Damage double 1\nLOOKUP_TABLE default\n";
        writeDoubleBlock(file, damageState);
    }
}

DetailedResults PostProcessor::computeEffectiveProperties(LoadCase lc, double appliedStrain) const {
    DetailedResults res;
    double totalArea = 0.0;
    double area_fibre = 0.0;
    
    // Sommes pondérées pour les moyennes de contraintes
    double sum_sig_xx = 0.0, sum_sig_yy = 0.0, sum_tau_xy = 0.0;
    double sum_sig_zz = 0.0; 

    int numElements = static_cast<int>(mesh.getTriangles().size());

    for (int t = 0; t < numElements; ++t) {
        const Triangle& tri = mesh.getTriangles()[t];
        
        // 1. Géométrie de l'élément (Aire exacte par le Jacobien)
        const Vertex& p0 = mesh.getVertices()[tri.v[0]];
        const Vertex& p1 = mesh.getVertices()[tri.v[1]];
        const Vertex& p2 = mesh.getVertices()[tri.v[2]];
        double area = 0.5 * std::abs((p1.x - p0.x)*(p2.y - p0.y) - (p2.x - p0.x)*(p1.y - p0.y));
        
        totalArea += area;
        if (tri.label == 2) area_fibre += area; // Label 2 = Fibre

        // 2. Calcul de la matrice de déformation B (Plane)
        double b[3] = {p1.y - p2.y, p2.y - p0.y, p0.y - p1.y};
        double c[3] = {p2.x - p1.x, p0.x - p2.x, p1.x - p0.x};
        
        Eigen::Matrix<double, 3, 6> B = Eigen::Matrix<double, 3, 6>::Zero();
        for(int i=0; i<3; ++i) {
            B(0, 2*i)   = b[i];
            B(1, 2*i+1) = c[i];
            B(2, 2*i)   = c[i];
            B(2, 2*i+1) = b[i];
        }
        B /= (2.0 * area);

        // 3. Extraction des déplacements locaux
        Eigen::Vector<double, 6> U_elem;
        for(int k=0; k<3; ++k) {
            U_elem(2*k)   = U(2*tri.v[k]);
            U_elem(2*k+1) = U(2*tri.v[k]+1);
        }

        // 4. Loi de Comportement 3D (Déformation Plane Généralisée)
        // Si on calcule Ez, on impose une déformation en Z (Ezz)
        double ezz = (lc == LoadCase::TensionZ) ? appliedStrain : 0.0;
        
        Eigen::Vector3d eps_xy = B * U_elem;
        Eigen::Matrix3d D_plan = matMgr.getHookeMatrix(tri.label, isPlaneStress);
        Eigen::Vector3d sig_th_plan = matMgr.getThermalStressVector(tri.label, deltaT, isPlaneStress);
        
        // Couplage Z (C13, C23)
        Eigen::Vector3d couplingZ = matMgr.getZCouplingVector(tri.label);
        
        // Contraintes dans le plan : Sigma = D*Eps - Sig_Th + Couplage_Z * Ezz
        Eigen::Vector3d sigma_plan = D_plan * eps_xy - sig_th_plan + (couplingZ * ezz);

        // Contrainte hors-plan : Sigma_zz = C31*Exx + C32*Eyy + C33*Ezz - Beta_z*dT
        double C33 = matMgr.getC33(tri.label);
        double E = matMgr.getYoungModulus(tri.label);
        double alpha = matMgr.getThermalExpansion(tri.label);
        
        double sigma_zz = couplingZ(0)*eps_xy(0) + couplingZ(1)*eps_xy(1) + C33*ezz - (E * alpha * deltaT);

        // 5. Intégration numérique
        sum_sig_xx += sigma_plan(0) * area;
        sum_sig_yy += sigma_plan(1) * area;
        sum_tau_xy += sigma_plan(2) * area;
        sum_sig_zz += sigma_zz * area;
    }

    // Calcul des moyennes macroscopiques
    res.Vf = area_fibre / totalArea;
    res.macro_sig_xx = sum_sig_xx / totalArea;
    res.macro_sig_yy = sum_sig_yy / totalArea;
    res.macro_tau_xy = sum_tau_xy / totalArea;
    res.macro_sig_zz = sum_sig_zz / totalArea;

    // Identification simplifiée des modules (sera affiné par l'inversion de matrice dans main)
    switch (lc) {
        case LoadCase::TensionX:
            res.E_eff = res.macro_sig_xx / appliedStrain;
            break;
        case LoadCase::TensionY:
            res.E_eff = res.macro_sig_yy / appliedStrain;
            break;
        case LoadCase::TensionZ:
            res.E_eff = res.macro_sig_zz / appliedStrain; 
            break;
        case LoadCase::Shear:
            res.G_eff = res.macro_tau_xy / appliedStrain;
            break;
        default:
            break;
    }

    return res;
}

void PostProcessor::updateDamageState(std::vector<double>& current_damage) const {
    if(current_damage.size() != mesh.getTriangles().size()) {
        current_damage.assign(mesh.getTriangles().size(), 0.0);
    }

    int numElements = static_cast<int>(mesh.getTriangles().size());
    
    for (int t = 0; t < numElements; ++t) {
        if (current_damage[t] > 0.9) continue; // Élément déjà cassé, on l'ignore

        const Triangle& tri = mesh.getTriangles()[t];
        const Vertex& p0 = mesh.getVertices()[tri.v[0]];
        const Vertex& p1 = mesh.getVertices()[tri.v[1]];
        const Vertex& p2 = mesh.getVertices()[tri.v[2]];
        double area = 0.5 * std::abs((p1.x - p0.x)*(p2.y - p0.y) - (p2.x - p0.x)*(p1.y - p0.y));

        double b[3] = {p1.y - p2.y, p2.y - p0.y, p0.y - p1.y};
        double c[3] = {p2.x - p1.x, p0.x - p2.x, p1.x - p0.x};
        
        Eigen::Matrix<double, 3, 6> B = Eigen::Matrix<double, 3, 6>::Zero();
        for(int i=0; i<3; ++i) {
            B(0, 2*i) = b[i]; B(1, 2*i+1) = c[i];
            B(2, 2*i) = c[i]; B(2, 2*i+1) = b[i];
        }
        B /= (2.0 * area);

        Eigen::Vector<double, 6> U_elem;
        for(int k=0; k<3; ++k) {
            U_elem(2*k)   = U(2*tri.v[k]);
            U_elem(2*k+1) = U(2*tri.v[k]+1);
        }

        // Calcul des contraintes locales
        Eigen::Vector3d eps = B * U_elem;
        Eigen::Matrix3d D = matMgr.getHookeMatrix(tri.label, isPlaneStress);
        Eigen::Vector3d sigma_th = matMgr.getThermalStressVector(tri.label, deltaT, isPlaneStress);
        Eigen::Vector3d sigma = D * eps - sigma_th;

        // Évaluation des critères de rupture (Failure Index)
        double FI = 0.0;
        if (tri.label == 2) { 
            // Fibre (Tsai-Wu)
            FI = matMgr.computeTsaiWuIsotropicPlaneStrain(tri.label, sigma(0), sigma(1), sigma(2));
        } else { 
            // Matrice & Interphase (Drucker-Prager sensible à la pression hydrostatique)
            FI = matMgr.computeDruckerPragerPlaneStrain(tri.label, sigma(0), sigma(1), sigma(2));
        }

        // Si le critère dépasse 1.0, le matériau casse localement
        if (FI >= 1.0) {
            current_damage[t] = 0.99; // L'élément perd 99% de sa rigidité pour le pas suivant
        }
    }
}