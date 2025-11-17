#include "maillage.h"



maillage::maillage(string file_name)
{
    ifstream fic(file_name);
    if (fic)
    {
        cout<<"Ouverture du fichier mesh réussi!"<<endl;
    }
    else
    {
        cout<<"Erreur dans l'ouverture du mesh"<<endl;
        return;
    }

    int nb_mailles;
    int nb_noeud;
    Matrix<double,-1,2> coord_noeud;
    Matrix<double,-1,3> noeud_maille;

    string line ;

    //3 premiere ligne de format
    getline(fic,line);
    getline(fic,line);
    getline(fic,line);

    getline(fic,line);
    fic>>nb_noeud;
    getline(fic,line);
    coord_noeud.resize(nb_noeud,2);
    for (int i = 0 ; i < nb_noeud ; i++)
    {
        fic>>coord_noeud(i,0)>>coord_noeud(i,1);
        cout<<coord_noeud(i)<<endl;
    }




}


maillage::~maillage()
{
}