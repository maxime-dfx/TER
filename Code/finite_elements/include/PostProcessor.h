#ifndef POSTPROCESSOR_H
#define POSTPROCESSOR_H

#include "Mesh.h"
#include "Material.h"
#include <Eigen/Dense>
#include <string>

// Structure enrichie pour l'analyse fine
struct DetailedResults {
    // Propriétés Globales
    double E_eff;
    double nu_eff;
    double Vf; // Taux de fibre calculé (Volume Fraction)

    // Moyennes par phase (pour comprendre le transfert de charge)
    double mean_sigma_f; // Contrainte moyenne dans la Fibre
    double mean_sigma_m; // Contrainte moyenne dans la Matrice
    double mean_eps_f;   // Déformation moyenne dans la Fibre
    double mean_eps_m;   // Déformation moyenne dans la Matrice
};

class PostProcessor {
private:
    const Mesh& mesh;
    const MaterialManager& matMgr;
    const Eigen::VectorXd& U;

    // Helper interne
    double getTriangleArea(int tIdx) const;

public:
    PostProcessor(const Mesh& m, const MaterialManager& mat, const Eigen::VectorXd& u);

    // Nouvelle méthode de calcul plus complète
    DetailedResults runAnalysis(double totalArea, double imposedStrain, double meshHeight, int labelFibre, int labelMatrice);

    void exportToVTK(const std::string& filename) const;
    void exportToGnuplot(const std::string& filename) const;
};

#endif