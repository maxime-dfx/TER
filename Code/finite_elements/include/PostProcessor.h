#ifndef POSTPROCESSOR_H
#define POSTPROCESSOR_H

#include "Mesh.h"
#include "Material.h"
#include <Eigen/Dense>
#include <string>

// Structure enrichie pour l'analyse du tenseur
struct DetailedResults {
    double E_eff;
    double nu_eff;
    double G_eff;        // NOUVEAU : Pour stocker le module de cisaillement
    double Vf; 

    double mean_sigma_f; 
    double mean_sigma_m; 
    double mean_eps_f;   
    double mean_eps_m;   
};

class PostProcessor {
private:
    const Mesh& mesh;
    const MaterialManager& matMgr;
    const Eigen::VectorXd& U;
    
    bool isPlaneStress; 

public:
    PostProcessor(const Mesh& m, const MaterialManager& mat, const Eigen::VectorXd& u, bool planeStress);


    DetailedResults runAnalysis(double totalArea, double imposedStrain, const BoundingBox& bb, int labelFibre, int labelMatrice, const std::string& loadCase);
    
    void exportToVTK(const std::string& filename) const;
    void exportToGnuplot(const std::string& filename) const;
};

#endif