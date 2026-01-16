#include "Material.h"
#include <stdexcept>

void MaterialManager::addMaterial(int label, double E, double nu) {
    materials[label] = {E, nu};
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
        // Déformation Plane (Cas standard composites coupe transversale)
        factor = E / ((1.0 + nu) * (1.0 - 2.0 * nu));
        D(0,0) = 1.0 - nu; D(0,1) = nu;
        D(1,0) = nu;       D(1,1) = 1.0 - nu;
        D(2,2) = (1.0 - 2.0 * nu) / 2.0;
    }

    return D * factor;
}