#ifndef SOLVER_H
#define SOLVER_H

#include "Mesh.h"
#include "Material.h"
#include "BoundaryCondition.h"

#include <vector>
#include <string>
#include <Eigen/Dense>
#include <Eigen/Sparse>

class Solver {
private:
    const Mesh& mesh;
    const MaterialManager& matMgr;

    // Système linéaire K*U = F
    Eigen::SparseMatrix<double> K_global;
    Eigen::VectorXd F;
    Eigen::VectorXd U;

    int numNodes;
    int numDof;

    // Stockage des Conditions aux Limites
    std::vector<BoundaryCondition> boundaryConditions;

public:
    Solver(const Mesh& m, const MaterialManager& mat);

    // Calcul de la matrice de raideur locale (6x6)
    Eigen::Matrix<double, 6, 6> computeElementStiffness(int triangleIndex);

    // Assemblage global (Parallélisé OpenMP)
    void assemble();

    // Ajout d'une condition aux limites (ex: bloquer x sur le bord gauche)
    void addBoundaryCondition(NodeSelector selector, int direction, double value);

    // Résolution du système (avec pénalisation adaptative)
    void solve(); 

    // Affichage des infos techniques sur la matrice
    void printSystemInfo() const;
    
    // Récupération des résultats
    const Eigen::VectorXd& getSolution() const { return U; }
};

#endif