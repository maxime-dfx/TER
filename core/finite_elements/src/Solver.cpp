#include "Solver.h"
#include "Logger.h" 
#include <omp.h>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <future>
#include <Eigen/SparseCholesky>
#include <Eigen/IterativeLinearSolvers>

using namespace std;
using namespace Eigen;

Solver::Solver(const Mesh& m, const MaterialManager& mat, bool planeStress, double dT) 
    : mesh(m), matMgr(mat), isPlaneStress(planeStress), isAntiPlane(false), deltaT(dT) {
}

// Mise à jour : Utilisation de std::function pour les KUBC géométriques
void Solver::addBoundaryCondition(NodeSelector selector, int direction, std::function<double(double, double)> valueFunc) {
    boundaryConditions.emplace_back(selector, direction, valueFunc);
}

void Solver::addPeriodicCondition(int nodeSlave, int nodeMaster, int direction, double delta) {
    periodicConditions.emplace_back(nodeSlave, nodeMaster, direction, delta);
}

// ============================================================================
//  1. PRE-CALCULS HPC (NUMA-Aware & Alignment & State Transfer)
// ============================================================================

void Solver::precomputeElementCachesNUMA() {
    int numElements = (int)mesh.getTriangles().size();
    elementCaches.resize(numElements);
    
    // 1. SÉCURITÉ D'ÉTAT : On n'initialise l'endommagement QUE si rien n'a été injecté
    if (elementDamage.size() != (size_t)numElements) {
        elementDamage.assign(numElements, 0.0);
        elementHistory.assign(numElements, 1.0);
    }

    // 2. SÉCURITÉ MÉMOIRE : Initialisation inconditionnelle des caches pour éviter le Segfault
    if (cached_Sxx.size() != (size_t)numElements) {
        cached_Sxx.assign(numElements, 0.0);
        cached_Syy.assign(numElements, 0.0);
        cached_Txy.assign(numElements, 0.0);
        cached_FI.assign(numElements, 0.0);
    }

    // Initialisation parallèle pour garantir l'affinité mémoire (NUMA First-Touch)
    #pragma omp parallel for schedule(static)
    for (int t = 0; t < numElements; ++t) {
        const Triangle& tri = mesh.getTriangles()[t];
        ElementCache& cache = elementCaches[t];
        cache.label = tri.label;
        
        if (isAntiPlane) {
            double G = matMgr.getShearModulus(tri.label); 
            const Vertex& p0 = mesh.getVertices()[tri.v[0]];
            const Vertex& p1 = mesh.getVertices()[tri.v[1]];
            const Vertex& p2 = mesh.getVertices()[tri.v[2]];

            double detJ = (p1.x - p0.x)*(p2.y - p0.y) - (p2.x - p0.x)*(p1.y - p0.y);
            cache.area = 0.5 * std::abs(detJ);
            double sign = (detJ > 0) ? 1.0 : -1.0;

            double b[3] = {p1.y - p2.y, p2.y - p0.y, p0.y - p1.y};
            double c[3] = {p2.x - p1.x, p0.x - p2.x, p1.x - p0.x};

            Matrix<double, 2, 3> B_ap;
            for(int i=0; i<3; ++i) { B_ap(0, i) = b[i]; B_ap(1, i) = c[i]; }
            B_ap *= sign / (2.0 * cache.area);
            cache.Ke_0_AP = B_ap.transpose() * G * B_ap * cache.area;
            
        } else {
            cache.D0 = matMgr.getHookeMatrix(tri.label, isPlaneStress); 
            cache.B = mesh.computeBMatrix(t, cache.area); // Assure-toi que cette méthode existe dans Mesh
            
            cache.sigma_th_0 = matMgr.getThermalStressVector(tri.label, 1.0, isPlaneStress); 
            cache.Ke_0 = cache.B.transpose() * cache.D0 * cache.B * cache.area;
            cache.Fe_th_0 = cache.B.transpose() * cache.sigma_th_0 * cache.area; 

            // NOUVEAU : Calcul du vecteur d'effort de couplage induit par Ezz
            Eigen::Vector3d sigma_Ezz_0 = matMgr.getZCouplingVector(tri.label);
            cache.Fe_Ezz_0 = cache.B.transpose() * sigma_Ezz_0 * cache.area;
        }
    }
}

