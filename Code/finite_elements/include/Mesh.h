#ifndef MESH_H
#define MESH_H

#include <vector>
#include <string>

// Structure pour un point (Sommet)
struct Vertex {
    double x, y;
    int label; // Le "ref" ou "tag" (ex: 1=bord bas, 2=bord droit...)
};

// Structure pour un élément (Triangle)
struct Triangle {
    int v[3];  // Les indices des 3 sommets dans le vecteur de sommets
    int label; // Le "ref" (ex: 0=Matrice, 1=Fibre)
};

// La classe principale
class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;

    // Constructeur
    Mesh();

    // Méthode de lecture
    void read(const std::string& filename);

    // Affichage des infos pour vérifier
    void printStats() const;
};

#endif