#include <iostream>
#include <complex>
#include "COO.h"




using namespace Eigen;
using namespace std;


COO::COO()
{}


COO::~COO()
{
}

COO::COO(VectorXi M1 , VectorXi M2 , VectorXd M3, int taille_A)
{
    this->_M1 = M1;
    this->_M2 = M2;
    this->_M3 = M3;
    this->_taille_A = taille_A;
}

COO::COO(function<double (const int, const int)> f,int m)
{
    //taille_pb sert à connaitre la taille hypothétique de la matrice A
    //qu'on ne veut surtout pas créer!!
    //Le 1 pour le nb de ligne et 2 pour colonnes
    //Je réfléchirai plus tard à un chgt
    //Note : Peut-être créer une classe fonction??
    //Je suis pas encore sur des paramètres nécéssaires à f

    int n(0);  //Compteurs pour taille matrices résultats

    VectorXi M1 , M2;
    VectorXd M3;
    //cout << "Allocation matrice check"<<endl;
    this->_taille_A = m;
   
    double terme(0.);


    for (int i = 0 ; i< n ; i++)
    {
        for (int j = 0 ; j < n ; j++)
        {
            terme = f(i,j);

            if (abs(terme) > 0)
            {
                 
                //cout << n<<endl;

                M1.conservativeResize(n+1);
                M2.conservativeResize(n+1);
                M3.conservativeResize(n+1);
                //cout <<"Pas de problème avec le conservative resize" << n+1 <<endl;
                
                M1(n) = i ;
                M2(n) = j ;
                M3(n) = terme ;
                //cout << terme <<endl;
                n ++;



            }
        }
    }
    this->_M1 = M1;
    this->_M2 = M2;
    this->_M3 = M3;
}


COO::COO(MatrixXd A)
{
    int n , m , k(0);
    n = A.rows();
    m = A.cols();
    int taille(0);
    
    for (int i = 0 ; i < n ; i ++)
    {
        for (int j = 0 ; j < m ; j ++)
        {
            if (A(i,j) > 1E-8)
            {
                taille ++;
            }
        }
    }
    VectorXi M1(taille) , M2(taille);
    VectorXd M3(taille);
    for (int i = 0 ; i < n ; i ++)
    {
        for (int j = 0 ; j < m ; j ++)
        {
            if (abs(A(i,j)) > 0)
            {
                M1(k) = i;
                M2(k) = j;
                M3(k) = A(i,j) ;
                k ++;
            }
        }
    }
    this->_M1 = M1;
    this->_M2 = M2;
    this->_M3 = M3;

}

void COO::Show() const
{
    cout << "Liste des indices des lignes   : " << this->_M1.transpose() <<endl;
    cout << "Liste des indices des colonnes : " << this->_M2.transpose()<<endl;
    cout << "Liste des valeurs              : " << this->_M3.transpose() <<endl;
}


const VectorXi COO::Get1() const
{
    return this->_M1;
}

const VectorXi COO::Get2() const
{
    return this->_M2;
}

const VectorXd COO::Get3() const
{
    return this->_M3;
}

const int COO::GetSize() const
{
    return this->_taille_A;
}


VectorXd COO::Prod(VectorXd u)
{
    VectorXi M1 = this->Get1();
    VectorXi M2 = this->Get2();
    VectorXd M3 = this->Get3();
    int m = M1.size();
    int n = u.size();
    VectorXd y(n);
    
    for (int k = 0 ; k<m ; k++)
    {
        //y(i) = aij * uj
        
        y(M1(k)) += M3(k) * u(M2(k));
    }
    

    return y;
}

COO COO::diagonal()
{
    int taille;
    int n = this->_M1.size();
    for (int i = 0 ; i< n ; i++)
    {
        if (this->_M1(i) == this->_M2(i))
        {
            taille++;
        }
    }
    VectorXi M1(taille) , M2(taille);
    VectorXd M3(taille);
    int k = 0;
    for (int i = 0 ; i< n ; i++)
    {
        if (this->_M1(i) == this->_M2(i))
        {
            M1(k)=this->_M1(i);
            M2(k)=this->_M2(i);
            M3(k) = this->_M3(i);
            k ++;
        }
    } 
    COO diag(M1,M2,M3,taille);
    return diag;
}

COO COO::tril()
{
    int taille;
    int n = this->_M1.size();
    for (int i = 0 ; i< n ; i++)
    {
        if (this->_M1(i) > this->_M2(i))
        {
            taille++;
        }
    }
    VectorXi M1(taille) , M2(taille);
    VectorXd M3(taille);
    int k = 0;
    for (int i = 0 ; i< n ; i++)
    {
        if (this->_M2(i) < this->_M1(i))
        {
            M1(k)=this->_M1(i);
            M2(k)=this->_M2(i);
            M3(k) = this->_M3(i);
            k ++;
        }
    } 
    COO tril(M1,M2,M3,taille);
    return tril;
}


COO COO::triu()
{
    int taille;
    int n = this->_M1.size();
    for (int i = 0 ; i< n ; i++)
    {
        if (this->_M1(i) < this->_M2(i))
        {
            taille++;
        }
    }
    VectorXi M1(taille) , M2(taille);
    VectorXd M3(taille);
    int k = 0;
    for (int i = 0 ; i< n ; i++)
    {
        if (this->_M2(i) > this->_M1(i))
        {
            M1(k)=this->_M1(i);
            M2(k)=this->_M2(i);
            M3(k) = this->_M3(i);
            k ++;
        }
    } 
    COO triu(M1,M2,M3,taille);
    return triu;
}