void Solver::buildDirectReductionMapping() {
    numDof = isAntiPlane ? (int)mesh.getVertices().size() : (int)mesh.getVertices().size() * 2;
    std::vector<int> master(numDof);
    std::vector<double> offset(numDof, 0.0);
    std::vector<bool> isFixed(numDof, false);
    std::vector<double> fixedVal(numDof, 0.0);
    
    for(int i=0; i<numDof; ++i) master[i] = i;

    for(const auto& pbc : periodicConditions) {
        int s = isAntiPlane ? pbc.nodeSlave : (2 * pbc.nodeSlave + pbc.direction);
        int m = isAntiPlane ? pbc.nodeMaster : (2 * pbc.nodeMaster + pbc.direction);
        master[s] = m;
        offset[s] = pbc.delta;
    }

    const auto& vertices = mesh.getVertices();
    for (int i = 0; i < (int)vertices.size(); ++i) {
        double x = vertices[i].x; 
        double y = vertices[i].y;
        for (const auto& bc : boundaryConditions) {
            if (bc.selector(x, y)) {
                int dof = isAntiPlane ? i : (2 * i + bc.direction);
                isFixed[dof] = true; 
                // VRAIE KUBC : Évaluation spatiale dynamique de la contrainte géométrique
                fixedVal[dof] = bc.valueFunc(x, y); 
            }
        }
    }

    numRedDof = 0;
    std::vector<int> redId(numDof, -1);
    for(int i=0; i<numDof; ++i) {
        if (master[i] == i && !isFixed[i]) redId[i] = numRedDof++;
    }

    dofMap.assign(numDof, -1);
    U0 = VectorXd::Zero(numDof);

    for(int i=0; i<numDof; ++i) {
        int m_root = master[i]; double total_offset = offset[i]; int depth = 10; 
        while(master[m_root] != m_root && depth-- > 0) { total_offset += offset[m_root]; m_root = master[m_root]; }
        
        if (depth <= 0) {
            LOG_ERROR("Boucle infinie détectée dans les CL Périodiques !");
            throw runtime_error("Erreur Topologique : Boucle infinie dans les CL !");
        }
        
        if (isFixed[m_root]) { dofMap[i] = -1; U0(i) = fixedVal[m_root] + total_offset; }
        else { dofMap[i] = redId[m_root]; U0(i) = total_offset; }
    }
}

// ============================================================================
//  2. ASSEMBLAGE RÉDUIT DIRECT (Thread-Safe Optimisé)
// ============================================================================

void Solver::assembleReducedDirectly(SparseMatrix<double>& K_red, VectorXd& F_red) {
    F_red.setZero();
    int numElements = static_cast<int>(mesh.getTriangles().size());
    int nodesPerElem = isAntiPlane ? 3 : 6;

    int max_threads = omp_get_max_threads();
    std::vector<std::vector<Triplet<double>>> thread_triplets(max_threads);
    std::vector<VectorXd> thread_F(max_threads, VectorXd::Zero(numRedDof));

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        thread_triplets[tid].reserve( (numElements / max_threads) * nodesPerElem * nodesPerElem );

        #pragma omp for schedule(static)
        for (int t = 0; t < numElements; ++t) {
            const Triangle& tri = mesh.getTriangles()[t];
            const ElementCache& cache = elementCaches[t];
            
            // Sécurité de rigidité minimale pour éviter la singularité
            double integrity = std::max(1e-6, 1.0 - elementDamage[t]); 

            int gdof[6];
            if (isAntiPlane) { 
                for(int k=0; k<3; ++k) gdof[k] = tri.v[k]; 
            } else { 
                for(int k=0; k<3; ++k) { 
                    gdof[2*k] = 2*tri.v[k]; 
                    gdof[2*k+1] = 2*tri.v[k]+1; 
                } 
            }

            for (int i = 0; i < nodesPerElem; ++i) {
                int red_i = dofMap[gdof[i]];
                if (!isAntiPlane && red_i != -1) {
                    // 1. Force thermique classique
                    thread_F[tid](red_i) += cache.Fe_th_0(i) * deltaT * integrity;
                    
                    // 2. NOUVEAU : Force induite par la déformation plane généralisée (Ezz)
                    // Le signe est négatif car l'effet Poisson induit une contraction interne
                    thread_F[tid](red_i) -= cache.Fe_Ezz_0(i) * imposed_Ezz * integrity;
                }

                for (int j = 0; j < nodesPerElem; ++j) {
                    double K_ij = (isAntiPlane ? cache.Ke_0_AP(i, j) : cache.Ke_0(i, j)) * integrity;
                    if (std::abs(K_ij) < 1e-16) continue;
                    
                    int red_j = dofMap[gdof[j]];
                    if (red_i != -1) {
                        if (red_j != -1) {
                            thread_triplets[tid].emplace_back(red_i, red_j, K_ij);
                        } else {
                            // Contribution des conditions de Dirichlet (U0)
                            thread_F[tid](red_i) -= K_ij * U0(gdof[j]);
                        }
                    }
                }
            }
        }
    }

    size_t total_triplets = 0;
    for(const auto& t : thread_triplets) total_triplets += t.size();
    
    std::vector<Triplet<double>> global_triplets;
    global_triplets.reserve(total_triplets);
    
    // Réduction globale
    for(int i = 0; i < max_threads; ++i) {
        global_triplets.insert(global_triplets.end(), thread_triplets[i].begin(), thread_triplets[i].end());
        F_red += thread_F[i];
    }
    
    K_red.setFromTriplets(global_triplets.begin(), global_triplets.end());
}

