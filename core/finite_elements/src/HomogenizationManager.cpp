#include "HomogenizationManager.h"

HomogenizationManager::HomogenizationManager(const SimulationContext& context)
    : ctx(context) {
    isPlaneStress = !ctx.config.isPlaneStrain();
    strain = ctx.config.getStrainTarget();
    deltaT = ctx.config.getDeltaT();
    bb = ctx.mesh.getBounds();
    Lx = bb.L;
    Ly = bb.H;
}

std::string HomogenizationManager::loadCaseToString(LoadCase lc) const {
    switch(lc) {
        case LoadCase::TensionX: return "traction_x";
        case LoadCase::TensionY: return "traction_y";
        case LoadCase::Shear:    return "cisaillement";
        case LoadCase::Thermal:  return "thermique";
        case LoadCase::ThermoMechanical: return "service_thermo_meca";
        default: return "inconnu";
    }
}

void HomogenizationManager::applyBoundaryConditions(Solver& solver, LoadCase load_case, bool isThermalOnly) const {
    bool isPeriodic = ctx.topo.isPeriodic();
    const auto& pairsLR = ctx.topo.getPairsLR();
    const auto& pairsBT = ctx.topo.getPairsBT();

    if (isThermalOnly) {
        if (isPeriodic) {
            solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmin)<1e-6 && std::abs(y-bb.ymin)<1e-6; }, 0, 0.0);
            solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmin)<1e-6 && std::abs(y-bb.ymin)<1e-6; }, 1, 0.0);
            solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmax)<1e-6 && std::abs(y-bb.ymin)<1e-6; }, 1, 0.0);
            for (auto p : pairsLR) {
                solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
                solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
            }
            for (auto p : pairsBT) {
                solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
                solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
            }
        } else {
            solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmin)<1e-6; }, 0, 0.0);
            solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmax)<1e-6; }, 0, 0.0);
            solver.addBoundaryCondition([this](double x, double y){ return std::abs(y-bb.ymin)<1e-6; }, 1, 0.0);
            solver.addBoundaryCondition([this](double x, double y){ return std::abs(y-bb.ymax)<1e-6; }, 1, 0.0);
        }
        return;
    }

    // TYPAGE FORT : Le switch sécurise les cas de charges
    switch (load_case) {
        case LoadCase::TensionX:
            if (isPeriodic) {
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmin)<1e-6 && std::abs(y-bb.ymin)<1e-6; }, 0, 0.0);
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmin)<1e-6 && std::abs(y-bb.ymin)<1e-6; }, 1, 0.0);
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmax)<1e-6 && std::abs(y-bb.ymin)<1e-6; }, 1, 0.0);
                for (auto p : pairsLR) {
                    solver.addPeriodicCondition(p.second, p.first, 0, Lx * strain);
                    solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
                }
                for (auto p : pairsBT) solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
            } else {
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmin)<1e-6; }, 0, 0.0);
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmax)<1e-6; }, 0, Lx * strain);
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmin)<1e-6 && std::abs(y-bb.ymin)<1e-6; }, 1, 0.0);
            }
            break;

        case LoadCase::TensionY:
            if (isPeriodic) {
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmin)<1e-6 && std::abs(y-bb.ymin)<1e-6; }, 0, 0.0);
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmin)<1e-6 && std::abs(y-bb.ymin)<1e-6; }, 1, 0.0);
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmin)<1e-6 && std::abs(y-bb.ymax)<1e-6; }, 0, 0.0);
                for (auto p : pairsBT) {
                    solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
                    solver.addPeriodicCondition(p.second, p.first, 1, Ly * strain);
                }
                for (auto p : pairsLR) solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
            } else {
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(y-bb.ymin)<1e-6; }, 1, 0.0);
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(y-bb.ymax)<1e-6; }, 1, Ly * strain);
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmin)<1e-6 && std::abs(y-bb.ymin)<1e-6; }, 0, 0.0);
            }
            break;

        case LoadCase::Shear:
            if (isPeriodic) {
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmin)<1e-6 && std::abs(y-bb.ymin)<1e-6; }, 0, 0.0);
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmin)<1e-6 && std::abs(y-bb.ymin)<1e-6; }, 1, 0.0);
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(x-bb.xmax)<1e-6 && std::abs(y-bb.ymin)<1e-6; }, 1, 0.0);
                for (auto p : pairsBT) {
                    solver.addPeriodicCondition(p.second, p.first, 0, Ly * strain);
                    solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
                }
                for (auto p : pairsLR) {
                    solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
                    solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
                }
            } else {
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(y-bb.ymin)<1e-6; }, 0, 0.0);
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(y-bb.ymin)<1e-6; }, 1, 0.0);
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(y-bb.ymax)<1e-6; }, 0, Ly * strain);
                solver.addBoundaryCondition([this](double x, double y){ return std::abs(y-bb.ymax)<1e-6; }, 1, 0.0);
            }
            break;
            
        default:
            break;
    }
}

