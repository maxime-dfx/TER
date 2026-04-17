#include "Solver.h"
#include <iostream>
#include <omp.h>
#include <vector>
#include <cmath>
#include <stdexcept>

using namespace std;
using namespace Eigen;

Solver::Solver(const Mesh& m, const MaterialManager& mat, bool planeStress, double dT) 
    : mesh(m), matMgr(mat), isPlaneStress(planeStress), isAntiPlane(false), deltaT(dT) {
}

// ============================================================================
//  1. MATRICES ÉLÉMENTAIRES (Mécanique et Thermique)
// ============================================================================

Matrix<double, 6, 6> Solver::computeElementStiffness(int tIdx) {
    const Triangle& tri = mesh.getTriangles()[tIdx];
    Matrix3d D = matMgr.getHookeMatrix(tri.label, isPlaneStress); 
    
    double area = 0.0;
    Matrix<double, 3, 6> B = mesh.computeBMatrix(tIdx, area);

    // K_e = intégrale( B^T * D * B ) dV
    return B.transpose() * D * B * area; 
}

Matrix<double, 6, 1> Solver::computeElementThermalForce(int tIdx) {
    const Triangle& tri = mesh.getTriangles()[tIdx];
    double area = 0.0;
    Matrix<double, 3, 6> B = mesh.computeBMatrix(tIdx, area);
    
    // F_th = intégrale( B^T * D * alpha * DeltaT ) dV
    Vector3d sigma_th = matMgr.getThermalStressVector(tri.label, deltaT, isPlaneStress);
    return B.transpose() * sigma_th * area;
}

Matrix<double, 3, 3> Solver::computeElementStiffnessAntiPlane(int tIdx) {
    const Triangle& tri = mesh.getTriangles()[tIdx];
    double G = matMgr.getShearModulus(tri.label); 
    
    const Vertex& p0 = mesh.getVertices()[tri.v[0]];
    const Vertex& p1 = mesh.getVertices()[tri.v[1]];
    const Vertex& p2 = mesh.getVertices()[tri.v[2]];

    double detJ = (p1.x - p0.x)*(p2.y - p0.y) - (p2.x - p0.x)*(p1.y - p0.y);
    double area = 0.5 * std::abs(detJ);
    double sign = (detJ > 0) ? 1.0 : -1.0;

    double b[3] = {p1.y - p2.y, p2.y - p0.y, p0.y - p1.y};
    double c[3] = {p2.x - p1.x, p0.x - p2.x, p1.x - p0.x};

    Matrix<double, 2, 3> B;
    for(int i=0; i<3; ++i) {
        B(0, i) = b[i];
        B(1, i) = c[i];
    }
    B *= sign / (2.0 * area);

    return B.transpose() * G * B * area;
}

// ============================================================================
//  2. ASSEMBLAGE GLOBAL (Lock-Free Multi-Threading & Variables d'état)
// ============================================================================

void Solver::assembleAntiPlane() {
    numDof = mesh.getVertices().size();
    F = VectorXd::Zero(numDof);

    int max_threads = omp_get_max_threads();
    vector<vector<Triplet<double>>> thread_triplets(max_threads);
    for(auto& t : thread_triplets) t.reserve((mesh.getTriangles().size() * 9) / max_threads + 100);

    #pragma omp parallel for
    for (int t = 0; t < (int)mesh.getTriangles().size(); ++t) {
        int tid = omp_get_thread_num();
        Matrix<double, 3, 3> Ke = computeElementStiffnessAntiPlane(t);
        const Triangle& tri = mesh.getTriangles()[t];

        for (int i=0; i<3; ++i) {
            for (int j=0; j<3; ++j) {
                if (std::abs(Ke(i, j)) > 1e-18)
                    thread_triplets[tid].emplace_back(tri.v[i], tri.v[j], Ke(i, j));
            }
        }
    }

    vector<Triplet<double>> triplets;
    for(const auto& t : thread_triplets) triplets.insert(triplets.end(), t.begin(), t.end());

    K_global.resize(numDof, numDof);
    K_global.setFromTriplets(triplets.begin(), triplets.end());
}