void Solver::recoverGlobalDisplacements(const VectorXd& U_red) {
    U.resize(numDof);
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < numDof; ++i) {
        U(i) = (dofMap[i] != -1) ? (U_red(dofMap[i]) + U0(i)) : U0(i);
    }
}

// ============================================================================
//  3. RÉSOLUTION LINÉAIRE ÉLASTIQUE 
// ============================================================================

void Solver::solve() {
    LOG_INFO("Setup & Pre-calculs HPC (NUMA)...");
    precomputeElementCachesNUMA();
    buildDirectReductionMapping();
    
    SparseMatrix<double> K_red(numRedDof, numRedDof);
    VectorXd F_red(numRedDof);
    assembleReducedDirectly(K_red, F_red);

    if (numRedDof < 200000) {
        LOG_INFO("Solveur Direct (SimplicialLLT) en cours (" + std::to_string(numRedDof) + " DDL).");
        SimplicialLLT<SparseMatrix<double>> solver(K_red);
        if(solver.info() != Success) {
            LOG_ERROR("Échec de la factorisation de Cholesky.");
            throw runtime_error("Echec Cholesky (Matrice singuliere) !");
        }
        recoverGlobalDisplacements(solver.solve(F_red));
    } else {
        LOG_INFO("Solveur Itératif (CG) en cours (" + std::to_string(numRedDof) + " DDL).");
        ConjugateGradient<SparseMatrix<double>, Lower|Upper, IncompleteCholesky<double>> solver(K_red);
        recoverGlobalDisplacements(solver.solve(F_red));
    }
}

// ============================================================================
//  4. MOTEUR NON-LINÉAIRE (PDA - HYBRID SOLVER & SCALING LOAD)
// ============================================================================

