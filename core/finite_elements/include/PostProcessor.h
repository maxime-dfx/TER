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
    double G_eff;        
    double Vf; 
    double alpha_x;
    double alpha_y;
    // NOUVEAU : Pour stocker les contraintes macroscopiques moyennes
    double macro_sig_xx; 
    double macro_sig_yy; 
    double macro_tau_xy; 

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
    double deltaT; 

public:
    PostProcessor(const Mesh& m, const MaterialManager& mat, const Eigen::VectorXd& u, bool planeStress, double dT = 0.0);

    DetailedResults runAnalysis(double totalArea, double imposedStrain, const BoundingBox& bb, int labelFibre, int labelMatrice, const std::string& loadCase);
    
    void exportToVTK(const std::string& filename) const;
};

#endif