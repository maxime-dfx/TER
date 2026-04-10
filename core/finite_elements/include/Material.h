#ifndef MATERIAL_H
#define MATERIAL_H

#include <map>
#include <Eigen/Dense>

struct Prop {
    double E;  
    double nu; 
    double alpha; // NOUVEAU : Coefficient de dilatation thermique
};

class MaterialManager {
private:
    std::map<int, Prop> materials;

public:
    // MISE À JOUR : Ajout du paramètre alpha (avec valeur par défaut à 0.0)
    void addMaterial(int label, double E, double nu, double alpha = 0.0);
    Prop getProperties(int label) const;
    
    // Retourne directement la matrice D (3x3)
    Eigen::Matrix3d getHookeMatrix(int label, bool isPlaneStress = false) const;
    
    // NOUVEAU : Retourne le vecteur des déformations thermiques (3x1)
    Eigen::Vector3d getThermalStrainVector(int label, double deltaT) const;

    // NOUVEAU : Retourne le vecteur des contraintes thermiques "pseudo-forces" (3x1)
    Eigen::Vector3d getThermalStressVector(int label, double deltaT, bool isPlaneStress = false) const;
};

#endif