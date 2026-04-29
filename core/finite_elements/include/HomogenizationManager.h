#ifndef HOMOGENIZATION_MANAGER_H
#define HOMOGENIZATION_MANAGER_H

#include <Eigen/Dense>
#include <string>
#include "SimulationContext.h"
#include "Solver.h"
#include "PostProcessor.h"
#include "Types.h"

class HomogenizationManager {
private:
    SimulationContext ctx; 
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
    
    // NOUVEAU : Exécute le cas anti-plan et renvoie directement le module G12 effectif
    double runLongitudinalShearCase(); 
    DetailedResults runElementaryCase(LoadCase load_case);

    // Construit le tenseur de rigidité macroscopique C_eff à partir des 3 essais
    Eigen::Matrix3d identifyStiffnessTensor(const DetailedResults& resTX, 
                                            const DetailedResults& resTY, 
                                            const DetailedResults& resShear) const;
    Eigen::Matrix3d computeEffectiveStiffness(double E1, double E2, double G12, double nu12, double nu21) const;
    void runProgressiveDamageAnalysis(LoadCase lc);
};

#endif // HOMOGENIZATION_MANAGER_H