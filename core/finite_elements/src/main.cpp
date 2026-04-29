#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include "Logger.h"
#include "Datafile.h"
#include "Mesh.h"
#include "Material.h"
#include "TopologyAnalyzer.h"
#include "SimulationContext.h"
#include "HomogenizationManager.h"
#include "ResultExporter.h"
#include "AnalyticalModels.h"

// Macro pour un affichage propre des sections
#define LOG_SECTION(msg) std::cout << "\n=======================================================\n" \
                                   << "   " << msg << "\n" \
                                   << "=======================================================\n";

using namespace std;

int main(int argc, char** argv) {
    // Utilisation du fichier passé en argument par Python (main.py)
    string configFile = (argc > 1) ? argv[1] : "input/input.toml";
    
    try {
        std::cout << "=== CompositeSim FE Engine : Analyse Modulaire 3D ===" << std::endl;
        
        // 1. Chargement de la configuration
        DataFile config(configFile);
        
        // 2. Chargement du Maillage
        LOG_INFO("Chargement du maillage : " + config.getMeshFile());
        Mesh mesh;
        mesh.read(config.getMeshFile()); 

        // 3. Initialisation des Matériaux
        MaterialManager mats;
        mats.addMaterialFromConfig(config, "matrice");
        mats.addMaterialFromConfig(config, "fibre");
        
        // On n'ajoute l'interphase que si elle est définie ou présente
        mats.addMaterialFromConfig(config, "interphase");

        // 4. Analyse Topologique (Périodicité)
        TopologyAnalyzer topo(mesh);
        topo.analyze();

        // 5. Initialisation du Contexte (Lien entre Maillage, Matériaux et Config)
        SimulationContext ctx{mesh, mats, topo, config};
        HomogenizationManager manager(ctx);
        ResultExporter exporter(config.getOutputDir());

        exporter.logTopology(topo);

        // ====================================================================
        // MODULE 1 : CARACTERISATION ELASTIQUE (MATRICE DE RIGIDITE 6x6)
        // ====================================================================
        if (config.runElasticity()) {
            LOG_SECTION("CARACTERISATION ELASTIQUE EQUIVALENTE");
            
            // Calculs élémentaires (Homogénéisation)
            DetailedResults resTX = manager.runElementaryCase(LoadCase::TensionX);
            DetailedResults resTY = manager.runElementaryCase(LoadCase::TensionY);
            DetailedResults resTZ = manager.runElementaryCase(LoadCase::TensionZ); 
            DetailedResults resShear = manager.runElementaryCase(LoadCase::Shear);
            
            // Construction de la matrice de rigidité (bloc normal)
            double s = config.getStrainTarget();
            Eigen::Matrix3d C_normal;
            C_normal << resTX.macro_sig_xx/s, resTY.macro_sig_xx/s, resTZ.macro_sig_xx/s,
                        resTX.macro_sig_yy/s, resTY.macro_sig_yy/s, resTZ.macro_sig_yy/s,
                        resTX.macro_sig_zz/s, resTY.macro_sig_zz/s, resTZ.macro_sig_zz/s;

            // Symétrisation pour lisser les erreurs numériques
            C_normal = 0.5 * (C_normal + C_normal.transpose()).eval();

            // Inversion pour obtenir les modules (Ex, Ey, Ez)
            Eigen::Matrix3d S_normal = C_normal.inverse();
            double Ex = 1.0 / S_normal(0,0);
            double Ey = 1.0 / S_normal(1,1);
            double Ez = 1.0 / S_normal(2,2); 

            // Calcul du cisaillement Hors-Plan (Anti-Plan)
            double G_xz = manager.runLongitudinalShearCase();

            // Assemblage du Tenseur de Rigidité Complet 6x6
            Eigen::Matrix<double, 6, 6> C_6x6 = Eigen::Matrix<double, 6, 6>::Zero();
            C_6x6.block<3,3>(0,0) = C_normal;
            C_6x6(3, 3) = G_xz; // yz
            C_6x6(4, 4) = G_xz; // xz
            C_6x6(5, 5) = resShear.G_eff; // xy

            std::cout << "\n   TENSEUR DE RIGIDITE EFFECTIF (MPa) :\n";
            Eigen::IOFormat HeavyFmt(Eigen::StreamPrecision, 0, "  ", "\n", "[ ", " ]");
            std::cout << C_6x6.format(HeavyFmt) << "\n\n";
            
            std::cout << "   -> Module Longitudinal Ez : " << Ez/1000.0 << " GPa\n";
            std::cout << "   -> Module Transverse Ex   : " << Ex/1000.0 << " GPa\n";
        }

        // ====================================================================
        // MODULE 2 : ANALYSE THERMIQUE (ALPHA)
        // ====================================================================
        if (config.runThermal()) {
            LOG_SECTION("CARACTERISATION THERMIQUE");
            DetailedResults resTh = manager.runElementaryCase(LoadCase::Thermal);
            std::cout << "   -> Coeff Dilatation AlphaX : " << resTh.macro_sig_xx << " K^-1\n";
            std::cout << "   -> Coeff Dilatation AlphaY : " << resTh.macro_sig_yy << " K^-1\n";
        }

        // ====================================================================
        // MODULE 3 : ANALYSE DE RUPTURE (PDA)
        // ====================================================================
        if (config.runPDA()) {
            string caseStr = config.getPDALoadCase();
            LoadCase pdaTarget = LoadCase::TensionY; // Defaut
            
            if (caseStr == "traction_x") pdaTarget = LoadCase::TensionX;
            else if (caseStr == "shear") pdaTarget = LoadCase::Shear;

            LOG_SECTION("ANALYSE DE DOMMAGE PROGRESSIF (PDA) : " + caseStr);
            manager.runProgressiveDamageAnalysis(pdaTarget);
        }

        LOG_INFO("Fin des analyses demandees.");

    } catch (const std::exception& e) {
        std::cerr << "\n[ERREUR FATALE] : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}