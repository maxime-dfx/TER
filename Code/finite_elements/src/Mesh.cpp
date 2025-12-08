#include "Mesh.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

using namespace std;

Mesh::Mesh() {
    // Rien de spécial à l'initialisation pour l'instant
}

void Mesh::read(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error("Erreur : Impossible d'ouvrir le fichier " + filename);
    }

    string keyword;
    while (file >> keyword) {
        
        // --- Lecture des Sommets ---
        if (keyword == "Vertices") {
            int nbVertices;
            file >> nbVertices;
            vertices.resize(nbVertices);

            for (int i = 0; i < nbVertices; ++i) {
                // Format: x y label
                file >> vertices[i].x >> vertices[i].y >> vertices[i].label;
            }
        }
        
        // --- Lecture des Triangles ---
        else if (keyword == "Triangles") {
            int nbTriangles;
            file >> nbTriangles;
            triangles.resize(nbTriangles);

            for (int i = 0; i < nbTriangles; ++i) {
                int n1, n2, n3, ref;
                file >> n1 >> n2 >> n3 >> ref;

                // ATTENTION : .mesh est en base-1, C++ en base-0
                triangles[i].v[0] = n1 - 1;
                triangles[i].v[1] = n2 - 1;
                triangles[i].v[2] = n3 - 1;
                triangles[i].label = ref;
            }
        }
        
        // --- Fin du fichier ---
        else if (keyword == "End") {
            break;
        }
        // On ignore les autres mots-clés (Edges, etc.) pour l'instant
    }
    
    file.close();
    cout << "[Mesh] Lecture terminee avec succes." << endl;
}

void Mesh::printStats() const {
    cout << "=== Statistiques du Maillage ===" << endl;
    cout << "Nombre de sommets   : " << vertices.size() << endl;
    cout << "Nombre de triangles : " << triangles.size() << endl;
    
    if (!vertices.empty()) {
        cout << "Premier sommet      : (" << vertices[0].x << ", " << vertices[0].y << ")" << endl;
    }
    cout << "================================" << endl;
}