DetailedResults HomogenizationManager::runMechanicalCase(LoadCase load_case) {
    Solver solver(ctx.mesh, ctx.mats, isPlaneStress, 0.0);
    applyBoundaryConditions(solver, load_case, false);
    
    solver.assemble();
    solver.applyBoundaryConditions();
    solver.solve();

    std::string str_case = loadCaseToString(load_case);
    PostProcessor post(ctx.mesh, ctx.mats, solver.getSolution(), isPlaneStress, 0.0);
    DetailedResults res = post.runAnalysis(Lx * Ly, strain, bb, ctx.config.getFibreLabel(), ctx.config.getMatriceLabel(), str_case);

    post.exportToVTK(ctx.config.getOutputDir() + "/" + str_case + "_" + ctx.config.getOutputVtk());
    return res;
}

DetailedResults HomogenizationManager::runThermalCase() {
    Solver solver_th(ctx.mesh, ctx.mats, isPlaneStress, deltaT);
    applyBoundaryConditions(solver_th, LoadCase::Thermal, true);
    
    solver_th.assemble();
    solver_th.applyBoundaryConditions();
    solver_th.solve();

    PostProcessor post_th(ctx.mesh, ctx.mats, solver_th.getSolution(), isPlaneStress, deltaT);
    DetailedResults res_th = post_th.runAnalysis(Lx * Ly, 0.0, bb, ctx.config.getFibreLabel(), ctx.config.getMatriceLabel(), "thermique");
    
    post_th.exportToVTK(ctx.config.getOutputDir() + "/thermique_" + ctx.config.getOutputVtk());
    return res_th;
}

DetailedResults HomogenizationManager::runThermoMechanicalCase(LoadCase load_case) {
    Solver solver_tm(ctx.mesh, ctx.mats, isPlaneStress, deltaT);
    applyBoundaryConditions(solver_tm, load_case, false);
    
    solver_tm.assemble();
    solver_tm.applyBoundaryConditions();
    solver_tm.solve();

    std::string str_case = loadCaseToString(load_case);
    PostProcessor post_tm(ctx.mesh, ctx.mats, solver_tm.getSolution(), isPlaneStress, deltaT);
    DetailedResults res_tm = post_tm.runAnalysis(Lx * Ly, strain, bb, ctx.config.getFibreLabel(), ctx.config.getMatriceLabel(), str_case); 
    
    post_tm.exportToVTK(ctx.config.getOutputDir() + "/service_thermo_meca_" + ctx.config.getOutputVtk());
    return res_tm;
}

Eigen::Matrix3d HomogenizationManager::computeEffectiveStiffness(double E1, double E2, double G12, double nu12, double nu21) const {
    Eigen::Matrix3d S = Eigen::Matrix3d::Zero();
    S(0, 0) = 1.0 / E1; S(1, 1) = 1.0 / E2;
    S(0, 1) = -nu12 / E1; S(1, 0) = -nu21 / E2; S(2, 2) = 1.0 / G12;
    return S.inverse();
}