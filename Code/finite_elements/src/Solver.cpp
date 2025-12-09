#include "Solver.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <omp.h>

using namespace std;
using namespace Eigen;

// =========================================================
// CONSTRUCTEUR
// =========================================================
Solver::Solver(const Mesh& m, const MaterialManager& mat) 
    : mesh(m), matMgr(mat) {
    
    numNodes = mesh.vertices.size();
    numDof = numNodes * 2; 

    F.resize(numDof);
    F.setZero();
    
    U.resize(numDof);
    U.setZero();
}

// =========================================================
// CALCUL ELEMENTAIRE (CORRIGÉ)
// =========================================================
Matrix<double, 6, 6> Solver::computeElementStiffness(int triangleIndex) {
    const Triangle& tri = mesh.triangles[triangleIndex];
    
    // Récupération des propriétés matériau (utilisation d'Eigen au lieu de tableau C)
    double D_array[3][3];
    matMgr.getHookeMatrix(tri.label, D_array, false);
    
    Matrix<double, 3, 3> D;
    for(int i=0; i<3; i++) 
        for(int j=0; j<3; j++) 
            D(i,j) = D_array[i][j];

    // Coordonnées
    double x[3], y[3];
    for(int i=0; i<3; ++i) {
        x[i] = mesh.vertices[tri.v[i]].x;
        y[i] = mesh.vertices[tri.v[i]].y;
    }

    // CORRECTION : Calcul correct de l'aire et vérification d'orientation
    double detJ = (x[1]-x[0])*(y[2]-y[0]) - (x[2]-x[0])*(y[1]-y[0]);
    
    if (std::abs(detJ) < 1e-16) {
        throw std::runtime_error("Triangle dégénéré détecté (aire nulle) - index: " + 
                                 std::to_string(triangleIndex));
    }
    
    double area = 0.5 * std::abs(detJ);
    double sign = (detJ > 0) ? 1.0 : -1.0;

    // Dérivées des fonctions de forme
    double b[3], c[3];
    b[0] = y[1]-y[2]; b[1] = y[2]-y[0]; b[2] = y[0]-y[1];
    c[0] = x[2]-x[1]; c[1] = x[0]-x[2]; c[2] = x[1]-x[0];

    // Matrice B (3x6)
    Matrix<double, 3, 6> B;
    B.setZero();
    for(int i=0; i<3; ++i) {
        B(0, 2*i)   = b[i];
        B(1, 2*i+1) = c[i];
        B(2, 2*i)   = c[i];
        B(2, 2*i+1) = b[i];
    }
    
    // CORRECTION MAJEURE : Division par (2*area) et non par detJ
    B *= sign / (2.0 * area);

    // Ke = B^T * D * B * area (épaisseur = 1.0)
    return B.transpose() * D * B * area; 
}

// =========================================================
// ASSEMBLAGE (PARALLELE OPTIMISÉ)
// =========================================================
void Solver::assemble() {
    cout << "   [Solver] Assemblage de la matrice..." << flush;
    
    // CORRECTION : Meilleure estimation (chaque élément contribue 36 termes)
    size_t estimated_triplets = mesh.triangles.size() * 36;
    vector<Triplet<double>> triplets;
    triplets.reserve(estimated_triplets);

    #pragma omp parallel
    {
        vector<Triplet<double>> local_triplets;
        // CORRECTION : Réserve plus réaliste par thread
        int num_threads = omp_get_num_threads();
        local_triplets.reserve((estimated_triplets / num_threads) + 100);

        #pragma omp for schedule(dynamic, 64)
        for (int t = 0; t < (int)mesh.triangles.size(); ++t) {
            Matrix<double, 6, 6> Ke = computeElementStiffness(t);
            const Triangle& tri = mesh.triangles[t];
            
            int gdof[6];
            for(int k=0; k<3; ++k) {
                gdof[2*k]   = 2 * tri.v[k];
                gdof[2*k+1] = 2 * tri.v[k] + 1;
            }

            for (int i = 0; i < 6; ++i) {
                for (int j = 0; j < 6; ++j) {
                    if (std::abs(Ke(i, j)) > 1e-16) {
                        local_triplets.push_back(Triplet<double>(gdof[i], gdof[j], Ke(i, j)));
                    }
                }
            }
        }

        #pragma omp critical
        {
            triplets.insert(triplets.end(), local_triplets.begin(), local_triplets.end());
        }
    }

    K_global.resize(numDof, numDof);
    K_global.setFromTriplets(triplets.begin(), triplets.end());
    
    cout << " Fait (" << triplets.size() << " termes non-nuls)." << endl;
}

