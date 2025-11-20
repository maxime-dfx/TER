#include <Eigen/Dense>
#include <iostream>
#include <complex>
#include <string>
#include "maillage.h"

using namespace Eigen;
using namespace std;

double f_test(int i , int j);

Matrix<double,3,6> Crea_B (maillage maille ,int k);

Matrix<double,6,6> Crea_D ();