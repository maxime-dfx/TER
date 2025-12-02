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
        getline(fic,line);
    }

    int n_temp(0);
    getline(fic,line);
    fic>>n_temp;
    getline(fic,line);
    for (int i = 0 ; i< n_temp ; i++)
    {
        getline(fic,line);
    }

    getline(fic,line);
    fic >> nb_mailles;
    getline(fic,line);
    noeud_maille.resize(nb_mailles,3);
    for (int i = 0 ; i < nb_mailles ; i++)
    {
        fic >> noeud_maille(i,0) >> noeud_maille(i,1) >> noeud_maille(i,2);
        getline(fic,line);
    }

    _nb_mailles = nb_mailles;
    _nb_noeud = nb_noeud;
    _noeud_maille = noeud_maille;
    _coord_noeud = coord_noeud;



}


maillage::~maillage()
{
}


Matrix<double,-1,2> maillage::Get_Coord()
{
    return this->_coord_noeud;
}

Matrix<double,-1,3> maillage::Get_Noeud()
{
    return this->_noeud_maille;
}