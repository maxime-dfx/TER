#include <iostream>
#include <vector>
#include <sys/stat.h>
#include "Datafile.h"
#include "Mesh.h"
#include "Material.h"
#include "TopologyAnalyzer.h"
#include "SimulationContext.h"
#include "HomogenizationManager.h"
#include "ResultExporter.h"

using namespace std;

void ensureDir(const string& path) {
    #if defined(_WIN32)
        mkdir(path.c_str());
    #else 
        mkdir(path.c_str(), 0755);
    #endif
}

int main(int argc, char** argv) {
    string configFile = (argc > 1) ? argv[1] : "simulation.toml";

    // --- GESTION ULTRA-FINE DES OPTIONS DE LANCEMENT ---
    bool runTx = true, runTy = true, runShear = true;
    bool runThermo = true, runCoupled = true;

    if (argc > 2) {
        // Dès qu'on met un flag, on désactive tout par défaut
        runTx = runTy = runShear = runThermo = runCoupled = false; 
        
        for (int i = 2; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--tx")      runTx = true;
            if (arg == "--ty")      runTy = true;
            if (arg == "--shear")   runShear = true;
            if (arg == "--meca")    { runTx = true; runTy = true; runShear = true; } // Raccourci
            if (arg == "--thermo")  runThermo = true;
            if (arg == "--couplage") runCoupled = true;
            if (arg == "--all")     { runTx = runTy = runShear = runThermo = runCoupled = true; }
        }
    }
    // ---------------------------------------------------

    try {
        cout << "\n=== SIMULATION TER : ANALYSE MULTI-PHYSIQUE ===" << endl;
        
        // 1. Initialisation
        DataFile config(configFile); 
        ensureDir(config.getOutputDir());
        ResultExporter logger(config.getOutputDir());

        logger.logInfo("Chargement du maillage : " + config.getMeshFile());
        Mesh mesh;
        mesh.read(config.getMeshFile());

        MaterialManager mats;
        mats.addMaterial(config.getMatriceLabel(), config.getMatriceE(), config.getMatriceNu(), config.getMatriceAlpha());
        mats.addMaterial(config.getFibreLabel(),   config.getFibreE(),   config.getFibreNu(),   config.getFibreAlpha());

        // 2. Topologie
        TopologyAnalyzer topo(mesh, 1e-5);
        topo.analyze();
        logger.logTopology(topo);

        // 3. Création du Contexte
        SimulationContext context { mesh, mats, topo, config };
        HomogenizationManager experiment(context);
        
        DetailedResults resTractionX, resTractionY, resShear, resThermique;

        // 4. Caractérisation Mécanique à la carte
        if (runTx || runTy || runShear) {
            logger.logSection("CARACTERISATION MECANIQUE (Delta T = 0)");
            
            if (runTx) {
                logger.logInfo("Cas : Traction X");
                resTractionX = experiment.runMechanicalCase(LoadCase::TensionX);
            }
            if (runTy) {
                logger.logInfo("Cas : Traction Y");
                resTractionY = experiment.runMechanicalCase(LoadCase::TensionY);
            }
            if (runShear) {
                logger.logInfo("Cas : Cisaillement");
                resShear = experiment.runMechanicalCase(LoadCase::Shear);
            }
        }

        // 5. Caractérisation Thermique
        if (runThermo && abs(config.getDeltaT()) > 1e-9) {
            logger.logSection("CARACTERISATION THERMIQUE");
            logger.logInfo("Calcul des precontraintes thermiques");
            resThermique = experiment.runThermalCase();
        }

        // 6. Essai en service couplé
        if (runCoupled && abs(config.getDeltaT()) > 1e-9 && abs(config.getStrainTarget()) > 1e-9) {
            logger.logSection("ESSAI EN SERVICE (Couplage Thermo-Meca)");
            logger.logInfo("Traction X avec Delta T");
            experiment.runThermoMechanicalCase(LoadCase::TensionX);
        }

        // 7. Synthèse Intelligente
        bool allMecaDone = (runTx && runTy && runShear);
        
        if (allMecaDone) {
            Eigen::Matrix3d C_eff = experiment.computeEffectiveStiffness(
                resTractionX.E_eff, resTractionY.E_eff, 
                resShear.G_eff, resTractionX.nu_eff, resTractionY.nu_eff
            );
            logger.printSynthesis(resTractionX, resTractionY, resShear, resThermique, C_eff, config);
            logger.appendToCSV("ver_results.csv", resTractionX, resTractionY, resShear, resThermique, config.getDeltaT());
        } else {
            logger.logSection("BILAN PARTIEL");
            logger.logInfo("/!\\ Calcul de C_eff et export CSV ignores.");
            logger.logInfo("    -> Motif : Vous n'avez pas lance tous les cas de charge mecaniques.");
            if (runTx) cout << "    -> E1 trouve = " << resTractionX.E_eff / 1000.0 << " GPa\n";
            if (runTy) cout << "    -> E2 trouve = " << resTractionY.E_eff / 1000.0 << " GPa\n";
            if (runShear) cout << "    -> G12 trouve = " << resShear.G_eff / 1000.0 << " GPa\n";
        }

    } catch (const exception& e) {
        cerr << "\nERREUR FATALE: " << e.what() << endl;
        return 1;
    }
    return 0;
}