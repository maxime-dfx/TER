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
    COO J ,L , U;
    J = A.diagonal();
    L = A.tril();
    U = A.triu();
    VectorXd z = r;
    VectorXd p;
    VectorXd q;
    double delta(0.);
    double rho0 = 1 ;
    double rho(0.);
    // cout<<" "<<endl<<" "<<endl;
    while(r.norm()/norm_b > epsilon)
    {
        // cout << "iteration : "<<k<<endl;
        // cout<<"r = "<<r<<endl;
        z = solve_tri(J-L,z,true);
        // cout<<"z = "<<z<<endl;
        z = J.Prod(z);
        // cout<<"z = "<<z<<endl;
        z = solve_tri(J-U,z,false);
        // cout<<"z = "<<z<<endl;
        rho = r.dot(z);
        
        if (abs(rho) < 1e-100)
        {
            cout <<"Echec rho = 0"<<endl;
            break;
        }
        
        if (k==0)
        {
            p = z;
        }
        else
        {
            p = (rho/rho0) * p + z ;
            cout<<"p = "<<p<<endl;
        }
        q = A.Prod(p);
        delta = p.dot(q);
        if (delta <1e-100)
        {
            cout <<"Echec delta =0"<<endl;
            break;
        }
        x += rho/delta * p;
        cout<< k<<" x = "<<x<<endl;
        cout << "Produit Ax = " <<endl<<A.Prod(x)<<endl;
        r -= rho/delta * q;
        rho0 = rho;
        k ++;
        // cout<<" "<<endl<<" "<<endl<<" "<<endl;
    }
    cout <<"Fait en "<< k << " iterations"<<endl;
    cout <<"x = " <<x<<endl;
    cout << "Produit Ax = " <<endl<<A.Prod(x)<<endl;


}


//Resoud Ax = b si A est triangulaire 'strict' (diag nulle)
VectorXd solve_tri(COO A , VectorXd b , bool lower)
{
    VectorXd x = 0*b;
    //cout <<"b = "<<endl<<b<<endl;
    double temp(0.);
    int n = A.Get1().size();
    //cout <<"Il y a "<<n<<" element non nul de A"<<endl;
    int m = A.GetSize();
    VectorXi M1 = A.Get1() , M2 = A.Get2() ;
    VectorXd M3 = A.Get3();
    if (lower == true)
    {
        for (int i = 0 ; i<m ; i++)
        {
            temp = 0;
            
            //Produit scalaire entre ligne i de A (ici tri inf) et x
            for (int j = 0 ; j<n ;j ++)
            {
                // cout <<"on va jusque la"<<i<<" "<<j<<endl;
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
        cout<<"Debut solver tri sup"<<endl;
        for (int i = 0 ; i<m ; i++)
        {
            temp = 0;
            //Produit scalaire entre ligne m-i de A (ici tri sup) et x
            for (int j = 0 ; j<n ;j ++)
            {
                if (M1(j) == m-i-1)
                {
                    temp += M3(j)*x(M2(j));
                }
            }
            // cout <<"i = "<<i<<" temp = "<<temp<<endl;
            x(m-i-1) = b(m-i-1) - temp;
            // cout <<x<<endl;
        }
    }

    return x;
}



