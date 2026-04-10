#include "Material.h"
#include <stdexcept>

void MaterialManager::addMaterial(int label, double E, double nu, double alpha) {
    materials[label] = {E, nu, alpha};
}

Prop MaterialManager::getProperties(int label) const {
    if (materials.find(label) == materials.end()) 
        throw std::runtime_error("Materiau introuvable ID: " + std::to_string(label));
    return materials.at(label);
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