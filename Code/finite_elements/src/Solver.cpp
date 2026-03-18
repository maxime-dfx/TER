#include "Solver.h"
#include <iostream>
#include <omp.h>

using namespace std;
using namespace Eigen;

Solver::Solver(const Mesh& m, const MaterialManager& mat, bool planeStress) 
    : mesh(m), matMgr(mat), isPlaneStress(planeStress) {
    numDof = mesh.getVertices().size() * 2;
    F = VectorXd::Zero(numDof);
    U = VectorXd::Zero(numDof);
}
Matrix<double, 6, 6> Solver::computeElementStiffness(int tIdx) {
    const Triangle& tri = mesh.getTriangles()[tIdx];
    Matrix3d D = matMgr.getHookeMatrix(tri.label, isPlaneStress); // Point 1.A conservé !
    
    double area = 0.0;
    Matrix<double, 3, 6> B = mesh.computeBMatrix(tIdx, area);

    return B.transpose() * D * B * area;
}

void Solver::assemble() {
    cout << "   [Solver] Assemblage Parallele (" << omp_get_max_threads() << " threads)..." << endl;
    
    vector<Triplet<double>> triplets;
    size_t est_size = mesh.getTriangles().size() * 36;
    triplets.reserve(est_size);

    #pragma omp parallel
    {
        vector<Triplet<double>> local_triplets;
        local_triplets.reserve(est_size / omp_get_num_threads());

        #pragma omp for
        for (int t = 0; t < (int)mesh.getTriangles().size(); ++t) {
            Matrix<double, 6, 6> Ke = computeElementStiffness(t);
            const Triangle& tri = mesh.getTriangles()[t];

            int gdof[6];
            for(int k=0; k<3; ++k) {
                gdof[2*k]   = 2 * tri.v[k];
                gdof[2*k+1] = 2 * tri.v[k] + 1;
            }

            for (int i=0; i<6; ++i) {
                for (int j=0; j<6; ++j) {
                    if (std::abs(Ke(i, j)) > 1e-16)
                        local_triplets.emplace_back(gdof[i], gdof[j], Ke(i, j));
                }
            }
        }
        #pragma omp critical
        triplets.insert(triplets.end(), local_triplets.begin(), local_triplets.end());
    }

    K_global.resize(numDof, numDof);
    K_global.setFromTriplets(triplets.begin(), triplets.end());
}

void Solver::addBoundaryCondition(NodeSelector selector, int direction, double value) {
    boundaryConditions.emplace_back(selector, direction, value);
}
void Solver::addPeriodicCondition(int nodeSlave, int nodeMaster, int direction, double delta) {
    periodicConditions.emplace_back(nodeSlave, nodeMaster, direction, delta);
}

void Solver::applyBoundaryConditions() {
    if (boundaryConditions.empty() && periodicConditions.empty()) 
        throw runtime_error("Aucune CL definie !");
    
    cout << "   [Solver] Application des CL (Standard: " << boundaryConditions.size() 
         << ", Periodiques: " << periodicConditions.size() << ")..." << endl;
    
    double max_diag = 0.0;
    for (int k=0; k<K_global.outerSize(); ++k)
        for (SparseMatrix<double>::InnerIterator it(K_global, k); it; ++it)
            if (it.row() == it.col()) max_diag = std::max(max_diag, std::abs(it.value()));

    double penalty = max_diag * 1.0e8;

    // --- 1. Conditions de Dirichlet Classiques (existantes) ---
    const auto& vertices = mesh.getVertices();
    for (int i = 0; i < (int)vertices.size(); ++i) {
        double x = vertices[i].x;
        double y = vertices[i].y;
        for (const auto& bc : boundaryConditions) {
            if (bc.selector(x, y)) {
                int dof = 2 * i + bc.direction;
                K_global.coeffRef(dof, dof) += penalty;
                F(dof) += penalty * bc.value;
            }
        }
    }

    // --- 2. Conditions Périodiques (NOUVEAU) ---
    // Résout U_slave - U_master = delta par pénalisation
    for (const auto& pbc : periodicConditions) {
        int dof_s = 2 * pbc.nodeSlave + pbc.direction;
        int dof_m = 2 * pbc.nodeMaster + pbc.direction;

        // Ajout dans la matrice de rigidité (couplage des deux DDL)
        K_global.coeffRef(dof_s, dof_s) += penalty;
        K_global.coeffRef(dof_m, dof_m) += penalty;
        K_global.coeffRef(dof_s, dof_m) -= penalty;
        K_global.coeffRef(dof_m, dof_s) -= penalty;

        // Ajout dans le vecteur second membre
        F(dof_s) += penalty * pbc.delta;
        F(dof_m) -= penalty * pbc.delta;
    }
}

void Solver::solve() {
    cout << "   [Solver] Resolution LLT..." << flush;
    SimplicialLLT<SparseMatrix<double>> solver;
    solver.compute(K_global);
    
    if(solver.info() != Success) throw runtime_error("Echec decomposition ! Matrice singuliere ?");
    
    U = solver.solve(F);
    if(solver.info() != Success) throw runtime_error("Echec resolution !");
    
    cout << " OK." << endl;
}