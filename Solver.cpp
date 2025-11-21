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


void grad_conj(COO A,VectorXd b,VectorXd x0,int iter_max,double epsilon)
{
    VectorXd x = 0*b;
    double norm_b = b.norm();
    if (norm_b < 1e-30)
    {
        cout << "x = "<<endl<<x<<endl;
    } 
    x = x0;
    VectorXd r = b - A.Prod(x);
    int k = 0;
    while(r.norm()/norm_b > epsilon)
    {
        COO J ,L , U;
        J = A.diagonal();
        L = A.tril();
        U = A.triu();
        VectorXd z = r;
        z = solve_tri(L,z,true);
        cout<<"z = "<<z<<endl;
        z = J.Prod(z);
        cout<<"z = "<<z<<endl;
        z = solve_tri(U,z,false);
        cout<<"z = "<<z<<endl;
        double rho = r.dot(z);
        double rho0 = 1 ;
        if (abs(rho) < 1e-100)
        {
            cout <<"Echec rho = 0"<<endl;
            break;
        }
        VectorXd p;
        if (k==0)
        {
            p = z;
        }
        else
        {
            double gamma = rho/rho0;
            p = gamma * p + z ;
        }
        VectorXd q = A.Prod(p);
        double delta = p.dot(q);
        if (delta <1e-100)
        {
            cout <<"Echec delta =0"<<endl;
            break;
        }
        x += rho/delta * p;
        r -= rho/delta * q;
        rho0 = rho;
        k ++;
    }
    cout <<"Fait en "<< k << " iterations"<<endl;
    cout <<"x = " <<x<<endl;

}


//Resoud Ax = b si A est triangulaire 'strict' (diag nulle)
VectorXd solve_tri(COO A , VectorXd b , bool lower)
{
    VectorXd x = 0*b;
    double temp(0.);
    int n = A.Get1().size();
    int m = A.GetSize();
    VectorXi M1 = A.Get1() , M2 = A.Get2() ;
    VectorXd M3 = A.Get3();
    if (lower == true)
    {
        for (int i = 0 ; i<m ; i++)
        {
            temp = 0;
            //Produit scalaire entre ligne i de A (ici tri inf) et x
            for (int j = 1 ; i<n ;i ++)
            {
                if (M1(j) == i)
                {
                    temp += M3(j)*x(M2(j));
                }
            }
            x(i) = b(i) - temp;
        }
    }
    else
    {
        for (int i = 0 ; i<m ; i++)
        {
            temp = 0;
            //Produit scalaire entre ligne m+1-i de A (ici tri inf) et x
            for (int j = 1 ; i<n ;i ++)
            {
                if (M1(j) == m+1-i)
                {
                    temp += M3(j)*x(M2(j));
                }
            }
            x(n+1-i) = b(n+1-i) - temp;
        }
    }

    return x;
}



