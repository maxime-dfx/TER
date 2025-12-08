#include "Solver.h"
#include <iostream>
#include <cmath>
#include <vector>

using namespace std;
using namespace Eigen;

// =========================================================
// CONSTRUCTEUR
// =========================================================
Solver::Solver(const Mesh& m, const MaterialManager& mat) 
    : mesh(m), matMgr(mat) {
    
    // 1. Initialisation des tailles
    numNodes = mesh.vertices.size();
    numDof = numNodes * 2; // 2 inconnues par noeud (Ux, Uy)

    // 2. On prépare les vecteurs (remplis de 0)
    F.resize(numDof);
    F.setZero();
    
    U.resize(numDof);
    U.setZero();
}

// =========================================================
// CALCUL ELEMENTAIRE (Matrice 6x6 d'un triangle)
// =========================================================
Matrix<double, 6, 6> Solver::computeElementStiffness(int triangleIndex) {
    
    // 1. Récupérer les sommets
    const Triangle& tri = mesh.triangles[triangleIndex];
    const Vertex& p1 = mesh.vertices[tri.v[0]];
    const Vertex& p2 = mesh.vertices[tri.v[1]];
    const Vertex& p3 = mesh.vertices[tri.v[2]];

    // 2. Géométrie (P1)
    double x1 = p1.x, y1 = p1.y;
    double x2 = p2.x, y2 = p2.y;
    double x3 = p3.x, y3 = p3.y;

    // Déterminant Jacobien (2 * Aire)
    double detJ = (x2 - x1)*(y3 - y1) - (x3 - x1)*(y2 - y1);
    double area = 0.5 * std::abs(detJ);

    // Dérivées des fonctions de forme
    double b1 = y2 - y3; double c1 = x3 - x2;
    double b2 = y3 - y1; double c2 = x1 - x3;
    double b3 = y1 - y2; double c3 = x2 - x1;

    // 3. Matrice B (Strain-Displacement)
    Matrix<double, 3, 6> B;
    B.setZero();
    
    B(0,0) = b1; B(0,2) = b2; B(0,4) = b3;
    B(1,1) = c1; B(1,3) = c2; B(1,5) = c3;
    B(2,0) = c1; B(2,1) = b1; 
    B(2,2) = c2; B(2,3) = b2; 
    B(2,4) = c3; B(2,5) = b3;
    
    B /= detJ;

    // 4. Matrice D (Loi de Hooke)
    Matrix3d D;
    double D_raw[3][3];
    matMgr.getHookeMatrix(tri.label, D_raw, false); // false = Déformation Plane

    for(int i=0; i<3; i++)
        for(int j=0; j<3; j++)
            D(i,j) = D_raw[i][j];

    // 5. Calcul Final : Ke = B^T * D * B * Aire
    return B.transpose() * D * B * area;
}

// =========================================================
// ASSEMBLAGE (Construction de K global)
// =========================================================
void Solver::assemble() {
    cout << "Assemblage de la matrice globale (" << numDof << " d.o.f)..." << endl;

    // 1. Liste de triplets pour Eigen
    vector<Triplet<double>> triplets;
    triplets.reserve(mesh.triangles.size() * 36);

    // 2. Boucle sur TOUS les triangles
    for (size_t t = 0; t < mesh.triangles.size(); ++t) {
        
        // A. Calcul matrice locale
        Matrix<double, 6, 6> Ke = computeElementStiffness(t);

        // B. Indices globaux
        const Triangle& tri = mesh.triangles[t];
        int nodes[3] = { tri.v[0], tri.v[1], tri.v[2] };

        int globalIndices[6];
        for (int i = 0; i < 3; ++i) {
            globalIndices[2*i]     = 2 * nodes[i];     // Ux
            globalIndices[2*i + 1] = 2 * nodes[i] + 1; // Uy
        }

        // C. Remplissage triplets
        for (int i = 0; i < 6; ++i) {
            for (int j = 0; j < 6; ++j) {
                double val = Ke(i, j);
                if (std::abs(val) > 1e-12) {
                    triplets.push_back(Triplet<double>(globalIndices[i], globalIndices[j], val));
                }
            }
        }
    }

    // 3. Construction matrice Sparse
    K_global.resize(numDof, numDof);
    K_global.setFromTriplets(triplets.begin(), triplets.end());

    cout << "Assemblage termine." << endl;
}

// =========================================================
// GESTION DES CL (Conditions aux Limites)
// =========================================================
void Solver::addBoundaryCondition(NodeSelector selector, int direction, double value) {
    boundaryConditions.push_back(BoundaryCondition(selector, direction, value));
}

// =========================================================
// RESOLUTION (K.U = F)
// =========================================================
void Solver::solve() {
    cout << "Application des Conditions aux Limites (" << boundaryConditions.size() << " CLs)..." << endl;

    double penalty = 1e14; // Facteur de pénalisation

    // On parcourt tous les noeuds
    for (int i = 0; i < numNodes; ++i) {
        double x = mesh.vertices[i].x;
        double y = mesh.vertices[i].y;

        // On teste toutes les CL pour ce noeud
        for (const auto& bc : boundaryConditions) {
            
            // Si le noeud est concerné (critère géométrique)
            if (bc.selector(x, y)) {
                
                int dof = 2 * i + bc.direction; // Indice global (Ux ou Uy)

                // Méthode de pénalisation : 
                // On ajoute un terme géant sur la diagonale et dans le vecteur force
                K_global.coeffRef(dof, dof) += penalty;
                F(dof) += penalty * bc.value;
            }
        }
    }

    cout << "Résolution du système (SimplicialLLT)..." << endl;
    
    // Résolution directe (Cholesky pour matrices creuses symétriques définies positives)
    Eigen::SimplicialLLT<Eigen::SparseMatrix<double>> solver;
    
    solver.compute(K_global);
    
    if(solver.info() != Eigen::Success) {
        cerr << "ERREUR CRITIQUE : La décomposition de la matrice a échoué." << endl;
        return;
    }

    U = solver.solve(F);

    if(solver.info() != Eigen::Success) {
        cerr << "ERREUR CRITIQUE : La résolution a échoué." << endl;
        return;
    }

    cout << "Résolution terminée." << endl;
    cout << " -> Déplacement Max : " << U.maxCoeff() << endl;
    cout << " -> Déplacement Min : " << U.minCoeff() << endl;
}

// =========================================================
// UTILITAIRE
// =========================================================
void Solver::printSystemInfo() const {
    cout << "Info Systeme Global :" << endl;
    cout << " - Taille matrice    : " << numDof << " x " << numDof << endl;
    cout << " - Termes non-nuls   : " << K_global.nonZeros() << endl;
    
    double fill = (double)K_global.nonZeros() / (double(numDof) * double(numDof));
    cout << " - Taux remplissage  : " << fill * 100.0 << " %" << endl;
}