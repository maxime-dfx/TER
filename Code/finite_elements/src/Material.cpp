#include "Material.h"
#include <stdexcept>

void MaterialManager::addMaterial(int label, double E, double nu) {
    materials[label] = {E, nu};
}

Prop MaterialManager::getProperties(int label) const {
    if (materials.find(label) == materials.end()) {
        throw std::runtime_error("Erreur : Matériau introuvable pour le label " + std::to_string(label));
    }
    return materials.at(label);
}

void MaterialManager::getHookeMatrix(int label, double D[3][3], bool isPlaneStress) const {
    Prop p = getProperties(label);
    double E = p.E;
    double nu = p.nu;
    double factor;

    // Initialisation à 0
    for(int i=0; i<3; i++) 
        for(int j=0; j<3; j++) 
            D[i][j] = 0.0;

    if (isPlaneStress) {
        // Contrainte Plane (plaque fine)
        factor = E / (1.0 - nu * nu);
        D[0][0] = 1.0;  D[0][1] = nu;
        D[1][0] = nu;   D[1][1] = 1.0;
        D[2][2] = (1.0 - nu) / 2.0;
    } else {
        // Déformation Plane (section de fibre infinie -> Ton cas probable)
        factor = E / ((1.0 + nu) * (1.0 - 2.0 * nu));
        D[0][0] = 1.0 - nu; D[0][1] = nu;
        D[1][0] = nu;       D[1][1] = 1.0 - nu;
        D[2][2] = (1.0 - 2.0 * nu) / 2.0;
    }

    // Appliquer le facteur
    for(int i=0; i<3; i++) 
        for(int j=0; j<3; j++) 
            D[i][j] *= factor;
}