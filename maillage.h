#include <Eigen/Dense>
#include <iostream>
#include <complex>
#include <string>
#include <fstream>

using namespace std;
using namespace Eigen;


class maillage
{
private:
    int _nb_mailles;
    int _nb_noeud;
    Matrix<double,-1,2> _coord_noeud;
    Matrix<double,-1,3>_noeud_maille;

public:
    maillage(string file_name);
    ~maillage();

    //Récupere les coordonnée des noeuds du maillage
    Matrix<double,-1,2> Get_Coord();
    //Recupere le tableaux donnant les noeuds présents dans une maille
    Matrix<double,-1,3> Get_Noeud();
};


