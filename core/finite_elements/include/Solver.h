#ifndef SOLVER_H
#define SOLVER_H

#include "Mesh.h"
#include "Material.h"
#include "BoundaryCondition.h"
#include <Eigen/Sparse>

class Solver {
private:
    const Mesh& mesh;
    const MaterialManager& matMgr;

    Eigen::SparseMatrix<double> K_global;
    Eigen::VectorXd F;
    Eigen::VectorXd U;

    std::vector<BoundaryCondition> boundaryConditions;
    std::vector<PeriodicCondition> periodicConditions; 
    int numDof;
    
    bool isPlaneStress; 
    double deltaT;

public:
    // MISE À JOUR : Ajout du paramètre deltaT dans le constructeur
    Solver(const Mesh& m, const MaterialManager& mat, bool planeStress, double deltaT = 0.0);

    Eigen::Matrix<double, 6, 6> computeElementStiffness(int triangleIndex);
    
    // NOUVEAU : Fonction pour calculer le vecteur force thermique local (6x1)
    Eigen::Matrix<double, 6, 1> computeElementThermalForce(int triangleIndex);
    
    void assemble();
    void addBoundaryCondition(NodeSelector selector, int direction, double value);
    void applyBoundaryConditions(); 
    void solve(); 

    const Eigen::VectorXd& getSolution() const { return U; }
    void addPeriodicCondition(int nodeSlave, int nodeMaster, int direction, double delta);
};

#endif