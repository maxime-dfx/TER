#include "ResultExporter.h"
#include "AnalyticalModels.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sys/stat.h>
#include <cmath>

using namespace std;

ResultExporter::ResultExporter(const std::string& directory) : outputDir(directory) {}

void ResultExporter::printLine(int width) const {
    cout << setfill('-') << setw(width) << "-" << setfill(' ') << endl;
}

void ResultExporter::printSynthesis(const DetailedResults& resTractionX, 
                                    const DetailedResults& resTractionY, 
                                    const DetailedResults& resShear,
                                    const DetailedResults& resThermal,
                                    const Eigen::Matrix3d& C_eff,
                                    const DataFile& config) const {
    double toGPa = 1.0 / 1000.0;
    double Vf = resTractionX.Vf;
    
    // Récupération des propriétés macroscopiques mécaniques
    double E1 = resTractionX.E_eff;
    double E2 = resTractionY.E_eff;
    double G12 = resShear.G_eff;
    double nu12 = resTractionX.nu_eff;
    double nu21 = resTractionY.nu_eff;

    // Modèles analytiques pour comparaison (Mécanique)
    double E_Voigt = AnalyticalModels::computeVoigtE2(config.getFibreE(), config.getFibreNu(), config.getMatriceE(), config.getMatriceNu(), Vf);
    double E_Reuss = AnalyticalModels::computeReussE2(config.getFibreE(), config.getFibreNu(), config.getMatriceE(), config.getMatriceNu(), Vf);
    double E_HalpinTsai = AnalyticalModels::computeHalpinTsaiE2(config.getFibreE(), config.getMatriceE(), config.getMatriceNu(), Vf, config.getHalpinTsaiXi());

    cout << "\n"; printLine();
    cout << left << setw(35) << "Fraction Volumique (Vf)" << setw(15) << Vf * 100.0 << " %" << endl;
    cout << left << setw(35) << "E1 (FEM - Traction X)"   << setw(15) << E1 * toGPa << " GPa" << endl;
    cout << left << setw(35) << "E2 (FEM - Traction Y)"   << setw(15) << E2 * toGPa << " GPa" << endl;
    cout << left << setw(35) << "G12 (FEM - Cisaillement)"<< setw(15) << G12 * toGPa << " GPa" << endl;
    cout << left << setw(35) << "nu12 / nu21 (FEM)"       << nu12 << " / " << nu21 << endl;
    printLine();
    
    // --- CARACTÉRISATION THERMIQUE ---
    // Correction : Calcul de Alpha à partir des contraintes de blocage macroscopiques
    double alphaX_fem = 0.0;
    double alphaY_fem = 0.0;

    if (std::abs(config.getDeltaT()) > 1e-9) {
        double dT = config.getDeltaT();
        // On récupère les contraintes moyennes générées par le blocage thermique
        double sigX = resThermal.macro_sig_xx;
        double sigY = resThermal.macro_sig_yy;

        // Calcul de Alpha via la matrice de souplesse (Loi de Hooke inverse)
        // Comme e_total = S*sig + alpha*dT = 0 (bloqué), alors alpha = - (S*sig)/dT
        alphaX_fem = - ( (1.0/E1)*sigX - (nu21/E2)*sigY ) / dT;
        alphaY_fem = - ( -(nu12/E1)*sigX + (1.0/E2)*sigY ) / dT;

        cout << left << setw(35) << "AlphaX (FEM - Thermique)" << setw(15) << alphaX_fem << " °C^-1" << endl;
        cout << left << setw(35) << "AlphaY (FEM - Thermique)" << setw(15) << alphaY_fem << " °C^-1" << endl;

        // Comparaison analytique : Modèle de Schapery
        double alpha_Schapery = AnalyticalModels::computeSchaperyTransverseAlpha(
            config.getFibreE(), config.getFibreAlpha(), config.getFibreNu(),
            config.getMatriceE(), config.getMatriceAlpha(), config.getMatriceNu(), Vf
        );
        cout << left << setw(35) << "Alpha Transverse (Schapery)" << setw(15) << alpha_Schapery << " °C^-1" << endl;
        printLine();
    }

    cout << left << setw(35) << "E1 Voigt (Borne Sup)" << setw(15) << E_Voigt * toGPa << " GPa" << endl;
    cout << left << setw(35) << "E2 Reuss (Borne Inf)" << setw(15) << E_Reuss * toGPa << " GPa" << endl;
    cout << left << setw(35) << "E2 Halpin-Tsai (Analytique)" << setw(15) << E_HalpinTsai * toGPa << " GPa" << endl;
    printLine();
    
    cout << "\n   TENSEUR DE RIGIDITE EFFECTIF C_eff (MPa) :" << endl;
    cout << C_eff << "\n" << endl;
    
    // Validation de la symétrie de Maxwell-Betti
    printLine();
    cout << "-> Verification Maxwell-Betti (Symetrie) :" << endl;
    cout << "   nu12/E1 = " << nu12 / E1 << " | nu21/E2 = " << nu21 / E2 << endl;
    if (std::abs(nu12/E1 - nu21/E2) > 1e-6) cout << "   /!\\ Attention : Asymetrie detectee." << endl;
    else cout << "   OK : Le tenseur est parfaitement symetrique." << endl;
    printLine();
}

