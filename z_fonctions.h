#include <Eigen/Dense>
#include <iostream>
#include <complex>
#include <string>

using namespace Eigen;
using namespace std;

double f_test(int i , int j);

Matrix<double,3,6> Crea_B (int k);






// embeter pour rien y'a un type fonction avec std
class fonction
{
private:
    double _x;
    function<double (const int , const int)> _f;
public:
    //constructeur par défaut : fonction nulle
    fonction();

    //constructeur avec un fichier organisé
    //fonction(string file_name);

    //constructeur avec une expression
    fonction(function<double (const int , const int)> f);

    //destructeur par défaut
    ~fonction();

    const double GetF() const;
};