void Solver::assemble() {
    if (isAntiPlane) {
        assembleAntiPlane();
        return;
    }

    numDof = mesh.getVertices().size() * 2;
    F = VectorXd::Zero(numDof); // Reset du second membre à chaque itération
    
    // Initialisation silencieuse de l'endommagement si ce n'est pas encore fait
    if (elementDamage.size() != mesh.getTriangles().size()) {
        elementDamage.assign(mesh.getTriangles().size(), 0.0);
    }

    int max_threads = omp_get_max_threads();
    vector<vector<Triplet<double>>> thread_triplets(max_threads);
    vector<VectorXd> thread_F(max_threads, VectorXd::Zero(numDof));
    
    for(auto& t : thread_triplets) t.reserve((mesh.getTriangles().size() * 36) / max_threads + 100);

    #pragma omp parallel for
    for (int t = 0; t < (int)mesh.getTriangles().size(); ++t) {
        int tid = omp_get_thread_num();
        Matrix<double, 6, 6> Ke = computeElementStiffness(t);
        Matrix<double, 6, 1> Fe_th = computeElementThermalForce(t);
        const Triangle& tri = mesh.getTriangles()[t];

        // DÉGRADATION DE KACHANOV : Chute de rigidité selon l'état d'endommagement
        // Si elementDamage[t] = 0.99, l'élément perd 99% de sa raideur (micro-vide équivalent)
        double integrity = 1.0 - elementDamage[t];
        Ke *= integrity;
        Fe_th *= integrity; // <- LIGNE CRITIQUE AJOUTÉE

        int gdof[6];
        for(int k=0; k<3; ++k) {
            gdof[2*k]   = 2 * tri.v[k];
            gdof[2*k+1] = 2 * tri.v[k] + 1;
        }

        for (int i=0; i<6; ++i) {
            thread_F[tid](gdof[i]) += Fe_th(i); 
            for (int j=0; j<6; ++j) {
                if (std::abs(Ke(i, j)) > 1e-18)
                    thread_triplets[tid].emplace_back(gdof[i], gdof[j], Ke(i, j));
            }
        }
    }

    vector<Triplet<double>> triplets;
    for(int i=0; i<max_threads; ++i) {
        triplets.insert(triplets.end(), thread_triplets[i].begin(), thread_triplets[i].end());
        F += thread_F[i];
    }

    K_global.resize(numDof, numDof);
    K_global.setFromTriplets(triplets.begin(), triplets.end());
}

// ============================================================================
//  3. ENREGISTREMENT DES CONDITIONS AUX LIMITES (Délégation au résolveur)
// ============================================================================

void Solver::applyBoundaryConditionsAntiPlane() { /* Délégué à condenseAndSolveSystem */ }
void Solver::applyBoundaryConditions() { /* Délégué à condenseAndSolveSystem */ }

void Solver::addBoundaryCondition(NodeSelector selector, int direction, double value) {
    boundaryConditions.emplace_back(selector, direction, value);
}

void Solver::addPeriodicCondition(int nodeSlave, int nodeMaster, int direction, double delta) {
    periodicConditions.emplace_back(nodeSlave, nodeMaster, direction, delta);
}

// ============================================================================
//  4. RÉSOLUTION LINÉAIRE ET CONDENSATION EXACTE
// ============================================================================

