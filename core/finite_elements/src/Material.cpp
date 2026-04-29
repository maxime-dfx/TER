#include "Material.h"
#include "Datafile.h"
#include <stdexcept>
#include <cmath>

using namespace Eigen;

void MaterialManager::addMaterialFromConfig(const DataFile& config, const std::string& type) {
    MaterialProps props;
    if (type == "matrice") {
        props.label = config.getMatriceLabel();
        props.E     = config.getMatriceE();
        props.nu    = config.getMatriceNu();
        props.alpha = config.getMatriceAlpha();
        props.Xt    = config.getMatriceXt();
        props.Xc    = config.getMatriceXc();
        props.GIc   = config.getMatriceG_Ic();
    } else if (type == "fibre") {
        props.label = config.getFibreLabel();
        props.E     = config.getFibreE();
        props.nu    = config.getFibreNu();
        props.alpha = config.getFibreAlpha();
        props.Xt    = config.getFibreXt();
        props.Xc    = config.getFibreXc();
        props.GIc   = config.getFibreG_Ic();
    } else if (type == "interphase") {
        props.label = config.getInterphaseLabel();
        props.E     = config.getInterphaseE();
        props.nu    = config.getInterphaseNu();
        props.alpha = config.getInterphaseAlpha();
        props.Xt    = config.getInterphaseXt();
        props.Xc    = config.getInterphaseXc();
        props.GIc   = config.getInterphaseG_Ic();
    } else {
        throw std::invalid_argument("Type de matériau inconnu : " + type);
    }
    materials[props.label] = props;
}

double MaterialManager::getYoungModulus(int label) const { return materials.at(label).E; }
double MaterialManager::getPoissonRatio(int label) const { return materials.at(label).nu; }
double MaterialManager::getShearModulus(int label) const { return materials.at(label).E / (2.0 * (1.0 + materials.at(label).nu)); }
double MaterialManager::getThermalExpansion(int label) const { return materials.at(label).alpha; }
double MaterialManager::getXt(int label) const { return materials.at(label).Xt; }
double MaterialManager::getXc(int label) const { return materials.at(label).Xc; }
double MaterialManager::getGIc(int label) const { return materials.at(label).GIc; }

Matrix3d MaterialManager::getHookeMatrix(int label, bool isPlaneStress) const {
    double E = getYoungModulus(label);
    double nu = getPoissonRatio(label);
    Matrix3d D = Matrix3d::Zero();
    
    if (isPlaneStress) {
        double factor = E / (1.0 - nu * nu);
        D(0, 0) = factor; D(1, 1) = factor;
        D(0, 1) = factor * nu; D(1, 0) = factor * nu;
        D(2, 2) = factor * (1.0 - nu) / 2.0;
    } else { 
        double factor = E / ((1.0 + nu) * (1.0 - 2.0 * nu));
        D(0, 0) = factor * (1.0 - nu); D(1, 1) = factor * (1.0 - nu);
        D(0, 1) = factor * nu; D(1, 0) = factor * nu;
        D(2, 2) = factor * (0.5 - nu);
    }
    return D;
}

Vector3d MaterialManager::getThermalStressVector(int label, double deltaT, bool isPlaneStress) const {
    Matrix3d D = getHookeMatrix(label, isPlaneStress);
    double alpha = getThermalExpansion(label);
    Vector3d eps_th;
    if (isPlaneStress) {
        eps_th << alpha * deltaT, alpha * deltaT, 0.0;
    } else {
        double nu = getPoissonRatio(label);
        double alpha_eff = (1.0 + nu) * alpha;
        eps_th << alpha_eff * deltaT, alpha_eff * deltaT, 0.0;
    }
    return D * eps_th;
}

double MaterialManager::computeTsaiWuIsotropicPlaneStrain(int label, double sig_xx, double sig_yy, double tau_xy) const {
    double Xt = getXt(label); double Xc = getXc(label);
    double F1 = 1.0 / Xt - 1.0 / Xc;
    double F11 = 1.0 / (Xt * Xc);
    double F12 = -0.5 * F11; 
    double sig_zz = getPoissonRatio(label) * (sig_xx + sig_yy); 
    
    double term1 = F1 * (sig_xx + sig_yy + sig_zz);
    double term2 = F11 * (sig_xx*sig_xx + sig_yy*sig_yy + sig_zz*sig_zz);
    double term3 = 2.0 * F12 * (sig_xx*sig_yy + sig_xx*sig_zz + sig_yy*sig_zz);
    double S = Xt / std::sqrt(3.0); 
    double F66 = 1.0 / (S * S);
    double term4 = F66 * (tau_xy * tau_xy);
    
    return term1 + term2 + term3 + term4; 
}

double MaterialManager::computeDruckerPragerPlaneStrain(int label, double sig_xx, double sig_yy, double tau_xy) const {
    double sig_zz = getPoissonRatio(label) * (sig_xx + sig_yy);
    double I1 = sig_xx + sig_yy + sig_zz; 
    double s_xx = sig_xx - I1/3.0; double s_yy = sig_yy - I1/3.0; double s_zz = sig_zz - I1/3.0;
    double J2 = 0.5 * (s_xx*s_xx + s_yy*s_yy + s_zz*s_zz) + tau_xy*tau_xy;
    
    double Xt = getXt(label); double Xc = getXc(label);
    double alpha = (Xc - Xt) / (std::sqrt(3.0) * (Xc + Xt));
    double k = (2.0 * Xc * Xt) / (std::sqrt(3.0) * (Xc + Xt));
    double sigma_eq = alpha * I1 + std::sqrt(J2);
    
    return sigma_eq / k; 
}

Vector3d MaterialManager::getZCouplingVector(int label) const {
    double E = getYoungModulus(label);
    double nu = getPoissonRatio(label);
    double factor = E / ((1.0 + nu) * (1.0 - 2.0 * nu));
    return Vector3d(factor * nu, factor * nu, 0.0); // Couplages C13 et C23
}

double MaterialManager::getC33(int label) const {
    double E = getYoungModulus(label);
    double nu = getPoissonRatio(label);
    return (E * (1.0 - nu)) / ((1.0 + nu) * (1.0 - 2.0 * nu)); // Rigidité axiale pure
}