#ifndef MATERIAL_H
#define MATERIAL_H

#include <map>
#include <Eigen/Dense>

struct Prop {
    double E;  
    double nu; 
    double alpha; // Coefficient de dilatation thermique
    double Xt;    // Limite de rupture en traction
    double Xc;    // Limite de rupture en compression
    double G_Ic;
};

class MaterialManager {
private:
    std::map<int, Prop> materials;

public:
    void addMaterial(int label, double E, double nu, double alpha = 0.0, double Xt = 1e9, double Xc = 1e9, double G_Ic = 1e9);
    
    Prop getProperties(int label) const;

    double getYoungModulus(int label) const;
    double getXt(int label) const;
    double getGIc(int label) const;
    
    Eigen::Matrix3d getHookeMatrix(int label, bool isPlaneStress = false) const;
    
    double getShearModulus(int label) const;
    
    Eigen::Vector3d getThermalStrainVector(int label, double deltaT) const;

    Eigen::Vector3d getThermalStressVector(int label, double deltaT, bool isPlaneStress = false) const;

    double computeTsaiWuIsotropicPlaneStrain(int matLabel, double sigma_x, double sigma_y, double tau_xy) const;
    double computeDruckerPragerPlaneStrain(int label, double Sxx, double Syy, double Txy) const;
};

#endif