// =========================================================
// GESTION CL ET RESOLUTION (CORRIGÉE)
// =========================================================
void Solver::addBoundaryCondition(NodeSelector selector, int direction, double value) {
    boundaryConditions.emplace_back(selector, direction, value);
}

void Solver::solve() {
    // CORRECTION : Vérification qu'il y a des CL
    if (boundaryConditions.empty()) {
        throw std::runtime_error("Aucune condition aux limites définie ! Le système est singulier.");
    }
    
    cout << "   [Solver] Application des CL (" << boundaryConditions.size() << " conditions)..." << endl;

    // CORRECTION : Pénalité plus robuste (max * 10^12 au lieu de 10^10)
    double max_diag = 0.0;
    for (int k=0; k<K_global.outerSize(); ++k) {
        for (SparseMatrix<double>::InnerIterator it(K_global, k); it; ++it) {
            if (it.row() == it.col()) {
                max_diag = std::max(max_diag, std::abs(it.value()));
            }
        }
    }
    
    if (max_diag < 1e-16) {
        throw std::runtime_error("Matrice globale nulle ou quasi-nulle ! Vérifiez les propriétés matériaux.");
    }
    
    double penalty = max_diag * 1.0e12;
    cout << "   [Solver] Pénalité = " << penalty << " (max_diag = " << max_diag << ")" << endl;

    // Application
    int applied_count = 0;
    for (int i = 0; i < numNodes; ++i) {
        double x = mesh.vertices[i].x;
        double y = mesh.vertices[i].y;

        for (const auto& bc : boundaryConditions) {
            if (bc.selector(x, y)) {
                int dof = 2 * i + bc.direction;
                K_global.coeffRef(dof, dof) += penalty;
                F(dof) += penalty * bc.value;
                applied_count++;
            }
        }
    }
    
    // CORRECTION : Vérifier qu'au moins un DDL a été bloqué
    if (applied_count == 0) {
        throw std::runtime_error("Aucun nœud ne satisfait les conditions aux limites ! Vérifiez vos sélecteurs.");
    }
    
    cout << "   [Solver] " << applied_count << " DDLs bloqués." << endl;

    // Résolution
    cout << "   [Solver] Factorisation Cholesky..." << flush;
    SimplicialLLT<SparseMatrix<double>> solver;
    solver.compute(K_global);
    
    if(solver.info() != Success) {
        throw std::runtime_error("La matrice est singulière ou mal conditionnée ! Vérifiez les CL.");
    }
    cout << " OK." << endl;

    cout << "   [Solver] Résolution du système..." << flush;
    U = solver.solve(F);
    
    if(solver.info() != Success) {
        throw std::runtime_error("Échec de la résolution du système linéaire !");
    }
    
    cout << " Terminé." << endl;
    
    // Affichage du déplacement max pour validation
    double max_displacement = U.cwiseAbs().maxCoeff();
    cout << "   [Solver] Déplacement max = " << max_displacement << endl;
}

void Solver::printSystemInfo() const {
    cout << "   [Info] Taille matrice : " << numDof << "x" << numDof << endl;
    cout << "   [Info] Termes non-nuls : " << K_global.nonZeros() << endl;
    cout << "   [Info] Mémoire approx : " << K_global.nonZeros() * sizeof(double) / 1024 / 1024 << " MB" << endl;
}