#ifndef RESULT_EXPORTER_H
#define RESULT_EXPORTER_H

#include <string>
#include <Eigen/Dense>
#include "PostProcessor.h" // Pour la structure DetailedResults
#include "Datafile.h"
#include "TopologyAnalyzer.h" // <-- AJOUTEZ CETTE LIGNE ICI

class ResultExporter {
private:
    std::string outputDir;

    // Méthode utilitaire pour tracer des lignes esthétiques dans la console
    void printLine(int width = 70) const;

public:
    ResultExporter(const std::string& directory);

    // Affichage des résultats dans le terminal
    void printSynthesis(const DetailedResults& resTractionX, 
                        const DetailedResults& resTractionY, 
                        const DetailedResults& resShear,
                        const DetailedResults& resThermal,
                        const Eigen::Matrix3d& C_eff,
                        const DataFile& config) const;

    // Ajout des résultats à la fin du fichier CSV pour les études statistiques (VER)
    void appendToCSV(const std::string& filename, 
                     const DetailedResults& resTractionX, 
                     const DetailedResults& resTractionY, 
                     const DetailedResults& resShear,
                     const DetailedResults& resThermal,
                     double deltaT) const;

    void logInfo(const std::string& message) const;
    void logSection(const std::string& title) const;
    void logTopology(const TopologyAnalyzer& topo) const;
};


#endif // RESULT_EXPORTER_H