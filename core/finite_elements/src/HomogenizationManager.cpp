#include "HomogenizationManager.h"
#include <iostream>
#include <cstdio>
#include <cmath>

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
        case LoadCase::LongitudinalShear: return "cisaillement_longitudinal";
        default: return "inconnu";
    }
}

// ============================================================================
// CONDITIONS AUX LIMITES (Isostatisme et Homogénéisation)
// ============================================================================

void HomogenizationManager::applyBoundaryConditions(Solver& solver, LoadCase load_case, bool isThermalOnly) const {
    bool isPeriodic = ctx.topo.isPeriodic();
    const auto& pairsLR = ctx.topo.getPairsLR();
    const auto& pairsBT = ctx.topo.getPairsBT();

    // Tolérance pour capturer les noeuds sur les bords (Gmsh precision)
    const double tol = 1e-6;

    if (isThermalOnly) {
        // Isostatisme par plans de symétrie : permet une expansion thermique libre sans rotation/translation
        // Bloque le bord X_min en Translation X et le bord Y_min en Translation Y
        solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x - bb.xmin) < tol; }, 0, 0.0);
        solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(y - bb.ymin) < tol; }, 1, 0.0);
        return;
    }

    switch (load_case) {
        case LoadCase::ThermoMechanical: 
        case LoadCase::TensionX:
            if (isPeriodic) {
                // Point de référence (origine)
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymin)<tol; }, 0, 0.0);
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymin)<tol; }, 1, 0.0);
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmax)<tol && std::abs(y-bb.ymin)<tol; }, 1, 0.0);
                
                for (auto p : pairsLR) {
                    solver.addPeriodicCondition(p.second, p.first, 0, Lx * strain);
                    solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
                }
                for (auto p : pairsBT) solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
            } else {
                // KUBC (Kinematic Uniform Boundary Conditions) via symétrie
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x - bb.xmin) < tol; }, 0, 0.0);
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(y - bb.ymin) < tol; }, 1, 0.0);
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x - bb.xmax) < tol; }, 0, Lx * strain);
            }
            break;

        case LoadCase::TensionY:
            if (isPeriodic) {
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymin)<tol; }, 0, 0.0);
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymin)<tol; }, 1, 0.0);
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymax)<tol; }, 0, 0.0);
                for (auto p : pairsBT) {
                    solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
                    solver.addPeriodicCondition(p.second, p.first, 1, Ly * strain);
                }
                for (auto p : pairsLR) solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
            } else {
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(y - bb.ymin) < tol; }, 1, 0.0);
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x - bb.xmin) < tol; }, 0, 0.0);
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(y - bb.ymax) < tol; }, 1, Ly * strain);
            }
            break;

        case LoadCase::Shear:
            if (isPeriodic) {
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymin)<tol; }, 0, 0.0);
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymin)<tol; }, 1, 0.0);
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmax)<tol && std::abs(y-bb.ymin)<tol; }, 1, 0.0);
                for (auto p : pairsBT) {
                    solver.addPeriodicCondition(p.second, p.first, 0, Ly * strain);
                    solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
                }
                for (auto p : pairsLR) {
                    solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
                    solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
                }
            } else {
                // Cisaillement simple (bords rigides)
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(y - bb.ymin) < tol; }, 0, 0.0);
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(y - bb.ymin) < tol; }, 1, 0.0);
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(y - bb.ymax) < tol; }, 0, Ly * strain);
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(y - bb.ymax) < tol; }, 1, 0.0);
            }
            break;

        case LoadCase::LongitudinalShear:
            if (isPeriodic) {
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymin)<tol; }, 0, 0.0);
                for (auto p : pairsLR) solver.addPeriodicCondition(p.second, p.first, 0, Lx * strain);
                for (auto p : pairsBT) solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
            } else {
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x - bb.xmin) < tol; }, 0, 0.0);
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x - bb.xmax) < tol; }, 0, Lx * strain);
            }
            break;
            
        default: break;
    }
}

