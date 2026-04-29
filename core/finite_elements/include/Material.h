#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>
#include <map>
#include <Eigen/Dense>

class DataFile; // Déclaration anticipée

struct MaterialProps {
    int label;
    double E;
    double nu;
    double alpha;
    double Xt;
    double Xc;
    double GIc;
};

class MaterialManager {
private:
    std::map<int, MaterialProps> materials;

public:
    MaterialManager() = default;
    
    void addMaterialFromConfig(const DataFile& config, const std::string& type);
    
    double getYoungModulus(int label) const;
    double getPoissonRatio(int label) const;
    double getShearModulus(int label) const;
    double getThermalExpansion(int label) const;
    double getXt(int label) const;
    double getXc(int label) const;
    double getGIc(int label) const;

    Eigen::Matrix3d getHookeMatrix(int label, bool isPlaneStress) const;
    Eigen::Vector3d getThermalStressVector(int label, double deltaT, bool isPlaneStress) const;
    Eigen::Vector3d getZCouplingVector(int label) const;
    double getC33(int label) const;
    double computeTsaiWuIsotropicPlaneStrain(int label, double sig_xx, double sig_yy, double tau_xy) const;
    double computeDruckerPragerPlaneStrain(int label, double sig_xx, double sig_yy, double tau_xy) const;
};

#endif // MATERIAL_H