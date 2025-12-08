#ifndef MATERIAL_H
#define MATERIAL_H

#include <map>
#include <iostream>

struct Prop {
    double E;  // Module de Young
    double nu; // Coefficient de Poisson
};

class MaterialManager {
private:
    // On associe un Label (int) à des Propriétés (Prop)
    std::map<int, Prop> materials;

public:
    // Ajoute un matériau (ex: label 101 -> E=3000, nu=0.3)
    void addMaterial(int label, double E, double nu);

    // Récupère les propriétés pour un label donné
    Prop getProperties(int label) const;
    
    // Calcule la matrice constitutive D (3x3) pour un label
    // C'est la loi de Hooke (Sigma = D * Epsilon)
    // isPlaneStress : true = contrainte plane, false = déformation plane
    void getHookeMatrix(int label, double D[3][3], bool isPlaneStress = false) const;
};

#endif