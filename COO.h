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

    public :

        // constructeur par défaut
        COO();

        //consructeur par fonction
        COO(function<double (const int, const int)> f ,int taille_pb_1 , int taille_pb_2);

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


        //Donne le produit de la matrice en COO par u
        VectorXd Prod(VectorXd u);




};