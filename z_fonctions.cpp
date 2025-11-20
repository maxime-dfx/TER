#include "z_fonctions.h"
#include <iostream>
#include <complex>
#include <fstream>

using namespace std;
using namespace Eigen;

using namespace std;


double f_test(int i , int j)
    {
        if (abs(i-j)<=1)
        {
            return i+j;
        }
        else
        {
            return 0.;
        }
    }



//Crée B en partant du principe qu'on a lu le gmsh comme dans le tp a rendre(en fortran)
Matrix<double,3,6> Crea_B (maillage maille , int k)
{
    //Calcul de Je:
    Matrix2d Je;
    Matrix<double,-1,2> coord_noeud;
    Matrix<double,-1,3> noeud_maille ;
    coord_noeud = maille.Get_Coord();
    noeud_maille = maille.Get_Noeud();
    
    int A,B,C;
    C = noeud_maille(k,0);
    B = noeud_maille(k,1);
    A = noeud_maille(k,2);
    Je(0,0) = coord_noeud(B,0)-coord_noeud(A,0);
    Je(0,1) = coord_noeud(C,0)-coord_noeud(A,0);
    Je(1,0) = coord_noeud(B,1)-coord_noeud(A,1);
    Je(1,1) = coord_noeud(C,1)-coord_noeud(A,1);


    //Calcul de inverse et transpose de Je
    Je = Je.inverse().transpose();


    //Calcul des grad(Ni):
    Vector2d dN1_ref , dN2_ref , dN3_ref , dN1 , dN2 , dN3;
    dN1_ref << -1.,-1.;
    dN2_ref << 1. , 0.;
    dN3_ref << 0. , 1.;

    dN1 = Je*dN1_ref;
    dN2 = Je*dN2_ref;
    dN3 = Je*dN3_ref;

    //Construction de B:

    Matrix<double,3,6> mat_B ;

    mat_B<< dN1(0) , 0. , dN2(0) , 0. , dN3(0) , 0. ,
             0. , dN1(1) , 0. , dN2(1) , 0. , dN3(1),
            dN1(1) , dN1(0) , dN2(1) , dN2(0) , dN3(1) , dN3(0);
    cout <<mat_B<<endl;

    return mat_B;
    }

Matrix<double,6,6> Crea_D()
{
    Matrix<double,6,6> mat_D;
    return mat_D;
}
    

