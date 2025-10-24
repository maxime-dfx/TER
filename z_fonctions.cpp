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
    