void Solver::solveNonLinear(double target_control_var, int numSteps, std::function<void(int, double)> onStepComplete) {
    int numTriangles = (int)mesh.getTriangles().size();
    precomputeElementCachesNUMA();
    buildDirectReductionMapping();
    
    // Sauvegarde des cibles (Thermique ou Mécanique) pour le pilotage proportionnel
    VectorXd U0_target = U0; 
    double target_dT = this->deltaT; 
    
    // Choix du solveur linéaire selon la taille du problème
    bool useDirect = (numRedDof < 200000);
    SimplicialLLT<SparseMatrix<double>> directSolver;
    ConjugateGradient<SparseMatrix<double>, Lower|Upper, IncompleteCholesky<double>> cgSolver;

    if (useDirect) {
        SparseMatrix<double> K_dummy(numRedDof, numRedDof); 
        VectorXd F_dummy(numRedDof);
        assembleReducedDirectly(K_dummy, F_dummy);
        directSolver.analyzePattern(K_dummy);
    }

    LOG_INFO("Début de l'analyse PDA...");

    for (int step = 1; step <= numSteps; ++step) {
        double load_fraction = (double)step / (double)numSteps;
        U0 = U0_target * load_fraction;
        this->deltaT = target_dT * load_fraction; 
        
        LOG_PROG("Palier " + std::to_string(step) + "/" + std::to_string(numSteps) + 
                 " | Fraction de charge = " + std::to_string(load_fraction*100.0) + "%");

        bool isEquilibriumReached = false;
        int iter = 0;
        SparseMatrix<double> K_red(numRedDof, numRedDof);
        VectorXd F_red(numRedDof), U_red;

        // Boucle itérative de Newton-Picard pour l'équilibre avec endommagement
        while (!isEquilibriumReached && iter++ < 50) {
            assembleReducedDirectly(K_red, F_red); 
            
            if (useDirect) { 
                directSolver.factorize(K_red); 
                if(directSolver.info() != Success) {
                    LOG_ERROR("Échec LLT au palier " + std::to_string(step) + " (Effondrement structurel)");
                    throw runtime_error("Echec LLT ! Matrice singulière.");
                }
                U_red = directSolver.solve(F_red); 
            } else { 
                cgSolver.compute(K_red); 
                U_red = cgSolver.solve(F_red); 
            }
            
            recoverGlobalDisplacements(U_red);
            
            double max_dD = 0.0;
            
            // Boucle de Physique Matériau (Parallélisée via OpenMP)
            #pragma omp parallel for reduction(max:max_dD) schedule(static)
            for (int t = 0; t < numTriangles; ++t) {
                const ElementCache& cache = elementCaches[t];
                const Triangle& tri = mesh.getTriangles()[t];
                
                VectorXd U_elem(6);
                for(int k=0; k<3; ++k) { 
                    U_elem(2*k) = U(2*tri.v[k]); 
                    U_elem(2*k+1) = U(2*tri.v[k]+1); 
                }
                
                // Calcul des contraintes effectives
                Vector3d sigma_eff = cache.D0 * (cache.B * U_elem) - (cache.sigma_th_0 * deltaT);
                
                // Critère de rupture selon le matériau (Fibre/Matrice)
                double FI = (cache.label == 1 || cache.label == 3) ? 
                             matMgr.computeDruckerPragerPlaneStrain(cache.label, sigma_eff(0), sigma_eff(1), sigma_eff(2)) :
                             matMgr.computeTsaiWuIsotropicPlaneStrain(cache.label, sigma_eff(0), sigma_eff(1), sigma_eff(2));

                if (FI > elementHistory[t]) elementHistory[t] = FI;
                double d_new = 0.0;
                
                // Loi d'évolution de l'endommagement (Bande de fissuration)
                if (elementHistory[t] > 1.0) {
                    double We = (matMgr.getXt(cache.label) * matMgr.getXt(cache.label)) / (2.0 * matMgr.getYoungModulus(cache.label));
                    double rf = matMgr.getGIc(cache.label) / (std::sqrt(cache.area) * We);
                    d_new = (rf <= 1.0 || elementHistory[t] >= rf) ? 0.95 : ((rf * (elementHistory[t] - 1.0)) / (elementHistory[t] * (rf - 1.0)));
                }

                max_dD = std::max(max_dD, std::abs(d_new - elementDamage[t]));
                elementDamage[t] = d_new;
                cached_FI[t] = elementHistory[t];
                
                // Stockage des contraintes apparentes (dégradées)
                cached_Sxx[t] = (1.0 - d_new) * sigma_eff(0); 
                cached_Syy[t] = (1.0 - d_new) * sigma_eff(1); 
                cached_Txy[t] = (1.0 - d_new) * sigma_eff(2);
            }
            
            // Convergence de l'incrément si la variation d'endommagement est minime
            if (max_dD < 1e-3) {
                isEquilibriumReached = true;
            }
        }

        if (!isEquilibriumReached) {
            LOG_WARN("Instabilité matérielle : Non-convergence à l'incrément " + std::to_string(step));
        }

        if (onStepComplete) {
            double current_val = target_control_var * load_fraction;
            onStepComplete(step, current_val);
        }
    }     
}

void Solver::injectInternalState(const std::vector<double>& init_damage, const std::vector<double>& init_history) {
    // Si les vecteurs fournis ont la bonne taille (c'est-à-dire qu'ils proviennent du même maillage)
    if (init_damage.size() == elementDamage.size() || elementDamage.empty()) {
        elementDamage = init_damage;
    }
    
    // Le paramètre "history" correspond à l'état de l'endommagement ou au Failure Index (FI)
    if (init_history.size() == cached_FI.size() || cached_FI.empty()) {
        cached_FI = init_history;
    }
}