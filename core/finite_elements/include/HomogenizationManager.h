#ifndef HOMOGENIZATION_MANAGER_H
#define HOMOGENIZATION_MANAGER_H

#include <Eigen/Dense>
#include <string>
#include "SimulationContext.h"
#include "Solver.h"
#include "PostProcessor.h"

// 1. TYPAGE FORT : Remplacement des std::string par une énumération stricte
enum class LoadCase {
    TensionX,
    TensionY,
    Shear,
    Thermal,
    ThermoMechanical
};

class HomogenizationManager {
private:
    SimulationContext ctx; // 3. CONTEXTE : Regroupement des dépendances
    bool isPlaneStress;
    double strain;
    double deltaT;
    BoundingBox bb;
    double Lx;
    double Ly;

    void applyBoundaryConditions(Solver& solver, LoadCase load_case, bool isThermalOnly = false) const;
    std::string loadCaseToString(LoadCase lc) const;

public:
    HomogenizationManager(const SimulationContext& context);

    // Retourne uniquement des résultats, aucun affichage console (2. ISOLATION MVC)
    DetailedResults runMechanicalCase(LoadCase load_case);
    DetailedResults runThermalCase();
    DetailedResults runThermoMechanicalCase(LoadCase load_case);

    Eigen::Matrix3d computeEffectiveStiffness(double E1, double E2, double G12, double nu12, double nu21) const;
};

#endif // HOMOGENIZATION_MANAGER_H