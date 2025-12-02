#include <Eigen/Dense>
#include <iostream>
#include <complex>

using namespace Eigen;
using namespace std;



class COO
{
    protected :

    VectorXi _M1 , _M2 ;
    VectorXd _M3;
    int _taille_A;

    public :

        // constructeur par défaut
        COO();
        COO(VectorXi M1 , VectorXi M2 , VectorXd M3, int taille_A);
        //consructeur par fonction
        COO(function<double (const int, const int)> f,int n);

        //Ce constructeur sera peu utilisé en réalité
        // Car en principe les matrices seront trop grandes pour etre 
        // généré mais c'est un bon moyen de prendre la main
        // #Echauffement
        COO(MatrixXd A);

        //destructeur par défaut
        virtual ~COO();


        // Permet d'afficher M1 , M2 et M3
        void Show() const ;

        // Permet de récupérer M1
        const VectorXi Get1() const;

        // Permet de récupérer M2
        const VectorXi Get2() const;

        // Permet de récupérer M3
        const VectorXd Get3() const;

        // Permet de récupérer la taille de la matrice creuse
        const int GetSize() const;


        //Donne le produit de la matrice en COO par u
        VectorXd Prod(VectorXd u);

        COO diagonal();
        COO triu();
        COO tril();
};

COO operator-(const COO & A , const COO & B);