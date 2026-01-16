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
    int numDof;

public:
    Solver(const Mesh& m, const MaterialManager& mat);

    Eigen::Matrix<double, 6, 6> computeElementStiffness(int triangleIndex);
    
    void assemble();
    void addBoundaryCondition(NodeSelector selector, int direction, double value);
    void applyBoundaryConditions(); // Nouvelle étape distincte
    void solve(); 

    const Eigen::VectorXd& getSolution() const { return U; }
};

#endif