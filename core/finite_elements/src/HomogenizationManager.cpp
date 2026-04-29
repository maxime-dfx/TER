#include "HomogenizationManager.h"
#include "json.hpp"
#include <iostream>
#include <fstream> 
#include <cstdio>
#include <cmath>
#include <vector>


using json = nlohmann::json;

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
        case LoadCase::TensionZ: return "traction_z";
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

    // Tolérance pour la géométrie
    const double tol = 1e-6;

    // Sélecteur universel pour capter TOUT le contour du VER (Requis pour les vraies KUBC)
    auto isBoundary = [this, tol](double x, double y) {
        return std::abs(x - bb.xmin) < tol || std::abs(x - bb.xmax) < tol ||
               std::abs(y - bb.ymin) < tol || std::abs(y - bb.ymax) < tol;
    };

    if (isThermalOnly) {
        // ISOSTATISME STRICT pour dilatation thermique libre (Évite la matrice singulière)
        // On bloque le noeud origine (xmin, ymin) en X et Y (Translation nulle)
        solver.addBoundaryCondition([this, tol](double x, double y){ 
            return std::abs(x - bb.xmin) < tol && std::abs(y - bb.ymin) < tol; 
        }, 0, [](double, double){ return 0.0; });
        solver.addBoundaryCondition([this, tol](double x, double y){ 
            return std::abs(x - bb.xmin) < tol && std::abs(y - bb.ymin) < tol; 
        }, 1, [](double, double){ return 0.0; });
        
        // On bloque le noeud (xmax, ymin) en Y uniquement (Rotation nulle)
        solver.addBoundaryCondition([this, tol](double x, double y){ 
            return std::abs(x - bb.xmax) < tol && std::abs(y - bb.ymin) < tol; 
        }, 1, [](double, double){ return 0.0; });
        return;
    }

    switch (load_case) {
        case LoadCase::ThermoMechanical: 
        case LoadCase::TensionX:
            if (isPeriodic) {
                // Origine fixée
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymin)<tol; }, 0, [](double, double){ return 0.0; });
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymin)<tol; }, 1, [](double, double){ return 0.0; });
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmax)<tol && std::abs(y-bb.ymin)<tol; }, 1, [](double, double){ return 0.0; });
                
                for (auto p : pairsLR) {
                    solver.addPeriodicCondition(p.second, p.first, 0, Lx * strain);
                    solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
                }
                for (auto p : pairsBT) solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
            } else {
                // VRAI KUBC Traction X : E11 = strain, E22 = 0, E12 = 0 appliquées sur TOUT le bord
                solver.addBoundaryCondition(isBoundary, 0, [this](double x, double y) { return strain * (x - bb.xmin); });
                solver.addBoundaryCondition(isBoundary, 1, [](double x, double y) { return 0.0; });
            }
            break;

        case LoadCase::TensionY:
            if (isPeriodic) {
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymin)<tol; }, 0, [](double, double){ return 0.0; });
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymin)<tol; }, 1, [](double, double){ return 0.0; });
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymax)<tol; }, 0, [](double, double){ return 0.0; });
                for (auto p : pairsBT) {
                    solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
                    solver.addPeriodicCondition(p.second, p.first, 1, Ly * strain);
                }
                for (auto p : pairsLR) solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
            } else {
                // VRAI KUBC Traction Y : E11 = 0, E22 = strain, E12 = 0 appliquées sur TOUT le bord
                solver.addBoundaryCondition(isBoundary, 0, [](double x, double y) { return 0.0; });
                solver.addBoundaryCondition(isBoundary, 1, [this](double x, double y) { return strain * (y - bb.ymin); });
            }
            break;

        case LoadCase::TensionZ:
            // Pour construire C33, C13, C23, on bloque les déformations dans le plan (KUBC strictes)
            solver.addBoundaryCondition(isBoundary, 0, [](double /*x*/, double /*y*/) { return 0.0; });
            solver.addBoundaryCondition(isBoundary, 1, [](double /*x*/, double /*y*/) { return 0.0; });
            break;

        case LoadCase::Shear:
            if (isPeriodic) {
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymin)<tol; }, 0, [](double, double){ return 0.0; });
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymin)<tol; }, 1, [](double, double){ return 0.0; });
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmax)<tol && std::abs(y-bb.ymin)<tol; }, 1, [](double, double){ return 0.0; });
                for (auto p : pairsBT) {
                    solver.addPeriodicCondition(p.second, p.first, 0, Ly * strain);
                    solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
                }
                for (auto p : pairsLR) {
                    solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
                    solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
                }
            } else {
                // VRAI KUBC Cisaillement (Simple Shear formulation) : ux = strain * y, uy = 0
                solver.addBoundaryCondition(isBoundary, 0, [this](double x, double y) { return strain * (y - bb.ymin); });
                solver.addBoundaryCondition(isBoundary, 1, [](double x, double y) { return 0.0; });
            }
            break;

        case LoadCase::LongitudinalShear:
            if (isPeriodic) {
                solver.addBoundaryCondition([this, tol](double x, double y){ return std::abs(x-bb.xmin)<tol && std::abs(y-bb.ymin)<tol; }, 0, [](double, double){ return 0.0; });
                for (auto p : pairsLR) solver.addPeriodicCondition(p.second, p.first, 0, Lx * strain);
                for (auto p : pairsBT) solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
            } else {
                // VRAI KUBC Cisaillement Anti-Plan : uz = strain * x
                solver.addBoundaryCondition(isBoundary, 0, [this](double x, double y) { return strain * (x - bb.xmin); });
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
    
    // Les appels solver.assemble() et solver.applyBoundaryConditions() ont été retirés.
    // Le solveur gère tout ça en interne grâce à la refonte HPC.
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

    // L'appel solver_th.applyBoundaryConditions() a été retiré.

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
    std::cout << "\n--- EXECUTION ANALYSE SERVICE COUPLÉ ---" << std::endl;
    
    Solver solver_tm(ctx.mesh, ctx.mats, isPlaneStress, 0.0);
    applyBoundaryConditions(solver_tm, load_case, false);

    // PILOTAGE VIA LE TOML
    if (ctx.config.includeCuringStresses()) {
        std::cout << " -> Mode Haute Fidélité : Calcul et injection des contraintes de cuisson." << std::endl;
        
        double dT_cuisson = ctx.config.getCuringDeltaT();
        Solver solver_cuisson(ctx.mesh, ctx.mats, isPlaneStress, 0.0);
        applyBoundaryConditions(solver_cuisson, LoadCase::Thermal, true);
        
        solver_cuisson.solveNonLinear(dT_cuisson, 10, nullptr);
        solver_tm.injectInternalState(solver_cuisson.getDamageState(), solver_cuisson.getFailureIndex());
    } else {
        std::cout << " -> Mode Simple : Matériau supposé vierge de contraintes initiales." << std::endl;
    }

    int load_steps = ctx.config.getLoadSteps();    
    std::string str_case = loadCaseToString(load_case) + "_coupled";

    auto export_step = [&](int step, double current_control_var) {
        PostProcessor post(ctx.mesh, ctx.mats, solver_tm.getSolution(), isPlaneStress, 0.0);
        post.setDamageState(solver_tm.getDamageState());
        post.setStressState(solver_tm.getSxx(), solver_tm.getSyy(), solver_tm.getTxy());
        post.setFailureIndex(solver_tm.getFailureIndex());
        
        char filename[512];
        std::snprintf(filename, sizeof(filename), "%s/%s_step_%03d.vtk", 
                      ctx.config.getOutputDir().c_str(), str_case.c_str(), step);
        post.exportToVTK(filename);
    };

    // On lance la traction
    solver_tm.solveNonLinear(strain, load_steps, export_step);
    return DetailedResults();
}
double HomogenizationManager::runLongitudinalShearCase() {
    Solver solver(ctx.mesh, ctx.mats, isPlaneStress, 0.0);
    solver.setAntiPlaneMode(true);
    applyBoundaryConditions(solver, LoadCase::LongitudinalShear, false);
    
    // Les appels solver.assemble() et solver.applyBoundaryConditions() ont été retirés.
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

DetailedResults HomogenizationManager::runElementaryCase(LoadCase load_case) {
    double dT = (load_case == LoadCase::Thermal) ? ctx.config.getDeltaT() : 0.0;
    Solver solver(ctx.mesh, ctx.mats, isPlaneStress, dT);
    
    // === INJECTION DE LA PHYSIQUE ===
    if (load_case == LoadCase::LongitudinalShear) {
        solver.setAntiPlaneMode(true); 
    } else if (load_case == LoadCase::TensionZ) {
        solver.setImposedEzz(strain); // Injection de la déformation axiale Z
    }
    
    applyBoundaryConditions(solver, load_case, (load_case == LoadCase::Thermal));
    solver.solve();
    
    PostProcessor post(ctx.mesh, ctx.mats, solver.getSolution(), isPlaneStress, dT);
    DetailedResults res = post.computeEffectiveProperties(load_case, strain);
    
    std::string filename = ctx.config.getOutputDir() + "/" + loadCaseToString(load_case) + "_linear.vtk";
    post.exportToVTK(filename);
    
    return res;
}

Eigen::Matrix3d HomogenizationManager::identifyStiffnessTensor(const DetailedResults& rx, 
                                                               const DetailedResults& ry, 
                                                               const DetailedResults& rs) const {
    Eigen::Matrix3d C = Eigen::Matrix3d::Zero();
    
    // Construction directe de la matrice de rigidité par la méthode des déformations unitaires (KUBC)
    // Puisque eps_imposé = strain, alors C_ij = sigma_i / strain
    C(0,0) = rx.macro_sig_xx / strain;
    C(1,0) = rx.macro_sig_yy / strain;
    C(2,0) = rx.macro_tau_xy / strain;
    
    C(0,1) = ry.macro_sig_xx / strain;
    C(1,1) = ry.macro_sig_yy / strain;
    C(2,1) = ry.macro_tau_xy / strain;
    
    C(0,2) = rs.macro_sig_xx / strain;
    C(1,2) = rs.macro_sig_yy / strain;
    C(2,2) = rs.macro_tau_xy / strain;
    
    // Symétrisation thermodynamique (Maxwell-Betti) pour corriger les asymétries numériques du maillage
    C(0,1) = 0.5 * (C(0,1) + C(1,0)); C(1,0) = C(0,1);
    C(0,2) = 0.5 * (C(0,2) + C(2,0)); C(2,0) = C(0,2);
    C(1,2) = 0.5 * (C(1,2) + C(2,1)); C(2,1) = C(1,2);
    
    return C;
}

void HomogenizationManager::runProgressiveDamageAnalysis(LoadCase lc) {
    std::cout << "\n=======================================================\n";
    std::cout << "   ANALYSE NON-LINEAIRE (PDA) : " << loadCaseToString(lc) << "\n";
    std::cout << "=======================================================\n";

    int numSteps = ctx.config.getLoadSteps();
    if (numSteps <= 0) numSteps = 20; 
    
    double target_strain = ctx.config.getStrainTarget();
    bool isThermal = (lc == LoadCase::Thermal);
    
    std::vector<double> current_damage(ctx.mesh.getTriangles().size(), 0.0);
    double max_stress = 0.0;
    int peak_step = 0;

    // --- 1. INITIALISATION DU JSON ---
    json j_results;
    j_results["metadata"]["load_case"] = loadCaseToString(lc);
    j_results["metadata"]["steps"] = numSteps;
    j_results["pda_curve"]["strain"] = json::array();
    j_results["pda_curve"]["stress"] = json::array();
    j_results["pda_curve"]["damage_volume_pct"] = json::array();

    for (int step = 1; step <= numSteps; ++step) {
        double current_strain = isThermal ? 0.0 : (target_strain / numSteps) * step;
        double current_dT = isThermal ? (ctx.config.getDeltaT() / numSteps) * step : 0.0;
        
        this->strain = current_strain; 
        this->deltaT = current_dT;
        
        Solver solver(ctx.mesh, ctx.mats, isPlaneStress, current_dT);
        solver.injectInternalState(current_damage, std::vector<double>());
        
        applyBoundaryConditions(solver, lc, isThermal);
        solver.solve();
        
        PostProcessor post(ctx.mesh, ctx.mats, solver.getSolution(), isPlaneStress, current_dT);
        DetailedResults res = post.computeEffectiveProperties(lc, current_strain);
        
        post.updateDamageState(current_damage);
        post.setDamageState(current_damage); 
        
        // Calcul du volume endommagé
        double damaged_area = 0.0, total_area = 0.0;
        for (size_t t = 0; t < current_damage.size(); ++t) {
            const auto& tri = ctx.mesh.getTriangles()[t];
            const auto& p0 = ctx.mesh.getVertices()[tri.v[0]];
            const auto& p1 = ctx.mesh.getVertices()[tri.v[1]];
            const auto& p2 = ctx.mesh.getVertices()[tri.v[2]];
            double a = 0.5 * std::abs((p1.x - p0.x)*(p2.y - p0.y) - (p2.x - p0.x)*(p1.y - p0.y));
            total_area += a;
            if (current_damage[t] > 0.5) damaged_area += a;
        }
        double dam_vol_pct = (damaged_area / total_area) * 100.0;
        
        double display_stress = 0.0, display_modulus = 0.0;
        if(lc == LoadCase::TensionX) { display_stress = res.macro_sig_xx; display_modulus = res.E_eff; }
        else if(lc == LoadCase::TensionY) { display_stress = res.macro_sig_yy; display_modulus = res.E_eff; }
        else if(lc == LoadCase::Shear) { display_stress = res.macro_tau_xy; display_modulus = res.G_eff; }
        
        if (std::abs(display_stress) > std::abs(max_stress)) {
            max_stress = std::abs(display_stress);
            peak_step = step;
        }

        // --- 2. STOCKAGE DES DONNÉES DANS LE JSON ---
        j_results["pda_curve"]["strain"].push_back(current_strain);
        j_results["pda_curve"]["stress"].push_back(display_stress);
        j_results["pda_curve"]["damage_volume_pct"].push_back(dam_vol_pct);
        
        // Affichage console 
        char line[256];
        if (isThermal) {
            std::snprintf(line, sizeof(line), "  %02d  | %9.1f | %10.1f | %11.1f | %12.2f", step, current_dT, display_stress, display_modulus / 1000.0, dam_vol_pct);
        } else {
            std::snprintf(line, sizeof(line), "  %02d  | %8.2f %% | %10.1f | %11.1f | %12.2f", step, current_strain * 100.0, display_stress, display_modulus / 1000.0, dam_vol_pct);
        }
        std::cout << line << "\n";
    }
    
    // --- 3. ÉCRITURE DU FICHIER JSON ---
    j_results["results"]["ultimate_strength"] = max_stress;
    j_results["results"]["peak_step"] = peak_step;
    
    // Sauvegarde dans le workspace (dossier de sortie configuré)
    std::string json_filename = ctx.config.getOutputDir() + "/results_pda_" + loadCaseToString(lc) + ".json";
    std::ofstream o(json_filename);
    o << std::setw(4) << j_results << std::endl; 
    
    std::cout << "-----------------------------------------------------------------------------\n";
    std::cout << " -> RESISTANCE ULTIME : " << max_stress << " MPa (atteinte au step " << peak_step << ")\n";
    std::cout << " -> [OK] Resultats JSON exportes dans : " << json_filename << "\n\n";
}