// ============================================================================
// GESTIONNAIRES D'ESSAIS (Simulation Runners)
// ============================================================================

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
    Solver solver_th(ctx.mesh, ctx.mats, isPlaneStress, 0.0);
    applyBoundaryConditions(solver_th, LoadCase::Thermal, true);
    solver_th.applyBoundaryConditions();

    int load_steps = ctx.config.getLoadSteps();
    std::cout << "\n--- EXECUTION ANALYSE THERMIQUE NON-LINEAIRE ---" << std::endl;

    // Exportation séquentielle VTK pour visualiser la cinétique de rupture
    auto export_step = [&](int step, double current_dT) {
        PostProcessor post(ctx.mesh, ctx.mats, solver_th.getSolution(), isPlaneStress, current_dT);
        post.setDamageState(solver_th.getDamageState());
        post.setStressState(solver_th.getSxx(), solver_th.getSyy(), solver_th.getTxy());
        post.setFailureIndex(solver_th.getFailureIndex());
        
        char filename[512];
        std::snprintf(filename, sizeof(filename), "%s/thermique_step_%03d.vtk", ctx.config.getOutputDir().c_str(), step);
        post.exportToVTK(filename);
    };

    solver_th.solveNonLinear(deltaT, load_steps, export_step);
    return DetailedResults(); 
}

DetailedResults HomogenizationManager::runThermoMechanicalCase(LoadCase load_case) {
    Solver solver_tm(ctx.mesh, ctx.mats, isPlaneStress, 0.0);
    applyBoundaryConditions(solver_tm, load_case, false);
    solver_tm.applyBoundaryConditions();

    int load_steps = ctx.config.getLoadSteps();    
    std::string str_case = loadCaseToString(load_case);
    std::cout << "\n--- EXECUTION ANALYSE SERVICE COUPLÉ ---" << std::endl;

    auto export_step = [&](int step, double current_dT) {
        PostProcessor post(ctx.mesh, ctx.mats, solver_tm.getSolution(), isPlaneStress, current_dT);
        post.setDamageState(solver_tm.getDamageState());
        post.setStressState(solver_tm.getSxx(), solver_tm.getSyy(), solver_tm.getTxy());
        post.setFailureIndex(solver_tm.getFailureIndex()); // <-- LIGNE A AJOUTER
        
        char filename[512];
        std::snprintf(filename, sizeof(filename), "%s/coupled_step_%03d.vtk", ctx.config.getOutputDir().c_str(), step);
        post.exportToVTK(filename);
    };

    solver_tm.solveNonLinear(deltaT, load_steps, export_step);
    return DetailedResults();
}

double HomogenizationManager::runLongitudinalShearCase() {
    Solver solver(ctx.mesh, ctx.mats, isPlaneStress, 0.0);
    solver.setAntiPlaneMode(true);
    applyBoundaryConditions(solver, LoadCase::LongitudinalShear, false);
    
    solver.assemble();
    solver.applyBoundaryConditions();
    solver.solve();

    const Eigen::VectorXd& U = solver.getSolution();
    double totalEnergy = 0.0;

    for (int t = 0; t < (int)ctx.mesh.getTriangles().size(); ++t) {
        const Triangle& tri = ctx.mesh.getTriangles()[t];
        double G = ctx.mats.getShearModulus(tri.label);
        double area;
        Eigen::Matrix<double, 3, 6> B_2D = ctx.mesh.computeBMatrix(t, area);
        
        Eigen::Matrix<double, 2, 3> B;
        for(int i = 0; i < 3; ++i) {
            B(0, i) = B_2D(0, 2*i);   
            B(1, i) = B_2D(1, 2*i+1); 
        }
        
        Eigen::Matrix<double, 3, 3> Ke = B.transpose() * G * B * area;
        Eigen::Vector3d ue(U(tri.v[0]), U(tri.v[1]), U(tri.v[2]));
        totalEnergy += 0.5 * ue.transpose() * Ke * ue;
    }

    double G12_eff = (2.0 * totalEnergy) / (Lx * Ly * strain * strain);
    std::cout << "   [Homogenization] G12 effectif : " << G12_eff / 1000.0 << " GPa" << std::endl;

    return G12_eff;
}

Eigen::Matrix3d HomogenizationManager::computeEffectiveStiffness(double E1, double E2, double G12, double nu12, double nu21) const {
    Eigen::Matrix3d S = Eigen::Matrix3d::Zero();
    S(0, 0) = 1.0 / E1; 
    S(1, 1) = 1.0 / E2;
    S(0, 1) = -nu12 / E1; 
    S(1, 0) = -nu21 / E2; 
    S(2, 2) = 1.0 / G12;
    return S.inverse();
}