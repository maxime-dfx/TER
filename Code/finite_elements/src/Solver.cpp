#include "Solver.h"
#include <iostream>
#include <omp.h>

using namespace std;
using namespace Eigen;

Solver::Solver(const Mesh& m, const MaterialManager& mat) : mesh(m), matMgr(mat) {
    numDof = mesh.vertices.size() * 2;
    F = VectorXd::Zero(numDof);
    U = VectorXd::Zero(numDof);
}

Matrix<double, 6, 6> Solver::computeElementStiffness(int tIdx) {
    const Triangle& tri = mesh.triangles[tIdx];
    
    // 1. Physique (Direct Eigen)
    Matrix3d D = matMgr.getHookeMatrix(tri.label, false);

    // 2. Géométrie (Via Mesh)
    double area = mesh.getTriangleArea(tIdx);
    
    // Matrice B
    const Vertex& p0 = mesh.vertices[tri.v[0]];
    const Vertex& p1 = mesh.vertices[tri.v[1]];
    const Vertex& p2 = mesh.vertices[tri.v[2]];

    double b[3] = {p1.y - p2.y, p2.y - p0.y, p0.y - p1.y};
    double c[3] = {p2.x - p1.x, p0.x - p2.x, p1.x - p0.x};

    Matrix<double, 3, 6> B = Matrix<double, 3, 6>::Zero();
    for(int i=0; i<3; ++i) {
        B(0, 2*i) = b[i]; B(1, 2*i+1) = c[i];
        B(2, 2*i) = c[i]; B(2, 2*i+1) = b[i];
    }
    
    // Vérification de l'orientation (DetJ)
    double detJ = (p1.x - p0.x)*(p2.y - p0.y) - (p2.x - p0.x)*(p1.y - p0.y);
    double sign = (detJ > 0) ? 1.0 : -1.0;
    
    B *= sign / (2.0 * area);

    return B.transpose() * D * B * area;
}

void Solver::assemble() {
    cout << "   [Solver] Assemblage Parallele (" << omp_get_max_threads() << " threads)..." << endl;
    
    vector<Triplet<double>> triplets;
    size_t est_size = mesh.triangles.size() * 36;
    triplets.reserve(est_size);

    #pragma omp parallel
    {
        vector<Triplet<double>> local_triplets;
        local_triplets.reserve(est_size / omp_get_num_threads());

        #pragma omp for
        for (int t = 0; t < (int)mesh.triangles.size(); ++t) {
            Matrix<double, 6, 6> Ke = computeElementStiffness(t);
            const Triangle& tri = mesh.triangles[t];

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

void Solver::applyBoundaryConditions() {
    if (boundaryConditions.empty()) throw runtime_error("Aucune CL définie !");
    
    cout << "   [Solver] Application des CL (" << boundaryConditions.size() << ")..." << endl;
    
    // Pénalité adaptative
    double max_diag = 0.0;
    for (int k=0; k<K_global.outerSize(); ++k)
        for (SparseMatrix<double>::InnerIterator it(K_global, k); it; ++it)
            if (it.row() == it.col()) max_diag = std::max(max_diag, std::abs(it.value()));

    double penalty = max_diag * 1.0e12;

    for (int i = 0; i < (int)mesh.vertices.size(); ++i) {
        double x = mesh.vertices[i].x;
        double y = mesh.vertices[i].y;

        for (const auto& bc : boundaryConditions) {
            if (bc.selector(x, y)) {
                int dof = 2 * i + bc.direction;
                K_global.coeffRef(dof, dof) += penalty;
                F(dof) += penalty * bc.value;
            }
        }
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