void ResultExporter::appendToCSV(const std::string& filename, 
                                 const DetailedResults& resTractionX, 
                                 const DetailedResults& resTractionY, 
                                 const DetailedResults& resShear,
                                 const DetailedResults& resThermal,
                                 double deltaT) const {
    
    std::string csvPath = outputDir + "/" + filename;
    struct stat buffer;
    bool fileExists = (stat(csvPath.c_str(), &buffer) == 0);
    
    std::ofstream csvFile(csvPath, std::ios::app);
    if (csvFile.is_open()) {
        if (!fileExists) {
            csvFile << "Vf,E1_GPa,E2_GPa,G12_GPa,nu12,AlphaX,AlphaY\n";
        }
        
        double toGPa = 1.0 / 1000.0;
        csvFile << (resTractionX.Vf * 100.0) << "," 
                << (resTractionX.E_eff * toGPa) << "," 
                << (resTractionY.E_eff * toGPa) << "," 
                << (resShear.G_eff * toGPa) << "," 
                << resTractionX.nu_eff << ",";
        
        if (std::abs(deltaT) > 1e-9) {
            // Recalcul identique pour le CSV afin d'avoir des statistiques correctes
            double E1 = resTractionX.E_eff;
            double E2 = resTractionY.E_eff;
            double nu12 = resTractionX.nu_eff;
            double nu21 = resTractionY.nu_eff;
            double sigX = resThermal.macro_sig_xx;
            double sigY = resThermal.macro_sig_yy;

            double ax = - ( (1.0/E1)*sigX - (nu21/E2)*sigY ) / deltaT;
            double ay = - ( -(nu12/E1)*sigX + (1.0/E2)*sigY ) / deltaT;

            csvFile << ax << "," << ay << "\n";
        } else {
            csvFile << "0.0,0.0\n";
        }
        
        csvFile.close();
        cout << "   [IO] Resultats ajoutes a " << csvPath << " pour l'etude statistique." << endl;
    }  
}

void ResultExporter::logInfo(const std::string& message) const {
    cout << ">> " << message << endl;
}

void ResultExporter::logSection(const std::string& title) const {
    cout << "\n--- " << title << " ---" << endl;
}

void ResultExporter::logTopology(const TopologyAnalyzer& topo) const {
    if (topo.isPeriodic()) {
        cout << "   -> OK : Maillage periodique detecte (" 
             << topo.getPairsLR().size() << " noeuds G/D, " 
             << topo.getPairsBT().size() << " noeuds B/H)." << endl;
    } else {
        cout << "   /!\\ Maillage non-periodique. Utilisation de Dirichlet classiques (KUBC)." << endl;
    }
}