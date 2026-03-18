#include <iostream>
#include <iomanip>
#include <sys/stat.h>
#include <vector>
#include <cmath>
#include <Eigen/Dense>

#include "Mesh.h"
#include "Material.h"
#include "Solver.h"
#include "PostProcessor.h"
#include "Datafile.h" 

using namespace std;

void ensureDir(const string& path) {
    #if defined(_WIN32)
        mkdir(path.c_str());
    #else 
        mkdir(path.c_str(), 0755);
    #endif
}

void printLine(int width = 70) {
    cout << setfill('-') << setw(width) << "-" << setfill(' ') << endl;
}

int main(int argc, char** argv) {
    string configFile = (argc > 1) ? argv[1] : "simulation.toml";

    try {
        cout << "\n=== SIMULATION TER : CARACTERISATION DU TENSEUR DE RIGIDITE ===" << endl;
        
        DataFile config(configFile); 
        
        cout << ">> Configuration chargee : " << configFile << endl;
        cout << "   -> Mesh : " << config.getMeshFile() << endl;

        ensureDir(config.getOutputDir());

        Mesh mesh;
        mesh.read(config.getMeshFile());
        auto bb = mesh.getBounds();

        MaterialManager mats;
        mats.addMaterial(config.getMatriceLabel(), config.getMatriceE(), config.getMatriceNu());
        mats.addMaterial(config.getFibreLabel(),   config.getFibreE(),   config.getFibreNu());

        // --- CORRECTION 1.A : Lecture de l'hypothèse (Déformation/Contrainte Plane) ---
        bool isPlaneStress = !config.isPlaneStrain();
        cout << "   -> Hypothese 2D : " << (isPlaneStress ? "Contraintes Planes" : "Deformations Planes") << endl;

        double strain = config.getStrainTarget();
        double Lx = bb.L;
        double Ly = bb.H;

        // --- PREPARATION DES PAIRES POUR CONDITIONS PERIODIQUES (PBC) ---
        const auto& verts = mesh.getVertices();
        double tol = 1e-5 * max(Lx, Ly);
        
        vector<int> nodesLeft, nodesRight, nodesBottom, nodesTop;
        for (size_t i = 0; i < verts.size(); ++i) {
            if (abs(verts[i].x - bb.xmin) < tol) nodesLeft.push_back(i);
            if (abs(verts[i].x - bb.xmax) < tol) nodesRight.push_back(i);
            if (abs(verts[i].y - bb.ymin) < tol) nodesBottom.push_back(i);
            if (abs(verts[i].y - bb.ymax) < tol) nodesTop.push_back(i);
        }

        // Appairage Gauche -> Droite (Même Y)
        vector<pair<int, int>> pairsLR; 
        for (int m : nodesLeft) {
            for (int s : nodesRight) {
                if (abs(verts[m].y - verts[s].y) < tol) {
                    pairsLR.push_back({m, s}); break;
                }
            }
        }

        // Appairage Bas -> Haut (Même X)
        vector<pair<int, int>> pairsBT; 
        for (int m : nodesBottom) {
            for (int s : nodesTop) {
                if (abs(verts[m].x - verts[s].x) < tol) {
                    pairsBT.push_back({m, s}); break;
                }
            }
        }

        // Vérification si le maillage permet les PBC
        bool isPeriodic = (pairsLR.size() == nodesLeft.size() && pairsBT.size() == nodesBottom.size() && !nodesLeft.empty());
        cout << "   -> Appairage periodique : " << pairsLR.size() << " noeuds G/D, " << pairsBT.size() << " noeuds B/H." << endl;
        
        if (!isPeriodic) {
            cout << "   /!\\ Le maillage n'est pas parfaitement periodique. Utilisation des CL de Dirichlet classiques (KUBC)." << endl;
        } else {
            cout << "   -> OK : Maillage periodique detecte. Utilisation des PBC." << endl;
        }

        double E1 = 0.0, E2 = 0.0, nu12 = 0.0, nu21 = 0.0, G12 = 0.0;
        double Vf = 0.0; 

        vector<string> cases = {"traction_x", "traction_y", "cisaillement"};

        cout << "\n--- LANCEMENT DES EXPERIENCES VIRTUELLES ---" << endl;

        for (const string& load_case : cases) {
            cout << "\n>> Cas de charge : " << load_case << " (Deformation: " << strain << ")" << endl;
            
            // --- CORRECTION 1.A : Transmission de l'hypothèse au Solveur ---
            Solver solver(mesh, mats, isPlaneStress);

            if (load_case == "traction_x") {
                if (isPeriodic) {
                    // Fixations de corps rigide (coins)
                    solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmin)<1e-6 && abs(y-bb.ymin)<1e-6; }, 0, 0.0);
                    solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmin)<1e-6 && abs(y-bb.ymin)<1e-6; }, 1, 0.0);
                    solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmax)<1e-6 && abs(y-bb.ymin)<1e-6; }, 1, 0.0);

                    for (auto p : pairsLR) {
                        solver.addPeriodicCondition(p.second, p.first, 0, Lx * strain);
                        solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
                    }
                    for (auto p : pairsBT) {
                        solver.addPeriodicCondition(p.second, p.first, 0, 0.0); // Couplage libre en V (Poisson)
                    }
                } else {
                    // Fallback KUBC
                    solver.addBoundaryCondition([&bb](double x, double /*y*/){ return abs(x-bb.xmin)<1e-6; }, 0, 0.0);
                    solver.addBoundaryCondition([&bb](double x, double /*y*/){ return abs(x-bb.xmax)<1e-6; }, 0, Lx * strain);
                    solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmin)<1e-6 && abs(y-bb.ymin)<1e-6; }, 1, 0.0);
                }
                
            } else if (load_case == "traction_y") {
                if (isPeriodic) {
                    solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmin)<1e-6 && abs(y-bb.ymin)<1e-6; }, 0, 0.0);
                    solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmin)<1e-6 && abs(y-bb.ymin)<1e-6; }, 1, 0.0);
                    solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmin)<1e-6 && abs(y-bb.ymax)<1e-6; }, 0, 0.0);

                    for (auto p : pairsBT) {
                        solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
                        solver.addPeriodicCondition(p.second, p.first, 1, Ly * strain);
                    }
                    for (auto p : pairsLR) {
                        solver.addPeriodicCondition(p.second, p.first, 1, 0.0); // Couplage libre en U (Poisson)
                    }
                } else {
                    solver.addBoundaryCondition([&bb](double /*x*/, double y){ return abs(y-bb.ymin)<1e-6; }, 1, 0.0);
                    solver.addBoundaryCondition([&bb](double /*x*/, double y){ return abs(y-bb.ymax)<1e-6; }, 1, Ly * strain);
                    solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmin)<1e-6 && abs(y-bb.ymin)<1e-6; }, 0, 0.0);
                }

            } else if (load_case == "cisaillement") {
                if (isPeriodic) {
                    solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmin)<1e-6 && abs(y-bb.ymin)<1e-6; }, 0, 0.0);
                    solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmin)<1e-6 && abs(y-bb.ymin)<1e-6; }, 1, 0.0);
                    solver.addBoundaryCondition([&bb](double x, double y){ return abs(x-bb.xmax)<1e-6 && abs(y-bb.ymin)<1e-6; }, 1, 0.0);

                    for (auto p : pairsBT) {
                        solver.addPeriodicCondition(p.second, p.first, 0, Ly * strain);
                        solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
                    }
                    for (auto p : pairsLR) {
                        solver.addPeriodicCondition(p.second, p.first, 0, 0.0);
                        solver.addPeriodicCondition(p.second, p.first, 1, 0.0);
                    }
                } else {
                    solver.addBoundaryCondition([&bb](double /*x*/, double y){ return abs(y-bb.ymin)<1e-6; }, 0, 0.0);
                    solver.addBoundaryCondition([&bb](double /*x*/, double y){ return abs(y-bb.ymin)<1e-6; }, 1, 0.0);
                    solver.addBoundaryCondition([&bb](double /*x*/, double y){ return abs(y-bb.ymax)<1e-6; }, 0, Ly * strain);
                    solver.addBoundaryCondition([&bb](double /*x*/, double y){ return abs(y-bb.ymax)<1e-6; }, 1, 0.0);
                }
            }
            
            solver.assemble();
            solver.applyBoundaryConditions();
            solver.solve();

            // --- CORRECTION 1.A : Transmission de l'hypothèse au PostProcessor ---
            PostProcessor post(mesh, mats, solver.getSolution(), isPlaneStress);
            
            // --- CORRECTION 1.B & 1.C : Passage de bb (BoundingBox) ---
            DetailedResults res = post.runAnalysis(Lx * Ly, strain, bb, config.getFibreLabel(), config.getMatriceLabel(), load_case);

            if (load_case == "traction_x") {
                E1 = res.E_eff;
                nu12 = res.nu_eff;
                Vf = res.Vf;
            } else if (load_case == "traction_y") {
                E2 = res.E_eff;
                nu21 = res.nu_eff;
            } else if (load_case == "cisaillement") {
                G12 = res.G_eff;
            }

            string fullPath = config.getOutputDir() + "/" + load_case + "_" + config.getOutputVtk();
            post.exportToVTK(fullPath);
        }

        double Vm = 1.0 - Vf;
        auto planeStrainProps = [](double E, double nu) { return E / (1.0 - nu*nu); };
        double E_f_ps = planeStrainProps(config.getFibreE(), config.getFibreNu());
        double E_m_ps = planeStrainProps(config.getMatriceE(), config.getMatriceNu());

        double E_Voigt = Vf * E_f_ps + Vm * E_m_ps; 
        double E_Reuss = 1.0 / (Vf/E_f_ps + Vm/E_m_ps); 

        double Em = config.getMatriceE();
        double Ef = config.getFibreE();
        double xi = config.getHalpinTsaiXi(); 
        double eta = ((Ef / Em) - 1.0) / ((Ef / Em) + xi);
        double E_HalpinTsai_3D = Em * (1.0 + xi * eta * Vf) / (1.0 - eta * Vf);
        double E_HalpinTsai_PS = E_HalpinTsai_3D / (1.0 - config.getMatriceNu()*config.getMatriceNu());

        Eigen::Matrix3d S = Eigen::Matrix3d::Zero();
        S(0, 0) = 1.0 / E1;
        S(1, 1) = 1.0 / E2;
        S(0, 1) = -nu12 / E1;
        S(1, 0) = -nu21 / E2;
        S(2, 2) = 1.0 / G12;

        Eigen::Matrix3d C_eff = S.inverse();

        double toGPa = 1.0/1000.0;
        cout << "\n";
        printLine();
        cout << left << setw(35) << "Fraction Volumique (Vf)" << setw(15) << Vf * 100.0 << " %" << endl;
        cout << left << setw(35) << "E1 (FEM - Traction X)"   << setw(15) << E1 * toGPa << " GPa" << endl;
        cout << left << setw(35) << "E2 (FEM - Traction Y)"   << setw(15) << E2 * toGPa << " GPa" << endl;
        cout << left << setw(35) << "G12 (FEM - Cisaillement)"<< setw(15) << G12 * toGPa << " GPa" << endl;
        cout << left << setw(35) << "nu12 / nu21 (FEM)"       << nu12 << " / " << nu21 << endl;
        printLine();
        cout << left << setw(35) << "E1 Voigt (Borne Sup idealisée)" << setw(15) << E_Voigt * toGPa << " GPa" << endl;
        cout << left << setw(35) << "E2 Reuss (Borne Inf idealisée)" << setw(15) << E_Reuss * toGPa << " GPa" << endl;
        cout << left << setw(35) << "E2 Halpin-Tsai (Estimation)"    << setw(15) << E_HalpinTsai_PS * toGPa << " GPa" << endl;
        printLine();
        
        cout << "\n   TENSEUR DE RIGIDITE EFFECTIF C_eff (MPa) :" << endl;
        cout << C_eff << "\n" << endl;
        
        printLine();
        cout << "-> Verification Symetrie Thermodynamique (Maxwell-Betti) :" << endl;
        cout << "   nu12/E1 = " << nu12 / E1 << endl;
        cout << "   nu21/E2 = " << nu21 / E2 << endl;
        
        if (abs(nu12/E1 - nu21/E2) > 1e-6) {
            cout << "   /!\\ Attention : Asymetrie detectee. Verifiez l'isotropie geometrique du maillage." << endl;
        } else {
            cout << "   OK : Le tenseur est parfaitement symetrique." << endl;
        }
        printLine();

    } catch (const exception& e) {
        cerr << "ERREUR FATALE: " << e.what() << endl;
        return 1;
    }
    return 0;
}