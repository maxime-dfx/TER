#ifndef MATERIAL_H
#define MATERIAL_H

#include <map>
#include <Eigen/Dense>

struct Prop {
    double E;  
    double nu; 
};

class MaterialManager {
private:
    std::map<int, Prop> materials;

public:
    void addMaterial(int label, double E, double nu);
    Prop getProperties(int label) const;
    
    // Retourne directement la matrice D (3x3)
    Eigen::Matrix3d getHookeMatrix(int label, bool isPlaneStress = false) const;
};

#endif