void Solver::condenseAndSolveSystem() {
    std::vector<int> master(numDof);
    std::vector<double> offset(numDof, 0.0);
    std::vector<bool> isFixed(numDof, false);
    std::vector<double> fixedVal(numDof, 0.0);
    
    for(int i=0; i<numDof; ++i) master[i] = i;

    // Mapping Périodique (Esclave -> Maître)
    for(const auto& pbc : periodicConditions) {
        int s, m;
        if (isAntiPlane) {
            s = pbc.nodeSlave; m = pbc.nodeMaster;
        } else {
            s = 2 * pbc.nodeSlave + pbc.direction;
            m = 2 * pbc.nodeMaster + pbc.direction;
        }
        master[s] = m;
        offset[s] = pbc.delta;
    }

    // Mapping de Dirichlet (Appuis fixes ou imposés)
    const auto& vertices = mesh.getVertices();
    for (int i = 0; i < (int)vertices.size(); ++i) {
        double x = vertices[i].x;
        double y = vertices[i].y;
        for (const auto& bc : boundaryConditions) {
            if (bc.selector(x, y)) {
                int dof = isAntiPlane ? i : (2 * i + bc.direction);
                isFixed[dof] = true;
                fixedVal[dof] = bc.value;
            }
        }
    }

    int numRed = 0;
    std::vector<int> redId(numDof, -1);
    for(int i=0; i<numDof; ++i) {
        if (master[i] == i && !isFixed[i]) {
            redId[i] = numRed++;
        }
    }

    std::vector<Triplet<double>> p_triplets;
    VectorXd U0 = VectorXd::Zero(numDof);
    
    for(int i=0; i<numDof; ++i) {
        int m_root = master[i];
        double total_offset = offset[i];
        int max_depth = 10; 
        
        while(master[m_root] != m_root && max_depth > 0) {
            total_offset += offset[m_root];
            m_root = master[m_root];
            max_depth--;
        }
        if (max_depth == 0) throw runtime_error("Erreur Topologique : Boucle infinie dans les CL periodiques !");
        
        if (isFixed[m_root]) {
            U0(i) = fixedVal[m_root] + total_offset;
        } else {
            if (redId[m_root] != -1) {
                p_triplets.emplace_back(i, redId[m_root], 1.0);
            }
            U0(i) = total_offset;
        }
    }

    SparseMatrix<double> P(numDof, numRed);
    P.setFromTriplets(p_triplets.begin(), p_triplets.end());

    // U = P * U_red + U0 -> K_red * U_red = F_red
    SparseMatrix<double> K_red = P.transpose() * K_global * P;
    VectorXd F_red = P.transpose() * (F - K_global * U0);

    SimplicialLLT<SparseMatrix<double>> solver;
    solver.compute(K_red);
    if(solver.info() != Success) throw runtime_error("Echec Cholesky (Matrice singuliere) ! Trop d'elements endommages simultanement ?");
    
    VectorXd U_red = solver.solve(F_red);
    if(solver.info() != Success) throw runtime_error("Echec resolution LLT !");
    
    U = P * U_red + U0; // Re-projection spatiale
}

void Solver::solve() {
    cout << "   [Solver] Resolution lineaire classique..." << endl;
    condenseAndSolveSystem();
}

// ============================================================================
//  5. MOTEUR NON-LINÉAIRE (PROGRESSIVE DAMAGE ANALYSIS - PDA)
// ============================================================================

