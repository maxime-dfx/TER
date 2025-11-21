#include <Eigen/Dense>
#include "COO.h"

using namespace Eigen;


//Méthode vu en cours de Solveur Linéaire
//Algo de M. Durufle
void pmres(COO A ,VectorXd b ,int iter_max);

//Méthode du TER de 1A fais main cette fois
void pgmres(COO A, VectorXd b,int iter_max);

void grad_conj(COO A,VectorXd b,VectorXd x0,int iter_max,double epsilon);

VectorXd solve_tri(COO A , VectorXd x , bool lower);



