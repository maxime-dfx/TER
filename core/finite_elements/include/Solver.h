#ifndef SOLVER_H
#define SOLVER_H

#include "Mesh.h"
#include "Material.h"
#include "BoundaryCondition.h"
#include <Eigen/Sparse>
#include <vector>

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
    bool isAntiPlane; 
    double deltaT;

    // ==========================================
    // VARIABLES D'ÉTAT (ENDOMMAGEMENT PDA)
    // ==========================================
    std::vector<double> elementDamage; // 0.0 = sain, 0.99 = micro-vide

    // ==========================================
    // MÉTHODES PRIVÉES
    // ==========================================
    Eigen::Matrix<double, 3, 3> computeElementStiffnessAntiPlane(int triangleIndex);
    void assembleAntiPlane();
    void applyBoundaryConditionsAntiPlane();
    
    // Isolation de la projection P.K.P et de Cholesky
    void condenseAndSolveSystem();
    std::vector<double> cached_Sxx;
    std::vector<double> cached_Syy;
    std::vector<double> cached_Txy;
    std::vector<double> cached_FI;


public:
    Solver(const Mesh& m, const MaterialManager& mat, bool planeStress, double deltaT = 0.0);

    void setAntiPlaneMode(bool mode) { isAntiPlane = mode; }

    Eigen::Matrix<double, 6, 6> computeElementStiffness(int triangleIndex);
    Eigen::Matrix<double, 6, 1> computeElementThermalForce(int triangleIndex);
    
    void assemble();
    void addBoundaryCondition(NodeSelector selector, int direction, double value);
    void applyBoundaryConditions(); 
    
    // Résolution élastique linéaire 
    void solve(); 
    
    // Résolution non-linéaire (Endommagement Progressif)
    void solveNonLinear(double final_deltaT, int numSteps, std::function<void(int, double)> onStepComplete = nullptr);
    
    const Eigen::VectorXd& getSolution() const { return U; }
    void addPeriodicCondition(int nodeSlave, int nodeMaster, int direction, double delta);
    const std::vector<double>& getDamageState() const { return elementDamage; }
    const std::vector<double>& getSxx() const { return cached_Sxx; }
    const std::vector<double>& getSyy() const { return cached_Syy; }
    const std::vector<double>& getTxy() const { return cached_Txy; }
    const std::vector<double>& getFailureIndex() const { return cached_FI; }
};

#endif