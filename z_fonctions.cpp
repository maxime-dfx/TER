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
// Matrix<double,3,6> Crea_B (int k)
// {
//     //Calcul de Je:
//     Matrix2d Je;
//     Je(0,0) = coord_noeud(noeud_maille(k,2),0);
//     //Calcul des dNi/dx et dy:
    
// }






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
    

