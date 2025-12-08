#ifndef SOLVER_H
#define SOLVER_H

#include "Mesh.h"
#include "Material.h"
#include "BoundaryCondition.h" // <--- NOUVEAU

#include <vector>
#include <Eigen/Dense>
#include <Eigen/Sparse>

class Solver {
private:
    const Mesh& mesh;
    const MaterialManager& matMgr;

    Eigen::SparseMatrix<double> K_global;
    Eigen::VectorXd F;
    Eigen::VectorXd U;

    int numNodes;
    int numDof;

    // Liste des conditions aux limites stockées
    std::vector<BoundaryCondition> boundaryConditions; // <--- NOUVEAU

public:
    Solver(const Mesh& m, const MaterialManager& mat);

    Eigen::Matrix<double, 6, 6> computeElementStiffness(int triangleIndex);
    void assemble();

    // --- NOUVEAU ---
    // Ajoute une CL à la liste
    void addBoundaryCondition(NodeSelector selector, int direction, double value);

    // La fonction solve ne prend plus d'argument, elle utilise la liste interne
    void solve(); 

    void printSystemInfo() const;
    
    // Getter pour récupérer le résultat U
    const Eigen::VectorXd& getSolution() const { return U; }
};

#endif