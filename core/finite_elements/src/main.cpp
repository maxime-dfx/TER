#include <iostream>
#include <vector>
#include <fstream>
#include <sys/stat.h>
#include <algorithm> 
#include "Datafile.h"
#include "Mesh.h"
#include "Material.h"
#include "TopologyAnalyzer.h"
#include "SimulationContext.h"
#include "HomogenizationManager.h"
#include "ResultExporter.h"

using namespace std;

void ensureDir(const string& path) {
    if (path.empty() || path == ".") return;
    #if defined(_WIN32)
        mkdir(path.c_str());
    #else 
        mkdir(path.c_str(), 0755);
    #endif
}

int main(int argc, char** argv) {
    string configFile = (argc > 1) ? argv[1] : "simulation.toml";

    bool runTx = true, runTy = true, runShear = true;
    bool runThermo = true, runCoupled = true;
    bool runLongShear = true;

    if (argc > 2) {
        runTx = runTy = runShear = runThermo = runCoupled = runLongShear = false; 
        for (int i = 2; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--tx")      runTx = true;
            if (arg == "--ty")      runTy = true;
            if (arg == "--shear")   runShear = true;
            if (arg == "--long-shear") runLongShear = true;
            if (arg == "--meca")    { runTx = true; runTy = true; runShear = true; runLongShear = true; }
            if (arg == "--thermo")  runThermo = true;
            if (arg == "--couplage") runCoupled = true;
            if (arg == "--all")     { runTx = runTy = runShear = runThermo = runCoupled = runLongShear = true; }
        }
    }

    try {
        cout << "\n=== SIMULATION TER : ANALYSE MULTI-PHYSIQUE ===" << endl;
        
        DataFile config(configFile); 
        ensureDir(config.getOutputDir());
        ResultExporter logger(config.getOutputDir());

        logger.logInfo("Chargement du maillage : " + config.getMeshFile());
        Mesh mesh;
        mesh.read(config.getMeshFile());

        MaterialManager mats;
        mats.addMaterial(config.getMatriceLabel(), config.getMatriceE(), config.getMatriceNu(), config.getMatriceAlpha(), config.getMatriceXt(), config.getMatriceXc());
        mats.addMaterial(config.getFibreLabel(), config.getFibreE(), config.getFibreNu(), config.getFibreAlpha(), config.getFibreXt(), config.getFibreXc());
        mats.addMaterial(config.getInterphaseLabel(), config.getInterphaseE(), config.getInterphaseNu(), config.getInterphaseAlpha(), config.getInterphaseXt(), config.getInterphaseXc());
                         
        TopologyAnalyzer topo(mesh, 1e-5);
        topo.analyze();
        logger.logTopology(topo);

        SimulationContext context { mesh, mats, topo, config };
        HomogenizationManager experiment(context);
        
        // Initialisation à zéro
        DetailedResults resTractionX{}, resTractionY{}, resShear{}, resThermique{};
        double G12_long_eff = 0.0;

        if (runTx || runTy || runShear || runLongShear) {
            logger.logSection("CARACTERISATION MECANIQUE (Delta T = 0)");
            if (runTx) resTractionX = experiment.runMechanicalCase(LoadCase::TensionX);
            if (runTy) resTractionY = experiment.runMechanicalCase(LoadCase::TensionY);
            if (runShear) resShear = experiment.runMechanicalCase(LoadCase::Shear);
            if (runLongShear) G12_long_eff = experiment.runLongitudinalShearCase();
        }

        if (runThermo && abs(config.getDeltaT()) > 1e-9) {
            logger.logSection("CARACTERISATION THERMIQUE");
            resThermique = experiment.runThermalCase();
        }

        if (runCoupled && abs(config.getDeltaT()) > 1e-9 && abs(config.getStrainTarget()) > 1e-9) {
            logger.logSection("ESSAI EN SERVICE (Couplage Thermo-Meca)");
            experiment.runThermoMechanicalCase(LoadCase::TensionX);
        }

        // ====================================================================
        // --- EXPORT JSON DYNAMIQUE ---
        // ====================================================================
        string outDir = config.getOutputDir();
        if (!outDir.empty() && outDir.back() != '/' && outDir.back() != '\\') outDir += "/";
        string jsonPath = outDir + "results.json";

        std::ofstream out_file(jsonPath);
        if (!out_file.is_open()) {
            jsonPath = "results.json";
            out_file.open(jsonPath);
        }

        if (out_file.is_open()) {
            out_file << "{\n";
            out_file << "  \"E1\": " << (resTractionX.E_eff / 1000.0) << ",\n"; 
            out_file << "  \"nu12\": " << resTractionX.nu_eff << ",\n";
            out_file << "  \"E2\": " << (resTractionY.E_eff / 1000.0) << ",\n"; 
            out_file << "  \"nu23\": " << resTractionY.nu_eff << ",\n";
            out_file << "  \"G12\": " << (G12_long_eff / 1000.0) << ",\n";
            out_file << "  \"AlphaX\": " << resThermique.alpha_x << ",\n";
            out_file << "  \"AlphaY\": " << resThermique.alpha_y << ",\n";
            
            // Legacy pour le runner principal
            double mod = 0.0, poi = 0.0;
            if (runTx) { mod = resTractionX.E_eff / 1000.0; poi = resTractionX.nu_eff; }
            else if (runTy) { mod = resTractionY.E_eff / 1000.0; poi = resTractionY.nu_eff; }
            else if (runLongShear) { mod = G12_long_eff / 1000.0; }
            else if (runShear) { mod = resShear.G_eff / 1000.0; }
            
            out_file << "  \"module\": " << mod << ",\n";
            out_file << "  \"poisson\": " << poi << "\n";
            out_file << "}\n";
            out_file.close();
            std::cout << "   -> Resultats exportes dans : " << jsonPath << std::endl;
        }

    } catch (const exception& e) {
        cerr << "\n[CRASH C++] ERREUR FATALE: " << e.what() << endl;
        return 1;
    }
    return 0;
}