void Solver::solveNonLinear(double final_deltaT, int numSteps, std::function<void(int, double)> onStepComplete) {
    int numTriangles = mesh.getTriangles().size();
    
    // Initialisation des variables d'état (Espace apparent et variable thermodynamique)
    elementDamage.assign(numTriangles, 0.0); 
    std::vector<double> elementHistory(numTriangles, 1.0); // r_t : Seuil irréversible d'endommagement initialisé à 1.0
    
    cached_Sxx.assign(numTriangles, 0.0);
    cached_Syy.assign(numTriangles, 0.0);
    cached_Txy.assign(numTriangles, 0.0);
    cached_FI.assign(numTriangles, 0.0);
    
    double step_dT = final_deltaT / (double)numSteps;
    double current_dT = 0.0;

    cout << "\n   >>> DEBUT DE L'ANALYSE D'ENDOMMAGEMENT PROGRESSIF (CDM - Crack Band Theory) <<<" << endl;

    for (int step = 1; step <= numSteps; ++step) {
        current_dT += step_dT;
        this->deltaT = current_dT; 
        
        cout << "\n   [PALIER " << step << "/" << numSteps << "] dT = " << current_dT << " °C" << endl;

        bool isEquilibriumReached = false;
        int max_iters = 50; 
        int iter = 0;
        double tolerance_d = 1e-3; // Critère de convergence interne sur l'évolution du dommage

        // Boucle de Newton Sécante : Recherche de l'équilibre thermodynamique
        while (!isEquilibriumReached && iter < max_iters) {
            iter++;
            
            // L'assemblage doit maintenant dégrader à la fois Ke ET Fe_th via (1 - d)
            assemble(); 
            condenseAndSolveSystem(); 
            
            double max_damage_increment = 0.0;
            
            for (int t = 0; t < numTriangles; ++t) {
                const Triangle& tri = mesh.getTriangles()[t];
                
                // Extraction du vecteur de déplacement local de l'élément
                VectorXd U_elem(6);
                for(int k=0; k<3; ++k) {
                    U_elem(2*k)   = U(2 * tri.v[k]);
                    U_elem(2*k+1) = U(2 * tri.v[k] + 1);
                }
                
                double area;
                Matrix<double, 3, 6> B = mesh.computeBMatrix(t, area);
                Vector3d eps = B * U_elem;

                // 1. ESPACE EFFECTIF : Contrainte sur le squelette intact
                Matrix3d D0 = matMgr.getHookeMatrix(tri.label, isPlaneStress);
                Vector3d sigma_th = matMgr.getThermalStressVector(tri.label, deltaT, isPlaneStress);
                Vector3d sigma_eff = D0 * eps - sigma_th;

                // 2. CRITÈRE D'INITIATION : Évaluation sur la contrainte effective
                double FI = 0.0;
                if (tri.label == 1 || tri.label == 3) { // Matrice Epoxy ou Interphase (Isotrope, sensible à la pression)
                    FI = matMgr.computeDruckerPragerPlaneStrain(tri.label, sigma_eff(0), sigma_eff(1), sigma_eff(2));
                } else if (tri.label == 2 || tri.label == 102) { // Fibre de carbone (Orthotrope)
                    FI = matMgr.computeTsaiWuIsotropicPlaneStrain(tri.label, sigma_eff(0), sigma_eff(1), sigma_eff(2));
                }

                // 3. VARIABLE D'HISTOIRE : Le dommage est irréversible, la matière "mémorise" le pire état
                if (FI > elementHistory[t]) {
                    elementHistory[t] = FI;
                }

                // 4. RÉGULARISATION ÉNERGÉTIQUE ET CALCUL DU DOMMAGE (Crack Band Theory)
                double d_new = 0.0;
                if (elementHistory[t] > 1.0) {
                    
                    double lc = std::sqrt(area); // Longueur caractéristique de l'élément 2D
                    
                    double E = matMgr.getYoungModulus(tri.label);
                    double Xt = matMgr.getXt(tri.label);
                    double GIc = matMgr.getGIc(tri.label); 

                    // Densité d'énergie de déformation élastique stockée au moment de l'initiation
                    double We = (Xt * Xt) / (2.0 * E);

                    // Seuil thermodynamique de rupture ultime (r_f)
                    double rf = GIc / (lc * We);

                    // GARDE-FOU : Vérification du Snap-Back constitutif
                    if (rf <= 1.0) {
                        throw std::runtime_error("\n[ERREUR] SNAP-BACK THERMODYNAMIQUE DETECTE !\n"
                                                 "L'element " + std::to_string(t) + " (Aire=" + std::to_string(area) + 
                                                 ") est trop grand pour dissiper l'energie GIc = " + std::to_string(GIc) + 
                                                 ". Le maillage doit etre raffine pour eviter une rupture instantanee non-physique.");
                    }

                    double r = elementHistory[t];
                    if (r >= rf) {
                        d_new = 0.99; // Rupture totale atteinte (on bloque à 0.99 pour ne pas rendre K singulière)
                    } else {
                        // Loi d'adoucissement linéaire (Linear Softening)
                        d_new = (rf * (r - 1.0)) / (r * (rf - 1.0));
                    }
                    
                    // Sécurité numérique absolue
                    if (d_new > 0.99) d_new = 0.99;
                    if (d_new < 0.0) d_new = 0.0;
                }

                // 5. SUIVI DE LA CONVERGENCE : Méthode de la sécante
                double delta_d = std::abs(d_new - elementDamage[t]);
                if (delta_d > max_damage_increment) {
                    max_damage_increment = delta_d;
                }

                // Application du nouveau dommage à l'élément
                elementDamage[t] = d_new;

                // 6. ESPACE APPARENT : Mise en cache des contraintes réelles pour Paraview
                cached_FI[t] = elementHistory[t]; // On visualise la pire sollicitation (la cause du dommage)
                cached_Sxx[t] = (1.0 - d_new) * sigma_eff(0);
                cached_Syy[t] = (1.0 - d_new) * sigma_eff(1);
                cached_Txy[t] = (1.0 - d_new) * sigma_eff(2);
            }

            if (max_damage_increment < tolerance_d) {
                isEquilibriumReached = true;
                cout << "      -> Equilibre interne atteint en " << iter << " iterations. (max dD = " << max_damage_increment << ")" << endl;
            }
        } 

        if (!isEquilibriumReached) {
            cout << "      /!\\ INSTABILITE NON-LINEAIRE : Pas de convergence apres " << max_iters << " iterations." << endl;
            // On continue malgré tout, typique des solveurs explicites ou lorsque la structure s'effondre.
        }
        
        // Export VTK / Callback python
        if (onStepComplete) {
            onStepComplete(step, current_dT);
        }
    } 
    cout << "   >>> FIN DE L'ANALYSE <<<" << endl;
}