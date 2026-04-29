#ifndef SOLVER_H
#define SOLVER_H

#include "Mesh.h"
#include "Material.h"
#include "BoundaryCondition.h" // Assure-toi que ce fichier définit NodeSelector, PeriodicCondition, etc.
#include <Eigen/Sparse>
#include <vector>
#include <functional>

// Structure de cache pour le multithreading (NUMA)
struct ElementCache {
    int label;
    double area;
    Eigen::Matrix3d D0;
    Eigen::Matrix<double, 3, 6> B;
    Eigen::Matrix<double, 6, 6> Ke_0;
    Eigen::Matrix<double, 3, 3> Ke_0_AP; // Pour le vrai Anti-Plan (Cisaillement longitudinal)
    Eigen::Vector3d sigma_th_0;
    Eigen::Matrix<double, 6, 1> Fe_th_0;
    Eigen::Matrix<double, 6, 1> Fe_Ezz_0; // NOUVEAU : Effort fictif induit par Ezz (Déformation Plane Généralisée)
};

class Solver {
private:
    const Mesh& mesh;
    const MaterialManager& matMgr;

    Eigen::VectorXd F;
    Eigen::VectorXd U;
    Eigen::VectorXd U0; // Vecteur de réduction de Dirichlet

    std::vector<BoundaryCondition> boundaryConditions;
    std::vector<PeriodicCondition> periodicConditions;
    
    int numDof;
    int numRedDof;
    std::vector<int> dofMap;

    bool isPlaneStress; 
    bool isAntiPlane; 
    double deltaT;
    double imposed_Ezz; // NOUVEAU : Déformation macroscopique imposée hors-plan

    std::vector<ElementCache> elementCaches;

    // ==========================================
    // VARIABLES D'ÉTAT (ENDOMMAGEMENT PDA)
    // ==========================================
    std::vector<double> elementDamage; 
    std::vector<double> elementHistory; 
    std::vector<double> cached_Sxx;
    std::vector<double> cached_Syy;
    std::vector<double> cached_Txy;
    std::vector<double> cached_FI;

    // ==========================================
    // MÉTHODES PRIVÉES (Cœur Mathématique)
    // ==========================================
    void precomputeElementCachesNUMA();
    void buildDirectReductionMapping();
    void assembleReducedDirectly(Eigen::SparseMatrix<double>& K_red, Eigen::VectorXd& F_red);
    void recoverGlobalDisplacements(const Eigen::VectorXd& U_red);

public:
    Solver(const Mesh& m, const MaterialManager& mat, bool planeStress, double dT = 0.0);

    // Contrôle de la physique
    void setAntiPlaneMode(bool mode) { isAntiPlane = mode; }
    void setImposedEzz(double ezz) { imposed_Ezz = ezz; } // NOUVEAU : Activation de Ezz

    // Conditions aux limites
    void addBoundaryCondition(NodeSelector selector, int direction, std::function<double(double, double)> valueFunc);
    void addPeriodicCondition(int nodeSlave, int nodeMaster, int direction, double delta);

    // Injection d'état (Thermique -> Méca)
    void injectInternalState(const std::vector<double>& init_damage, const std::vector<double>& init_history);

    // Résolution
    void solve(); 
    void solveNonLinear(double target_control_var, int numSteps, std::function<void(int, double)> onStepComplete = nullptr);
    
    // Accesseurs
    const Eigen::VectorXd& getSolution() const { return U; }
    const std::vector<double>& getDamageState() const { return elementDamage; }
    const std::vector<double>& getFailureIndex() const { return cached_FI; }
    const std::vector<double>& getSxx() const { return cached_Sxx; }
    const std::vector<double>& getSyy() const { return cached_Syy; }
    const std::vector<double>& getTxy() const { return cached_Txy; }
};

#endif // SOLVER_H