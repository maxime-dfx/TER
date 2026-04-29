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
    cout << setfill('-') << setw(width) << "-" << setfill(' ') << "\n";
}

void ResultExporter::printSynthesis(const DetailedResults& resTractionX, 
                                    const DetailedResults& resTractionY, 
                                    const DetailedResults& resShear,
                                    const DetailedResults& resThermal,
                                    const Eigen::Matrix3d& C_eff,
                                    const DataFile& config) const {
    double toGPa = 1.0 / 1000.0;
    double Vf = resTractionX.Vf;
    
    // --- LA VRAIE PHYSIQUE : Inversion du tenseur de rigidité ---
    // La matrice S_eff (Souplesse) contient les vrais modules libérés des contraintes de bord
    Eigen::Matrix3d S_eff = C_eff.inverse();
    
    double Ex = 1.0 / S_eff(0,0);
    double Ey = 1.0 / S_eff(1,1);
    double Gxy = 1.0 / S_eff(2,2);
    
    double nuxy = -S_eff(0,1) * Ex; 
    double nuyx = -S_eff(1,0) * Ey; 

    // Récupération des propriétés des phases
    double Ef = config.getFibreE();
    double Em = config.getMatriceE();
    double nuf = config.getFibreNu();
    double num = config.getMatriceNu();

    // --- MODELES ANALYTIQUES ---
    double E_Voigt_E1 = AnalyticalModels::computeVoigtE1(Ef, Em, Vf); // Direction Z (Hors-Plan)
    double E_Reuss_E2 = AnalyticalModels::computeReussE2(Ef, Em, Vf); // Direction Transverse
    double E_HalpinTsai_E2 = AnalyticalModels::computeHalpinTsaiE2(Ef, Em, Vf, config.getHalpinTsaiXi());
    double E_Chamis_E2 = AnalyticalModels::computeChamisE2(Ef, Em, Vf);
    
    double Gf = Ef / (2.0 * (1.0 + nuf));
    double Gm = Em / (2.0 * (1.0 + num));
    double G12_MoriTanaka = AnalyticalModels::computeMoriTanakaG12(Gf, Gm, Vf);

    // --- AFFICHAGE SYNTHETIQUE ---
    cout << "\n"; printLine(70);
    cout << " COMPARAISON : ELEMENTS FINIS VS THEORIE (Vf = " << Vf * 100.0 << " %)\n";
    printLine(70);
    
    cout << left << setw(40) << " [Module Longitudinal E1 (Direction Z)]" << "\n";
    cout << left << setw(40) << "   - Analytique (Voigt - Borne Sup)" << setw(15) << E_Voigt_E1 * toGPa << " GPa\n";
    cout << left << setw(40) << "   - Elements Finis" << setw(15) << "Non mesurable en 2D\n\n";

    cout << left << setw(40) << " [Modules Transverses (Plan XY)]" << "\n";
    cout << left << setw(40) << "   - Analytique (Reuss - Borne Inf)" << setw(15) << E_Reuss_E2 * toGPa << " GPa\n";
    cout << left << setw(40) << "   - Analytique (Halpin-Tsai)" << setw(15) << E_HalpinTsai_E2 * toGPa << " GPa\n";
    cout << left << setw(40) << "   - Analytique (Chamis)" << setw(15) << E_Chamis_E2 * toGPa << " GPa\n";
    cout << left << setw(40) << "   - Elements Finis Ex (Traction X)" << setw(15) << Ex * toGPa << " GPa\n";
    cout << left << setw(40) << "   - Elements Finis Ey (Traction Y)" << setw(15) << Ey * toGPa << " GPa\n\n";

    cout << left << setw(40) << " [Cisaillement In-Plane Gxy]" << "\n";
    cout << left << setw(40) << "   - Analytique (Mori-Tanaka)" << setw(15) << G12_MoriTanaka * toGPa << " GPa\n";
    cout << left << setw(40) << "   - Elements Finis (Shear)" << setw(15) << Gxy * toGPa << " GPa\n\n";

    cout << left << setw(40) << " [Coefficients de Poisson]" << "\n";
    cout << left << setw(40) << "   - nuxy / nuyx (FEM)" << nuxy << " / " << nuyx << "\n";
    printLine(70);

    cout << "\n   TENSEUR DE RIGIDITE EFFECTIF C_eff (MPa) :\n" << C_eff << "\n\n";
    printLine(70);
    cout << flush;
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
        if (!fileExists) csvFile << "Vf,E1_GPa,E2_GPa,G12_GPa,nu12,AlphaX,AlphaY\n";
        
        double toGPa = 1.0 / 1000.0;
        csvFile << (resTractionX.Vf * 100.0) << "," 
                << (resTractionX.E_eff * toGPa) << "," 
                << (resTractionY.E_eff * toGPa) << "," 
                << (resShear.G_eff * toGPa) << "," 
                << resTractionX.nu_eff << ",";
        
        if (std::abs(deltaT) > 1e-9) {
            double ax = - ( (1.0/resTractionX.E_eff)*resThermal.macro_sig_xx - (resTractionY.nu_eff/resTractionY.E_eff)*resThermal.macro_sig_yy ) / deltaT;
            double ay = - ( -(resTractionX.nu_eff/resTractionX.E_eff)*resThermal.macro_sig_xx + (1.0/resTractionY.E_eff)*resThermal.macro_sig_yy ) / deltaT;
            csvFile << ax << "," << ay << "\n";
        } else {
            csvFile << "0.0,0.0\n";
        }
        csvFile.close();
        cout << "   [IO] Resultats ajoutes a " << csvPath << " pour l'etude statistique.\n";
    }  
}

void ResultExporter::logInfo(const std::string& message) const { cout << ">> " << message << "\n"; }
void ResultExporter::logSection(const std::string& title) const { cout << "\n--- " << title << " ---\n"; }
void ResultExporter::logTopology(const TopologyAnalyzer& topo) const {
    if (topo.isPeriodic()) cout << "   -> OK : Maillage periodique detecte (" << topo.getPairsLR().size() << " noeuds G/D, " << topo.getPairsBT().size() << " noeuds B/H).\n";
    else cout << "   /!\\ Maillage non-periodique. Utilisation des conditions cinematiques uniformes (KUBC).\n";
}