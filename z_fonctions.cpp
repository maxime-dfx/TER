#include "z_fonctions.h"
#include "maillage.h"
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
    
    //Calcul des dNi/dx et dy:
    
}






// Pas besoin de regarder aussi bas

fonction::fonction() : _x(0.)
{
}

fonction::~fonction()
{
}

    
// fonction::fonction(string file_name)
// {
//     ifstream fic(file_name);
    
//     if (!fic)
//     {
//         cerr << "Erreur ouvertur fichier" <<endl;
//     }

// }

   
fonction::fonction(function<double (const int , const int)> f)
{
    this->_f = f;
}


const double fonction::GetF() const
{
    return this->_x;
}
    

