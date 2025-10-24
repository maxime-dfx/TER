#include <Eigen/Dense>
#include <iostream>
#include "z_fonctions.h"
#include "Solver.h"

using namespace Eigen;
using namespace std;


int main()
{

    char rep;
    cout << "Le test 1 concerne la création d'un objet COO"<<endl;
    cout <<"en utilisant une matrice construite"<<endl;
    cout << "Afficher le test 1?  y/n"<<endl;
    cin >> rep;
    switch (rep)
    {
    case 'y':
    {
        cout << "===================================================="<<endl;
        Matrix<double,3,3> A;
        A << 1 , 2 , 0 , 5 , 0 , 6 , 8 , 0 , 0 ;
        cout <<"La matrice A : " << endl;
        cout << A <<endl;
        cout << "===================================================="<<endl;
        cout << "Cette matrice devrait donner les matrices COO suivantes : "<<endl;
        cout <<"Mat1 : 0 0 1 1 2"<<endl;
        cout <<"Mat2 : 0 1 0 2 1"<<endl;
        cout <<"Mat3 : 1 2 5 6 8"<<endl;

        cout << "===================================================="<<endl;

        cout <<"En passant par la class COO on obtient le résultat suivant : "<<endl;
        COO test(A); //crée les mat1 mat2 mat3 de A
        cout <<"Construction réussi "<<endl;
        test.Show();
        cout << "1er test avec une matrice construite réussi!"<<endl;
        cout << "===================================================="<<endl;
        break;
    }
    default:
    {
        break;
    }
    }
    
    cout <<"Le test 2 concerne la création d'un objet COO"<<endl;
    cout <<"en utilisant une fonction pour définir les coefficients"<<endl;
    cout <<"de la matrice sans avoir besoin de la stocker"<<endl;
    cout << "Afficher le test 2?  y/n"<<endl;
    cin >> rep;
    switch (rep)
    {
    case 'y' :
    {
        cout << "Test 2 : construction avec une fonction"<<endl;
        cout << "===================================================="<<endl;

        cout <<"La fonction choisie donnerait une matrice A :"<<endl;
        cout <<"0 1 0 0 0 " << endl;
        cout <<"1 2 3 0 0 " <<endl;
        cout <<"0 3 4 5 0 " <<endl;
        cout <<"0 0 5 6 7" << endl;
        cout <<"0 0 0 7 8" << endl;
        cout <<"     "<<endl;
        cout <<"Cette matrice n'est jamais créer ni utilisé"<<endl;
        cout << "===================================================="<<endl;
        cout <<"Le résultat attendu est :"<<endl;
        cout <<"Mat 1 : 0 1 1 1 2 2 2 3 3 3 4 4 "<<endl;
        cout <<"Mat 2 : 1 0 1 2 1 2 3 2 3 4 3 4 "<<endl;
        cout <<"Mat 3 : 1 1 2 3 3 4 5 5 6 7 7 8 "<<endl;

        cout << "===================================================="<<endl;
        cout <<"En passant par la classe COO on obtient : "<<endl;
        COO test2(f_test,5,5); //crée mat1 mat2 mat3 à partir de la fonction fournie
        cout<<"Création réussi de l'objet COO"<<endl;
        test2.Show();
        cout << "===================================================="<<endl;
        cout << "2e test avec une fonction réussi!"<<endl;
        cout << "===================================================="<<endl;
        break;
    }
    default :
        break;
    }
    

    cout <<"Le test 3 concerne l'utilisation d'une fonction"<<endl;
    cout <<"Prod qui effectue le produit matrice vecteur"<<endl;
    cout <<"a partir de l'objet COO"<<endl;
    cout << "Afficher le test 3?  y/n"<<endl;
    cin >> rep;
    switch (rep)
    {
        case 'y' :
        {
            cout <<"Test 3 : Produit Matrice COO vecteur :"<<endl;
            cout <<"Soit la matrice A = "<<endl;
            Matrix<double,3,3> B;
            B << 1 , 2 , 0 , 5 , 0 , 6 , 8 , 0 , 0 ;
            cout <<B<<endl;
            COO Bcoo(B);
            cout << "Et le vecteur y = "<<endl;
            Vector3d y ;
            y << 2 , 4 , 6 ;
            cout << y << endl;
            cout << "Le produit est censé donné : " << endl;
            cout <<" 10 46 16"<<endl;

            cout <<"En passant par la classe COO on obtient : "<<endl;
            cout << Bcoo.Prod(y) <<endl;
            break;
        }
        default :
            break;
    }
    
    


    

    






    return 0;
}



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