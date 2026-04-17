#include "Material.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

// MISE À JOUR : Enregistrement de Xt et Xc dans la structure
void MaterialManager::addMaterial(int label, double E, double nu, double alpha, double Xt, double Xc, double G_Ic) {
    materials[label] = {E, nu, alpha, Xt, Xc, G_Ic};
}

Prop MaterialManager::getProperties(int label) const {
    if (materials.find(label) == materials.end()) 
        throw std::runtime_error("Materiau introuvable ID: " + std::to_string(label));
    return materials.at(label);
}

// ==========================================
//  GETTERS POUR L'ENDOMMAGEMENT (CDM)
// ==========================================

double MaterialManager::getYoungModulus(int label) const {
    return getProperties(label).E;
}

double MaterialManager::getXt(int label) const {
    return getProperties(label).Xt;
}

double MaterialManager::getGIc(int label) const {
    return getProperties(label).G_Ic;
}

Eigen::Matrix3d MaterialManager::getHookeMatrix(int label, bool isPlaneStress) const {
    Prop p = getProperties(label);
    double E = p.E, nu = p.nu;
    
    double factor;
    Eigen::Matrix3d D = Eigen::Matrix3d::Zero();

    if (isPlaneStress) {
        factor = E / (1.0 - nu * nu);
        D(0,0) = 1.0;  D(0,1) = nu;
        D(1,0) = nu;   D(1,1) = 1.0;
        D(2,2) = (1.0 - nu) / 2.0;
    } else {
        factor = E / ((1.0 + nu) * (1.0 - 2.0 * nu));
        D(0,0) = 1.0 - nu; D(0,1) = nu;
        D(1,0) = nu;       D(1,1) = 1.0 - nu;
        D(2,2) = (1.0 - 2.0 * nu) / 2.0;
    }

    return D * factor;
}

// ==========================================
//  CISAILLEMENT ANTI-PLAN
// ==========================================

// NOUVEAU : Calcul du module de cisaillement G = E / (2 * (1 + nu))
double MaterialManager::getShearModulus(int label) const {
    Prop p = getProperties(label);
    return p.E / (2.0 * (1.0 + p.nu));
}

// ==========================================
//  THERMIQUE
// ==========================================

Eigen::Vector3d MaterialManager::getThermalStrainVector(int label, double deltaT) const {
    Prop p = getProperties(label);
    Eigen::Vector3d eps_th = Eigen::Vector3d::Zero();
    
    eps_th(0) = p.alpha * deltaT; 
    eps_th(1) = p.alpha * deltaT;
    
    return eps_th;
}

Eigen::Vector3d MaterialManager::getThermalStressVector(int label, double deltaT, bool isPlaneStress) const {
    Prop p = getProperties(label);
    Eigen::Vector3d sigma_th = Eigen::Vector3d::Zero();
    
    double factor;
    if (isPlaneStress) {
        factor = (p.E * p.alpha * deltaT) / (1.0 - p.nu);
    } else {
        factor = (p.E * p.alpha * deltaT) / ((1.0 + p.nu) * (1.0 - 2.0 * p.nu));
    }
    
    sigma_th(0) = factor;
    sigma_th(1) = factor; 
    
    return sigma_th;
}

#include <algorithm>
#include <stdexcept>
#include <iostream>

// ==========================================
//  MÉCANIQUE DE LA RUPTURE (TSAI-WU)
// ==========================================

double MaterialManager::computeTsaiWuIsotropicPlaneStrain(int matLabel, double sigma_x, double sigma_y, double tau_xy) const {
    
    Prop p = getProperties(matLabel);
    double Xt = p.Xt;     
    double Xc = std::abs(p.Xc);
    double nu = p.nu;        

    if (Xt <= 1e-6 || Xc <= 1e-6) {
        return 0.0; 
    }

    double F11 = 1.0 / (Xt * Xc);

    double F1_star = (1.0 + nu) * (1.0 / Xt - 1.0 / Xc);
    double F11_star = (1.0 - nu + nu * nu) * F11;
    double two_F12_star = (2.0 * nu * nu - 2.0 * nu - 1.0) * F11;
    
    double F66 = 3.0 * F11;

    double FI_TW = F1_star * (sigma_x + sigma_y) 
              + F11_star * (sigma_x * sigma_x + sigma_y * sigma_y) 
              + two_F12_star * (sigma_x * sigma_y) 
              + F66 * (tau_xy * tau_xy);

    return std::max(0.0, FI_TW);
}

double MaterialManager::computeDruckerPragerPlaneStrain(int matLabel, double Sxx, double Syy, double Txy) const {

    Prop p = getProperties(matLabel);
    double Xt = p.Xt;     
    double Xc = std::abs(p.Xc);
    double nu = p.nu;
    if (Xt <= 0.0 || Xc <= 0.0) {
        throw std::invalid_argument("Strength limits Xt and Xc must be strictly positive.");
    }

    double Szz = nu * (Sxx + Syy);
    double I1 = Sxx + Syy + Szz;

    double meanStress = I1 / 3.0;
    double devSxx = Sxx - meanStress;
    double devSyy = Syy - meanStress;
    double devSzz = Szz - meanStress;

    double J2 = 0.5 * (devSxx * devSxx + devSyy * devSyy + devSzz * devSzz) + (Txy * Txy);
    
    double root3 = std::sqrt(3.0);
    double alpha = (1.0 / root3) * ((Xc - Xt) / (Xc + Xt));
    double k = (2.0 / root3) * ((Xc * Xt) / (Xc + Xt));

    double FI_DP = (std::sqrt(J2) + alpha * I1) / k;

    return FI_DP;
}