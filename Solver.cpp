//Dans ce fichier je ne fais que traduire en C++ 
//L'algorithme GMRES donné en TER 1A

#include <iostream>
#include "Solver.h"

using namespace std;
using namespace Eigen;



void pmres(COO A , VectorXd b , int iter_max)
{
    double const tol = 1E-5; 
    VectorXd x = 0* b;
    VectorXd r = b , w;

    int k(0);
    double alpha ;

    while ((r.norm()/b.norm() > tol) && (k<=iter_max))
    {
        w = A.Prod(r);
        alpha = w.dot(r)/w.dot(w) ;
        x += alpha * r;
        r -= alpha*w ; 
        k++;

    }
    cout << x <<endl;
}





void pgmres(COO A, VectorXd b,int iter_max)
{
    VectorXd r0;
    //On prendra x0 nul 
    r0 = b ;
    VectorXd v0 = r0 / r0.norm();
    Matrix<VectorXd,1,Dynamic> V , W ;
    Matrix<MatrixXd,1,Dynamic> H;
    H.resize(1,iter_max);
    V.resize(1,iter_max);
    W.resize(1,iter_max);
    V(0) = v0;
    H(0).resize(1,1);
    H(0)(0,0) = 0.;
    VectorXd v , w ;
    for (int k = 1 ; k < iter_max ; k++)
    {
        H(k).resize(k,k);
        W(k) = A.Prod(V(k));
        for (int j = 1 ; j<k ; j++)
        {
            H(k)(j,k) = V(j).transpose() * W(k) ;
            W(k) = W(k) - H(k)(j,k) * V(j);
        }
        H(k)(k,k) = W(k).norm() ;
        V(k+1) = W(k) / H(k)(k,k) ;
        H(k).block(0,0,k-1,k-1) = H(k-1);
        H(k).block(k,0,k,k) = H(k).block(0,k,k,k).transpose();

        /* Instruction finale de chatgpt:
        on calcul y comme y = 'argmin_y(||H(k)*y-beta * e1||)
        où e1 est le vecteur de base de la base euclidienne
        si ||A*xk - b || < tolerance alors c'est fini
        on obtient la solution comme 
        x = x0 + somme de 1 a kmax de (yk*vk)*/

    }





}
